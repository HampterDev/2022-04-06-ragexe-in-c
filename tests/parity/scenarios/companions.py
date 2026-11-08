"""Companions: an Alchemist's homunculus (its window, each grown stat hovered for its rank and
average, fed, a skill learned and used from the hotbar for its delay), commanded with Alt and a right press on the ground, on a window
(which must not command it) and twice on a Poring (the first press picks it, the second attacks); a mercenary from a scroll, commanded with Alt and a
left press; a Merchant's cart and its window; a Knight's Peco; a pet hatched from an egg; and
the looks toggles for companions and mounts."""
from common import scenario

PORING = 1002
ALCHEMIST = 18
CENTRE = (512, 344)
GROUND = (400, 460)
GROUND_2 = (620, 460)
ON_WINDOW = (50, 50)                                            # the Basic Info window's body
INV_SLOT = lambda n: (55 + 32 * (n % 6), 37 + 32 * (n // 6))   # the inventory's grid, opened at 0,0
HOMUN_SKILL_ICON = (416, 233)                                   # Touch of Heal, the skill list at 360,200
HOTBAR_SLOT = lambda n: (246 + 29 * n, 20)                      # the shortcut bar's first row
STATS = (('str', (0.29, 0.88)), ('agi', (0.60, 0.88)), ('vit', (0.91, 0.88)),
         ('int', (0.29, 0.95)), ('dex', (0.60, 0.95)), ('luk', (0.91, 0.95)))


s = scenario('parity12')
s.say('@baselvl 60', then=30).say('@joblvl 40', then=30).say('@job 18', then=60).say('@allskill', then=40)
s.say('@makehomun 6001', then=120).snap('homunculus')
s.say('@homlevel 20', then=90).say('@homstats', then=30).snap('homstats')
s.say('@item 537 1', then=30)
s.baseline().key('alt+r', then=60).snap('homunculus_window')
for name, at in STATS:
    s.hover_ui('UIHomunInfoWnd', at=at, then=40).snap(f'homun_stat_{name}')
s.click_ui('UIHomunInfoWnd/UIBitmapButton#282', then=40).snap('homun_feed_asked')
s.click_ui('UINormalMB/UIBitmapButton#179', then=60).snap('homun_fed')    # OK
s.click_ui('UIHomunInfoWnd/UIBitmapButton#281', then=60).snap('homun_skills')
for n in range(3):
    s.click_ui('UISkillListWnd/UIBitmapButton#231', then=30)                # Lv UP
s.snap('homun_skill_learned')
s.drag(*HOMUN_SKILL_ICON, *HOTBAR_SLOT(0), steps=12, then=40).snap('homun_skill_on_hotbar')
s.close_new()
s.key('f1', then=20).click_actor(CENTRE, job=ALCHEMIST, then=60).snap('homun_skill_delay')
s.rclick(*GROUND, mods=('alt',), then=24).snap('homun_alt_ground')
s.rclick(*ON_WINDOW, mods=('alt',), then=90).snap('homun_alt_on_window')
s.say('@monster poring 1', then=90)
s.click_actor(CENTRE, job=PORING, button='right', mods=('alt',), lead=8, then=20).snap('homun_alt_target')
s.click_actor(CENTRE, job=PORING, button='right', mods=('alt',), lead=8, then=150).snap('homun_alt_attack')
s.say('@killmonster', then=60)
s.say('/companionlooks', then=30).snap('companionlooks_off').say('/companionlooks', then=30)
s.say('@item 12153 1', then=30)
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40)
s.click(*INV_SLOT(0), double=True, then=120).snap('mercenary').close_new()
s.baseline().key('ctrl+r', then=60).snap('mercenary_window').close_new()
s.click(*GROUND_2, mods=('alt',), then=24).snap('mercenary_alt_ground')
s.say('@monster poring 1', then=90)
s.click_actor(CENTRE, job=PORING, mods=('alt',), lead=8, then=20).snap('mercenary_alt_target')
s.click_actor(CENTRE, job=PORING, mods=('alt',), lead=8, then=150).snap('mercenary_alt_attack')
s.say('@killmonster', then=60)
s.say('@job 5', then=60).say('@cart 1', then=60).snap('cart')
s.baseline().key('alt+w', then=60).snap('cart_window').close_new()
s.say('@job 7', then=60).say('@allskill', then=40).say('@mount', then=60).snap('peco')
s.say('/mountlooks', then=30).snap('mountlooks_off').say('/mountlooks', then=30)
s.say('@mount', then=60)
s.say('@makeegg 1002', then=60)
s.baseline().say('@hatch', then=60).snap('hatch')
s.click_ui('UIPetEggListWnd', at=(0.3, 0.21), then=20).click_ui('UIPetEggListWnd/UIBitmapButton#176', then=120)
s.snap('pet')
s.baseline().key('alt+j', then=60).snap('pet_window').close_new()
s.quit()
STEPS = s.steps
