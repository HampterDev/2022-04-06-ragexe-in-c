"""The camera on a map whose view is locked (ViewPointTable: Endless Tower holds the tilt at -22
and the zoom within 380..510): Prontera's camera zoomed out and tilted, the tower entered (its
camera starts from the player's zoom, held to its limits), and Prontera again (the player's own
camera back, not the tower's)."""
from common import scenario

CENTRE = (512, 384)

s = scenario('parity34')
s.wheel(*CENTRE, -6, then=30)
s.drag(*CENTRE, 512, 320, button='right', steps=20, mods=('shift',), then=30).snap('prontera_camera')
s.say('@warp e_tower 81 100', then=240).snap('tower_camera')
s.say('@warp prontera 156 186', then=240).snap('prontera_again')
s.quit()
STEPS = s.steps
