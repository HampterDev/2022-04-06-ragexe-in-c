"""The map name banner on a screen wider than 2048 px: 2560x1440 picked from the resolution list by
typing into it and applied live (parity.dll keeps the device windowed and its window off every monitor,
so the monitor is never switched), then a warp to Geffen, whose banner is built at the new width and
snapped while it shows, and 1024x768 restored. The resolution list is the adapter's: the machine
that records needs a 2560x1440 mode."""
from common import scenario


SETTINGS = 'UIGraphicSettingWnd'
GRAPHICS_TAB = (0.12, 0.5)
RESOLUTION = f'{SETTINGS}/UINoneVisibleButton#507'
APPLY, CLOSE, OK = 263, 193, 179


def open_settings(s, name):
    s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#185', then=30)
    s.click_ui('UIEscOptionWnd/UIBitmapButton#450', then=40)
    return s.click_ui(f'{SETTINGS}/UITabControl', at=GRAPHICS_TAB, then=30).snap(f'{name}_settings')


def resolution(s, typed, name):
    s.click_ui(RESOLUTION, then=30)
    s.type(typed, then=20).key('enter', then=20).snap(f'{name}_picked')
    s.click_ui(f'{SETTINGS}/UIBitmapButton#{APPLY}', then=40).snap(f'{name}_asks')
    s.click_ui(f'UIMessageBox/UIBitmapButton#{OK}', then=120).snap(name)
    # The settings window stands over the Esc menu, so it goes first.
    s.click_ui(f'{SETTINGS}/UIBitmapButton#{CLOSE}', then=20)
    return s.close_new()


s = scenario('parity44')
open_settings(s, 'wide')
resolution(s, '2560', 'res_2560x1440')
# After the settings window the first Enter is taken by nothing; the second opens the chat input, and a
# click on it takes the keys.
s.key('enter', then=30).key('enter', then=30)
s.click_ui('UINewChatWnd/UIChatEditCtrl', then=10)
s.type('@warp geffen 119 59').key('enter', then=120).snap('geffen_banner_120')
s.wait(60).snap('geffen_banner_180')
s.wait(60).snap('geffen_banner_240')

open_settings(s, 'last')
resolution(s, '1024', 'res_1024x768')
s.quit()
STEPS = s.steps
