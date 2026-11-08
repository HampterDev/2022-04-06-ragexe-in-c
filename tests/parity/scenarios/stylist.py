"""The Stylist's window: opened from a Stylist NPC, a look picked on each of its tabs (Hair
Style, Hair Color, Clothing Color), the grid scrolled, Hide headgear, the characters turned, and
the looks applied."""
from common import scenario

STYLIST_AT = (806, 370)               # Parity Stylist, east of the other parity NPCs
DYEINGER = 122                        # its sprite
FRAME = 'UIFrameWnd'                  # the Stylist window, opened at 221,96
TAB = lambda n: ((0.14, 0.44, 0.79)[n], 0.5)
CELL = lambda n: (265 + 66 * (n % 8), 230 + 125 * (n // 8))
GRID = (500, 400)
HIDE_HEADGEAR, TURN, APPLY = 835, 837, 833


s = scenario('parity22')
s.snap('arrived')
s.baseline().click_actor(STYLIST_AT, job=DYEINGER, then=90).snap('stylist')
s.click(*CELL(5), then=30).snap('hair_style_picked')
s.wheel(*GRID, -3, then=30).snap('hair_style_scrolled')
s.click_ui(f'{FRAME}/UITabControl', at=TAB(1), then=40).snap('tab_hair_color')
s.click(*CELL(3), then=30).snap('hair_color_picked')
s.click_ui(f'{FRAME}/UITabControl', at=TAB(2), then=40).snap('tab_clothing_color')
s.click(*CELL(2), then=30).snap('clothing_color_picked')
s.click_ui(f'{FRAME}/UICheckBox#{HIDE_HEADGEAR}', then=30).snap('hide_headgear')
s.click_ui(f'{FRAME}/UIBitmapButton#{TURN}', then=30).snap('turned')
s.click_ui(f'{FRAME}/UIBasicButton#{APPLY}', then=90).snap('applied')
s.close_new()
s.wait(30).snap('new_look')
s.quit()
STEPS = s.steps
