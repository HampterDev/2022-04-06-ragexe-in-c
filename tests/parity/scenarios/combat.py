"""Fighting: a levelled Swordman, Porings spawned next to it and attacked by clicking, with the
damage numbers hidden and shown (/nodmg), the outline under the pointer (/targetoutline) and
/targetlock; an MVP spawned (the character hidden) for its aura (/mvpaura); the map cleared."""
from common import scenario

PORING = 1002
CENTRE = (512, 344)


s = scenario('parity07')
s.say('@baselvl 49', then=40).say('@joblvl 9', then=30).say('@job 1', then=60).say('@allstats 99', then=40)
s.snap('swordman')
s.say('@monster poring 3', then=90).snap('porings')
s.say('/targetoutline', then=20)
s.hover(*CENTRE, then=10)
s.click_actor(CENTRE, job=PORING, then=150).snap('attack_1')
s.say('/nodmg', then=20)
s.click_actor(CENTRE, job=PORING, then=150).snap('attack_nodmg')
s.say('/nodmg', then=20)
s.say('/dmgfilter', then=20)
s.click_actor(CENTRE, job=PORING, then=150).snap('attack_dmgfilter')
s.say('/dmgfilter', then=20)
s.say('/targetlock', then=20).say('@monster poring 2', then=90)
s.click_actor(CENTRE, job=PORING, then=60).snap('targetlock_attack')
s.click(*CENTRE, then=90).snap('targetlock_ground_click')
s.say('/targetlock', then=20).say('/targetoutline', then=20)
s.say('@hide', then=30).say('@monster 1039 1', then=120).snap('mvp_aura')
s.say('/mvpaura', then=30).snap('mvp_aura_off').say('/mvpaura', then=30)
s.say('@killmonster', then=60).say('@hide', then=240).snap('cleared')
s.quit()
STEPS = s.steps
