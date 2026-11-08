"""Markets and drops: a Poring killed for its drops' beams, and ParityBot's stall found with the
Universal Catalog's store search by an item name typed in."""
from common import scenario


BOT = 'vendor'
PORING = 1002
CENTRE = (512, 344)
INV_SLOT = lambda n: (55 + 32 * (n % 6), 37 + 32 * (n // 6))   # the inventory's grid, opened at 0,0
SEARCH = 'UISerachStoreInfoWnd'


def frames(s, name, count, step):
    for n in range(count):
        s.wait(step).snap(f'{name}_{n}')
    return s


s = scenario('parity36')
s.say('@item 12580 1', then=30).say('@allstats 99', then=30)
s.say('@beamscli include 909', then=30)         # the beams plugin lights Jellopy drops for this player
s.say('@monster poring 1', then=150).snap('poring')
s.click_actor(CENTRE, job=PORING, then=0)
frames(s, 'drops', 4, 30)

s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40)
s.click(*INV_SLOT(0), double=True, then=90).snap('store_search')
s.click_ui(f'{SEARCH}/UIEditCtrl', then=10).type('Red Potion', then=10).snap('search_typed')
s.click_ui(f'{SEARCH}/UIBitmapButton#357', then=90).snap('search_results')
s.click_ui(f'{SEARCH}/UIBitmapButton#193', then=30).close_new()
s.quit()
STEPS = s.steps
