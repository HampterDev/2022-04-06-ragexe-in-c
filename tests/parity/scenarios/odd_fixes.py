"""Fixes that ship without a feature flag, each where it shows: a Taekwon's High Jump to another
cell and a walk from where it landed, a headgear worn under the orcish face, a mail from a
stranger opened (no warning), and an Alchemist's own homunculus under the pointer and clicked."""
from common import scenario


JUMP_TO = (620, 460)
WALK_TO = (420, 300)
SC_ORCISH = 137
INV_SLOT = lambda n: (55 + 32 * (n % 6), 37 + 32 * (n // 6))   # the inventory's grid, opened at 0,0
INV_TAB = lambda n: (0.5, (n + 0.5) / 4)                        # Use, Eqp, Etc, Fav
HOMUN = 4                                                       # the probe's kind for a companion

s = scenario('parity40')
s.say('@allskill', then=40)                     # the seeded High Jump lacks its prerequisites
s.key('f1', then=10).hover(*JUMP_TO, then=10).click(*JUMP_TO, then=60).snap('high_jump')
s.click(*WALK_TO, then=90).snap('walk_after_jump')
s.say('@warp prontera 156 186', then=150)

s.say('@item 2220 1', then=30)
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#186', then=40)
s.click_ui('UIItemWnd/UITabControl', at=INV_TAB(1), then=30)
s.click(*INV_SLOT(0), double=True, then=60).close_new()
s.say(f'@paritysc {SC_ORCISH} 30000', then=60).snap('orcish_with_headgear')

s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#468', then=60).snap('mailbox')
s.click_ui('UIRodexInboxWnd/UITextButton#315', then=90).snap('stranger_mail')        # its title
s.close_new()

s.say('@job 18', then=60).say('@makehomun 6001', then=150)
s.click_actor((512, 420), kind=HOMUN, then=30).snap('homun_clicked')
s.quit()
STEPS = s.steps
