"""Saved states a herc-client install can hold, for `parity.py upgrade ... --state DIR`.

    python upgrade_states.py OUT_DIR

writes OUT_DIR\\on, OUT_DIR\\off, OUT_DIR\\uiscale and OUT_DIR\\oldparty, each laid out as the client's folder
(savedata\\...) plus registry.txt (the virtual registry's KEY|NAME|TYPE|VALUE lines). Every file is
in the form herc writes it, OptionInfo.lua and ChatWndInfo_U.lua built from the herc text the
persistence scenario's golden keeps.

  on       every on/off chat command's file '1'; herc's other savedata files, the OptionInfo.lua
           options and CmdOnOffList lines, the chat tabs and the window places at values no
           scenario saves.
  off      every on/off chat command's file '0'; other values, and the forms older herc builds
           wrote: fpscap.txt and fpscap_bg.txt as one number (before 2026-10-01), OptionInfo.lua
           without the display keys herc added from 2026-09-05 on but with the MSAAEDGES line of
           the 2026-09-27 builds, the Choose window's height 0xb8 (2026-09-06..16), the
           Navigation window's width 0x110 (before 2026-09-29), the Status window's old heights
           and the 400 x 350 Detail Level window (before 2026-09-26).
  uiscale  `on` with OptionInfo.lua's UISCALE 2 (UI at 3/2).
  oldparty savedata\\MiniPartyInfo.lua alone, the one party plate file herc builds before
           2026-09-18 wrote for every character (the party_plates character's plate at another
           cell); follow a scenario of another account with party_plates.

Display values stay as the harness runs (windowed 1024 x 768, the device and mode indices 0 in
`on`), and LockMouse stays 0: a client must never take the user's display or pointer.
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

TOGGLES = ['nodmg', 'spritefilter', 'spritedepth', 'dmgfilter', 'mvpaura', 'mapculling', 'mapquality', 'stats',
           'targetlock', 'quickcast', 'quickcastaim', 'quickcastcursor', 'quickcastsnap', 'castarea',
           'targetoutline', 'targetindicator', 'ignoreblocked', 'refuseblocked', 'smoothself', 'smoothplayers',
           'smoothmonsters', 'altpathing', 'companionlooks', 'mountlooks', 'spbar', 'partybuffs', 'itembind',
           'equiponly', 'hotbarfade', 'chatfade', 'wordkeys', 'cmdpalette', 'boldnames']

# herc's own savedata files (src/*.c), in the forms herc writes
FILES = {
    'on': {
        'darkmode.txt': '1', 'loadingrotate.txt': '0', 'shortsongs.txt': '0', 'outlines.txt': '0',
        'uitext.txt': '2', 'namestyle.txt': '11', 'iteminfo.txt': '1',
        'audio.txt': '40 -1 70 10 1 0 0 1\r\n',
        'fpscap.txt': '144 1\r\n', 'fpscap_bg.txt': '60 1\r\n',
        'npcdialogrows.txt': '9\r\n', 'npcmenurows.txt': '5\r\n',
        'picture.txt': '150 60 30\r\n',
        'unitfades.txt': '120\r\n0\r\n500\r\n',
        'statswnd.txt': '300 200 1\r\n',
        'mobinfownd.txt': '100 50\r\n', 'whodropswnd.txt': '120 70\r\n', 'meterwnd.txt': '140 90\r\n',
    },
    'off': {
        'darkmode.txt': '0', 'loadingrotate.txt': '1', 'shortsongs.txt': '1', 'outlines.txt': '1',
        'uitext.txt': '1', 'namestyle.txt': '10', 'iteminfo.txt': '0',
        'audio.txt': '0 0 100 100 0 0 0 0\r\n',
        'fpscap.txt': '0\r\n', 'fpscap_bg.txt': '0\r\n',
        'npcdialogrows.txt': '7\r\n', 'npcmenurows.txt': '4\r\n',
        'picture.txt': '20 200 0\r\n',
        'unitfades.txt': '0\r\n500\r\n10\r\n',
        'statswnd.txt': '0 0 0\r\n',
    },
}

# OptionInfoList values over herc's text: (key, value as herc writes it)
OPTIONS = {
    'on': {'Trilinear': '1', 'Bgm_Volume': '40', 'Effect_Volume': '30', 'AutoOpen1to1Window': '0',
           'AutoOpen1to1Window_Friend': '0', 'PlaySound_Open1to1Window': '0',
           'Outdoor_ViewLatitude': '-30.000000', 'Outdoor_ViewDistance': '450.000000',
           'Indoor_ViewLatitude': '-40.000000', 'Indoor_ViewDistance': '250.000000',
           'Simplicity_SkillList': '1', 'Show_SkillDescript': '1', 'ShowBattleFieldIcon': '0',
           'ISKEEPTASKBAR': '1', 'MSAA': '4', 'VSYNC': '0', 'ANISOTROPY': '8', 'TEXTUREATLAS': '1',
           'SPRITEMODE': '1', 'TEXTUREMODE': '1', 'bLockItemDropFromItemWnd': '1', 'bWarningPriceCheck': '1',
           'bSoundEffectInItemSellWnd': '1', 'bGuildMemberListSort': '0', 'bBlockPrivateTab': '1',
           'bItemDefaultSort': '1', 'bItemDescendingOrder': '1', '/zoom': '1', 'Emblem Frame': '1',
           'bExtendDamage': '0', 'bShowMapName': '0'},
    'off': {'Bgm_Volume': '0', 'Effect_Volume': '0', 'Outdoor_ViewLatitude': '-65.000000',
            'Outdoor_ViewDistance': '400.000000', 'Indoor_ViewLatitude': '-55.000000',
            'Indoor_ViewDistance': '300.000000', 'ChannelCopID': '1', 'MODECNT': '99', 'DEVICECNT': '7',
            'bItemAscendingOrder': '1', 'bShowMenuIcon': '1', '/zoom': '0'},
}
# the display keys herc added (resize.c), left out of `off`'s file as an older herc wrote it
DISPLAY_KEYS = ['ISBORDERLESS', 'ISKEEPTASKBAR', 'UISCALE', 'MSAA', 'VSYNC', 'ANISOTROPY', 'TEXTUREATLAS']

# CmdOnOffList commands left as they are: they would take the pointer (/camera keeps it) or hide the
# windows the readback session clicks
CMD_KEPT = {'/camera', '/window'}

TABS = {
    'on': [('On', 0, 0, 0, 0, 'Guild & Party', [1, 0] * 11),
           ('On', 0, 0, 0, 0, 'Battle Log', [0, 1, 1] * 7 + [0]),
           ('Off', 600, 300, 300, 120, 'Trade', [1] * 6 + [0] * 16)],
    'off': [('On', 0, 0, 0, 0, 'Regular Chat', [1] * 22)],
}
CAN_DRAG = {'on': 0, 'off': 1}

# the party_plates character's plate (savedata\MiniPartyInfo_290133.lua in its golden: {name,found,id,cell})
# at another cell
OLD_PARTY = '{Parity33,1,290098,40}\r\n'

UI = r'Software\Gravity Soft\Ragnarok\UIRectInfo'
REGISTRY = {
    'on': {'ITEMWNDINFO.X': 300, 'ITEMWNDINFO.Y': 150, 'ITEMWNDINFO.CURTAB': 1, 'ITEMWNDINFO.CURRADIO1': 1,
           'STATUSWNDINFO.X': 200, 'STATUSWNDINFO.Y': 300, 'STATUSWNDINFO.W': 0x118, 'STATUSWNDINFO.H': 0x97,
           'STATUSWNDINFO.ORGHEIGHT': 0x97, 'CHOOSEWNDINFO.X': 100, 'CHOOSEWNDINFO.Y': 100,
           'CHOOSEWNDINFO.W': 0x118, 'CHOOSEWNDINFO.H': 0x78, 'SAYDIALOGWNDINFO.X': 50, 'SAYDIALOGWNDINFO.Y': 60,
           'SAYDIALOGWNDINFO.W': 0x118, 'SAYDIALOGWNDINFO.H': 0xb4, 'DETAILLEVELWNDINFO.X': 100,
           'DETAILLEVELWNDINFO.Y': 50, 'DETAILLEVELWNDINFO.W': 500, 'DETAILLEVELWNDINFO.H': 422,
           'M_CHATWNDHEIGHT': 160, 'M_CHATWNDY': 560, 'MINIMAPWNDINFO.X': 700, 'MINIMAPWNDINFO.Y': 40},
    'off': {'CHOOSEWNDINFO.X': 120, 'CHOOSEWNDINFO.Y': 140, 'CHOOSEWNDINFO.W': 0x118, 'CHOOSEWNDINFO.H': 0xb8,
            'NAVIGATIONWNDINFO.X': 30, 'NAVIGATIONWNDINFO.Y': 40, 'NAVIGATIONWNDINFO.W': 0x110,
            'NAVIGATIONWNDINFO.H': 0xe4, 'STATUSWNDINFO.X': 220, 'STATUSWNDINFO.Y': 310,
            'STATUSWNDINFO.W': 0x118, 'STATUSWNDINFO.H': 0xad, 'STATUSWNDINFO.ORGHEIGHT': 0x8d,
            'DETAILLEVELWNDINFO.X': 90, 'DETAILLEVELWNDINFO.Y': 60, 'DETAILLEVELWNDINFO.W': 400,
            'DETAILLEVELWNDINFO.H': 350},
}


def herc_texts():
    with open(os.path.join(HERE, 'recordings', 'persistence.golden.json'), encoding='utf-8') as f:
        return json.load(f)['saved']['files']


def option_info(base, name):
    text = base
    for key, value in OPTIONS.get(name, {}).items():
        text, n = re.subn(r'(OptionInfoList\["%s"\] = )[^\r\n]*' % re.escape(key), lambda m: m.group(1) + value, text)
        assert n == 1, key

    def flip(m):
        return m.group(0) if m.group(1) in CMD_KEPT else f'CmdOnOffList["{m.group(1)}"] = {1 - int(m.group(2))}'

    if name == 'on':
        text = re.sub(r'CmdOnOffList\["([^"]+)"\] = (\d+)', flip, text)
    if name == 'off':
        for key in DISPLAY_KEYS:
            text, n = re.subn(r'OptionInfoList\["%s"\] = [^\r\n]*\r\n' % key, '', text)
            assert n == 1, key
        text = text.replace('OptionInfoList["WIDTH"]', 'OptionInfoList["MSAAEDGES"] = 1\r\nOptionInfoList["WIDTH"]', 1)
    return text


def chat_tabs(name):
    out = f'CanDragWnd = {CAN_DRAG[name]}\r\n\r\n'
    for i, (state, x, y, w, h, tab, options) in enumerate(TABS[name], 1):
        label = f'[[{tab}]]' if state == 'On' else f'"{tab}"'
        out += (f'ChatSubWnd_{i} = {{\r\n\tXPos = {x},\r\n\tYPos = {y},\r\n\tWidth = {w},\r\n\tHeight = {h},\r\n'
                f'\tTabState = "{state}",\r\n\tTabName = {label},\r\n')
        out += ''.join(f'\toption{k} = {v},\r\n' for k, v in enumerate(options, 1))
        out += '}\r\n\r\n'
    return out


def write_state(out, name, base, uiscale=False):
    save = os.path.join(out, 'savedata')
    os.makedirs(save, exist_ok=True)
    files = dict(FILES[name])
    for t in TOGGLES:
        files[t + '.txt'] = '1' if name == 'on' else '0'
    options = option_info(base['OptionInfo.lua'], name)
    if uiscale:
        options, n = re.subn(r'(OptionInfoList\["UISCALE"\] = )\d+', r'\g<1>2', options)
        assert n == 1
    files['OptionInfo.lua'] = options
    files['ChatWndInfo_U.lua'] = chat_tabs(name)
    for rel, text in files.items():
        with open(os.path.join(save, rel), 'wb') as f:
            f.write(text.encode('cp949'))
    with open(os.path.join(out, 'registry.txt'), 'w', encoding='mbcs') as f:
        for value, n in REGISTRY[name].items():
            f.write(f'{UI}|{value}|dword|{n}\n')


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    base = herc_texts()
    write_state(os.path.join(sys.argv[1], 'on'), 'on', base)
    write_state(os.path.join(sys.argv[1], 'off'), 'off', base)
    write_state(os.path.join(sys.argv[1], 'uiscale'), 'on', base, uiscale=True)
    old = os.path.join(sys.argv[1], 'oldparty', 'savedata')
    os.makedirs(old, exist_ok=True)
    with open(os.path.join(old, 'MiniPartyInfo.lua'), 'wb') as f:
        f.write(OLD_PARTY.encode('cp949'))


if __name__ == '__main__':
    main()
