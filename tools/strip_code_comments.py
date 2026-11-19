from __future__ import annotations

import ast
import io
import re
import tokenize
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCOPES = ('src', 'tests', 'tools', 'utilities', 'build-scripts', 'android/app/src')
CLANG_TYPES = {'.cpp', '.h', '.hpp', '.cc', '.c', '.java'}
PY_TYPES = {'.py'}
KEEP = re.compile(
    r'copyright|licen[sc]e|spdx|all rights reserved|iwyu|nolint|clang.format|'
    r'cppcheck|noqa|pylint|mypy|pyright|type:\s*ignore|fmt:\s*(?:on|off|skip)|'
    r'ruff|nosec|pragma|generated (?:by|file)|do not edit|cspell|shellcheck|'
    r'\btranslators?\b|\bfall.?through\b|^//~', re.IGNORECASE
)
C_RAW = re.compile(r'(?:u8|u|U|L)?R"([^()\\\s]{0,16})\(')


def keep_comment(comment: str) -> bool:
    return bool(KEEP.search(comment))


def strip_clike(source: str) -> tuple[str, int]:
    out = []
    i = 0
    removed = 0
    line_start = 0
    size = len(source)
    while i < size:
        ch = source[i]
        if ch == '\n':
            out.append(ch)
            i += 1
            line_start = i
            continue
        if source.startswith('"""', i):
            end = source.find('"""', i + 3)
            end = size if end < 0 else end + 3
            chunk = source[i:end]
            out.append(chunk)
            nl = chunk.rfind('\n')
            if nl >= 0:
                line_start = end - (len(chunk) - nl - 1)
            i = end
            continue
        raw = C_RAW.match(source, i)
        if raw:
            end_marker = ')' + raw.group(1) + '"'
            end = source.find(end_marker, raw.end())
            end = size if end < 0 else end + len(end_marker)
            chunk = source[i:end]
            out.append(chunk)
            nl = chunk.rfind('\n')
            if nl >= 0:
                line_start = end - (len(chunk) - nl - 1)
            i = end
            continue
        if ch in ('"', "'"):
            j = i + 1
            while j < size:
                if source[j] == '\\':
                    j += 2
                    continue
                if source[j] == ch:
                    j += 1
                    break
                j += 1
            chunk = source[i:j]
            out.append(chunk)
            nl = chunk.rfind('\n')
            if nl >= 0:
                line_start = j - (len(chunk) - nl - 1)
            i = j
            continue
        if source.startswith('//', i):
            end = source.find('\n', i)
            end = size if end < 0 else end
            comment = source[i:end]
            before = source[line_start:i]
            if (keep_comment(comment) or comment.rstrip().endswith('\\')
                    or before.rstrip().endswith('\\')):
                out.append(comment)
            else:
                while out and out[-1] in (' ', '\t'):
                    out.pop()
                removed += 1
            i = end
            continue
        if source.startswith('/*', i):
            end = source.find('*/', i + 2)
            if end < 0:
                out.append(source[i:])
                break
            end += 2
            comment = source[i:end]
            before = source[line_start:i]
            if keep_comment(comment) or before.rstrip().endswith('\\'):
                out.append(comment)
            else:
                while out and out[-1] in (' ', '\t'):
                    out.pop()
                removed += 1
                if '\n' in comment:
                    out.append('\n' * comment.count('\n'))
                elif end < size and source[end] not in ('\n', '\r', ' ', '\t'):
                    out.append(' ')
            last_nl = comment.rfind('\n')
            if last_nl >= 0:
                line_start = end - (len(comment) - last_nl - 1)
            i = end
            continue
        out.append(ch)
        i += 1
    return ''.join(out), removed


def strip_python(source: str) -> tuple[str, int]:
    lines = source.splitlines(keepends=True)
    removed = 0
    for token in tokenize.generate_tokens(io.StringIO(source).readline):
        if token.type != tokenize.COMMENT:
            continue
        comment = token.string
        if (token.start[0] in (1, 2) and
                (comment.startswith('#!') or 'coding' in comment)) or keep_comment(comment):
            continue
        line_idx = token.start[0] - 1
        line = lines[line_idx]
        lines[line_idx] = (line[:token.start[1]].rstrip(' \t')
                           + line[token.end[1]:])
        removed += 1
    result = ''.join(lines)
    if removed:
        ast.parse(result)
    return result, removed


def files():
    for scope in SCOPES:
        root = ROOT / scope
        if not root.exists():
            continue
        for file in root.rglob('*'):
            if not file.is_file() or file.suffix not in CLANG_TYPES | PY_TYPES:
                continue
            relative = file.relative_to(ROOT)
            if any(part in {'third-party', 'vendor', 'node_modules', 'generated',
                            '.git', '.gradle', 'build', '__pycache__'}
                   for part in relative.parts):
                continue
            if relative.parts[:3] == ('tools', 'clang-tidy-plugin', 'test'):
                continue
            yield file


def main():
    count = changed = removed = 0
    for file in files():
        try:
            source = file.read_text(encoding='utf-8')
        except UnicodeError:
            continue
        count += 1
        try:
            if file.suffix in PY_TYPES:
                output, n = strip_python(source)
            else:
                output, n = strip_clike(source)
        except (SyntaxError, tokenize.TokenError, IndentationError) as error:
            raise RuntimeError(f'Unable to safely clean {file}: {error}') from error
        if n and output != source:
            output = output.rstrip() + '\n' if output.strip() else ''
            file.write_text(output, encoding='utf-8')
            changed += 1
            removed += n
    print(f'Scanned {count} first-party source files; changed {changed}; '
          f'removed {removed} nonessential comments.')


if __name__ == '__main__':
    main()
