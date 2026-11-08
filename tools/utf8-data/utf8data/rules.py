"""Which data files the custom client loads, and which of their fields are display text.

Every string the client can show is DISPLAY and goes CP949 -> UTF-8.  Every
string that becomes a file name or resource lookup is RESOURCE and keeps its
CP949 bytes, because GRF entry names and the client's path literals are CP949.
Anything not matched by a rule is KEEP: left as CP949 and listed in the report.

Lua field patterns match a dump path (see dump.py): a tuple of
    'Name'   a string key with exactly that text
    '#'      a numeric key
    '[]'     a stand-in key (an expression such as jobtbl.JT_PORING)
    '*'      any one key
    '**'     any run of keys, possibly empty
Call patterns ('@call', 'Function', argument) match arguments the data passes
to client functions (InsertAchieveInfo, AddItem, ...).

Text table rules name the line kinds that are display text; table_reader.c
splits on '#', ends a line at '\r' and skips fields that start with '//'.
"""

DISPLAY = 'display'
RESOURCE = 'resource'
KEEP = 'keep'

# How the client finds a file (see src/contents/client_lua.c, lua_data.c, table_reader.c):
CFILE_LUA = 'cfile-lua'      # client_lua_load_file(name, 1, 0): data\LuaFiles514\<name>.lub|.lua, GRF first
DISK_LUA = 'disk-lua'        # client_lua_load_file(name, 0, 1): <name>.lub|.lua on disk only
DISK_MAIN = 'disk-main'      # lua_data_run / item_info_mgr: luaL_loadfile(path) then main()
CFILE_TABLE = 'cfile-table'  # table_reader_read / file_open(name, 0, 1, 0): data\<name>, GRF first


class Spec:
    def __init__(self, name, loader, kind, rules=(), note='', main=False):
        self.name = name          # logical name as the client source spells it
        self.loader = loader
        self.kind = kind          # 'lua' or 'table'
        self.rules = rules if isinstance(rules, str) else list(rules)
        self.note = note
        self.main = main          # call main() after loading, like lua_data_run

    def __repr__(self):
        return 'Spec(%r)' % self.name


def _match(pattern, path, pi=0, ki=0):
    while pi < len(pattern):
        p = pattern[pi]
        if p == '**':
            return any(_match(pattern, path, pi + 1, k) for k in range(ki, len(path) + 1))
        if ki >= len(path):
            return False
        key = path[ki]
        if p == '*':
            pass
        elif p == '#':
            if key[0] != 'n':
                return False
        elif p == '[]':
            if key[0] != 'p':
                return False
        else:
            if key[0] != 's' or key[1] != p.encode('ascii'):
                return False
        pi += 1
        ki += 1
    return ki == len(path)


def classify(spec, path):
    """Class of the string stored at `path` (a dump path), by the first matching rule."""
    if path and path[0] == '@call':
        _, _, callee, arg = path
        for pattern, cls in spec.rules:
            if pattern[0] == '@call' and pattern[1].encode('ascii') == callee and pattern[2] == arg:
                return cls
        return KEEP
    for pattern, cls in spec.rules:
        if pattern and pattern[0] != '@call' and _match(pattern, path):
            return cls
    return KEEP


def _lua(name, rules, note='', loader=CFILE_LUA, main=False):
    return Spec(name, loader, 'lua', rules, note, main)


def _table(name, rules, note=''):
    return Spec(name, CFILE_TABLE, 'table', rules, note)


ALL_LINES = 'all'            # table rule: every non-comment field is display text

# Files the custom client loads (checked against src/ at 46ecf0b) that hold non-ASCII
# text.  Files whose non-ASCII bytes are all resource names or comments are listed
# too, with no display rule, so the report shows that they were looked at.
#
# Not listed: every other loaded file is pure ASCII.  The homunculus/mercenary AI
# scripts (AI\*.lua, run in their own Lua state) hold Korean only in comments, and
# what they print goes to TraceAI.txt, not to the screen.
SPECS = [
    # --- session Lua state (src/session/session_lua.c), data\LuaFiles514\Lua Files\...
    _lua(r'Lua Files\Agit\AgitConfig', [(('tbl_AgitInfo', '*', 'Name'), DISPLAY)],
         'castle names, shown through get_agit_name (guild_agit.c, agit_list.c, guild_manage.c)'),
    _lua(r'Lua Files\DataInfo\AddRandomOptionNameTable', [(('NameTable_VAR', '*'), DISPLAY)],
         'random option texts (GetVarOptionName)'),
    _lua(r'Lua Files\DataInfo\HelpMsgStr', [(('HelpMsgStr', '#'), DISPLAY)], '/tip texts (getHelpMsg)'),
    _lua(r'Lua Files\DataInfo\TitleTable', [(('title_tbl', '#'), DISPLAY)], 'title names (GetTitleString)'),
    _lua(r'Lua Files\DataInfo\PetInfo', [
        (('PetIllustNameTable', '*'), RESOURCE),
        (('PetAccActNameTable', '*'), RESOURCE),
        (('PetStringTable', '*'), KEEP),
    ], 'illustration and accessory names are file names; PetStringTable (pet names) is only '
       'reachable through GetPetString, which no client code calls, so it is kept'),
    _lua(r'Lua Files\DataInfo\jobName', [(('JobNameTable', '*'), RESOURCE)], 'sprite names'),
    _lua(r'Lua Files\DataInfo\jobName_F', [], 'path fragments inside functions'),
    _lua(r'Lua Files\DataInfo\accName', [(('AccNameTable', '*'), RESOURCE)], 'headgear sprite names'),
    _lua(r'Lua Files\DataInfo\SpriteRobeName', [(('RobeNameTable', '*'), RESOURCE)], 'robe sprite names'),
    _lua(r'Lua Files\DataInfo\WeaponTable', [
        (('WeaponNameTable', '*'), RESOURCE),
        (('WeaponHitWaveNameTable', '*'), RESOURCE),
    ], 'weapon sprite suffixes and hit sounds'),
    _lua(r'Lua Files\DataInfo\WeaponTable_F', [], 'comments only'),
    _lua(r'Lua Files\DressRoom\jobDressList', [
        (('_DressList', '**'), DISPLAY),
        (('_DressHeadstyleList', '**'), DISPLAY),
        (('_DressHeadPaletteList', '**'), DISPLAY),
        (('_DressBodyPaletteList', '**'), DISPLAY),
    ], 'dress room style labels'),
    _lua(r'Lua Files\Emotion\EmotionList', [(('@call', 'InsertEmotionMsgListTable', 2), DISPLAY)],
         'emotion commands, listed in the emotion window and matched against typed chat'),
    _lua(r'Lua Files\Navigation\Navi_Map_krpri', [(('Navi_Map', '#', '#'), DISPLAY)],
         'column 2: map display name'),
    _lua(r'Lua Files\Navigation\Navi_Npc_krpri', [(('Navi_Npc', '#', '#'), DISPLAY)],
         'columns 5/6: NPC name and alias (navi_lua_tables.c)'),
    _lua(r'Lua Files\Navigation\Navi_Mob_krpri', [(('Navi_Mob', '#', '#'), DISPLAY)],
         'column 5: monster name (column 6 is the ASCII sprite name)'),
    _lua(r'Lua Files\Navigation\Navi_Link_krpri', [(('Navi_Link', '#', '#'), DISPLAY)],
         'columns 5/6: warp/NPC name and alias'),
    _lua(r'Lua Files\SkillEffectInfo\SkillEffectInfoList', [
        (('SKILL_EFFECT_INFO_LIST', '*', 'waveFileName'), RESOURCE),
        (('SKILL_EFFECT_INFO_LIST', '*', 'targetWaveFileName'), RESOURCE),
    ], 'sound file names'),
    _lua(r'Lua Files\SkillInfoz\SkillDescript', [(('SKILL_DESCRIPT', '**'), DISPLAY)],
         'already UTF-8'),
    _lua(r'Lua Files\SprEditInfo\2DLayerDir_F', [], 'comments only'),
    _lua(r'Lua Files\StateIcon\StateIconInfo', [(('StateIconList', '**'), DISPLAY)],
         'status icon descriptions (GetStateIconDescript)'),
    _lua(r'Lua Files\StateIcon\StateIconImgInfo', [(('StateIconImgList', '**'), RESOURCE)],
         'status icon image names'),
    _lua(r'Lua Files\worldviewdata\worldviewdata_Language', [(('WORLD_MSGID', '*'), DISPLAY)],
         'world map labels'),

    # --- files read from disk (System\, luaL_loadfile / client_lua_load_file disk-only)
    _lua(r'System\OngoingQuestInfoList_True', [
        (('QuestInfoList', '*', 'Title'), DISPLAY),
        (('QuestInfoList', '*', 'Summary'), DISPLAY),
        (('QuestInfoList', '*', 'Description', '#'), DISPLAY),
    ], 'quest window texts (renew_quest_info.c)', loader=DISK_LUA),
    _lua(r'System\Achievement_list.lub', [
        (('achievement_tbl', '**', 'title'), DISPLAY),
        (('achievement_tbl', '**', 'summary'), DISPLAY),
        (('achievement_tbl', '**', 'details'), DISPLAY),
        (('achievement_tbl', '**', 'text'), DISPLAY),
        (('@call', 'InsertAchieveInfo', 2), DISPLAY),
        (('@call', 'InsertAchieveInfo', 3), DISPLAY),
        (('@call', 'InsertAchieveInfo', 4), DISPLAY),
        (('@call', 'InsertAchieveResource', 2), DISPLAY),
    ], 'achievement window texts (achievement_lua.c)', loader=DISK_MAIN, main=True),
    _lua(r'System\PrivateAirplane_True.lub', [
        (('NeedItem', '*', '#'), DISPLAY),
        (('@call', 'InsertNeedItem', 2), DISPLAY),
    ], 'item name next to the ticket ID; private_airplane_mgr.c only checks that it is a string',
        loader=DISK_MAIN, main=True),
    _lua(r'System\iteminfo_true.lub', [
        (('tbl', '*', 'identifiedResourceName'), RESOURCE),
        (('tbl', '*', 'unidentifiedResourceName'), RESOURCE),
        (('tbl', '*', 'identifiedDisplayName'), DISPLAY),
        (('tbl', '*', 'unidentifiedDisplayName'), DISPLAY),
        (('tbl', '*', 'identifiedDescriptionName', '#'), DISPLAY),
        (('tbl', '*', 'unidentifiedDescriptionName', '#'), DISPLAY),
        (('@call', 'AddItem', 2), DISPLAY),
        (('@call', 'AddItem', 3), RESOURCE),
        (('@call', 'AddItem', 4), DISPLAY),
        (('@call', 'AddItem', 5), RESOURCE),
        (('@call', 'AddItemIdentifiedDesc', 2), DISPLAY),
        (('@call', 'AddItemUnidentifiedDesc', 2), DISPLAY),
    ], 'item names and descriptions; resource names become sprite and texture paths',
        loader=DISK_MAIN, main=True),

    # --- '#'-separated tables (table_reader.c), data\<name>
    _table('msgStringTable.txt', ALL_LINES, 'message table'),
    _table('CardPrefixNameTable.txt', ALL_LINES, 'card prefix/suffix names'),
    _table('MetalProcessItemList.txt', ALL_LINES, 'refine material lists, shown as text'),
    _table('DC_scream.txt', ALL_LINES, 'Dancer scream lines, sent as chat'),
    _table('BA_frostjoke.txt', ALL_LINES, 'Bard frost joke lines, sent as chat'),
    _table('Num2CardIllustNameTable.txt', [], 'card illustration file names'),
    _table('ResNameTable.txt', [], 'resource aliases'),
    _table('CardPostfixNameTable.txt', [], 'comments only'),
    _table('BuyingStoreItemList.txt', [], 'comments only'),
    _table('indoorRswTable.txt', [], 'comments only'),
    _table('mp3NameTable.txt', [], 'comments; values are BGM paths'),
    _table('ItemMoveInfoV5.txt', [], 'comments only'),
    _table('mapPosTable.txt', [], 'comments only'),
    _table('fogParameterTable.txt', [], 'comments only'),
    _table('ViewPointTable.txt', [], 'comments only'),
]


def find(name):
    for s in SPECS:
        if s.name.lower() == name.lower():
            return s
    raise KeyError(name)
