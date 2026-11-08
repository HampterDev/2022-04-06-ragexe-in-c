"""Body palettes by job: parity41's ten characters (server/paritysrv_test.go, seedPalettes), each dyed with
a clothing colour whose palette differs between the job's own family and the family the client's table for
other service types gives it, side by side on the character selection; then in game the Crusader as each
job in turn (a job change keeps the clothing colour), a Crusader's and a Lord Knight's peco, and the
Stylist's clothing colours shown and one applied."""
from common import login
from inputs import Seq

STYLIST_AT = (806, 370)               # Parity Stylist, east of the other parity NPCs
DYEINGER = 122                        # its sprite
FRAME = 'UIFrameWnd'                  # the Stylist window
TAB = lambda n: ((0.14, 0.44, 0.79)[n], 0.5)
CELL = lambda n: (265 + 66 * (n % 8), 230 + 125 * (n // 8))
APPLY = 833

JOBS = [(18, 'alchemist'), (4012, 'sniper'), (4049, 'soul_linker'), (25, 'ninja'), (24, 'gunslinger'),
        (4008, 'lord_knight'), (4015, 'paladin'), (4018, 'stalker'), (4047, 'star_gladiator')]


s = login(Seq(), 'parity41', char_select_capture=True)
s.snap('crusader')
s.say('@allskill', then=40).say('@mount', then=60).snap('crusader_peco').say('@mount', then=60)
for job, name in JOBS:
    s.say(f'@job {job}', then=60).snap(name)
    if job == 4008:
        s.say('@allskill', then=40).say('@mount', then=60).snap('lord_knight_peco').say('@mount', then=60)
s.say('@job 14', then=60).snap('crusader_again')
s.baseline().click_actor(STYLIST_AT, job=DYEINGER, then=90).snap('stylist')
s.click_ui(f'{FRAME}/UITabControl', at=TAB(2), then=40).snap('stylist_clothing_colors')
s.click(*CELL(3), then=30).snap('stylist_color_picked')
s.click_ui(f'{FRAME}/UIBasicButton#{APPLY}', then=90).snap('stylist_applied')
s.close_new()
s.wait(30).snap('dyed_at_stylist')
s.quit()
STEPS = s.steps
