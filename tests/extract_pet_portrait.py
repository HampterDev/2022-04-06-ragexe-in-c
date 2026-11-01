"""Compile the production portrait filename helper with a controlled Lua boundary."""
from pathlib import Path
import sys

source, output = map(Path, sys.argv[1:])
output.mkdir(parents=True, exist_ok=True)
text = (source / 'src/session/pet.c').read_text(encoding='utf-8')
signature = 'void session_get_pet_illust_name('
assert text.count(signature) == 1
start = text.index(signature)
function = text[start:text.index('\n}', start) + 2]
(output / 'pet_portrait.inc').write_text(function + '\n', encoding='utf-8')
