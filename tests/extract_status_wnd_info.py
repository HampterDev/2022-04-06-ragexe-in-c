"""Extract the status window's layout state machine (info, expand, minimize)."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
source = (root / 'src/ui/character/status_wnd.c').read_text(encoding='utf-8')
prefix_end = source.index('\nint session_get_param(')
parts = [source[:prefix_end]]
declaration = re.findall(r'^int ui_status_wnd_send_msg\([^;{]*\);$', source, re.M)
assert len(declaration) == 1
parts.append(declaration[0])
signatures = [
    'void ui_status_wnd_store_info(',
    'static void status_place_buttons(',
    'static void status_toggle_expand(',
    'static void status_plus(',
    'static void status_on_command(',
    'int ui_status_wnd_send_msg(',
]
for signature in signatures:
    matches = list(re.finditer(r'^' + re.escape(signature) + r'[^{]*\n\{', source, re.M))
    assert len(matches) == 1, signature
    start = matches[0].start()
    parts.append(source[start:source.index('\n}', start) + 2])
output.mkdir(parents=True, exist_ok=True)
(output / 'status_wnd_info.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
