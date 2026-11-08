"""Item links in chat: the map server's @iteminfo and @mobinfo lines carry item links (the row under
each link drawn in the link's colour), and a link hovered."""
from common import scenario

FIRST_LINK = (55, 693)                 # <Santa's Bag>, the first link of @mobinfo's drop lines

s = scenario('parity38')
s.say('@iteminfo jellopy', then=40).snap('iteminfo_link')
s.say('@mobinfo poring', then=60).snap('mobinfo_links')
s.hover(*FIRST_LINK, then=30).snap('link_hovered')
s.quit()
STEPS = s.steps
