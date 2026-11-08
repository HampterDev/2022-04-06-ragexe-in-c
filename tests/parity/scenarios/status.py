"""The status window's points: allocated and held pending until Apply,
Reset, then allocated and applied (Apply and Reset hidden while nothing is pending)."""
from common import scenario

STATUS = 'UIStatusWnd'
STR_UP, AGI_UP, DEX_UP = 197, 198, 201
APPLY, RESET = 256, 348




def press(s, button, name=None, then=20):
    s.click_ui(f'{STATUS}/UIBitmapButton#{button}', then=then)
    return s.snap(name) if name else s


s = scenario('parity15')
s.say('@baselvl 40', then=60)
s.baseline().key('alt+a', then=60).snap('status_window')
press(s, STR_UP, 'str_pending')
press(s, STR_UP)
press(s, AGI_UP, 'agi_pending')
press(s, RESET, 'reset', then=40)
press(s, DEX_UP)
press(s, DEX_UP, 'dex_pending')
press(s, APPLY, 'applied', then=60)
s.close_new()
s.quit()
STEPS = s.steps
