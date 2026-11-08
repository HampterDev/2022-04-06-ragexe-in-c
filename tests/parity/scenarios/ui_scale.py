"""The UI scale raised past what the screen holds: the inventory moved to the screen's lower right
and closed, the UI scale raised to 1.33x on the Interface tab (its tip says a 1024x768 window
draws the UI at 1.00x) and applied with its confirmation, then the inventory opened again in its
kept place. The display mode is never changed. A scale that does take effect needs a larger
window, which is the display scenario's (run only while the user is away)."""
from common import scenario


SETTINGS = 'UIGraphicSettingWnd'
INTERFACE_TAB = (0.375, 0.5)
UI_SCALE_SLIDER = 'UIScrollBar'           # the Interface tab's first slider (UI Scale)
APPLY, OK = 263, 179
INV_TITLE = (60, 8)                       # the inventory's title bar, opened at 0,0

s = scenario('parity37')
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40)
s.drag(*INV_TITLE, 760, 480, steps=16, then=30).snap('inventory_moved').close_new()

s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#185', then=30)
s.click_ui('UIEscOptionWnd/UIBitmapButton#450', then=40)
s.click_ui(f'{SETTINGS}/UITabControl', at=INTERFACE_TAB, then=30)
s.click_ui(f'{SETTINGS}/{UI_SCALE_SLIDER}', at=(0.9, 0.5), then=20)
s.click_ui(f'{SETTINGS}/{UI_SCALE_SLIDER}', at=(0.9, 0.5), then=20).snap('ui_scale_raised')
s.click_ui(f'{SETTINGS}/UIBitmapButton#{APPLY}', then=40).snap('apply_asks')
s.click_ui(f'UIMessageBox/UIBitmapButton#{OK}', then=90).snap('applied')
s.close_new()
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=60).snap('inventory_after')
s.close_new()
s.quit()
STEPS = s.steps
