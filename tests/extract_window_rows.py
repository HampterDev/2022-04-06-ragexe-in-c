"""Extract the window table that src/ui/wndmgr/window_registry.c assembles, for the small tests.

The rows live in the window cluster files next to the factories and close functions they
name, which the small window-manager tests cannot link. This writes window_rows.inc with every
table of g_windowTable's list (the row array and its WmWindowRows) and the list and
g_windowTable themselves, copied from the source. A test includes it after window_rows_data.h,
whose row macros keep only the data columns (ID, slot, flags), so no factory or close
function is referenced. Each table and row array must be found exactly once.
Usage: extract_window_rows.py ROOT OUT
"""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
wndmgr = root / 'src' / 'ui' / 'wndmgr'
sources = {p: p.read_bytes().decode('utf-8').replace('\r\n', '\n') for p in sorted(wndmgr.glob('*.c'))}
registry = sources[wndmgr / 'window_registry.c']
list_match = re.search(r'^static const struct WmWindowRows \*const k_window_tables\[\] = \{\n(.*?)^\};\n',
                       registry, re.M | re.S)
assert list_match, 'k_window_tables'
tables = re.findall(r'&(\w+),', list_match.group(1))
assert tables


def only(pattern, what):
    found = [(p, m) for p, text in sources.items() for m in re.finditer(pattern, text, re.M | re.S)]
    assert len(found) == 1, (what, [str(p) for p, _ in found])
    return found[0]


parts = []
for table in tables:
    path, m = only(r'^(?:static )?const struct WmWindowRows ' + table + r' = WM_WINDOW_ROWS\((\w+)\);\n', table)
    rows_path, r = only(r'^static const struct WmWindowRow ' + m.group(1) + r'\[\] = \{\n.*?^\};\n', m.group(1))
    assert rows_path == path, table
    parts.append(f'/* {path.relative_to(root).as_posix()} */\n' + r.group(0) + '\n' + m.group(0))
composition = re.search(r'^const struct WmWindowTable g_windowTable = .*?;\n', registry, re.M)
assert composition
parts.append('/* src/ui/wndmgr/window_registry.c */\n' + list_match.group(0) + '\n' + composition.group(0))
output.mkdir(parents=True, exist_ok=True)
(output / 'window_rows.inc').write_text('\n'.join(parts), encoding='utf-8')
