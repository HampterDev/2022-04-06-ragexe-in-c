"""What the skill sweep scenarios (skills_*.py) share: a character that takes one class after
another and casts every skill of each from its shortcut bar.

Each sweep account's character (parity61.., seedSkillSweep in server/paritysrv_test.go) is a level
99 novice with every stat at 99, its shortcut bar holding the skills its scenario casts in the
order the scenario lists them: the first tab's 36 slots, then the second tab's. A class is taken
with @job and its skills learned with @allskill (which @job takes away again); a skill is used the
way a player uses it, a double-click on its slot, then a click on the target the skill asks for:
the Arena dummy (a Poring that neither moves nor fights back), the character itself, or a cell.

Waits are the skill's own: its cast time as DEX 99 shortens it, its delay after the cast, and a
margin for the server's animation gate. Frame numbers assume the client paced at 16 ms a frame.

A skill a class shares with an earlier one (the Super Novice's, the Gypsy's) is cast only in the
first. Not swept: the third classes and every class past Soul Linker (Expanded Super Novice,
Kagerou, Oboro, Rebellion, Summoner, Star Emperor, Soul Reaper), whose @job the test server refuses
(it allows no job past Soul Linker), and whose skills the data set's skillinfolist mostly lacks
(Rebellion's stop the client in SkillInfo_F.lua); the High first classes and the baby classes,
which have no skills of their own.
"""
from common import scenario

DUMMY = 1002                    # the Arena dummy's job (a Poring)
CENTRE = (512, 344)
SPAWN = (156, 186)
BAR = 'UIShortCutWnd'
BAR_X, BAR_Y = 230, 0           # the shortcut bar's top left corner, where a new character has it
SLOTS_PER_ROW, ROWS, TAB_SLOTS = 9, 4, 36
CHAT_INPUT = 'UINewChatWnd/UIChatEditCtrl'
# Points 8 cells straight south and north of the character: the camera's tilt makes a cell deeper on
# the screen below the character than above it.
WALK_SOUTH, WALK_NORTH = (508, 613), (508, 186)
TRAP = 2                        # the state probe's kind for a skill unit (a trap, an Ice Wall's cell)
FRAME_MS = 16
CAST_SHARE = 0.4                # of a cast time left at DEX 99 (it scales as 1 - DEX/150), with room
MARGIN_MS = 700


def cell(dx, dy):
    """The screen point of the cell (dx, dy) from the character's, under the default camera: a cell
    is about 36.7 pixels across and 25.8 deep, north up the screen."""
    return int(508 + 36.7 * dx), int(381 - 25.8 * dy)


# Cells for ground skills, away from the dummy (2 cells east) and the parity NPCs (2 cells south,
# east of the character): NEAR for skills of range 3 or less, FAR for the others.
NEAR = [(-2, 0), (-2, 2), (0, 2), (-1, -2), (-3, 1), (1, 3)]
FAR = [(-5, 0), (-4, 4), (0, 5), (-5, -3), (3, 5), (-7, 2)]


def frames(ms):
    return max(1, -(-int(ms) // FRAME_MS))


class Sweep:
    """The steps of one sweep scenario. `hotbar` is the skills on the shortcut bar, in slot order."""

    def __init__(self, account, hotbar):
        self.s = scenario(account)
        self.hotbar = list(hotbar)
        self.tab = 0
        self.ground = {'near': 0, 'far': 0}
        self.cls = None
        s = self.s
        # the bar shown with all four rows of a tab
        for _ in range(3):
            s.click_ui(f'{BAR}/UIBitmapButton#208', then=12)
        s.wait(20).state('bar')

    def say(self, line, then=30):
        """A chat line, the chat input clicked first: a window a skill opened and closed again (a shop,
        a list) can leave the focus elsewhere, and the line would be lost."""
        self.s.click_ui(CHAT_INPUT, then=4).say(line, then=then)
        return self

    def job(self, job, name, mount=False, setup=()):
        """The class `job`, its skills, full HP and SP, a fresh dummy, then `setup` (chat lines)."""
        self.cls = name
        self.ground = {'near': 0, 'far': 0}
        s = self.s
        self.say(f'@job {job}', then=60).say('@allskill', then=40)
        if mount:
            self.say('@mount', then=40)
        self.say('@heal', then=20).say('@parityclear', then=20).say('@paritydummy', then=20)
        for line in setup:
            self.say(line, then=20)
        s.wait(40).snap(name)
        return self

    def _slot(self, skill):
        index = self.hotbar.index(skill)
        tab, at = divmod(index, TAB_SLOTS)
        if tab != self.tab:
            self.s.click_ui(f'{BAR}/UICheckBox#541', then=10)
            self.tab = tab
        row, col = divmod(at, SLOTS_PER_ROW)
        return BAR_X + 29 + 29 * col, BAR_Y + 17 + 33 * row

    def _spot(self, near):
        key = 'near' if near else 'far'
        spots = NEAR if near else FAR
        spot = spots[self.ground[key] % len(spots)]
        self.ground[key] += 1
        return cell(*spot)

    def cast(self, skill, name, how, cast_ms=0, after_ms=0, near=False, at=None):
        """One skill: its slot double-clicked, then its target: 'self' (none), 'enemy' (the dummy),
        'friend' (the character), 'ground' (a cell, `at` or the next free NEAR/FAR one), 'trap' (the
        skill unit nearest `at`). A capture and a state once the cast should be done, then the rest of
        the wait."""
        s = self.s
        x, y = self._slot(skill)
        s.click(x, y, double=True, then=6)
        if how == 'enemy':
            s.click_actor(CENTRE, job=DUMMY, then=0)
        elif how == 'friend':
            s.click_actor(CENTRE, then=0)
        elif how == 'ground':
            gx, gy = at or self._spot(near)
            s.hover(gx, gy, then=4).click(gx, gy, then=0)
        elif how == 'trap':
            s.click_actor(at or CENTRE, kind=TRAP, then=0)
        cast = frames(cast_ms * CAST_SHARE)
        s.wait(cast + 24).snap(f'{self.cls}_{name}')
        s.wait(frames(after_ms + MARGIN_MS))
        return self

    def cancel(self, then=20):
        """A right click on open ground, which ends a skill's aiming without opening anything."""
        self.s.rclick(*cell(-3, -4), then=then)
        return self

    def back_to_spawn(self, then=150):
        """A warp to the spawn cell (the dummy stays where it is)."""
        self.say('@warp prontera %d %d' % SPAWN, then=then)
        return self

    def leave_and_return(self, name):
        """A walk south far enough that every unit cast here leaves the view (three clicks of 8 cells,
        along the main street, clear of the town's doors; the server keeps a unit in view up to 14 cells
        away, and the units stand up to 5 cells north), a state there, the walk back north and a
        state on return. It starts from the spawn cell, so the walk ends where it began whatever moved
        the character before (a skill that knocks back or moves it, a status that holds it in place)."""
        s = self.s
        self.back_to_spawn()
        for _ in range(3):
            s.click(*WALK_SOUTH, then=150)
        s.wait(30).snap(f'{self.cls}_{name}_away')
        for _ in range(3):
            s.click(*WALK_NORTH, then=150)
        s.wait(30).snap(f'{self.cls}_{name}_back')
        return self

    def warp_away_and_back(self, name):
        """A warp to another map and back to the spawn cell, with a state after each."""
        s = self.s
        self.say('@warp morocc 159 93', then=150)
        s.state(f'{self.cls}_{name}_away')
        self.say('@warp prontera %d %d' % SPAWN, then=150)
        s.snap(f'{self.cls}_{name}_back')
        return self

    def die_and_revive(self, name):
        s = self.s
        self.say('@die', then=90)
        s.snap(f'{self.cls}_{name}_dead')
        self.say('@alive', then=60)
        self.close_restart()
        s.snap(f'{self.cls}_{name}_alive')
        return self

    def close_restart(self):
        """Return to game on the window a death opens, which a revival leaves open."""
        self.s.click_ui('UIEscOptionWnd/UIBitmapButton#177', then=30)
        return self

    def end(self):
        self.s.quit()
        return self.s.steps
