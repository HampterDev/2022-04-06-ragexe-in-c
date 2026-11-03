"""Extract the private-airplane map and server moves with their packet from private_airplane.c."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
source = (root / 'src/game/private_airplane.c').read_text(encoding='utf-8')


def unique(pattern):
    matches = list(re.finditer(pattern, source, re.M))
    assert len(matches) == 1, pattern
    return matches[0]


def function(signature):
    start = unique(r'^' + re.escape(signature) + r'[^{;]*\n\{').start()
    return source[start:source.index('\n}', start) + 2]


struct = unique(r'^struct PacketNpcAckMapMove \{\n').start()
parts = ['#pragma pack(push, 1)\n' + source[struct:source.index('\n};', struct) + 3] + '\n#pragma pack(pop)',
         unique(r'^enum \{ HEADER_ZC_NPCACK_MAPMOVE\b.*$').group(0)]
size = re.search(r'^#define AIRPLANE_MAP_NAME\b.*$', source, re.M)
if size:
    parts.append(size.group(0))
parts += [function('void game_mode_on_map_move_private_airplane('),
          function('void game_mode_on_server_move_private_airplane(')]
output.mkdir(parents=True, exist_ok=True)
(output / 'private_airplane.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
