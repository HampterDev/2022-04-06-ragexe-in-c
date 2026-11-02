"""Extract the Lapine upgrade result handler and the window IDs it uses."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
source = (root / 'src/game/packets_inventory.c').read_text(encoding='utf-8')
ids = list(re.finditer(r'^enum \{\n(?:    WID_[A-Z_]+ = 0x[0-9a-f]+,\n)+\};\n', source, re.M))
assert len(ids) == 1
signature = 'void game_mode_zc_ack_random_upgrade_item(struct GameMode *self, unsigned result)'
matches = list(re.finditer(r'^' + re.escape(signature) + r'\n\{', source, re.M))
assert len(matches) == 1, signature
start = matches[0].start()
function = source[start:source.index('\n}', start) + 2]
output.mkdir(parents=True, exist_ok=True)
(output / 'lapine_upgrade_result.inc').write_text(ids[0].group(0) + '\n' + function + '\n',
                                                  encoding='utf-8')
