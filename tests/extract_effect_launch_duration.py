"""Extract rag_effect_init (with its file prefix) so its duration lookup can run alone."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
source = (root / 'src/effects/rag/rag_str_anim.c').read_text(encoding='utf-8')
prefix_end = source.index('\nstatic int roll(')
parts = [source[:prefix_end]]
for signature in ['static int roll(', 'void rag_effect_init(']:
    matches = list(re.finditer(r'^' + re.escape(signature) + r'[^{]*\n\{', source, re.M))
    assert len(matches) == 1, signature
    start = matches[0].start()
    parts.append(source[start:source.index('\n}', start) + 2])
output.mkdir(parents=True, exist_ok=True)
(output / 'effect_launch_duration.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
