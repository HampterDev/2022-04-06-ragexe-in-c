"""The tip box (Alt+D): the next and previous tips, the tip search and its list, the "do not show" box;
and the quest window (Alt+U) with no quests: its tabs and its checkbox, then closed."""
from common import scenario


TIPS = 'UITipboxWnd'
QUESTS = 'UIRenewQuestWnd'

s = scenario('parity47')
s.baseline().key('alt+d', then=40).snap('tipbox')
s.click_ui(f'{TIPS}/UIBitmapButton#210', then=20).snap('tip_next')
s.click_ui(f'{TIPS}/UIBitmapButton#210', then=20)
s.click_ui(f'{TIPS}/UIBitmapButton#238', then=20).snap('tip_prev')
s.click_ui(f'{TIPS}/UIEditCtrl4', then=10).type('item', then=10)
s.click_ui(f'{TIPS}/UIBitmapButton#357', then=30).snap('tip_search')
s.click_ui(f'{TIPS}/UICheckBox#205', then=20).snap('tip_dont_show')
s.click_ui(f'{TIPS}/UICheckBox#205', then=20)
s.click_ui(f'{TIPS}/UIBmpTextButton#193', then=30).snap('tipbox_closed')
s.baseline().key('alt+u', then=40).snap('quests')
for n, at in enumerate(((0.5, 0.05), (0.5, 0.2), (0.5, 0.35), (0.5, 0.5))):
    s.click_ui(f'{QUESTS}/UITab', at=at, then=20).snap(f'quest_tab_{n}')
s.click_ui(f'{QUESTS}/UICheckBox#205', then=20).snap('quest_check')
s.click_ui(f'{QUESTS}/UICheckBox#205', then=20)
s.close_new().snap('quests_closed')
s.quit()
STEPS = s.steps
