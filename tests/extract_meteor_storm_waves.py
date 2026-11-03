"""Extract rag_meteor_storm_waves from rag_unreferenced.c. Usage: ROOT OUT"""
import re
import sys
from pathlib import Path

root, out = Path(sys.argv[1]), Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
text = (root / 'src' / 'effects' / 'rag' / 'rag_unreferenced.c').read_bytes().decode('utf-8').replace('\r\n', '\n')
match = re.search(r'^unsigned char rag_meteor_storm_waves\(struct RagEffect \*self\)\n\{\n.*?^\}\n', text, re.M | re.S)
if not match:
    sys.exit('rag_meteor_storm_waves not found')
(out / 'meteor_storm_waves.inc').write_text(match.group(0), encoding='utf-8')
