"""Extract the pet and monster talk-line functions and the talk-word table from talk_tables.c."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
source = (root / 'src/session/talk_tables.c').read_text(encoding='utf-8')


def function(signature, optional=False):
    matches = list(re.finditer(r'^' + re.escape(signature) + r'[^{;]*\n\{', source, re.M))
    if optional and not matches:
        return []
    assert len(matches) == 1, signature
    start = matches[0].start()
    return [source[start:source.index('\n}', start) + 2]]


words = source.index('BYTE g_talkWordCount')
parts = [source[words:source.index('\n};', words) + 3]]
parts += function('static char *node_text(')
parts += function('const char *session_get_state_name(')
parts += function('static void talk_copy(', optional=True)
parts += function('void session_get_monster_talk_info(')
parts += function('void session_get_pet_speak(')
parts += function('void session_format_chat_line(')
output.mkdir(parents=True, exist_ok=True)
(output / 'talk_lines.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
