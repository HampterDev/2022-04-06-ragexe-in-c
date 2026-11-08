"""Other players (ParityBot, the parity server's bot, which answers whatever is asked of it):
its whisper and coloured chat line received, the right-click menu on it (a friend request, a
trade it accepts and cancels, a 1:1 chat it answers, its equipment), the messenger window."""
from common import scenario

BOT = 'social'
BOT_AT = (472, 318)                   # ParityBot stands two cells west of the spawn cell
CHAT = (300, 715)                     # the chat history: a click here closes a menu harmlessly
# the player menu's rows, as fractions of its height
CHECK_EQUIPMENT, REQUEST_DEAL, ONE_TO_ONE, ADD_FRIEND = 0.074, 0.19, 0.35, 0.47




def menu(s, row, name, then=120):
    s.baseline().click_actor(BOT_AT, button='right', then=30).snap(f'{name}_menu')
    s.click_ui('UIMenuWnd', at=(0.3, row), then=then).snap(name)
    return s


s = scenario('parity08')
s.wait(300).snap('whisper_received')
menu(s, ADD_FRIEND, 'friend_request', then=180).close_new()
menu(s, REQUEST_DEAL, 'trade', then=200).wait(500).snap('trade_cancelled').close_new()
menu(s, ONE_TO_ONE, 'one_to_one', then=60)
s.text('hi bot').key('enter', then=200).snap('one_to_one_answered').close_new()
menu(s, CHECK_EQUIPMENT, 'bot_equipment', then=120).close_new()
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#191', then=60).snap('messenger').close_new()
s.baseline().key('alt+z', then=60).snap('alt_z').close_new()
s.click(*CHAT, then=20)
s.quit()
STEPS = s.steps
