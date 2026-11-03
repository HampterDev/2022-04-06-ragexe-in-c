"""Extract the window manager's ID lookup and ESC-list helpers from window_slots.c."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
source = (root / 'src/ui/wndmgr/window_slots.c').read_text(encoding='utf-8')
parts = []
for signature in [
    'struct UIWindow *ui_window_mgr_get_window(',
    'BYTE ui_window_mgr_is_untracked_id(',
    'void ui_window_mgr_forget_window_id(',
]:
    matches = list(re.finditer(r'^' + re.escape(signature) + r'[^{]*\n\{', source, re.M))
    assert len(matches) == 1, signature
    start = matches[0].start()
    parts.append(source[start:source.index('\n}', start) + 2])
output.mkdir(parents=True, exist_ok=True)
(output / 'window_id_list.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
