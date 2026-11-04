"""Extract the status window's layout state machine (saved info and minimize)."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
source = (root / 'src/ui/character/status_wnd.c').read_text(encoding='utf-8')
prefix_end = source.index('\nvoid ui_status_wnd_store_info(')
parts = [source[:prefix_end]]
# The window message functions return int, or intptr_t since the 64-bit port widened them.
declaration = re.findall(r'^(?:int|intptr_t) ui_status_wnd_send_msg\([^;{]*\);$', source, re.M)
assert len(declaration) == 1
parts.append(declaration[0])
send_type = declaration[0].split(' ', 1)[0]
signatures = [
    'void ui_status_wnd_store_info(',
    'static void status_place_buttons(',
    'static void status_plus(',
    'static void status_on_command(',
    send_type + ' ui_status_wnd_send_msg(',
]
for signature in signatures:
    matches = list(re.finditer(r'^' + re.escape(signature) + r'[^{]*\n\{', source, re.M))
    assert len(matches) == 1, signature
    start = matches[0].start()
    parts.append(source[start:source.index('\n}', start) + 2])
output.mkdir(parents=True, exist_ok=True)
(output / 'status_wnd_info.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
