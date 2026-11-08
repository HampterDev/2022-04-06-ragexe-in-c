"""Odds and ends of the windows and the screen: an equipment item and an etc item put on the
hotbar, potions on a quick slot with their count, the screen under Illusion (no wobble), the
Game Settings rows' tips on every tab, and the main window's style (its title bar has the system
menu, which every state's window line records)."""
from common import scenario


INV_SLOT = lambda n: (55 + 32 * (n % 6), 37 + 32 * (n // 6))   # the inventory's grid, opened at 0,0
INV_TAB = lambda n: (0.5, (n + 0.5) / 4)                        # Use, Eqp, Etc, Fav
HOTBAR_SLOT = lambda n: (246 + 29 * n, 20)                      # the shortcut bar's first row
QUICK_SLOT = (915, 17)                                          # the quick slot window's first slot
SETTINGS = 'UIGraphicSettingWnd'
TABS = [('graphics', 0.12), ('interface', 0.375), ('gameplay', 0.62), ('audio', 0.87)]
ROW_LABEL = lambda n: (0.2, (60 + 26 * n) / 520)                # a row's label, as a fraction of the window
SC_ILLUSION = 52


def frames(s, name, count, step):
    for n in range(count):
        s.wait(step).snap(f'{name}_{n}')
    return s


s = scenario('parity32')
for item, count in ((1201, 1), (909, 5), (501, 30)):
    s.say(f'@item {item} {count}', then=30)
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40)
s.click_ui('UIItemWnd/UITabControl', at=INV_TAB(1), then=30)
s.drag(*INV_SLOT(0), *HOTBAR_SLOT(0), steps=12, then=30).snap('equipment_on_hotbar')
s.click_ui('UIItemWnd/UITabControl', at=INV_TAB(2), then=30)
s.drag(*INV_SLOT(0), *HOTBAR_SLOT(1), steps=12, then=30).snap('etc_on_hotbar')
s.click_ui('UIItemWnd/UITabControl', at=INV_TAB(0), then=30)
s.drag(*INV_SLOT(0), *QUICK_SLOT, steps=12, then=30).hover(600, 600, then=30).snap('potion_on_quick_slot')
s.close_new()

s.say(f'@paritysc {SC_ILLUSION} 20000', then=10)
frames(s, 'illusion', 3, 20)

s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#185', then=30)
s.click_ui('UIEscOptionWnd/UIBitmapButton#450', then=40).snap('settings')
for tab, at in TABS:
    s.click_ui(f'{SETTINGS}/UITabControl', at=(at, 0.5), then=30)
    for row in range(3):
        s.hover_ui(SETTINGS, at=ROW_LABEL(row), then=50).snap(f'tip_{tab}_{row}')
s.close_new()
s.quit()
STEPS = s.steps
