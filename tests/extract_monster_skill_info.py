"""Extract session_get_monster_skill_info with its static helpers from talk_tables.c."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])


def functions(source, signatures, optional=()):
    parts = []
    for signature in signatures:
        matches = list(re.finditer(r'^' + re.escape(signature) + r'[^{;]*\n\{', source, re.M))
        if not matches and signature in optional:
            continue
        assert len(matches) == 1, signature
        start = matches[0].start()
        parts.append(source[start:source.index('\n}', start) + 2])
    return parts


talk = (root / 'src/session/talk_tables.c').read_text(encoding='utf-8')
parts = functions(talk, ['static char *node_text(', 'static int node_number(',
                         'void session_get_monster_skill_info('],
                  optional=('static int node_number(',))
output.mkdir(parents=True, exist_ok=True)
(output / 'monster_skill_info.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
