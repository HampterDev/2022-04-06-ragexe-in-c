"""Party plates with ParityBot: the bot whispers, joins the party, and its plate (dragged out of the
party window) shows its buffs as badges with their times running down; the bot logs out, its
plate stays; the character goes back to character selection and in again, the map server
replays the whisper, and the offline member is dragged out of the party window to a plate."""
from common import GAME_START, scenario


BOT = 'social:Parity33:75'            # the bot logs out 75 seconds after it logs in
BOT_AT = (472, 318)                   # ParityBot stands two cells west of the spawn cell
INVITE_ROW = 0.265                    # "Ask (ParityBot) to join your party" in the player menu
MEMBER_ROW = lambda n: (376, 232 + 34 * n)   # the party window's members, opened at 300,200
PLATE_AT = (800, 560)
SC_BLESSING, SC_INC_AGI = 30, 32


def frames(s, name, count, step):
    for n in range(count):
        s.wait(step).snap(f'{name}_{n}')
    return s


s = scenario('parity33')
s.say('@chathistory', then=30)                  # the map server replays the chat history at login
s.wait(270).snap('whisper_received')
s.baseline().say('@party Plate Party', then=60).snap('party_made')
s.close_new()                                   # the party window it opens stands over the bot
s.baseline().click_actor(BOT_AT, button='right', then=30).snap('bot_menu')
s.click_ui('UIMenuWnd', at=(0.3, INVITE_ROW), then=150).snap('bot_joined').close_new()
s.baseline().key('alt+z', then=60)
s.drag(*MEMBER_ROW(1), *PLATE_AT, steps=12, then=60).close_new()
s.say(f'@paritysc {SC_BLESSING} 60000 10 0 ParityBot', then=20)
s.say(f'@paritysc {SC_INC_AGI} 30000 10 0 ParityBot', then=40).snap('plate_buffs')
frames(s, 'plate_buff_times', 3, 120)
s.at(4900).snap('bot_gone')                      # past the bot's 75 seconds

s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#185', then=30)
s.click_ui('UIEscOptionWnd/UIBitmapButton#363', then=200).snap('char_select')
s.click(*GAME_START, then=300).key('enter', then=60).snap('back_on_map')
s.wait(60).snap('whisper_replayed')
s.baseline().key('alt+z', then=60).snap('party_offline')
s.drag(*MEMBER_ROW(1), *PLATE_AT, steps=12, then=60).snap('offline_plate').close_new()
s.wait(30).snap('offline_plate_kept')
s.quit()
STEPS = s.steps
