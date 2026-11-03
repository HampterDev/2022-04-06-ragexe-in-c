"""Extract both <URL> link handlers (text viewer and tagged text viewer) for one test.

Writes url_link_handlers.inc with the text viewer's prefix and its on_link
dispatch, plus the tagged viewer's URL handler, and url_link_helper.inc with
src/ui/controls/url_link.c when that file exists (it is empty otherwise, so the
same test also runs against sources from before the shared helper).
"""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])


def functions(source, signatures):
    parts = []
    for signature in signatures:
        matches = list(re.finditer(r'^' + re.escape(signature) + r'[^{;]*\n\{', source, re.M))
        assert len(matches) == 1, signature
        start = matches[0].start()
        parts.append(source[start:source.index('\n}', start) + 2])
    return parts


viewer = (root / 'src/ui/controls/text_viewer_links.c').read_text(encoding='utf-8')
parts = [viewer[:viewer.index('\nstatic int link_arg_number(')]]
parts += functions(viewer, ['static int link_arg_number(', 'static void navi_link(',
                            'static void item_link(', 'static void on_link('])
tagged = (root / 'src/ui/quests/tag_text_viewer.c').read_text(encoding='utf-8')
parts.append('#include "ui/quests/tag_text_viewer.h"')
parts += functions(tagged, ['static unsigned link_arg_count(', 'static struct TextView link_arg(',
                            'void ui_tag_text_viewer_on_url_link('])
output.mkdir(parents=True, exist_ok=True)
(output / 'url_link_handlers.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
helper = root / 'src/ui/controls/url_link.c'
(output / 'url_link_helper.inc').write_text(
    helper.read_text(encoding='utf-8') if helper.exists() else '', encoding='utf-8')
