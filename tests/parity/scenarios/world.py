"""Moving about Prontera: the NPCs around the spawn point clicked and talked through, then
clicks to walk in every direction, a walk with the button held, the camera turned
(right-drag) and zoomed (wheel), sitting, and the walking commands with the clicks they
change."""
from common import scenario

CENTRE = (512, 384)
NPCS = [(300, 165), (447, 195), (478, 135), (580, 125), (612, 195), (760, 175), (850, 55)]
WALKS = [(512, 250), (700, 384), (512, 560), (330, 384), (620, 300), (400, 470)]
WALL = (90, 470)                      # the lamp post's base: not walkable
SPAWN_WARP = '@warp prontera 166 186'  # the seeded character's cell: every NPC is clicked from it


s = scenario('parity04')
s.snap('start')
for i, (x, y) in enumerate(NPCS):
    s.say(SPAWN_WARP, then=150).baseline()
    s.click(x, y, then=150).snap(f'npc_{i}')
    s.converse(f'npc_{i}_talk', rounds=10)
    s.close_new().snap(f'npc_{i}_done')
s.say(SPAWN_WARP, then=150)
for i, (x, y) in enumerate(WALKS):
    s.click(x, y, then=90).snap(f'walk_{i}')
s.drag(512, 384, 700, 300, steps=40, then=60).snap('held_walk')
s.drag(512, 384, 650, 384, button='right', steps=20, then=30).snap('camera_turned')
s.drag(512, 384, 380, 384, button='right', steps=20, then=30).snap('camera_back')
s.wheel(*CENTRE, 3, then=30).snap('zoom_in')
s.wheel(*CENTRE, -6, then=30).snap('zoom_out')
s.wheel(*CENTRE, 3, then=30).snap('zoom_back')
s.key('insert', then=40).snap('sitting')
s.key('insert', then=40).snap('standing')
s.say('/hold').click(*WALKS[0], then=60).snap('hold_click').say('/hold')
s.say('/showpath').click(*WALKS[1], then=10).snap('showpath_walking').wait(80).say('/showpath')
s.say('/altpathing').click(*WALKS[2], then=90).snap('altpathing_walk').say('/altpathing')
s.say('/refuseblocked').click(*WALL, then=60).snap('refuseblocked_click').say('/refuseblocked')
s.say('/ignoreblocked').click(*WALL, then=60).snap('ignoreblocked_click').say('/ignoreblocked')
s.click(*WALL, then=60).snap('blocked_click')
s.quit()
STEPS = s.steps
