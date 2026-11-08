"""The Shortcut Settings window (Game Options' BM/Shortcut Settings): its four tabs, the
Interface tab's second page with herc's rows (the windows herc adds, Master Bank), Ctrl+J bound
to Master Bank and saved, the key opening the bank, and Escape closing the window."""
from common import scenario


HOTKEYS = 'UIHotKeyWnd'
TAB = lambda n: ((n + 0.5) / 4, 0.5)     # Hotkey Bar, Hotkey Bar 2, Interface, Macros
NEXT_PAGE, OK = 309, 176
KEY_CELL = lambda col, row: ((479, 694)[col], 152 + 20 * row)   # a row's key, the window at 300,100
MASTER_BANK = KEY_CELL(1, 2)


def open_shortcuts(s, name):
    s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#185', then=40)
    return s.click_ui('UIEscOptionWnd/UIBitmapButton#362', then=60).snap(name)


s = scenario('parity25')
open_shortcuts(s, 'shortcut_window')
for n, name in enumerate(('hotkey_bar', 'hotkey_bar_2', 'interface', 'macros')):
    s.click_ui(f'{HOTKEYS}/UITabControl', at=TAB(n), then=30).snap(f'tab_{name}')
s.click_ui(f'{HOTKEYS}/UITabControl', at=TAB(2), then=30)
s.click_ui(f'{HOTKEYS}/UIBitmapButton#{NEXT_PAGE}', then=30).snap('interface_page_2')
s.click(*MASTER_BANK, then=20).snap('master_bank_waiting')
s.key('ctrl+j', then=30).snap('master_bank_bound')
s.click_ui(f'{HOTKEYS}/UIBitmapButton#{OK}', then=30).snap('saved')
s.close_new()                                   # Game Options
s.baseline().key('ctrl+j', then=60).snap('master_bank_key').close_new()
open_shortcuts(s, 'shortcut_window_again')
s.key('esc', then=40).snap('esc_closes')
s.close_new()
s.quit()
STEPS = s.steps
