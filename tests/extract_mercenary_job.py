"""Extract the mercenary-job predicate from predicates.c (found by content: its name differs
between branches) and the actor facing from position.c."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])


def function_at(source, start):
    return source[start:source.index('\n}', start) + 2]


def function(source, signature):
    matches = list(re.finditer(r'^' + re.escape(signature) + r'[^{;]*\n\{', source, re.M))
    assert len(matches) == 1, signature
    return function_at(source, matches[0].start())


predicates = (root / 'src/session/predicates.c').read_text(encoding='utf-8')
matches = list(re.finditer(r'^int (session_is_mercenary\w*_job)\(struct Session \*self, int id\)\n\{', predicates, re.M))
assert len(matches) == 1
name = matches[0].group(1)
position = (root / 'src/actor/position.c').read_text(encoding='utf-8')
covers = 1 if 'homun_s' in name else 0
parts = [f'#define MERCENARY_JOB {name}', f'#define MERCENARY_JOB_COVERS_HOMUN_S {covers}',
         function_at(predicates, matches[0].start()),
         function(position, 'static float view_facing('), function(position, 'static int dir_rounded('),
         function(position, 'int game_actor_dir_of(')]
output.mkdir(parents=True, exist_ok=True)
(output / 'mercenary_job.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
