"""Links in text: the Advanced item info file (/iteminfo), whose descriptions end in a database <URL> link,
and an item's description opened from the inventory, its link right-clicked and hovered; then the parity
server's @paritylinks dialog, whose page holds an <ITEM>, a <NAVI>, a <URL> and a <TIPBOX> link: the item
link opens the item's description, the tip link the tip box, the place link the navigation, and the item
link right-clicked. A <URL> link is never clicked: it opens the system's browser. The Basic file is loaded
again at the end."""
from common import scenario


INV_SLOT_0 = (55, 37)           # the inventory's first slot, opened at 0,0
DESC = 'UIItemCollectionWnd/UITextViewer'
LINKS = 'UISayDialogWnd/UITagTextViewer'
URL, NAVI, ITEM, TIPBOX = 0x1ad, 0x1ae, 0x1c8, 0x1fd

s = scenario('parity49')
s.say('/iteminfo', then=60).snap('advanced')
s.say('@item 501 1', then=30)
s.click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40)
s.baseline().click(*INV_SLOT_0, button='right', then=60).snap('description')
s.hover_ui(f'{DESC}/UITextButton#{URL}', then=30).snap('description_link_hover')
s.click_ui(f'{DESC}/UITextButton#{URL}', button='right', then=40).snap('description_link_rclick')
s.close_new()
s.baseline().say('@paritylinks', then=60).snap('dialog')
s.click_ui(f'{LINKS}/UITextButton#{ITEM}', then=60).snap('dialog_item')
s.click_ui('UIItemCollectionWnd/UIBitmapButton#193', then=30).snap('dialog_item_closed')
s.click_ui(f'{LINKS}/UITextButton#{TIPBOX}', then=60).snap('dialog_tip')
s.click_ui('UITipboxWnd/UIBmpTextButton#193', then=30).snap('dialog_tip_closed')
s.click_ui(f'{LINKS}/UITextButton#{NAVI}', then=60).snap('dialog_navi')
s.click_ui(f'{LINKS}/UITextButton#{ITEM}', button='right', then=40).snap('dialog_item_rclick')
s.converse('dialog_close', rounds=1)
s.close_new()
s.say('/iteminfo', then=60).snap('basic')
s.quit()
STEPS = s.steps
