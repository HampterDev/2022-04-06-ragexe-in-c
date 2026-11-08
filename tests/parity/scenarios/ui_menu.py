"""The menu icons under Basic Info: each icon opens its window, which closes again through its
own close button; then the Basic Info window's minimize button and the minimap's buttons."""
from common import scenario

ICONS = [184, 187, 186, 188, 191, 365, 353, 211, 422, 185, 453, 391, 468, 362, 536]


s = scenario('parity01')
s.snap('map')
for icon in ICONS:
    s.open_close(f'UIMenuIconWnd/UIMenuIcon#{icon}', f'icon_{icon}')
s.snap('menu_done')
s.click_ui('UIBasicInfoWnd/UIBitmapButton#194', then=30).snap('basicinfo_small')
s.click_ui('UIBasicInfoWnd/UIBitmapButton#194', then=30).snap('basicinfo_full')
for button in (208, 209, 181, 446, 444):
    s.baseline().click_ui(f'UIMinimapZoomWnd/UIBitmapButton#{button}', then=30).snap(f'minimap_{button}').close_new()
s.quit()
STEPS = s.steps
