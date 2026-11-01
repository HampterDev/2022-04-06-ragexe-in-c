"""Exercise production pet-action handling against transport/actor boundaries."""
from pathlib import Path
import re
import sys

source, output = map(Path, sys.argv[1:])
output.mkdir(parents=True, exist_ok=True)

def function(path, signature):
    text = (source / path).read_text(encoding='utf-8')
    assert text.count(signature) == 1
    start = text.index(signature)
    return text[start:text.index('\n}', start) + 2]

receive = (source / 'src/game/packets_pet.c').read_text(encoding='utf-8')
start = receive.index('#pragma pack(push, 1)\nstruct PACKET_ZC_PET_ACT')
end = receive.index('#pragma pack(pop)', start) + len('#pragma pack(pop)')
init = (source / 'src/session/session_init.c').read_text(encoding='utf-8')
start_table = init.index('static const int s_petEmotions')
end_table = init.index('\n};', start_table) + 3
send = (source / 'src/game/msg_misc.c').read_text(encoding='utf-8')
parts = [re.search(r'^#define CZ_PET_ACT[^\n]+', send, re.M)[0],
         receive[start:end], init[start_table:end_table],
         function('src/session/session_init.c', 'void session_init_pet_emotions('),
         function('src/session/pet.c', 'int session_get_pet_emotion('),
         function('src/game/msg_misc.c', 'int game_mode_msg_pet_act('),
         function('src/game/packets_pet.c', 'void game_mode_zc_pet_act(')]
(output / 'pet_actions.inc').write_text('\n\n'.join(parts) + '\n', encoding='utf-8')
