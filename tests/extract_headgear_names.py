"""Compile the production headgear lookup with real Lua and owned strings."""
from pathlib import Path
import sys

source, output = map(Path, sys.argv[1:])
output.mkdir(parents=True, exist_ok=True)
text = (source / 'src/session/sprite_names.c').read_text(encoding='utf-8')
signature = 'struct String *session_get_accessory_name('
assert text.count(signature) == 1
start = text.index(signature)
function = text[start:text.index('\n}', start) + 2]
(output / 'headgear_names.inc').write_text(function + '\n', encoding='utf-8')
