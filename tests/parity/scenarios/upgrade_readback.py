"""What the client read from the saved state it started with, for `parity.py upgrade SCENARIO --then
upgrade_readback`: not a recorded scenario of its own (it has no golden), it plays against the settings
scenario's recording (RECORDING: the same account, and nothing sent past the login but keepalives).

Every on/off chat command is typed once (each turns the other way and writes its savedata file, so a
state read wrong is written wrong), the Game Settings window's four tabs are shown and scrolled through
(their boxes' states), and the client leaves through Exit to Windows, which the replay acknowledges
(ANSWER_QUIT): the exit writes OptionInfo.lua, the chat tabs and the registry from what the client
holds, so a value read wrong is saved wrong."""
import re

from common import scenario

RECORDING = 'settings'
ANSWER_QUIT = True

WND = 'UIGraphicSettingWnd'
TABS = [('graphics', 0.12), ('interface', 0.375), ('gameplay', 0.62), ('audio', 0.87)]
CONTENT = (450, 450)                 # a point over the settings list, for the wheel

# every on/off chat command with a savedata file; /cmdpalette last, since it changes what typing a
# command line does
TOGGLES = ['/nodmg', '/spritefilter', '/spritedepth', '/dmgfilter', '/mvpaura', '/mapculling', '/mapquality',
           '/stats', '/targetlock', '/quickcast', '/quickcastaim', '/quickcastcursor', '/quickcastsnap',
           '/castarea', '/targetoutline', '/targetindicator', '/ignoreblocked', '/refuseblocked', '/smoothself',
           '/smoothplayers', '/smoothmonsters', '/altpathing', '/companionlooks', '/mountlooks', '/spbar',
           '/partybuffs', '/itembind', '/equiponly', '/hotbarfade', '/chatfade', '/boldnames', '/wordkeys',
           '/cmdpalette']


def show_settings(s, prefix):
    s.baseline().click_ui('UIMenuIconWnd/UIMenuIcon#185', then=30)
    s.click_ui('UIEscOptionWnd/UIBitmapButton#450', then=40).state(f'{prefix}_opened')
    for tab, at in TABS:
        s.click_ui(f'{WND}/UITabControl', at=(at, 0.5), then=30).state(f'{prefix}_{tab}')
        for page in range(3):
            s.wheel(*CONTENT, -6, then=20).state(f'{prefix}_{tab}_{page}')
        s.wheel(*CONTENT, 18, then=20)
    s.click_ui(f'{WND}/UIBitmapButton#193', index=1, then=30)
    return s.close_new()                    # Game Options


# The Audio tab's Sound quality and Simultaneous sounds rows are ragexe's own (AUDIO_QUALITY_SETTING, an
# approved improvement): their two list buttons are left out of the settings window, and with them the
# places of its controls, which then compare in order, by kind, id and state; on the Audio tab the
# pointer, left where the wheel turned, lies over another row, so a hover balloon is left out there too.
SETTINGS_ROW = re.compile(r'^(  +)(\w+)@@ id=(\d+) rect=[-\d,]+ (.*)$')


def state_form(name, text):
    """A state as parity.py upgrade compares it (STATE_FORM)."""
    if not name.startswith('settings_'):
        return text
    out, inside, balloon = [], False, False
    for line in text.splitlines(True):
        if not line.startswith(' '):
            inside = line.startswith(WND + '@@')
            balloon = name.startswith('settings_audio') and line.startswith('UITransBalloonText@@')
        if balloon:
            continue
        if inside and line.startswith(' '):
            m = SETTINGS_ROW.match(line.rstrip('\r\n'))
            if m and m.group(2) == 'UINoneVisibleButton' and m.group(3) in ('510', '511'):
                continue
            if m:
                line = f'{m.group(1)}{m.group(2)}@@ id={m.group(3)} {m.group(4)}\n'
        out.append(line)
    return ''.join(out)


STATE_FORM = state_form

s = scenario('parity03')
s.state('logged_in')
for cmd in TOGGLES:
    s.say(cmd, then=12)
s.state('toggled')
show_settings(s, 'settings')
s.exit_game()
STEPS = s.steps
