"""Extract the code that copies server, user and client text into fixed buffers.

Small functions are taken whole. From large functions only the statement that
fills the buffer is taken (an if statement with its body, a braced block, or a
switch case up to its break), so the test runs exactly that production code with
its own locals. Each piece goes to its own .inc file in the output directory.
"""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])


def read(name):
    return (root / 'src' / name).read_text(encoding='utf-8')


def unique(source, pattern):
    matches = list(re.finditer(pattern, source, re.M))
    assert len(matches) == 1, pattern
    return matches[0]


def optional_function(source, signature):
    """The function, or nothing in sources from before it existed."""
    if not re.search(r'^' + re.escape(signature), source, re.M):
        return []
    return [function(source, signature)]


def function(source, signature):
    start = unique(source, r'^' + re.escape(signature) + r'[^{;]*\n\{').start()
    return source[start:source.index('\n}', start) + 2]


def struct(source, name):
    start = unique(source, r'^struct ' + re.escape(name) + r' \{\n').start()
    return '#pragma pack(push, 1)\n' + source[start:source.index('\n};', start) + 3] + '\n#pragma pack(pop)'


def line(source, pattern):
    match = unique(source, r'^' + pattern + r'.*$')
    return match.group(0)


def statement(source, pattern):
    """The statement starting on the line that matches: up to the first ';' outside
    parentheses and braces, or the '}' that closes its first brace."""
    start = source.rindex('\n', 0, unique(source, pattern).start()) + 1
    parens = braces = 0
    i, quote = start, None
    while True:
        c = source[i]
        if quote:
            if c == '\\':
                i += 1
            elif c == quote:
                quote = None
        elif c in '"\'':
            quote = c
        elif c == '(':
            parens += 1
        elif c == ')':
            parens -= 1
        elif c == '{':
            braces += 1
        elif c == '}':
            braces -= 1
            if braces == 0:
                return source[start:i + 1]
        elif c == ';' and parens == 0 and braces == 0:
            return source[start:i + 1]
        i += 1


def case(source, label):
    start = unique(source, r'^[ \t]*case ' + re.escape(label) + r':[ \t]*$').start()
    end = source.index('break;', start)
    return source[start:end + len('break;')]


pieces = {}
companion = read('game/companion_packets.c')
pieces['homun_packet'] = [struct(companion, 'HomunPropertyHead'), function(companion, 'static void homun_name(')]
names = read('game/actor_names.c')
pieces['pet_named'] = [line(names, r'#define NAME_LENGTH\b'), function(names, 'static void pet_named(')]
pet = read('game/packets_pet.c')
pieces['pet_packet'] = [line(pet, r'#define WND_PET_INFO_OTHER\b'), line(pet, r'#define PET_LEVEL_OTHER\b'),
                        struct(pet, 'PACKET_ZC_PROPERTY_PET'),
                        function(pet, 'void game_mode_zc_property_pet(')]
social = read('game/packets_social.c')
pieces['couple_name'] = [line(social, r'#define NAME_LENGTH\b'), struct(social, 'PACKET_ZC_COUPLENAME'),
                         function(social, 'void game_mode_zc_couple_name(')]
world = read('game/packets_world.c')
enums = unique(world, r'^enum \{\n\s+WMSG_CUTIN_LAYOUT').start()
pieces['cut_in'] = [line(world, r'static int g_illustShownPosition\b'), line(world, r'char g_illustImageName\['),
                    world[enums:unique(world, r'^void game_mode_update_cut_in\(').start()].rstrip(),
                    function(world, 'void game_mode_update_cut_in(')]
login = read('login/login_states.c')
pieces['server_list'] = (optional_function(login, 'static void server_line(') +
                         [function(login, 'static void server_list(')])
pieces['map_name'] = [statement(read('game/mode_connection.c'), r'\(\*\(char \(\*\)\[20\]\)&g_session1\.mapName\)')]
homun = read('ui/character/companion_info.c')
pieces['homun_default'] = [statement(homun, r'if \(strcmp\(g_session1\.homunName, ""\) == 0\)')]
pieces['homun_rename'] = [statement(homun, r'\w+\(g_session1\.homunName,[^;]*ui_edit_ctrl_get_text\(self->name_edit\)')]
pet_info = read('ui/items/pet_info.c')
pieces['pet_default'] = [statement(pet_info, r'if \(strcmp\(g_session1\.petName, ""\) == 0\)')]
pieces['pet_rename'] = [statement(pet_info, r'\w+\(g_session1\.petName,[^;]*ui_edit_ctrl_get_text\(self->name_edit\)')]
pieces['style_save'] = [statement(read('ui/items/styling_shop.c'), r'^[ \t]*if \(out\) \{')]
pieces['state_line'] = [case(read('game/state_messages_timed.c'), '0x27c')]

output.mkdir(parents=True, exist_ok=True)
for name, parts in pieces.items():
    (output / f'name_text_{name}.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
