"""Copy diag/perf.c for the overlay test, and extract the screenshot keys of
ui_window_mgr_handle_key from wndmgr_keys.c."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
perf = (root / 'src/diag/perf.c').read_text(encoding='utf-8')
keys = (root / 'src/ui/wndmgr/wndmgr_keys.c').read_text(encoding='utf-8')
define = re.search(r'^#define MODE_MSG_SCREENSHOT\b.*$', keys, re.M).group(0)
matches = list(re.finditer(r'^[ \t]*case VK_SNAPSHOT:[ \t]*$', keys, re.M))
assert len(matches) == 1
start = matches[0].start()
end = keys.index('break;', start) + len('break;')
output.mkdir(parents=True, exist_ok=True)
(output / 'perf_overlay.inc').write_text(perf, encoding='utf-8')
(output / 'screenshot_keys.inc').write_text(define + '\n', encoding='utf-8')
(output / 'screenshot_case.inc').write_text(keys[start:end] + '\n', encoding='utf-8')
