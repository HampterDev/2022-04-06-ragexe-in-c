"""Extract the effect-type tables that src/effects/effect_registry.c assembles, for one test.

The tables live in the effect class files next to their constructors, which the effect
factory test cannot compile. This writes effect_type_tables.inc with, for every entry of
g_effectTypes' list, the class's row array and table, or the startup function that adds
types (with the #define lines of its file that the function uses), copied from the source.
Each item must be found exactly once.
"""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
src = root / 'src'
registry = (src / 'effects/effect_registry.c').read_text(encoding='utf-8')
list_body = registry[registry.index('k_effect_type_sources[] = {'):]
list_body = list_body[:list_body.index('\n};')]
tables = re.findall(r'\{ &(\w+) \}', list_body)
functions = re.findall(r'\{ NULL, (\w+) \}', list_body)
assert tables and functions

sources = {p: p.read_text(encoding='utf-8') for p in sorted((src / 'effects').rglob('*.c'))}


def only(pattern, what):
    found = [(p, m) for p, text in sources.items() for m in re.finditer(pattern, text, re.M)]
    assert len(found) == 1, (what, [str(p) for p, _ in found])
    return found[0]


parts = []
for table in tables:
    path, m = only(r'^const struct EffectTypeTable ' + table + r' = EFFECT_TYPE_TABLE\((\w+)\);\n',
                   table)
    rows = m.group(1)
    text = sources[path]
    r = list(re.finditer(r'^static const struct EffectTypeRow ' + rows + r'\[\] = \{\n.*?^\};\n',
                         text, re.M | re.S))
    assert len(r) == 1, rows
    parts.append(f'/* {path.relative_to(root).as_posix()} */\n' + r[0].group(0) + m.group(0))
for function in functions:
    path, m = only(r'^unsigned char ' + function + r'\(struct EffectMgr \*mgr\)\n\{\n', function)
    text = sources[path]
    body = text[m.start():text.index('\n}\n', m.end()) + 3]
    defines = [d.group(0) for d in re.finditer(r'^#define (\w+) [^\n]*\n', text, re.M)
               if re.search(r'\b' + d.group(1) + r'\b', body)]
    parts.append(f'/* {path.relative_to(root).as_posix()} */\n' + ''.join(defines) + body)
output.mkdir(parents=True, exist_ok=True)
(output / 'effect_type_tables.inc').write_text('\n'.join(parts), encoding='utf-8')
