"""Exercise production accessory lookup and packet presentation paths with real Lua."""
from pathlib import Path
import sys

source, output = map(Path, sys.argv[1:])
output.mkdir(parents=True, exist_ok=True)

def function(path, signature):
    text = (source / path).read_text(encoding='utf-8')
    assert text.count(signature) == 1
    start = text.index(signature)
    return text[start:text.index('\n}', start) + 2]

pet = (source / 'src/session/pet.c').read_text(encoding='utf-8')
start = pet.index('struct PetAccessoryLookup {')
parts = [pet[start:pet.index('\n};', start) + 3],
         function('src/session/pet.c', 'static int lookup_pet_accessory('),
         function('src/session/pet.c', 'const char *session_get_pet_accessory_act('),
         function('src/actor/game_actor.c', 'void game_actor_set_pet_accessory('),
         function('src/game/packets_entry.c', 'static void entry_note_pet('),
         function('src/game/packets_entry.c', 'static void entry_note_pet_existing(')]
hp = function('src/game/packets_entry_hp.c', 'static void entry_hp_tail(')
start = hp.index('    if (type == ENTRY_OBJ_PET) {')
end = hp.index(' else if (type == ENTRY_OBJ_HOMUN', start)
parts.append('static void receive_hp(struct GameActor *actor, unsigned aid, int accessory)\n{\n'
             '    int type = ENTRY_OBJ_PET;\n' + hp[start:end] + '\n}')
state = function('src/game/packets_pet.c', 'void game_mode_zc_change_state_pet(')
start = state.index('    case PET_STATE_ACCESSORY:')
end = state.index('    case PET_STATE_PERFORMANCE:', start)
parts.append('static void receive_accessory(struct GameActor *pet, unsigned aid, int value)\n{\n'
             '    switch (PET_STATE_ACCESSORY) {\n' + state[start:end] + '    }\n}')
(output / 'pet_accessory.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
