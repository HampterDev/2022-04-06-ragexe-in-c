"""The display settings, applied live (run only while the user is away; parity.dll still keeps the
device windowed and refuses display mode changes, so the monitor is never switched): the inventory
moved near the lower right and closed, the resolution changed to 1280x720 from the resolution
list by typing into it, the inventory opened again in its kept place (its distance to the lower
right kept), 1600x900 with the UI scale raised so that it takes effect, fullscreen on with
Borderless off (the client's exclusive mode at 1600x900: its window and its flags; the harness
keeps the swap chain windowed) and off again, and 1024x768 restored. The resolution list is the
adapter's: on another machine its rows can differ."""
from common import scenario


SETTINGS = 'UIGraphicSettingWnd'
GRAPHICS_TAB, INTERFACE_TAB = (0.12, 0.5), (0.375, 0.5)
RESOLUTION = f'{SETTINGS}/UINoneVisibleButton#507'
FULLSCREEN, BORDERLESS = 0, 1              # the screen row's boxes, the Graphics tab's first two
APPLY, CLOSE, OK = 263, 193, 179
INV_TITLE = (60, 8)                         # the inventory's title bar, opened at 0,0


def open_settings(s, name, tab=GRAPHICS_TAB):
    s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#185', then=30)
    s.click_ui('UIEscOptionWnd/UIBitmapButton#450', then=40)
    return s.click_ui(f'{SETTINGS}/UITabControl', at=tab, then=30).snap(f'{name}_settings')


def apply(s, name):
    s.click_ui(f'{SETTINGS}/UIBitmapButton#{APPLY}', then=40).snap(f'{name}_asks')
    s.click_ui(f'UIMessageBox/UIBitmapButton#{OK}', then=120).snap(name)
    # The settings window stands over the Esc menu, so it goes first.
    s.click_ui(f'{SETTINGS}/UIBitmapButton#{CLOSE}', then=20)
    return s.close_new()


def resolution(s, typed, name):
    s.click_ui(RESOLUTION, then=30)
    s.type(typed, then=20).key('enter', then=20).snap(f'{name}_picked')
    return apply(s, name)


s = scenario('parity43')
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40)
s.drag(*INV_TITLE, 760, 500, steps=16, then=30).snap('inventory_lower_right').close_new()

open_settings(s, 'first')
resolution(s, '1280', 'res_1280x720')
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40).snap('inventory_kept_1280').close_new()

open_settings(s, 'second')
resolution(s, '1600', 'res_1600x900')
open_settings(s, 'scale', INTERFACE_TAB)
s.click_ui(f'{SETTINGS}/UIScrollBar', at=(0.9, 0.5), then=20)
s.click_ui(f'{SETTINGS}/UIScrollBar', at=(0.9, 0.5), then=20).snap('ui_scale_raised')
apply(s, 'ui_scale_1600')
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40).snap('inventory_scaled').close_new()

open_settings(s, 'fullscreen')
s.click_ui(f'{SETTINGS}/UICheckBox', index=BORDERLESS, then=20)
s.click_ui(f'{SETTINGS}/UICheckBox', index=FULLSCREEN, then=20).snap('fullscreen_ticked')
apply(s, 'fullscreen')
open_settings(s, 'windowed')
s.click_ui(f'{SETTINGS}/UICheckBox', index=FULLSCREEN, then=20)
s.click_ui(f'{SETTINGS}/UICheckBox', index=BORDERLESS, then=20)
apply(s, 'windowed_again')

open_settings(s, 'scale_back', INTERFACE_TAB)
s.click_ui(f'{SETTINGS}/UIScrollBar', at=(0.02, 0.5), then=20)
s.click_ui(f'{SETTINGS}/UIScrollBar', at=(0.02, 0.5), then=20)
apply(s, 'ui_scale_back')
open_settings(s, 'last')
resolution(s, '1024', 'res_1024x768')
s.quit()
STEPS = s.steps
