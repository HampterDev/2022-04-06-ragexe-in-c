"""Extract case 0x20 of the actor message handler (actor_sendmsg.c) and its window helper.

Usage: extract_chat_room_title.py ROOT OUT"""
import re
import sys
from pathlib import Path

root, out = Path(sys.argv[1]), Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
text = (root / 'src' / 'actor' / 'actor_sendmsg.c').read_bytes().decode('utf-8').replace('\r\n', '\n')
helper = re.search(r'^static struct UIWindow \*sm_window\([^)]*\)\n\{\n.*?^\}\n', text, re.M | re.S)
case = re.search(r'^    case 0x20: \{\n.*?^    \}\n', text, re.M | re.S)
if not helper or not case:
    sys.exit('sm_window or case 0x20 not found')
(out / 'chat_room_title.inc').write_text(
    helper.group(0) + '\n'
    'static void actor_chat_room_message(struct GameActor *self, RenderMsgArg par1)\n{\n'
    '    switch (0x20) {\n' + case.group(0) + '    }\n}\n', encoding='utf-8')
