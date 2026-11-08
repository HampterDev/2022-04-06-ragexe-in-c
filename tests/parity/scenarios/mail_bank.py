"""The bank (a deposit, a withdrawal past the balance refused) and the mail (RODEX: a letter to
ParityBot with zeny, whose fee the TAX row shows, and an item from the inventory, sent)."""
from common import scenario

BANK = 'UIBank_NewWnd'
DEPOSIT, WITHDRAW, PLUS_100K = 454, 455, 384
COMPOSE = 'UIRodexComposeWnd'
VALIDATE, SEND = 205, 337
INV_SLOT = lambda n: (55 + 32 * (n % 6), 37 + 32 * (n // 6))   # the inventory's grid, opened at 0,0
MAIL_ITEM_SLOT = (342, 515)                                     # the letter's first item slot


s = scenario('parity18')
s.say('@zeny 200000', then=40).say('@item 501 10', then=40)
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#453', then=60).snap('bank')
s.click_ui(f'{BANK}/UIEditCtrl', then=10).type('50000', then=10).snap('bank_amount')
s.click_ui(f'{BANK}/UIBitmapButton#{DEPOSIT}', then=60).snap('bank_deposited')
s.click_ui(f'{BANK}/UIBitmapButton#{PLUS_100K}', then=20).snap('bank_plus_100k')
s.click_ui(f'{BANK}/UIBitmapButton#{WITHDRAW}', then=60).snap('bank_withdraw_too_much')
s.close_new()
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#468', then=60).snap('mailbox')
s.click_ui('UIRodexInboxWnd/UIBitmapButton#297', then=60).snap('mail_write')
s.click_ui(f'{COMPOSE}/UIEditCtrl_Custom', then=10).type('ParityBot', then=10)
s.click_ui(f'{COMPOSE}/UIBmpTextButton#{VALIDATE}', then=60).snap('mail_recipient')
s.click_ui(f'{COMPOSE}/UIEditCtrl_Custom', index=1, then=10).type('Parity mail', then=10)
s.click_ui(f'{COMPOSE}/UIRodexEditCtrl3', then=10).type('A letter for the bot.', then=10)
s.click_ui(f'{COMPOSE}/UIEditCtrl_Currency', then=10).type('1000', then=20).snap('mail_zeny_fee')
s.click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40)
s.drag(*INV_SLOT(0), *MAIL_ITEM_SLOT, steps=12, then=40).type('3', then=10).key('enter', then=40).snap('mail_item')
s.click_ui(f'{COMPOSE}/UIBasicButton#{SEND}', then=90).snap('mail_sent')
s.close_new()
s.quit()
STEPS = s.steps
