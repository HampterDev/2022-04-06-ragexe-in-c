"""Extract text_dc_select_font, the g_3dDevice initializer with its face tables and
get_code_page_from_charset for the face charset test."""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])


def read(name):
    return (root / 'src' / name).read_text(encoding='utf-8')


def function(source, signature):
    matches = list(re.finditer(r'^' + re.escape(signature) + r'[^{;]*\n\{', source, re.M))
    assert len(matches) == 1, signature
    start = matches[0].start()
    return source[start:source.index('\n}', start) + 2]


device = read('render/c3ddevice.c')
start = device.index('#define FACE_GULIM ')
end = device.index('\n};', device.index('struct C3dDevice g_3dDevice = {')) + 3
parts = [device[start:end],
         function(read('platform/charset.c'), 'UINT get_code_page_from_charset('),
         function(read('render/textdc.c'), 'void text_dc_select_font(')]
output.mkdir(parents=True, exist_ok=True)
(output / 'face_charset.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
