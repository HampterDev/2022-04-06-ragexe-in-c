"""herc-client's lookup windows and their controls: each opened by its command, every button,
checkbox and scroll bar on it clicked (the window found as the one the command opened), the rows
it draws hovered and clicked, its search box used where it has one, and everything closed again.
Then the stats window and the command list."""
from common import scenario

WINDOWS = [
    ('mobinfo', '/mobinfo poring', 'poporing'),
    ('whodrops', '/whodrops jellopy', 'apple'),
    ('meter', '/meter', None),
    ('killcount', '/killcount', None),
    ('recipes', '/recipes', 'potion'),
    ('guildlog', '/guildlog', None),
    ('whosell', '/whosell jellopy', 'apple'),
    ('whobuy', '/whobuy', None),
    ('beams', '/beams', None),
]
NOT_CLOSE = lambda n, ws: n['id'] not in (193, 194)
# Rows the windows draw themselves, as fractions of the window: hovered (a tooltip, a map's
# picture) and clicked (an item, directions).
ROWS = {
    'mobinfo': [(0.09, 0.756), (0.09, 0.806), (0.42, 0.806), (0.76, 0.21), (0.76, 0.29)],
    'whodrops': [(0.2, 0.30), (0.2, 0.35), (0.2, 0.40)],
    'recipes': [(0.2, 0.20), (0.2, 0.30), (0.2, 0.40)],
    'killcount': [(0.2, 0.30), (0.2, 0.40)],
}


s = scenario('parity10')
for name, cmd, search in WINDOWS:
    s.baseline().say(cmd, then=60).snap(f'{name}_open')
    s.click_each('@new/UIFrameWnd/UITabControl', f'{name}_tab', at=((0.1, 0.5), (0.3, 0.5), (0.5, 0.5), (0.7, 0.5)),
                 per=30, limit=2)
    s.click_each('@new/UIFrameWnd/UICheckBox', f'{name}_check', per=30, limit=12)
    s.click_each('@new/UIFrameWnd/UIScrollBar', f'{name}_scroll', at=((0.5, 0.9), (0.5, 0.1)), per=20, limit=4)
    s.click_each('@new/UIFrameWnd/UIBitmapButton', f'{name}_button', per=40, limit=12, where=NOT_CLOSE)
    s.click_each('@new/UIFrameWnd/UIBasicButton', f'{name}_text_button', per=40, limit=12)
    for i, at in enumerate(ROWS.get(name, ())):
        s.hover_ui('@new/UIFrameWnd', at=at, then=40).snap(f'{name}_row_{i}_hover')
        s.click_ui('@new/UIFrameWnd', at=at, then=50).snap(f'{name}_row_{i}_click')
    if search:
        s.click_ui('@new/UIFrameWnd/UIEditCtrl', then=10).text(search).key('enter', then=60).snap(f'{name}_search')
    s.close_new().snap(f'{name}_closed')
s.baseline().say('/stats', then=90).snap('stats_open')
s.say('/stats', then=40).snap('stats_closed')
s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#185', then=30).click_ui('UIEscOptionWnd/UIBitmapButton#450', then=40)
s.click_ui('UIGraphicSettingWnd/UIBasicButton#502', then=40).snap('command_list')
s.click_each('@new/UIFrameWnd/UIScrollBar', 'command_list_scroll', at=((0.5, 0.9),), per=20, limit=2)
s.close_new()
s.quit()
STEPS = s.steps
