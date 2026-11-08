"""A buying store: the license used from the inventory, its setup window (the weight footer),
two items listed with amounts and prices, the footer's weight turning red past 90% with OK
disabled, the amount fixed, the purchase limit set, and the store opened."""
from common import scenario

INV_SLOT = lambda n: (55 + 32 * (n % 6), 37 + 32 * (n // 6))   # the inventory's grid, opened at 0,0
MIRROR_SLOT = lambda n: (56 + 32 * n, 156)      # "Available items", opened at 0,120
STORE_ROW = lambda n: (40, 314 + 42 * n)        # "Buying Store Window", opened at 0,240
SHOP = 'UIMerchantShopMakeWnd'
# its boxes: each row's amount and price, then the purchase zeny limit
AMOUNT, PRICE, LIMIT = (lambda n: 2 * n), (lambda n: 2 * n + 1), 4




def box(s, index, text):
    s.click_ui(f'{SHOP}/UIEditCtrlWheel', index=index, then=10).key('end', then=2)
    for _ in range(10):
        s.key('back', then=1)
    return s.type(text, then=10)


s = scenario('parity24')
for item in (12548, 6377, 909, 512):
    s.say(f'@item {item} 1', then=30)
s.say('@zeny 100000', then=30)
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40).snap('inventory')
s.click(*INV_SLOT(0), double=True, then=90).snap('store_setup')
s.click_ui(f'{SHOP}/UIEditCtrl', then=10).type('Parity buys', then=10)
s.drag(*MIRROR_SLOT(0), *STORE_ROW(0), steps=12, then=40).snap('store_jellopy')
s.drag(*MIRROR_SLOT(0), *STORE_ROW(1), steps=12, then=40).snap('store_apple')   # the list closes up
box(s, AMOUNT(0), '9999')
box(s, PRICE(0), '10')
box(s, AMOUNT(1), '5')
box(s, PRICE(1), '20').snap('store_over_weight')
s.click_ui(f'{SHOP}/UIBitmapButton#176', then=60).snap('store_ok_disabled')
box(s, AMOUNT(0), '10').snap('store_fixed')
box(s, LIMIT, '50000').snap('store_limit')
s.click_ui(f'{SHOP}/UIBitmapButton#176', then=120).snap('store_open')
s.quit()
STEPS = s.steps
