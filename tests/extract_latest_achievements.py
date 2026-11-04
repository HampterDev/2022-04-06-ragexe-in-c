"""Extract session_get_latest_achievements (session_achievements.c). Usage: ROOT OUT"""
import re
import sys
from pathlib import Path

root, out = Path(sys.argv[1]), Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
text = (root / 'src' / 'session' / 'session_achievements.c').read_bytes().decode('utf-8').replace('\r\n', '\n')
match = re.search(r'^void session_get_latest_achievements\([^)]*\)\n\{\n.*?^\}\n', text, re.M | re.S)
if not match:
    sys.exit('session_get_latest_achievements not found')
(out / 'latest_achievements.inc').write_text(match.group(0), encoding='utf-8')
