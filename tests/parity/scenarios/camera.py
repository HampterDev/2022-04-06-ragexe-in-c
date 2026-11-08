"""The camera: tilted with Shift and a right drag, zoomed with Ctrl and a right drag, tilted with
Shift and the wheel (the glide, then the band's limit), the tilt reset with Shift and a double
right click, and the black beyond the map's edge."""
from common import scenario

CENTRE = (512, 384)

s = scenario('parity26')
s.snap('start')
s.drag(*CENTRE, 512, 300, button='right', steps=20, mods=('shift',), then=30).snap('tilt_drag')
s.drag(*CENTRE, 512, 460, button='right', steps=20, mods=('ctrl',), then=30).snap('zoom_drag')
s.wheel(*CENTRE, 2, mods=('shift',), then=4).snap('tilt_wheel_gliding')
s.wait(60).snap('tilt_wheel_done')
s.wheel(*CENTRE, 20, mods=('shift',), then=90).snap('tilt_limit')
s.click(*CENTRE, button='right', double=True, mods=('shift',), then=90).snap('tilt_reset')
s.say('@warp prontera 10 200', then=200).snap('map_edge')
s.quit()
STEPS = s.steps
