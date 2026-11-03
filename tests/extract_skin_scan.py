"""Extract skin_mgr_scan with its helpers from skin_mgr.c, and str_find_last and
text_to_lower from text_util.c."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])


def functions(name, signatures):
    source = (root / 'src' / name).read_text(encoding='utf-8')
    parts = []
    for signature in signatures:
        matches = list(re.finditer(r'^' + re.escape(signature) + r'[^{;]*\n\{', source, re.M))
        assert len(matches) == 1, signature
        start = matches[0].start()
        parts.append(source[start:source.index('\n}', start) + 2])
    return parts


parts = functions('core/text_util.c', ['void text_to_lower(', 'char *str_find_last('])
parts += functions('ui/misc/skin_mgr.c', ['static void push_string(', 'static void drop_extension(',
                                          'void skin_mgr_scan('])
output.mkdir(parents=True, exist_ok=True)
(output / 'skin_scan.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
