"""Extract the basic-info factory that reopens windows saved as shown."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
source = (root / 'src/ui/wndmgr/make_window_character.c').read_text(encoding='utf-8')
signature = 'static struct UIWindow *make_basic_info(struct UIWindowMgr *self, int window_id)'
matches = list(re.finditer(r'^' + re.escape(signature) + r'\n\{', source, re.M))
assert len(matches) == 1, signature
start = matches[0].start()
function = source[start:source.index('\n}', start) + 2]
output.mkdir(parents=True, exist_ok=True)
(output / 'basic_info_restore.inc').write_text(function + '\n', encoding='utf-8')
