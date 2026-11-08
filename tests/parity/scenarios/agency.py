"""The Adventurer's Agency: its board's level boxes (kept under the cap), the category drop-down
(typed into while open), a job slot, a search and its reset."""
from common import scenario

BOARD = 'UIAdvenPartyBoardWnd'
SEARCH, RESET = 357, 355


s = scenario('parity21')
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#536', then=40).hover(600, 600, then=40).snap('agency')
s.click_ui(f'{BOARD}/UIEditCtrl2', then=10).type('999', then=10)
s.click_ui(f'{BOARD}/UIEditCtrl2', index=1, then=10).type('5', then=20).snap('levels_capped')
s.click_ui(f'{BOARD}/UIComboBoxDef2Wnd/UIBitmapButton#265', then=20).snap('category_open')
s.type('d', then=20).snap('category_typed_d')     # Dungeon
s.type('f', then=20).snap('category_typed_df')    # "df" names nothing, so Dungeon stays
s.wait(120).type('f', then=20).snap('category_typed_f')   # a fresh search once the typing pauses: Field
s.key('enter', then=20).snap('category_picked')
s.click_ui(f'{BOARD}/UIJobSlotButton', then=30).snap('job_slot')
s.click_ui(f'{BOARD}/UIEditCtrl5', then=10).type('test', then=10)
s.click_ui(f'{BOARD}/UIBmpTextButton#{SEARCH}', then=60).snap('searched')
s.click_ui(f'{BOARD}/UIBmpTextButton#{RESET}', then=40).snap('reset')
s.close_new()
s.quit()
STEPS = s.steps
