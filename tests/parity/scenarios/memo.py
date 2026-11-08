"""The memo window (/memo opens it instead of recording a memo point at once): an Acolyte's
Warp Portal slots, a point recorded in slot 0 and again in slot 1."""
from common import scenario

FRAME = 'UIFrameWnd'                  # the memo window
SLOT = lambda n: 769 + n              # its "Memo here" buttons

s = scenario('parity23')
s.say('@job 4', then=60).say('@allskill', then=40)
s.baseline().say('/memo', then=60).snap('memo_window')
s.click_ui(f'{FRAME}/UIBasicButton#{SLOT(0)}', then=60).snap('memo_slot_0')
s.click_ui(f'{FRAME}/UIBasicButton#{SLOT(1)}', then=60).snap('memo_slot_1')
s.close_new()
s.quit()
STEPS = s.steps
