"""What the client keeps between sessions: switches turned (/wordkeys off, /hotbarfade and
/chatfade on, /boldnames, /itembind separate, /spbar off), the inventory moved, the camera
zoomed out, then the client left through Exit to Windows (its own exit saves what it keeps) and
started again in the same folder, where every one of them comes back. The golden keeps the registry and the savedata
files each session left."""
from common import scenario


CENTRE = (512, 384)
INV_TITLE = (60, 8)                       # the inventory's title bar, opened at 0,0
SWITCHES = ['/wordkeys', '/hotbarfade', '/chatfade', '/boldnames', '/itembind', '/spbar']

first = scenario('parity39')
for cmd in SWITCHES:
    first.say(cmd, then=20)
first.snap('switched')
first.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40)
first.drag(*INV_TITLE, 700, 420, steps=16, then=30).snap('inventory_moved').close_new()
first.wheel(*CENTRE, -6, then=40).snap('zoomed_out')
first.exit_game()

second = scenario('parity39')
second.snap('second_session')
second.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40).snap('inventory_kept').close_new()
second.type('one two', then=10).key('ctrl+back', then=10).snap('word_keys_still_off')
second.key('enter', then=30)
second.exit_game()

PHASES = [first.steps, second.steps]
