#!/usr/bin/env python3
"""Find accessibility integration candidates; this is NOT a coverage certificate.

Literal registrations, dynamic registrations and custom UI call sites are kept
separate. Compile-time branches and runtime-generated screens still need manual
review. No gameplay data or player saves are read by this tool.
"""

import argparse
import collections
import json
import pathlib
import re
import subprocess


# Mask comments without changing offsets, preserving strings (including raw
# strings) so comment markers inside labels do not hide real registrations.
TOKEN = re.compile(
    r'R"(?P<raw_delim>[^ ()\\\t\r\n]{0,16})\(.*?\)(?P=raw_delim)"'
    r'|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    r'|//[^\n]*|/\*.*?\*/', re.DOTALL
)
CALLS = (
    ('registered_action', re.compile(r'\bregister_action\s*\(\s*(?P<arg>[^\n,)]*)')),
    ('manual_key', re.compile(r'\bregister_manual_key\s*\(\s*(?P<arg>[^\n,)]*)')),
    ('input_context', re.compile(
        r'\binput_context\s+(?!const\b)(?:[A-Za-z_]\w*\s*)?\(\s*(?P<arg>[^\n,)]*)')),
    ('imgui', re.compile(r'\bImGui::(?P<arg>Begin\w*|Selectable|Button|Checkbox|Input\w*|Slider\w*)\s*\(')),
    ('legacy_input', re.compile(r'\b(?P<arg>(?:w)?getch|inp_mngr\.get_input_event)\s*\(')),
    ('native_dialog', re.compile(r'\b(?P<arg>AlertDialog\.Builder|ProgressDialog)\s*\(')),
)
LITERAL = re.compile(r'"((?:\\.|[^"\\])*)"\s*$')


def mask_comments(text):
    def mask(match):
        value = match.group()
        if value.startswith(('//', '/*')):
            return ''.join('\n' if c == '\n' else ' ' for c in value)
        return value
    return TOKEN.sub(mask, text)


def scan(path, text):
    masked = mask_comments(text)
    # Scan code outside string bodies: a documentation string mentioning a call
    # must not itself become a call site. Arguments are read from `masked`.
    code = TOKEN.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m.group()), masked)
    for kind, pattern in CALLS:
        for match in pattern.finditer(masked):
            if code[match.start():match.start() + 1].isspace():
                continue
            arg = match.group('arg').strip()
            literal = LITERAL.fullmatch(arg)
            yield {
                'kind': kind,
                'path': path,
                'line': masked.count('\n', 0, match.start()) + 1,
                'value': literal.group(1) if literal else arg,
                'resolution': 'literal' if literal else 'manual_review',
                'support': 'not_assessed',
            }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=pathlib.Path,
                        default=pathlib.Path(__file__).resolve().parents[2])
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    root = args.root.resolve()
    paths = sorted(set(root.joinpath('src').glob('*.cpp'))
                   | set(root.joinpath('src').glob('*.h'))
                   | set(root.joinpath('android/app/src/main/java').rglob('*.java')))
    entries = []
    for path in paths:
        entries.extend(scan(path.relative_to(root).as_posix(), path.read_text(encoding='utf-8')))
    commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
    report = {
        'schema_version': 1,
        'source_commit': commit,
        'limitations': [
            'Heuristic source inventory, not an AST or runtime screen inventory.',
            'All compile-time branches are included; no platform filtering.',
            'Dynamic registrations and custom UIs require manual review.',
            'No entry is marked accessible by source discovery.',
        ],
        'files_scanned': len(paths),
        'counts': dict(sorted(collections.Counter(x['kind'] for x in entries).items())),
        'entries': entries,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'source_commit': commit, 'files_scanned': len(paths), 'counts': report['counts']}))


if __name__ == '__main__':
    main()
