"""What scenarios share: the way into the game, and the parity server's accounts.

Each scenario logs in with an account of its own (parity01..parity60, server/paritysrv_test.go),
so the server state one scenario leaves behind never reaches another. Frame numbers assume the
client paced at 16 ms per frame, as the runner records it.
"""
from inputs import Seq

GAME_START = (910, 545)       # the character selection's Game Start button, at 1024x768

# The account the scenario logs in with; the runner restarts the server stack before recording
# with an account the running stack has already seen, so every recording starts from the seed.
CURRENT_ACCOUNT = None


def login(s, account, password=None, char_select_capture=False):
    """From start-up to standing on the map: Enter on Select Service, the account and its
    password, Game Start for the character in slot 0, and time for the map to load."""
    global CURRENT_ACCOUNT
    CURRENT_ACCOUNT = account
    password = password or account + 'pw'
    s.at(130).key('enter').at(210).text(account).key('tab').text(password).key('enter')
    s.at(430)
    if char_select_capture:
        s.snap('char_select')
    # On the map the chat input is not focused yet: the first Enter focuses it, so a scenario's
    # first typed line would otherwise go nowhere.
    s.click(*GAME_START).at(640).key('enter').at(700)
    return s


def scenario(account):
    """A Seq already logged in and standing on the map."""
    return login(Seq(), account)
