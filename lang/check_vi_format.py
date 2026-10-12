#!/usr/bin/env python3
"""Check syntactic/formatting invariants in the Vietnamese gettext catalog.

Reports potentially dangerous substitutions without judging translation quality.
"""
import collections
import json
import re
import subprocess
import sys
from pathlib import Path

import polib


# Accept C printf format placeholders including positional parameters.
PRINTF = re.compile(
    r"%(?:(?P<pos>[1-9][0-9]*)\$)?"
    r"[-+ #0']*(?:\*|[0-9]+)?(?:\.(?:\*|[0-9]+))?"
    r"(?:hh|ll|[hljztL])?(?P<type>[diuoxXfFeEgGaAcspn])"
)
CURLY = re.compile(
    r"(?<!\{)\{(?P<field>[0-9]+|[a-zA-Z_][a-zA-Z_0-9]*)?"
    r"(?:![rsa])?(?::[^{}]*)?\}(?!\})"
)
TAG = re.compile(r"</?(?:[a-zA-Z][a-zA-Z_0-9-]*)(?:\s[^<>]*)?>")
VARIABLE = re.compile(r"<([a-zA-Z_][a-zA-Z_0-9]*)>")
MUST_PRESERVE_VARIABLES = (
    "npcname", "playername", "itemname", "u_name", "npc_name",
    "name", "actor_name", "target_name",
)


def printf_tokens(text):
    tokens = []
    pos = 0
    while pos < len(text):
        i = text.find("%", pos)
        if i < 0:
            break
        if i + 1 < len(text) and text[i + 1] == "%":
            pos = i + 2
            continue
        m = PRINTF.match(text, i)
        if m:
            typ = m.group("type").lower()
            group = "float" if typ in "fega" else ("integer" if typ in "diuox" else typ)
            tokens.append((m.group("pos"), group, m.group("type")))
            pos = m.end()
        else:
            pos = i + 1
    return tokens


def compare_printf(source, translated):
    original = printf_tokens(source)
    if not original:
        return False, None
    target = printf_tokens(translated)
    if not target:
        return True, {"english": original, "vietnamese": []}
    # Preserve argument types, allow reorder only with explicit positional specifiers.
    a = collections.Counter((position, typ) for position, _, typ in original)
    b = collections.Counter((position, typ) for position, _, typ in target)
    if a == b:
        return False, None
    # Handle consistent non-positional sequences by conversion type (order matters).
    # Note: positional modifiers such as %1$s cannot be compared as bare %s.
    return True, {"english": original, "vietnamese": target}


def curly_tokens(text):
    return [m.group("field") or "{}" for m in CURLY.finditer(text)]


def compare_curly(source, translated):
    a = curly_tokens(source)
    if not a:
        return False, None
    b = curly_tokens(translated)
    if a == b or (
        all(token != "{}" for token in a) and
        collections.Counter(a) == collections.Counter(b)
    ):
        return False, None
    return True, {"english": a, "vietnamese": b}


def tag_tokens(text):
    tags = TAG.findall(text)
    counts = collections.Counter()
    for tag in tags:
        label = re.match(r"</?([a-zA-Z][a-zA-Z_0-9-]*)", tag).group(1)
        if label.startswith("color_"):
            counts["color_open"] += 1
        elif label == "color" and tag.startswith("</"):
            counts["color_close"] += 1
        else:
            counts[(("close_" if tag.startswith("</") else "open_") + label)] += 1
    return counts


def entry_texts(entry):
    if entry.msgid_plural:
        return [value for value in entry.msgstr_plural.values()]
    return [entry.msgstr]


def audit(english, vietnamese):
    en = polib.pofile(english)
    vi = polib.pofile(vietnamese)
    en_map = {(e.msgctxt or "", e.msgid): e for e in en if e.msgid and not e.obsolete}
    findings = []
    counts = collections.Counter()
    examples = collections.defaultdict(list)

    for e in vi:
        if not e.msgid or e.obsolete:
            continue
        source = en_map.get((e.msgctxt or "", e.msgid))
        if source is None:
            continue
        candidates = [source.msgid]
        if source.msgid_plural:
            candidates.append(source.msgid_plural)
        translations = entry_texts(e)
        for idx, translated in enumerate(translations):
            if not translated:
                continue
            counts["strings_checked"] += 1
            ref = e.occurrences[:2]
            location = ", ".join(f"{a}:{b}" if b else a for a, b in ref)
            base = {
                "location": location,
                "context": e.msgctxt or "",
                "english": e.msgid[:400],
                "vietnamese": translated[:400],
                "plural_index": idx if e.msgid_plural else None,
            }

            problems = []
            for name, checker in (
                ("printf_argument_mismatch", compare_printf),
                ("brace_placeholder_mismatch", compare_curly),
            ):
                if not any(checker(c, translated)[0] is False for c in candidates):
                    # Only flag when none of the singular/plural sources match.
                    details = checker(candidates[0], translated)[1]
                    if details is not None:
                        problems.append((name, details))

            src_tags = tag_tokens(source.msgid)
            dst_tags = tag_tokens(translated)
            if src_tags != dst_tags and not (
                source.msgid_plural and tag_tokens(source.msgid_plural) == dst_tags
            ):
                problems.append(("markup_tag_mismatch", {
                    "english": dict(src_tags), "vietnamese": dict(dst_tags)
                }))

            for var in MUST_PRESERVE_VARIABLES:
                name = f"<{var}>"
                if source.msgid.count(name) != translated.count(name):
                    problems.append(("runtime_variable_mismatch", {
                        "placeholder": name,
                        "english": source.msgid.count(name),
                        "vietnamese": translated.count(name)
                    }))

            if source.msgid.startswith("\n") and not translated.startswith("\n"):
                problems.append(("leading_newline_mismatch", {}))
            if source.msgid.endswith("\n") and not translated.endswith("\n"):
                problems.append(("trailing_newline_mismatch", {}))

            for problem, details in problems:
                counts[problem] += 1
                finding = {**base, "type": problem, "details": details}
                findings.append(finding)
                if len(examples[problem]) < 25:
                    examples[problem].append(finding)

    counts["issues"] = len(findings)
    return dict(counts), findings, examples


def main():
    if len(sys.argv) != 3:
        sys.exit("Usage: check_vi_format.py english.pot vietnamese.po")
    en, vi = sys.argv[1], sys.argv[2]
    out = Path("audit_results/vietnamese")
    out.mkdir(parents=True, exist_ok=True)

    commands = [
        ("gzip_integrity", ["gzip", "-t", "lang/po/vi.po.gz"]),
        ("gettext_format", ["msgfmt", "--check", "--check-format",
                            "--statistics", "-o", "/dev/null", vi]),
        ("unicode_check", [sys.executable, "lang/unicode_check.py", vi]),
    ]
    diagnostics = {}
    for name, args in commands:
        proc = subprocess.run(args, capture_output=True, text=True)
        diagnostics[name] = {
            "ok": proc.returncode == 0,
            "return_code": proc.returncode,
            "message": (proc.stderr + proc.stdout)[-2000:],
        }

    counts, findings, examples = audit(en, vi)
    summary = {"diagnostics": diagnostics, "counts": counts}
    (out / "format_stats.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    with (out / "format_findings.jsonl").open("w", encoding="utf-8") as f:
        for entry in findings:
            f.write(json.dumps(entry, ensure_ascii=False) + "\n")

    lines = [
        "# Vietnamese translation: technical-format audit",
        "",
        "This report checks compressed catalog integrity, gettext validation,",
        "Unicode validation, and common runtime placeholder/markup tokens.",
        "Possible mismatches need contextual review and are not automatically errors.",
        "",
        "## Compile and Unicode checks",
        "",
        "| Test | Result | Diagnostic |",
        "|---|---|---|",
    ]
    for name, info in diagnostics.items():
        lines.append(
            f"| {name} | {'PASS' if info['ok'] else 'FAIL'} | "
            f"{info['message'].strip().replace(chr(10), ' ')[:160]} |"
        )
    lines.extend(["", "## Potential structural mismatches", "",
                  "| Category | Count |", "|---|---:|"])
    for name, total in sorted(counts.items()):
        lines.append(f"| {name} | {total} |")
    for group, subset in sorted(examples.items()):
        lines += ["", f"## {group}: up to 25 examples", ""]
        for finding in subset:
            lines.append("- **English**: " + repr(finding["english"]))
            lines.append("  - **Vietnamese**: " + repr(finding["vietnamese"]))
            lines.append("  - **Reference**: " + finding["location"])
            lines.append("  - **Tokens**: " + repr(finding["details"]))
    lines += ["", "Full findings in `format_findings.jsonl`.",
              "False positives are possible, especially for punctuation and colors.", ""]
    (out / "format_report.md").write_text("\n".join(lines), encoding="utf-8")
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
