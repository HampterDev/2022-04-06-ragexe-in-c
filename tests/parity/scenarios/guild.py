"""Guilds: a guild founded with a space in its name (@guild), its window (wider than stock) on
each of its tabs, with the castle list under the Castles Owned line, and its notice written."""
from common import scenario

# The tabs (Guild Info, Member, Position, Guild Skill, Expel List, Announcement) are windows of
# their own classes, opened in the same place (0,34) with the same strip of tabs.
TAB = lambda n: (34 + 66 * n, 63)
NOTICE_BODY, NOTICE_EDIT = (220, 214), (425, 335)   # the Announcement tab's contents and its edit button


s = scenario('parity17')
s.say('@guild Parity Guild', then=90).snap('founded')
s.baseline().key('alt+g', then=60).snap('guild_info')
for n, name in enumerate(('info', 'member', 'position', 'skill', 'expel', 'announcement')):
    s.click(*TAB(n), then=40).snap(f'guild_tab_{name}')
s.type('Parity notice', then=10).click(*NOTICE_BODY, then=10).type('Hello guild', then=10).snap('notice_typed')
s.click(*NOTICE_EDIT, then=40).snap('notice_edit')     # the button becomes "send"
s.click(*NOTICE_EDIT, then=90).snap('notice_sent')
s.close_new()
s.quit()
STEPS = s.steps
