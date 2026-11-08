"""The emotion list (Alt+L) and the chat macro window (Alt+M): emotions picked from the list with a double
click (each sent to the server and shown over the player), two macros typed (a line and an emotion
command), the macro window's emotion list button, then each macro sent with its Alt+number key."""
from common import scenario



LIST = 'UIEmotionListWnd'
MACROS = 'UIEmotionWnd'

s = scenario('parity45')
s.baseline().key('alt+l', then=40).snap('emotion_list')
for n in (0, 4, 7, 12, 19, 28):
    s.click_ui(f'{LIST}/UISpriteButton#275', index=n, double=True, then=50).snap(f'emotion_{n}')
s.key('alt+l', then=40).snap('emotion_list_closed')
s.baseline().key('alt+m', then=40).snap('macro_window')
s.click_ui(f'{MACROS}/UIEditCtrl2', index=0, then=10).type('hello from macro one', then=10)
s.click_ui(f'{MACROS}/UIEditCtrl2', index=1, then=10).type('/heh', then=10).snap('macro_typed')
s.click_ui(f'{MACROS}/UIBitmapButton#275', then=40).snap('macro_list_button')
s.close_new()
s.key('alt+1', then=60).snap('macro_one_sent')
s.key('alt+2', then=60).snap('macro_two_sent')
s.quit()
STEPS = s.steps
