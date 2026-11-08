"""Before the map: Escape on the login window back to service select, the login window's
account-type drop-down and Settings button, the character selection, a character made (hair
style, colour, the model turned, female), its deletion reserved (the countdown) and cancelled."""
import common
from inputs import Seq

ACCOUNT = 'parity19'
common.CURRENT_ACCOUNT = ACCOUNT
LOGIN = 'UILoginWnd'
SELECT = 'UINewSelectCharWnd'
MAKE = 'UINewMakeCharWnd'
DELETE = 399                                            # Delete Character, then Cancel Deletion
HAIR_COLOUR = lambda n: (745 + 27 * (n % 5), 490 + 24 * (n // 5))   # the make window's colours, at 115,173


s = Seq()
s.at(120).snap('service_select')
s.key('enter').at(210).snap('login_window')
s.key('esc', then=40).snap('esc_asks')
s.click_ui('UIMessageBox/UIBitmapButton#179', then=60).snap('back_to_service_select')
s.key('enter', then=80).snap('login_again')
s.click_ui(f'{LOGIN}/UIComboBoxDef2Wnd/UIBitmapButton#265', then=20).snap('account_type_open')
s.key('esc', then=20).snap('account_type_closed')
s.baseline().click_ui(f'{LOGIN}/UIBmpTextButton#277', then=60).snap('login_settings').close_new()
s.click_ui(f'{LOGIN}/UIEditCtrl_Custom', index=1, then=10).type(ACCOUNT).key('tab').type(ACCOUNT + 'pw')
s.key('enter', then=220).snap('char_select')
s.click_ui(f'{SELECT}/UISlotBmpButton', index=1, then=60).snap('char_make')
s.click_ui(f'{MAKE}/UIHairStyleButton', index=5, then=20).snap('hair_5')
s.click_ui(f'{MAKE}/UIHairStyleButton', index=20, then=20).snap('hair_late')
s.click(*HAIR_COLOUR(3), then=20).snap('hair_colour')
s.click_ui(f'{MAKE}/UIBitmapButton#219', then=20).snap('turned')
s.click_ui(f'{MAKE}/UIBitmapButton#517', index=1, then=30).snap('female')
s.click_ui(f'{MAKE}/UIEditCtrl2', then=10).type('ParityMade', then=10)
s.click_ui(f'{MAKE}/UIBmpTextButton#176', then=120).snap('made')
s.click_ui(f'{SELECT}/UISlotBmpButton', index=1, then=30)
s.click_ui(f'{SELECT}/UIBmpTextButton#{DELETE}', then=60).snap('delete_reserved')
s.wait(300).snap('delete_countdown_over')
s.click_ui(f'{SELECT}/UIBmpTextButton#{DELETE}', then=60).snap('delete_cancelled')
s.quit()
STEPS = s.steps
