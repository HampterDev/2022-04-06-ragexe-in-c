"""Extract the private-airplane window slot handling and what destroys the window.

From navigation_cmd.c the statements that keep the window in g_windowMgr.private_airplane_wnd
around its modal loop; from modal.c the modal loop; from window_base.c ui_window_destruct.
Usage: ROOT OUT"""
import re
import sys
from pathlib import Path

root, out = Path(sys.argv[1]) / 'src', Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)


def read(name):
    return (root / name).read_bytes().decode('utf-8').replace('\r\n', '\n')


def function(text, pattern, name):
    match = re.search(r'^' + pattern + r'\([^)]*\)\n\{\n.*?^\}\n', text, re.M | re.S)
    if not match:
        sys.exit(name + ' not found')
    return match.group(0)


navigation = read('ui/quests/navigation_cmd.c')
closed = re.search(r'^#define AIRPLANE_RESULT_CLOSED .*\n', navigation, re.M)
slot = re.search(r'^    g_windowMgr\.private_airplane_wnd = wnd;\n(?:    .*\n)*?        g_windowMgr\.private_airplane_wnd = NULL;\n',
                 navigation, re.M)
if not closed or not slot:
    sys.exit('airplane slot statements not found')
modal = read('ui/wndmgr/modal.c')
parts = [
    closed.group(0),
    'static void open_airship_slot(struct UIWindow *wnd)\n{\n' + slot.group(0) + '}\n',
    function(modal, r'int ui_window_mgr_modal_loop', 'ui_window_mgr_modal_loop'),
    function(modal, r'static int run_modal', 'run_modal'),
    function(modal, r'int ui_window_mgr_do_modal', 'ui_window_mgr_do_modal'),
    function(read('ui/controls/window_base.c'), r'void ui_window_destruct', 'ui_window_destruct'),
]
(out / 'airplane_window_slot.inc').write_text('\n'.join(parts), encoding='utf-8')
