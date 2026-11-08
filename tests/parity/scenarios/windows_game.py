"""Every window the window manager makes, in the game: each id made through MakeWindow (the probe's
PARITY_CALL_MAKE_WINDOW), a state the next frame and four frames later, closed through PostQuit1, a state three
frames after (Seq.window_sweep), then the next id. No context is given: each window is made as the window
manager makes it with none. The ids left out fault or end the client in both clients when made so (a
survey under the probe's exception handler, PARITY_CALL_GUARD)."""
from common import scenario
from inputs import PARITY_CALL_MAKE_WINDOW

# The ids MakeWindow's switch takes (the original's, sub_6349a0, up to 0x165), and two past it.
IDS = range(0x168)
# Removed from ragexe on purpose (d3200461 battleground queues, roulette, reputation, pet evolution, the
# select package; 409849b6 the NPC shop's ATK/DEF estimate; 6d99c755 Twitter; 90ac1d6c the video window):
# the original makes a window, ragexe none, and the roulette's asks the server, whose answer goes on into
# the chat of every later state.
REMOVED = [
    0x32, 0x9d, 0xd1, 0xd2, 0xd3, 0x105, 0x10c, 0x10d, 0x11a, 0x11b, 0x130, 0x15a, 0x165]
# Both clients fault in MakeWindow (0x4, the Lua windows 0x9e 0xa5 0xa6 0xa7 0xb6 0xb7 0xbf, 0xb8, 0x116,
# 0x13a) or end the client (0x2, 0x157) with no context. 0x82 and 0x83, the sub-chat windows, and 0x84, a
# chat tab's message options, made with none add a sub-chat tab (subChatWnds key 0) that no sub-chat window
# of the manager's holds, and the next chat line (a server broadcast) ends both clients in the chat window's
# add-line.
STOCK_FAULTS = [
    0x2, 0x4, 0x82, 0x83, 0x84, 0x9e, 0xa5, 0xa6, 0xa7, 0xb6, 0xb7, 0xb8, 0xbf, 0x116, 0x13a, 0x157]
# Left out for what it shows, not for a fault: the captcha window (0x11c) writes the client's folder, herc-0
# or ragexe-0.
NOT_COMPARED = [0x11c]
# Made again once closed: the client's own code takes them to be there (a server broadcast with the chat
# window closed through PostQuit1 ends both clients in the chat window's add-line).
REOPEN = [0x0, 0x1]

s = scenario('parity55')
s.wait(30).state('game_before')
for wid in IDS:
    if wid in REMOVED or wid in STOCK_FAULTS or wid in NOT_COMPARED:
        continue
    s.window_sweep([wid], 'game')
    if wid in REOPEN:
        s.call(PARITY_CALL_MAKE_WINDOW, wid, then=2)
s.state('game_after').quit()
STEPS = s.steps
