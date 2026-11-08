"""The login server's refusal of a wrong password, twice, each box closed with Enter and the account and
password typed again in the login window it returns to (which keeps neither), then the right password
and on to the map."""
import common
from common import GAME_START
from inputs import Seq


ACCOUNT = 'parity48'
common.CURRENT_ACCOUNT = ACCOUNT
LOGIN = 'UILoginWnd'
ID_FIELD, PASSWORD_FIELD = 1, 0     # the login window's two UIEditCtrl_Custom children, in its order


def retype(s, field, text):
    s.click_ui(f'{LOGIN}/UIEditCtrl_Custom', index=field, then=5).key('end', then=2)
    for _ in range(16):
        s.key('back', then=1)
    return s.text(text, then=4)


s = Seq()
s.at(130).key('enter').at(210).text(ACCOUNT).key('tab').text('wrongpassword').key('enter')
s.wait(90).snap('refused_password')
s.key('enter', then=40).snap('box_closed')
retype(s, ID_FIELD, ACCOUNT)
retype(s, PASSWORD_FIELD, 'stillwrong').key('enter')
s.wait(90).snap('refused_again')
s.key('enter', then=40)
retype(s, ID_FIELD, ACCOUNT)
retype(s, PASSWORD_FIELD, ACCOUNT + 'pw').key('enter')
s.wait(200).snap('char_select')
s.click(*GAME_START).wait(210).key('enter', then=60).snap('on_map')
s.quit()
STEPS = s.steps
