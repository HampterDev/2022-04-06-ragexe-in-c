"""Walking where the ground is awkward: the pointer and a click on the wall between two cells of
different heights (Aldebaran, a 20-unit step at 211..212,71: the higher cell is picked), and a
walk onto the cell the character's pet stands on."""
from common import scenario


PET_CELL = (576, 462)                   # the pet beside the character (the pick list leaves pets out)


s = scenario('parity35')
s.say('@warp aldebaran 209 71', then=240).snap('aldebaran')
for n, x in enumerate(range(560, 680, 20)):
    s.hover(x, 384, then=6).snap(f'wall_pointer_{n}')
s.click(620, 384, then=90).snap('wall_click')

s.say('@warp prontera 156 186', then=240)
s.say('@makeegg 1002', then=60).say('@hatch', then=60)
s.click_ui('UIPetEggListWnd', at=(0.3, 0.21), then=20).click_ui('UIPetEggListWnd/UIBitmapButton#176', then=150)
s.snap('pet_out')
s.click(*PET_CELL, then=90).snap('walked_onto_pet')
s.quit()
STEPS = s.steps
