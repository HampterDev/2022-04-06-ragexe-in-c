"""A Hunter (parity29: a bow, arrows, traps and a falcon; Double Strafe F1, Arrow Shower F2, Ankle
Snare F3, Blitz Beat F4, Spring Trap F5, Claymore Trap F6): the falcon following a walk frame by
frame, Double Strafe's two arrows and their yellow total, Blitz Beat, traps set and clicked (a plain
click walks onto them), Arrow Shower over a trap and a click on it after, Spring Trap at a trap,
an arrow shot at an Ice Wall, and the bow drawn on a Sniper."""
from common import scenario


PORING = 1002
CENTRE = (512, 344)
WALK_DOWN = (512, 600)
TRAP_A, TRAP_B = (600, 430), (420, 430)
EAST = (620, 380)
SPAWN = '@warp prontera 156 186'
SKILL_UNIT = 2                          # the probe's kind for a skill unit (a trap, an Ice Wall's cell)


def frames(s, name, count, step):
    for n in range(count):
        s.wait(step).snap(f'{name}_{n}')
    return s


def aim(s, key, at, name, then=60):
    """A ground skill: its key, the pointer over the cell a few frames, then the click."""
    return s.key(key, then=10).hover(*at, then=10).snap(name + '_aiming').click(*at, then=then).snap(name)


s = scenario('parity29')
s.say('@allskill', then=40).snap('hunter')     # the seeded traps lack their prerequisite skills
s.click(*WALK_DOWN, then=0)
frames(s, 'falcon_walk', 4, 8)
s.say(SPAWN, then=150)

s.say('@monster poring 2', then=90)
s.key('f1', then=10).click_actor(CENTRE, job=PORING, then=0)
frames(s, 'double_strafe', 4, 10)
s.wait(60).key('f4', then=10).click_actor(CENTRE, job=PORING, then=0)
frames(s, 'blitz_beat', 3, 15)
s.say('@killmonster', then=40)

aim(s, 'f3', TRAP_A, 'ankle_snare')
aim(s, 'f6', TRAP_B, 'claymore')
s.click_actor(TRAP_A, kind=SKILL_UNIT, then=60).snap('click_on_trap')
s.say('@warp prontera 156 185', then=150)
aim(s, 'f2', TRAP_B, 'arrow_shower_on_trap', then=40)
s.click_actor(TRAP_B, kind=SKILL_UNIT, then=60).snap('click_after_shower')
s.say('@warp prontera 156 185', then=150)
s.key('f5', then=10).click_actor(TRAP_B, kind=SKILL_UNIT, then=0)
frames(s, 'spring_trap', 3, 15)

s.say('@parityground 87 10 3 0 -2 0', then=150).snap('ice_wall')
s.click_actor(EAST, kind=SKILL_UNIT, then=0)
frames(s, 'arrow_at_ice_wall', 4, 8)
s.say('@parityclear', then=30)

s.say('@job 4012', then=60).snap('sniper_bow')
s.quit()
STEPS = s.steps
