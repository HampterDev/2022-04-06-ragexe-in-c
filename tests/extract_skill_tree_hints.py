"""Use actual window list/pinning helpers without linking the full UI renderer."""
from pathlib import Path
import sys
import re

root, output = map(Path, sys.argv[1:])
source = (root / 'src/ui/character/skill_tree.c').read_text(encoding='utf-8')
signatures = [
    'int skill_tree_tab_count(',
    'struct List *skill_tree_shown_list(',
    'struct SkillRequirement *skill_tree_level_list_find(',
    'static BYTE in_cells(',
    'static int cell_at(',
    'void ui_new_skill_list_wnd_on_r_btn_up(',
]
parts = []
for signature in signatures:
    matches = list(re.finditer(r'^' + re.escape(signature) + r'[^\n]*\n\{', source, re.M))
    assert len(matches) == 1, signature
    start = matches[0].start()
    parts.append(source[start:source.index('\n}', start) + 2])
points = (root / 'src/ui/character/skill_tree_points.c').read_text(encoding='utf-8')
for signature in ['static void mark_required_tabs(', 'void skill_tree_mark_required(']:
    matches = list(re.finditer(r'^' + re.escape(signature) + r'[^\n]*\n\{', points, re.M))
    if not matches and signature.startswith('static '):
        continue  # The pre-fix source has no helper; run the same regression test.
    assert len(matches) == 1, signature
    start = matches[0].start()
    parts.append(points[start:points.index('\n}', start) + 2])
output.mkdir(parents=True, exist_ok=True)
(output / 'skill_tree_hints_helpers.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
