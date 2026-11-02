"""Extract the window manager's ID lookup so the name-plate case can run alone."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
source = (root / 'src/ui/wndmgr/window_slots.c').read_text(encoding='utf-8')
signature = 'struct UIWindow *ui_window_mgr_get_window(struct UIWindowMgr *self, int window_id)'
matches = list(re.finditer(r'^' + re.escape(signature) + r'\n\{', source, re.M))
assert len(matches) == 1, signature
start = matches[0].start()
function = source[start:source.index('\n}', start) + 2]
output.mkdir(parents=True, exist_ok=True)
(output / 'name_plate_lookup.inc').write_text(function + '\n', encoding='utf-8')
