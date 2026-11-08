"""Skills (parity11's Mage, its skills on F1..F9 by the parity server): target, ground and self
skills cast from the hotbar at Porings and the ground, and the casting commands around them:
quick cast and its aiming, cursor and snap modes, the cast area outline, the target indicator,
and Blink (the server's custom skill 3100)."""
from common import scenario

PORING = 1002
CENTRE = (512, 344)
GROUND = (400, 460)                   # open ground left of the character, away from the parity NPCs
FIRE_BOLT, FIRE_WALL, THUNDER_STORM, NAPALM_BEAT, SIGHT, SAFETY_WALL, FROST_DIVER, SOUL_STRIKE, BLINK = (
    'f1', 'f2', 'f3', 'f4', 'f5', 'f6', 'f7', 'f8', 'f9')



def cast_at_poring(s, key, name, then=120):
    s.key(key, then=10).click_actor(CENTRE, job=PORING, then=then).snap(name)


def cast_on_ground(s, key, name, then=120):
    s.key(key, then=10).hover(*GROUND, then=10).snap(name + '_aiming').click(*GROUND, then=then).snap(name)


s = scenario('parity11')
s.say('@monster poring 4', then=90).snap('porings')
cast_at_poring(s, FIRE_BOLT, 'fire_bolt')
cast_at_poring(s, NAPALM_BEAT, 'napalm_beat')
cast_on_ground(s, FIRE_WALL, 'fire_wall')
cast_on_ground(s, SAFETY_WALL, 'safety_wall')
s.key(SIGHT, then=90).snap('sight')
s.say('/castarea', then=20)
cast_on_ground(s, THUNDER_STORM, 'thunder_storm_castarea')
s.say('/castarea', then=20)
s.say('/quickcast', then=20)
s.hover(*GROUND, then=10).key(THUNDER_STORM, then=120).snap('quickcast_ground')
s.say('/quickcastaim', then=20).hover(*GROUND, then=10).key(THUNDER_STORM, then=120).snap('quickcastaim_ground')
s.say('/quickcastaim', then=20)
s.say('/quickcastcursor', then=20).key(FROST_DIVER, then=10).snap('quickcastcursor').key('esc', then=30)
s.say('/quickcastcursor', then=20)
s.say('/quickcastsnap', then=20).hover(*CENTRE, then=10).key(SOUL_STRIKE, then=120).snap('quickcastsnap')
s.say('/quickcastsnap', then=20).say('/quickcast', then=20)
s.say('/targetindicator', then=20)
cast_at_poring(s, FROST_DIVER, 'targetindicator_frost_diver')
s.say('/targetindicator', then=20)
cast_on_ground(s, BLINK, 'blink', then=150)
s.say('@killmonster', then=240).snap('cleared')
s.quit()
STEPS = s.steps
