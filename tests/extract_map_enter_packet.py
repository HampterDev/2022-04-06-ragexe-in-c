"""Extract the login-mode map-entry sender so its packet bytes can be checked."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
source = (root / 'src/login/login_states.c').read_text(encoding='utf-8')
signature = 'static void enter_map(struct LoginMode *self)'
matches = list(re.finditer(r'^' + re.escape(signature) + r'\n\{', source, re.M))
assert len(matches) == 1, signature
start = matches[0].start()
function = source[start:source.index('\n}', start) + 2]
output.mkdir(parents=True, exist_ok=True)
(output / 'map_enter_packet.inc').write_text(function + '\n', encoding='utf-8')
