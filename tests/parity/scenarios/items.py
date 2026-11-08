"""Items: equipment and consumables from GM commands, the inventory's tabs, equipping by
double-click, the equipment window and every button on it, a potion on the hotbar used
with its key, the item information window, an item dropped on the ground and picked up, and
the item commands (/itembind, /equiponly) around them."""
from common import scenario

INV_SLOT = lambda n: (55 + 32 * (n % 6), 37 + 32 * (n // 6))   # the inventory's grid, opened at 0,0
INV_TAB = lambda n: (0.5, (n + 0.5) / 4)                        # Use, Eqp, Etc, Fav
HOTBAR_SLOT = lambda n: (246 + 29 * n, 20)                      # the shortcut bar's first row
GROUND = (512, 520)


s = scenario('parity06')
for item, count in ((1201, 1), (2301, 1), (2101, 1), (2401, 1), (2501, 1), (501, 30), (601, 5), (512, 20),
                    (4001, 1)):
    s.say(f'@item {item} {count}', then=30)
s.click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40).snap('inventory')
for tab in range(4):
    s.click_ui('UIItemWnd/UITabControl', at=INV_TAB(tab), then=30).snap(f'inventory_tab_{tab}')
s.click_ui('UIItemWnd/UITabControl', at=INV_TAB(1), then=30)
for n in range(5):
    s.click(*INV_SLOT(0), double=True, then=60).snap(f'equipped_{n}')
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#187', then=40).snap('equipment_window')
s.click_each('UIEquipWnd/UIBitmapButton', 'equipment_button', per=60, limit=8,
             where=lambda n, ws: n['id'] not in (193, 194))
s.close_new()
s.click_ui('UIItemWnd/UITabControl', at=INV_TAB(0), then=30)
s.drag(*INV_SLOT(0), *HOTBAR_SLOT(0), steps=12, then=40).snap('potion_on_hotbar')
s.key('f1', then=40).snap('potion_used')
s.say('/itembind', then=20).key('f1', then=40).snap('itembind_f1').say('/itembind', then=20)
s.say('/equiponly', then=20).snap('equiponly').say('/equiponly', then=20)
s.baseline().click(*INV_SLOT(1), button='right', then=60).snap('item_info').close_new()
s.drag(*INV_SLOT(1), *GROUND, steps=12, then=40).text('3').key('enter', then=60).snap('dropped')
s.click_actor(GROUND, kind=1, then=120).snap('picked_up')
s.close_new()
s.quit()
STEPS = s.steps
