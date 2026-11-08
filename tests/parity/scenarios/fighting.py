"""Fighting as a Knight (parity28, Bash on F1, Endure on F2): a bolt's hits taken without and with
Endure (the flinch, the yellow total), a monster's hits while walking, a walk clicked during a
swing (sent at once, no lunge on its first frame), a retarget during a swing, Bash pressed again
during its delay, a wandering Poring chased, an item picked up during a walk and during an
attack, Shift held while the button turns the character, potion heals with /dmgfilter off and
on, a monster's long cast first seen half done, the poison sheet while walking, and chat under
Berserk."""
from common import scenario


PORING = 1002
CENTRE = (512, 344)
LEFT, RIGHT = (330, 330), (700, 330)
WALK_FAR = (512, 600)
GROUND = (420, 470)
SPAWN = '@warp prontera 156 186'
INV_SLOT = lambda n: (55 + 32 * (n % 6), 37 + 32 * (n // 6))   # the inventory's grid, opened at 0,0
SC_POISON, SC_BERSERK = 4, 112


def frames(s, name, count, step):
    for n in range(count):
        s.wait(step).snap(f'{name}_{n}')
    return s


s = scenario('parity28')
s.snap('knight')

# a three-hit bolt from a Poring two cells east: its cast bar, then the hits; then the same with Endure
s.say('@paritybolt 14 3', then=40).snap('bolt_casting')
frames(s, 'bolt_hits', 4, 30)
s.key('f2', then=40).snap('endure')
s.say('@paritybolt 14 3 -2 0', then=40)
frames(s, 'bolt_hits_enduring', 4, 30)
s.say('@parityclear', then=40)

# hits while walking: a Poring attacks, the knight walks away and keeps walking between hits
s.say('@parityattackme', then=90).snap('attacked')
s.click(*WALK_FAR, then=0)
frames(s, 'walk_hit', 6, 12)
s.say('@parityclear', then=30).say(SPAWN, then=150)

# swings: a walk clicked mid-swing, a retarget mid-swing, Bash again during its delay
s.say('@monster poring 3', then=90).snap('porings')
s.click_actor(LEFT, job=PORING, then=14)
s.click(*GROUND, then=0)
frames(s, 'move_during_swing', 4, 4)
s.wait(60).click_actor(LEFT, job=PORING, then=14)
s.click_actor(RIGHT, job=PORING, then=0)
frames(s, 'retarget', 3, 10)
s.wait(60).key('f1', then=4).click_actor(LEFT, job=PORING, then=8).snap('bash')
s.key('f1', then=4).click_actor(LEFT, job=PORING, then=8).snap('bash_during_delay')
s.say('@killmonster', then=30).say('@monster poring 1', then=150)
s.click_actor(CENTRE, job=PORING, then=0)       # a Poring that wanders: the approach aims ahead of it
frames(s, 'chase', 4, 15)
s.say('@killmonster', then=60).say(SPAWN, then=150)

# pick-ups: an item dropped, picked up during a walk, then during an attack
s.say('@item 909 3', then=30)
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40)
s.click_ui('UIItemWnd/UITabControl', at=(0.5, 2.5 / 4), then=30)
s.drag(*INV_SLOT(0), *GROUND, steps=12, then=40).type('1').key('enter', then=60)
s.close_new().snap('dropped')
s.click(*WALK_FAR, then=10).click_actor(GROUND, kind=1, then=0)
frames(s, 'pickup_while_walking', 3, 20)
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40)
s.click_ui('UIItemWnd/UITabControl', at=(0.5, 2.5 / 4), then=30)
s.drag(*INV_SLOT(0), *GROUND, steps=12, then=40).type('1').key('enter', then=40).close_new()
s.say('@monster poring 1', then=60)
s.click_actor(CENTRE, job=PORING, then=14).click_actor(GROUND, kind=1, then=0)
frames(s, 'pickup_while_attacking', 3, 20)
s.say('@killmonster', then=60).say(SPAWN, then=150)

# Shift held: the button held down turns the knight toward the pointer, at most so often
s.drag(512, 250, 700, 344, steps=16, mods=('shift',), then=0).snap('shift_turn_0')
s.drag(700, 344, 512, 500, steps=16, mods=('shift',), then=0).snap('shift_turn_1')
s.drag(512, 500, 330, 344, steps=16, mods=('shift',), then=20).snap('shift_turn_2')

# heals: potions drunk below full HP, the green numbers shown and then filtered out
s.say('@heal -5000', then=30).say('@item 501 5', then=30)
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40)
s.click_ui('UIItemWnd/UITabControl', at=(0.5, 0.5 / 4), then=30)
s.click(*INV_SLOT(0), double=True, then=8).snap('heal_number')
s.say('/dmgfilter', then=20)
s.click(*INV_SLOT(0), double=True, then=8).snap('heal_number_filtered')
s.say('/dmgfilter', then=20).close_new()

# a Poring twenty cells south starts Storm Gust on its own cell; the knight arrives mid-cast
s.say('@parityground 89 10 0 -20 0 -20', then=90)
s.say('@warp prontera 156 174', then=90).snap('cast_seen_midway')
s.wait(90).snap('cast_later')
s.say('@parityclear', then=40).say(SPAWN, then=150)

# poison: the sheet around the knight follows it as it walks
s.say(f'@paritysc {SC_POISON} 20000', then=40).snap('poisoned')
s.click(*GROUND, then=0)
frames(s, 'poisoned_walk', 4, 15)
s.say(SPAWN, then=150)

# Berserk: chat still goes out
s.say(f'@paritysc {SC_BERSERK} 20000', then=40).say('still talking', then=40).snap('berserk_chat')
s.quit()
STEPS = s.steps
