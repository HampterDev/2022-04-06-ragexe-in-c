"""Compile the production emote sender against a controlled clock/transport."""
from pathlib import Path
import re
import sys

source, output = map(Path, sys.argv[1:])
output.mkdir(parents=True, exist_ok=True)
text = (source / 'src/game/msg_misc.c').read_text(encoding='utf-8')
signature = 'int game_mode_msg_emotion('
assert text.count(signature) == 1
start = text.index(signature)
parts = [re.search(r'^#define CZ_REQ_EMOTION[^\n]+', text, re.M)[0],
         re.search(r'^#define EMOTION_WAIT[^\n]+', text, re.M)[0],
         text[start:text.index('\n}', start) + 2]]
(output / 'emote_timing.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
