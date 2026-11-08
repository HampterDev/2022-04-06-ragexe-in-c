"""The Game Settings window (herc-client's settings window): every tab, every checkbox and
slider on it clicked, the list scrolled and swept again, then More commands, Apply and Reset
(each confirmed) and Close.

The Graphics tab's device and resolution drop-downs and its screen row (Fullscreen,
Borderless, Show Taskbar) change the display and are never clicked here: the sweep leaves out
every checkbox on the screen row, which lies one row below the resolution drop-down
(UINoneVisibleButton#507) wherever the list is scrolled."""
from common import scenario
from inputs import find_ui

WND = 'UIGraphicSettingWnd'
TABS = [('graphics', 0.12), ('interface', 0.375), ('gameplay', 0.62), ('audio', 0.87)]
CONTENT = (450, 450)                 # a point over the settings list, for the wheel
ROW = 26                             # the height of one settings row




def not_display_row(node, windows):
    resolution = find_ui(windows, f'{WND}/UINoneVisibleButton#507', shown_only=False)
    if not resolution:
        return True
    rx, ry = resolution[0]['rel'][:2]
    if rx <= -100 or ry <= -100:
        return True                  # not on the Graphics tab
    y = resolution[0]['rect'][1]
    return node['rect'][1] > y + ROW + 12 or node['rect'][1] < y - ROW - 12


s = scenario('parity03')
s.click_ui('UIMenuIconWnd/UIMenuIcon#185', then=30)
s.click_ui('UIEscOptionWnd/UIBitmapButton#450', then=40).snap('opened')
for tab, at in TABS:
    s.click_ui(f'{WND}/UITabControl', at=(at, 0.5), then=30).snap(f'{tab}_tab')
    for page in range(3):
        s.click_each(f'{WND}/UICheckBox', f'{tab}_p{page}_check', per=16, limit=40, where=not_display_row)
        s.click_each(f'{WND}/UIScrollBar', f'{tab}_p{page}_slider', at=((0.25, 0.5), (0.98, 0.5)), per=16,
                     limit=12, where=not_display_row)
        s.wheel(*CONTENT, -6, then=20).snap(f'{tab}_scrolled_{page}')
s.baseline().click_ui(f'{WND}/UIBasicButton#502', then=40).snap('more_commands').close_new()
s.click_ui(f'{WND}/UIBitmapButton#263', then=40).snap('apply_asks')
s.click_ui('UIMessageBox/UIBitmapButton#179', then=40).snap('applied')
s.click_ui(f'{WND}/UIBitmapButton#355', then=40).snap('reset_asks')
s.click_ui('UIMessageBox/UIBitmapButton#179', then=40).snap('reset_done')
s.click_ui(f'{WND}/UIBitmapButton#193', index=1, then=30).snap('closed')
s.quit()
STEPS = s.steps
