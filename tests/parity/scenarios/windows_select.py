"""Every window the window manager makes, at the character selection: each id made through MakeWindow, held
and closed as windows_game does it. Most are the game's windows, made where the original's character
selection never makes them; both clients must agree there too. Left out as at the login window
(windows_login): the ids that fault or end the client in both clients when made here, the ones the original
faults on alone (0xf, 0x8c, 0x125, 0x126; ragexe survives them), 0xe (ragexe reads the game mode it lacks,
herc is kept up by its null game-mode guard), and the ids whose state shows what differs by the run's folder
or uninitialised data. 0x82, 0x83 and 0x84 leave a sub-chat tab no window holds, as in windows_game: the next
chat line ends both clients."""
import common
from inputs import Seq

# The ids MakeWindow's switch takes (the original's, sub_6349a0, up to 0x165), and two past it.
IDS = range(0x168)
# Removed from ragexe on purpose (d3200461 battleground queues, roulette, reputation, pet evolution, the
# select package; 409849b6 the NPC shop's ATK/DEF estimate; 6d99c755 Twitter; 90ac1d6c the video window):
# the original makes a window, ragexe none, and the roulette's asks the server, whose answer goes on into
# the chat of every later state.
REMOVED = [
    0x32, 0x9d, 0xd1, 0xd2, 0xd3, 0x105, 0x10c, 0x10d, 0x11a, 0x11b, 0x130, 0x15a, 0x165]
STOCK_FAULTS = [
    0x0, 0xe, 0xf, 0x22, 0x2b, 0x44, 0x45, 0x4d, 0x82, 0x83, 0x84, 0x8c, 0x9e, 0xa5, 0xa6, 0xa7, 0xb1, 0xb6, 0xb7, 0xbb, 0xbf, 0x107,
    0x108, 0x111, 0x119, 0x122, 0x125, 0x126, 0x12e, 0x13a, 0x13e, 0x15b, 0x15c]
# Left out for what they show here, not for a fault: 0x9b (the Escape menu reads whether the player is dead;
# there is no player here, and herc's null guard stands a dummy in that reads as dead), 0x11c (the captcha
# window writes the client's folder, herc-0 or ragexe-0) and 0x143 (the agency's register window shows the
# character's name, which no session holds yet: herc's bytes are whatever its buffer held).
NOT_COMPARED = [0x9b, 0x11c, 0x143]

ACCOUNT = 'parity56'
common.CURRENT_ACCOUNT = ACCOUNT
s = Seq()
s.at(130).key('enter').at(210).text(ACCOUNT).key('tab').text(ACCOUNT + 'pw').key('enter')
s.at(450).state('select_before')
s.window_sweep([i for i in IDS if i not in REMOVED and i not in STOCK_FAULTS and i not in NOT_COMPARED], 'select')
s.state('select_after').quit()
STEPS = s.steps
