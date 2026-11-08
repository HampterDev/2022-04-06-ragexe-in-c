"""Buying, selling and storing: zeny and items from GM commands, the Parity Shop (buy and
sell, the cart's weight row) and the Parity Storage keeper (server/paritysrv_test.go) next to
the spawn cell: items moved in from the inventory and back out, the search (in lower case),
the type tabs and the filter buttons (each opens a window of its own, closed again)."""
from common import scenario

NPC_JOB = 117                         # the parity NPCs' sprite (4_F_KAFRA1)
SHOP_AT, STORAGE_AT, TALK_AT = (585, 370), (696, 370), (806, 370)
SHOP_ROW = lambda n: (100, 156 + 32 * n)   # a row of the shop's item list, at its item's name
CART = (420, 320)                     # the Buying Items list
INV_SLOT = lambda n: (55 + 32 * (n % 6), 37 + 32 * (n // 6))   # the inventory's grid, opened at 0,0
STORAGE_ROW = lambda n: (430, 240 + 32 * n)                    # the storage window's list
STORAGE_SEARCH = (480, 644)


s = scenario('parity05')
s.say('@zeny 1000000', then=40)
for item, count in ((501, 20), (909, 50), (1201, 1), (2301, 1), (512, 30)):
    s.say(f'@item {item} {count}', then=30)
s.snap('stocked')

s.baseline().click_actor(SHOP_AT, job=NPC_JOB, then=120).snap('shop_open')
s.click_ui('UIChooseSellBuyWnd/UIBitmapButton#216', then=60).snap('buy_open')
s.drag(*SHOP_ROW(0), *CART, steps=12, then=40).snap('buy_drag_potion')
s.text('5').key('enter', then=40).snap('buy_count_potion')
s.drag(*SHOP_ROW(4), *CART, steps=12, then=40).snap('buy_drag_knife')
s.key('enter', then=40).snap('buy_count_knife')
s.click_ui('UIItemPurchaseWnd/UIBitmapButton#176', then=60).snap('bought')
s.close_new()

s.baseline().click_actor(SHOP_AT, job=NPC_JOB, then=120).snap('shop_again')
s.click_ui('UIChooseSellBuyWnd/UIBitmapButton#215', then=60).snap('sell_open')
s.drag(*SHOP_ROW(1), *CART, steps=12, then=40).snap('sell_drag_jellopy')
s.text('10').key('enter', then=40).snap('sell_count_jellopy')
s.click_ui('UIItemSellWnd/UIBitmapButton#176', then=60).snap('sold')
s.close_new()

s.say('@item 909 30', then=30)
s.baseline().click_actor(STORAGE_AT, job=NPC_JOB, then=120).snap('storage_talk')
s.converse('storage', rounds=3)
s.snap('storage_open')
s.click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40).snap('storage_with_inventory')
s.drag(*INV_SLOT(0), *STORAGE_ROW(0), steps=12, then=40).text('10').key('enter', then=40).snap('stored_potions')
s.drag(*INV_SLOT(1), *STORAGE_ROW(0), steps=12, then=40).key('enter', then=40).snap('stored_apples')
s.click_ui('UIItemWnd/UITabControl', at=(0.5, 2.5 / 4), then=30).snap('inventory_etc')
s.drag(*INV_SLOT(0), *STORAGE_ROW(0), steps=12, then=40).text('20').key('enter', then=40).snap('stored_jellopy')
s.click(*STORAGE_SEARCH, then=10).text('red').key('enter', then=40).snap('storage_search_lower')
for _ in range(3):
    s.key('back', then=2)
s.key('enter', then=40).snap('storage_search_cleared')
for i in range(8):
    s.click_ui('UIItemStoreWnd/UITab', at=(0.5, (i + 0.5) / 8), then=30).snap(f'storage_tab_{i}')
s.click_ui('UIItemStoreWnd/UITab', at=(0.5, 0.5 / 8), then=30)
s.click_each('UIItemStoreWnd/UICheckBox', 'storage_filter_on', per=24, limit=10)
s.click_each('UIItemStoreWnd/UICheckBox', 'storage_filter_off', per=24, limit=10)
s.drag(*STORAGE_ROW(0), 150, 150, steps=12, then=40).text('5').key('enter', then=40).snap('taken_back')
s.close_new().snap('storage_closed')
s.quit()
STEPS = s.steps
