#!/usr/bin/env python3
"""Audit Vietnamese gettext coverage against newly extracted English source strings."""

import collections
import json
import re
import subprocess
import sys
from pathlib import Path

import polib


def key(entry):
    return (entry.msgctxt or "", entry.msgid)


def is_translated(entry):
    if "fuzzy" in entry.flags:
        return False
    if entry.msgid_plural:
        return bool(entry.msgstr_plural) and all(
            bool(value) for value in entry.msgstr_plural.values()
        )
    return bool(entry.msgstr)


def display(entry):
    refs = [f"{p}:{n}" if n else p for p, n in entry.occurrences[:2]]
    return {
        "english": entry.msgid[:240],
        "context": entry.msgctxt or "",
        "reference": ", ".join(refs),
        "vietnamese": (entry.msgstr or next(iter(entry.msgstr_plural.values()), ""))[:240],
    }


def category(entry):
    if any(path.startswith("src/") for path, _ in entry.occurrences):
        return "C++/UI"
    return "JSON/game content"


def main():
    if len(sys.argv) != 3:
        sys.exit("Usage: audit_vi.py ENGLISH.pot VIETNAMESE.po")
    english = polib.pofile(sys.argv[1])
    vietnamese = polib.pofile(sys.argv[2])
    en_entries = [e for e in english if not e.obsolete and e.msgid]
    vi_entries = [e for e in vietnamese if not e.obsolete and e.msgid]
    en_map = {key(e): e for e in en_entries}
    vi_map = {key(e): e for e in vi_entries}

    missing = [e for e in en_entries if key(e) not in vi_map]
    untranslated = [e for e in en_entries if key(e) in vi_map and
                    not vi_map[key(e)].msgstr and
                    not vi_map[key(e)].msgstr_plural]
    fuzzy = [e for e in en_entries if key(e) in vi_map and
             "fuzzy" in vi_map[key(e)].flags]
    translated = [e for e in en_entries if key(e) in vi_map and
                  is_translated(vi_map[key(e)])]
    stale = [e for e in vi_entries if key(e) not in en_map]
    identical = [e for e in translated if
                 vi_map[key(e)].msgstr == e.msgid and
                 vi_map[key(e)].msgstr]
    incomplete_plural = [e for e in en_entries if key(e) in vi_map and
                         vi_map[key(e)].msgid_plural and
                         not is_translated(vi_map[key(e)])]

    fmt = subprocess.run(
        ["msgfmt", "-c", "--statistics", "-o", "/dev/null", sys.argv[2]],
        text=True, capture_output=True, check=False,
    )

    by_category = {}
    for name in ("C++/UI", "JSON/game content"):
        subset = [e for e in en_entries if category(e) == name]
        by_category[name] = {
            "english_keys": len(subset),
            "valid_translations": sum(
                1 for e in subset if key(e) in vi_map and is_translated(vi_map[key(e)])
            ),
            "missing_keys": sum(1 for e in subset if key(e) not in vi_map),
        }

    counts = {
        "english_current_total": len(en_entries),
        "vietnamese_catalog_entries": len(vi_entries),
        "matched_current_keys": len(en_entries) - len(missing),
        "valid_translations": len(translated),
        "coverage_pct": round(100 * len(translated) / max(1, len(en_entries)), 3),
        "missing_from_catalog": len(missing),
        "empty_translations": len(untranslated),
        "fuzzy_translations": len(fuzzy),
        "not_ready_current": len(en_entries) - len(translated),
        "stale_catalog_keys": len(stale),
        "same_as_english": len(identical),
        "incomplete_plurals_including_fuzzy": len(incomplete_plural),
        "msgfmt_exit_code": fmt.returncode,
        "msgfmt_output": (fmt.stderr + fmt.stdout)[-12000:],
        "categories": by_category,
    }

    out = Path("audit_results/vietnamese")
    out.mkdir(parents=True, exist_ok=True)
    (out / "stats.json").write_text(
        json.dumps(counts, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )

    lines = [
        "# Vietnamese vs English: translation audit",
        "",
        "Source: freshly extracted English gettext template from this commit.",
        "Comparison uses exact gettext (context, msgid) keys.",
        "",
        "| Measure | Count |",
        "|---|---:|",
    ]
    for name, k in [
        ("Current English keys", "english_current_total"),
        ("Current keys with usable Vietnamese", "valid_translations"),
        ("Coverage (%)", "coverage_pct"),
        ("English keys missing from Vietnamese catalog", "missing_from_catalog"),
        ("Present but empty", "empty_translations"),
        ("Marked fuzzy (needs review)", "fuzzy_translations"),
        ("Overall not ready for translation", "not_ready_current"),
        ("Old catalog keys not in current English", "stale_catalog_keys"),
        ("Identical to English (may be proper names)", "same_as_english"),
        ("Plural translations requiring review", "incomplete_plurals_including_fuzzy"),
    ]:
        lines.append(f"| {name} | {counts[k]} |")

    lines += ["", "## By source type", "", "| Type | English | Usable VI | Missing |",
              "|---|---:|---:|---:|"]
    for name, x in by_category.items():
        lines.append(
            f"| {name} | {x['english_keys']} | {x['valid_translations']} | {x['missing_keys']} |"
        )

    def samples(title, entries, max_count=30):
        lines.extend(["", f"## {title} (sample)", ""])
        if not entries:
            lines.append("None.")
        for e in entries[:max_count]:
            fields = display(e)
            lines.append(f"- **English:** `{fields['english'].replace(chr(10), ' ')}`")
            if fields["reference"]:
                lines.append(f"  - Source: `{fields['reference']}`")
            if fields["context"]:
                lines.append(f"  - Context: `{fields['context']}`")
            if fields["vietnamese"]:
                lines.append(f"  - Vietnamese: {fields['vietnamese'].replace(chr(10), ' ')}")

    samples("Missing from Vietnamese catalog", missing)
    samples("Empty translations", untranslated)
    samples("Fuzzy translations", fuzzy)
    samples("Outdated keys", stale, 15)
    samples("Same as English (examples, not necessarily errors)", identical, 15)
    lines += ["", "## Gettext format / catalog validation", "",
              f"Exit code: {fmt.returncode}", "", "```text",
              counts["msgfmt_output"][-7000:] or "(no output)", "```", "",
              "Note: A translation key present in the catalog is not proof of linguistic quality;",
              "this audit measures gettext completeness and compilation diagnostics."]
    (out / "report.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in counts.items() if k != "msgfmt_output"},
                     ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
