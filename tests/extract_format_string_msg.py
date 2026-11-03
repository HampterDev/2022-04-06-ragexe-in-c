"""Extract game_mode_format_string_msg with its packet, chat helper and argument stepping."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
source = (root / 'src/game/system_messages.c').read_text(encoding='utf-8')


def unique(pattern):
    matches = list(re.finditer(pattern, source, re.M))
    assert len(matches) == 1, pattern
    return matches[0]


def function(signature, optional=False):
    if optional and not re.search(r'^' + re.escape(signature), source, re.M):
        return []
    start = unique(r'^' + re.escape(signature) + r'[^{;]*\n\{').start()
    return [source[start:source.index('\n}', start) + 2]]


struct = unique(r'^struct PACKET_ZC_FORMATSTRING_MSG \{\n').start()
parts = [unique(r'^#define COLOUR_WHITE\b.*$').group(0),
         '#pragma pack(push, 1)\n' + source[struct:source.index('\n};', struct) + 3] + '\n#pragma pack(pop)']
parts += function('static void chat(')
parts += function('static const char *next_argument(', optional=True)
parts += function('void game_mode_format_string_msg(')
output.mkdir(parents=True, exist_ok=True)
(output / 'format_string_msg.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
