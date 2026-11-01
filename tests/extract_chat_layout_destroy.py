"""Exercise the actual chat-window cleanup and save-warning decision."""
from pathlib import Path
import sys

root, output = map(Path, sys.argv[1:])
output.mkdir(parents=True, exist_ok=True)
source = (root / 'src/ui/chat/chat_window.c').read_text(encoding='utf-8')
signature = 'void ui_new_chat_wnd_destroy(struct UIWindow *window)\n{'
assert source.count(signature) == 1
start = source.index(signature)
body = source[start:source.index('\n}', start) + 2]
(output / 'chat_layout_destroy.inc').write_text(body + '\n', encoding='utf-8')
