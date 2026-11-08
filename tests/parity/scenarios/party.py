"""A party: made in the party window's create dialog (the EXP share option), ParityBot invited
from its menu and joining, the party window and the members' plates."""
from common import scenario

BOT = 'social:Parity20'
BOT_AT = (472, 318)                   # ParityBot stands two cells west of the spawn cell
GROUP = 'UIMessengerGroupWnd'
SETTING = 'UIPartySettingWnd'
CREATE = 437                          # the party window's Create Party button
INVITE_ROW = 0.265                    # "Ask (ParityBot) to join your party" in the player menu
MEMBER_ROW = lambda n: (376, 232 + 34 * n)   # the party window's members, opened at 300,200
PLATE_AT = (800, 560)                 # where a member dragged out of the party window is dropped



s = scenario('parity20')
s.wait(300).snap('bot_here')
s.baseline().key('alt+z', then=60).snap('party_window')
# The party window's buttons press only under a pointer that was already over them.
s.hover_ui(f'{GROUP}/UIBitmapButton#{CREATE}', then=20)
s.click_ui(f'{GROUP}/UIBitmapButton#{CREATE}', then=60).snap('party_create')
s.click_ui(f'{SETTING}/UIEditCtrl', then=10).type('Parity Party', then=10)
s.click_ui(f'{SETTING}/UIRadioBtn', index=1, then=10).snap('party_create_filled')
s.click_ui(f'{SETTING}/UIBitmapButton#176', then=90).snap('party_made')
s.close_new()                                   # the party window stands over ParityBot
s.baseline().click_actor(BOT_AT, button='right', then=30).snap('bot_menu')
s.click_ui('UIMenuWnd', at=(0.3, INVITE_ROW), then=150).snap('bot_joined')
s.close_new()
s.baseline().key('alt+z', then=60).snap('party_of_two')
s.drag(*MEMBER_ROW(1), *PLATE_AT, steps=12, then=60).snap('plate_dragged')
s.close_new()
s.wait(120).snap('plate_stays')
s.quit()
STEPS = s.steps
