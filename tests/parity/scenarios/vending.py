"""Vending: ParityBot's stall visited and bought from (the weight row), and the player's own
stall set up from its cart: the client's check of a price over 50 million, the server refusing
takings past the zeny cap (the setup kept), the Stall Setups window, the stall opened and
closed again."""
from common import scenario

BOT = 'vendor'
BOT_ROW = lambda n: (60, 156 + 32 * n)          # "Merchant Shop - ParityBot", opened at 0,120
PURCHASE_DROP = (380, 340)                      # "Buying Items", opened at 280,280
MIRROR_SLOT = lambda n: (56 + 32 * n, 156)      # "Available Items for Vending", opened at 0,120
STALL_ROW = lambda n: (40, 314 + 42 * n)        # "Opening a stall", opened at 0,240
MESSAGE_OK = (0.74, 0.88)                       # UIMessageBox draws its own buttons
SETUP_SLOT = lambda n: (0.3, (84 + 18 * n) / 378)  # Stall Setups' rows
SETUPS = 'UIFrameWnd'
COPY, PASTE, SAVE, APPLY, DELETE = 1025, 1026, 1027, 1028, 1030
SHOP = 'UIMerchantShopMakeWnd'




def price(s, row, text):
    s.click_ui(f'{SHOP}/UIEditCtrlWheel', index=row, then=10).key('end', then=2)
    for _ in range(10):
        s.key('back', then=1)
    return s.text(text, then=10)


def setups_button(s, button, name, then=40):
    return s.click_ui(f'{SETUPS}/UIBasicButton#{button}', then=then).snap(name)


s = scenario('parity13')
s.say('@cart 1', then=60).snap('arrived')
s.baseline().click_ui('UIMerchantShopTitle', then=90).snap('bot_stall')
s.drag(*BOT_ROW(0), *PURCHASE_DROP, steps=12, then=40).snap('bot_drag_potion')
s.text('5').key('enter', then=40).snap('bot_cart_potion')
s.drag(*BOT_ROW(2), *PURCHASE_DROP, steps=12, then=40).snap('bot_cart_shirt')
s.click_ui('UIMerchantItemPurchaseWnd/UIBitmapButton#176', then=60).snap('bot_buy_confirm')
s.click_ui('UIMerchantItemPurchaseMSGWnd/UIBitmapButton#176', then=90).snap('bot_bought')
s.close_new()
s.baseline().key('f1', then=90).snap('shop_setup')
s.click_ui(f'{SHOP}/UIEditCtrl', then=10).text('Parity13 goods', then=10)
s.drag(*MIRROR_SLOT(0), *STALL_ROW(0), steps=12, then=40).snap('stall_drag_potion')
s.text('20').key('enter', then=40).snap('stall_potion')
s.drag(*MIRROR_SLOT(3), *STALL_ROW(1), steps=12, then=40).snap('stall_knife')
price(s, 0, '999999999')
s.click_ui(f'{SHOP}/UIEditCtrlWheel', index=1, then=30).snap('price_check')
s.click_ui('UIMessageBox', at=MESSAGE_OK, then=30)
price(s, 1, '800').snap('stall_filled')
s.click_ui(f'{SHOP}/UIBitmapButton#176', then=90).snap('stall_refused')
price(s, 0, '120').snap('stall_fixed')
s.click_ui(f'{SHOP}/UIBasicButton#552', then=60).snap('stall_setups')
setups_button(s, COPY, 'setups_copy')
s.click_ui(SETUPS, at=SETUP_SLOT(0), then=20)
setups_button(s, SAVE, 'setups_save', then=60)
setups_button(s, APPLY, 'setups_apply', then=60)
setups_button(s, DELETE, 'setups_delete_once', then=30)
setups_button(s, DELETE, 'setups_delete', then=60)
setups_button(s, PASTE, 'setups_paste', then=60)
s.click_ui(f'{SETUPS}/UIBitmapButton#193', then=30)
s.baseline().click_ui(f'{SHOP}/UIBitmapButton#176', then=120).snap('stall_open')
s.click_ui('UIMerchantItemMyShopWnd/UICheckBox#205', then=20).snap('my_shop_notify')
s.baseline().click_ui('UIMerchantItemMyShopWnd/UIBitmapButton#193', then=90).snap('stall_closed')
s.close_new()                                  # the sell history the closed stall leaves
s.quit()
STEPS = s.steps
