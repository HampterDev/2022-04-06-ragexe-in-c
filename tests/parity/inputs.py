"""Scripted input for the parity runner, and the Seq builder scenario files use.

Every input is a list of frames, each a list of commands for parity.dll: the key state it
reports, the cursor it reports and the window messages posted to the client. Keys go to the
window the client last gave focus to, mouse messages to the main window, in client
coordinates (parity.dll turns a wheel's into screen ones).

A scenario file builds its steps with Seq, one action after another:

    s = Seq()
    login(s, 'parity01')                 # scenarios/common.py
    s.key('alt+e').capture('equipment')
    s.click_ui('UIEquipWnd/UIBitmapButton#123')
    s.say('/stats')
    s.quit()
    STEPS = s.steps

click_ui and expect name a window by its path in the state probe's tree: classes (the RTTI
name without "@@"), each optionally "#id", separated by "/" from a top-level window down.
They are resolved while the scenario runs, from a state taken the frame before.
"""
import re
import struct

from protocol import CMD_CALL, CMD_CURSOR, CMD_KEY, CMD_POST, TARGET_FOCUS, TARGET_MAIN

WM_KEYDOWN, WM_KEYUP, WM_CHAR, WM_SYSKEYDOWN, WM_SYSKEYUP = 0x100, 0x101, 0x102, 0x104, 0x105
WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK = 0x200, 0x201, 0x202, 0x203
WM_RBUTTONDOWN, WM_RBUTTONUP, WM_RBUTTONDBLCLK, WM_MOUSEWHEEL = 0x204, 0x205, 0x206, 0x20A
MK_LBUTTON, MK_RBUTTON, MK_SHIFT, MK_CONTROL = 1, 2, 4, 8

VK = {'enter': 0x0D, 'tab': 0x09, 'esc': 0x1B, 'back': 0x08, 'space': 0x20, 'up': 0x26, 'down': 0x28,
      'left': 0x25, 'right': 0x27, 'alt': 0x12, 'ctrl': 0x11, 'shift': 0x10, 'insert': 0x2D, 'delete': 0x2E,
      'home': 0x24, 'end': 0x23, 'pageup': 0x21, 'pagedown': 0x22, 'lbutton': 0x01, 'rbutton': 0x02}
VK.update({f'f{i}': 0x6F + i for i in range(1, 13)})
SCAN = {0x0D: 0x1C, 0x09: 0x0F, 0x1B: 0x01, 0x08: 0x0E, 0x20: 0x39, 0x26: 0x48, 0x28: 0x50, 0x25: 0x4B,
        0x27: 0x4D, 0x12: 0x38, 0x11: 0x1D, 0x10: 0x2A, 0x2D: 0x52, 0x2E: 0x53, 0x24: 0x47, 0x23: 0x4F,
        0x21: 0x49, 0x22: 0x51}
SCAN.update({0x6F + i: 0x3A + i for i in range(1, 11)})
SCAN.update({0x7A: 0x57, 0x7B: 0x58})
_LETTER_SCAN = dict(zip('QWERTYUIOP', range(0x10, 0x1A)))
_LETTER_SCAN.update(zip('ASDFGHJKL', range(0x1E, 0x27)))
_LETTER_SCAN.update(zip('ZXCVBNM', range(0x2C, 0x33)))
_DIGIT_SCAN = dict(zip('1234567890', range(0x02, 0x0C)))


# parity_world.h's ParityCallOp and ParityItemWord
PARITY_CALL_MAKE_WINDOW, PARITY_CALL_CLOSE_WINDOW, PARITY_CALL_ITEM_SHOW, PARITY_CALL_ITEM_RECORD = 1, 2, 3, 4
PARITY_CALL_GUARD, PARITY_ITEM_HOLD = 0x100, 1


def item_words(item_id, item_type=3, refine=0, cards=(0, 0, 0, 0), identified=1, damaged=0, expire=0,
               expire_hours=0, grade=0, options=()):
    """PARITY_CALL_ITEM_SHOW's words for an item: `options` up to five (id, value, param)."""
    opts = list(options) + [(0, 0, 0)] * (5 - len(options))
    return ([item_id, item_type, refine] + list(cards) + [identified, damaged, expire, expire_hours, grade,
                                                          len(options)] + [v for o in opts for v in o] + [0])


def u32(*v):
    return struct.pack('<' + 'I' * len(v), *(x & 0xffffffff for x in v))


def _vk(name):
    if name in VK:
        return VK[name]
    if len(name) == 1 and name.isalnum():
        return ord(name.upper())
    raise ValueError(f'unknown key {name!r}')


def _scan(vk):
    c = chr(vk) if 0x30 <= vk <= 0x5A else ''
    return SCAN.get(vk) or _LETTER_SCAN.get(c) or _DIGIT_SCAN.get(c, 0)


def _key_msg(vk, down, alt):
    scan = _scan(vk)
    lp = 1 | scan << 16 | (1 << 29 if alt else 0) | (0 if down else 3 << 30)
    if alt or vk == VK['alt']:
        msg = WM_SYSKEYDOWN if down else WM_SYSKEYUP
    else:
        msg = WM_KEYDOWN if down else WM_KEYUP
    return (CMD_POST, u32(TARGET_FOCUS, msg, vk, lp))


def key(spec):
    """'enter', 'f1', 'e', 'alt+e', 'ctrl+shift+v': the modifiers go down in the first frame,
    the key down and up in the next two, the modifiers up in the last."""
    parts = spec.lower().split('+')
    mods = [VK[m] for m in parts[:-1]]
    vk = _vk(parts[-1])
    alt = VK['alt'] in mods
    frames = []
    if mods:
        frames.append([c for m in mods for c in ((CMD_KEY, u32(m, 1)), _key_msg(m, True, alt and m != VK['alt']))])
    frames.append([(CMD_KEY, u32(vk, 1)), _key_msg(vk, True, alt)])
    frames.append([(CMD_KEY, u32(vk, 0)), _key_msg(vk, False, alt)])
    if mods:
        frames.append([c for m in reversed(mods) for c in ((CMD_KEY, u32(m, 0)), _key_msg(m, False, False))])
    return frames


def text(s):
    """Typed characters, one per frame (code page 949 bytes for anything beyond ASCII), each a
    WM_CHAR alone. The client takes a character only while its key dispatcher holds the last key
    press as text, so a character that follows a press something else took (a word key, the
    command palette) is dropped; type() presses the keys instead."""
    out = []
    for ch in s:
        for b in ch.encode('cp949'):
            out.append([(CMD_POST, u32(TARGET_FOCUS, WM_CHAR, b, 1))])
    return out


# US layout: the characters type() can press, as (virtual key, with Shift)
_TYPE_KEYS = {' ': (0x20, False), '-': (0xBD, False), '_': (0xBD, True), '.': (0xBE, False),
              ',': (0xBC, False), '/': (0xBF, False), '@': (0x32, True), '!': (0x31, True)}
_TYPE_KEYS.update({c: (ord(c.upper()), False) for c in 'abcdefghijklmnopqrstuvwxyz'})
_TYPE_KEYS.update({c: (ord(c), True) for c in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'})
_TYPE_KEYS.update({c: (ord(c), False) for c in '0123456789'})


def typed(s):
    """Characters typed as a keyboard types them, two frames each: the key (and Shift for a
    capital) goes down with WM_KEYDOWN, which the client's TranslateMessage turns into its
    WM_CHAR, then up with WM_KEYUP."""
    out = []
    shift = VK['shift']
    for ch in s:
        if ch not in _TYPE_KEYS:
            raise ValueError(f'type() has no key for {ch!r}')
        vk, shifted = _TYPE_KEYS[ch]
        lp = 1 | _scan(vk) << 16
        down = [(CMD_KEY, u32(shift, 1))] if shifted else []
        down += [(CMD_KEY, u32(vk, 1)), (CMD_POST, u32(TARGET_FOCUS, WM_KEYDOWN, vk, lp))]
        up = [(CMD_KEY, u32(vk, 0)), (CMD_POST, u32(TARGET_FOCUS, WM_KEYUP, vk, lp | 3 << 30))]
        up += [(CMD_KEY, u32(shift, 0))] if shifted else []
        out += [down, up]
    return out


def _lp(x, y):
    return (y & 0xffff) << 16 | (x & 0xffff)


def _move(x, y, held=0):
    return [(CMD_CURSOR, u32(x, y)), (CMD_POST, u32(TARGET_MAIN, WM_MOUSEMOVE, held, _lp(x, y)))]


def _buttons(button):
    if button == 'left':
        return WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK, MK_LBUTTON, VK['lbutton']
    return WM_RBUTTONDOWN, WM_RBUTTONUP, WM_RBUTTONDBLCLK, MK_RBUTTON, VK['rbutton']


def hover(x, y):
    return [_move(x, y)]


def click(x, y, button='left', double=False, mods=()):
    """The pointer moves to (x, y), then the button goes down and up (twice for a double-click,
    the second press as a double-click message). `mods` ('alt', 'ctrl', 'shift') are held in the
    key state from the move until a frame after the last release; no key messages are posted
    for them, so Alt opens no menu and the client sees them only through Get(Async)KeyState
    and the buttons' MK_ flags."""
    down, up, dbl, mk, vk = _buttons(button)
    held = [VK[m] for m in mods]
    flags = (MK_SHIFT if 'shift' in mods else 0) | (MK_CONTROL if 'ctrl' in mods else 0)
    lp = _lp(x, y)
    frames = [[(CMD_KEY, u32(m, 1)) for m in held] + _move(x, y, flags),
              [(CMD_KEY, u32(vk, 1)), (CMD_POST, u32(TARGET_MAIN, down, mk | flags, lp))],
              [(CMD_KEY, u32(vk, 0)), (CMD_POST, u32(TARGET_MAIN, up, flags, lp))]]
    if double:
        frames += [[(CMD_KEY, u32(vk, 1)), (CMD_POST, u32(TARGET_MAIN, dbl, mk | flags, lp))],
                   [(CMD_KEY, u32(vk, 0)), (CMD_POST, u32(TARGET_MAIN, up, flags, lp))]]
    if held:
        frames.append([(CMD_KEY, u32(m, 0)) for m in held])
    return frames


def _held(mods):
    """The key state commands and the MK_ flags for modifiers held through a mouse gesture."""
    flags = (MK_SHIFT if 'shift' in mods else 0) | (MK_CONTROL if 'ctrl' in mods else 0)
    return [VK[m] for m in mods], flags


def drag(x1, y1, x2, y2, button='left', steps=8, mods=()):
    """Press at (x1, y1), move to (x2, y2) over `steps` frames with the button held, let go;
    `mods` held as click() holds them."""
    down, up, _, mk, vk = _buttons(button)
    held, flags = _held(mods)
    frames = [[(CMD_KEY, u32(m, 1)) for m in held] + _move(x1, y1, flags),
              [(CMD_KEY, u32(vk, 1)), (CMD_POST, u32(TARGET_MAIN, down, mk | flags, _lp(x1, y1)))]]
    for i in range(1, steps + 1):
        x = x1 + (x2 - x1) * i // steps
        y = y1 + (y2 - y1) * i // steps
        frames.append(_move(x, y, mk | flags))
    frames.append([(CMD_KEY, u32(vk, 0)), (CMD_POST, u32(TARGET_MAIN, up, flags, _lp(x2, y2)))])
    if held:
        frames.append([(CMD_KEY, u32(m, 0)) for m in held])
    return frames


def wheel(x, y, notches, mods=()):
    """The wheel turned at (x, y): positive away from the user (zoom in, scroll up); `mods` held
    as click() holds them."""
    held, flags = _held(mods)
    frames = [[(CMD_KEY, u32(m, 1)) for m in held] + _move(x, y, flags),
              [(CMD_POST, u32(TARGET_MAIN, WM_MOUSEWHEEL, (120 * notches & 0xffff) << 16 | flags, _lp(x, y)))]]
    if held:
        frames.append([(CMD_KEY, u32(m, 0)) for m in held])
    return frames


# ---- the state probe's tree ----------------------------------------------------------------

# The world section's lines (src/parity/parity_world.c): no input is resolved from them.
WORLD_LINES = ('world ', 'wplayer ', 'wactor ', 'witem ', 'wunit ', 'wpending ')
# The records of the calls since the last state end it (parity_world.h's parity_calls_write): an item's
# description windows among them are not the UI's.
RECORD_LINES = ('call ', 'item ')


# The number fields of a control's value on its window line (src/parity/parity_world.h).
CONTROL_INTS = ('on', 'dim', 'pos', 'max', 'page', 'total', 'tab', 'row', 'top', 'open', 'masked', 'lines', 'links',
                'size')


def _unescape(text):
    r"""A text field's value: the probe writes every byte but printable ASCII other than "\", "=" and "@"
    as \xHH, and the bytes are the client's code page."""
    return re.sub(rb'\\x([0-9a-f]{2})', lambda m: bytes([int(m.group(1), 16)]),
                  text.encode('latin-1', 'replace')).decode('mbcs', 'replace')


def parse_state(path):
    """The probe's file as (fields, windows): each window a dict with class, id, rect (absolute),
    shown, modal, chat and children, and a control's value as its line gives it (parity_world.h): the
    numbers (a checkbox's on and dim, a radio button's on, a scroll bar's pos, max, page and total, a tab's
    tab, a list's row and top, a drop-down's open, a masked edit's length masked, a text viewer's lines and
    links, a tag viewer's size) as ints (a timed bar's pos as None), an edit's selection sel as a pair, a
    control's text and a text button's args unescaped (a viewer's hash is left as its hex text)."""
    fields = {}
    roots = []
    stack = []
    with open(path, encoding='mbcs', errors='replace') as f:
        for line in f:
            if line.startswith(RECORD_LINES):
                break
            if not line.strip():
                continue
            if line.startswith(WORLD_LINES):
                continue
            if line.startswith('actor '):
                a = dict(w.split('=', 1) for w in line.split()[1:])
                x, y, wdt, hgt = (int(v) for v in a['rect'].split(','))
                fields.setdefault('actors', []).append(
                    {'id': int(a['id']), 'job': int(a['job']), 'kind': int(a['kind']), 'rect': (x, y, wdt, hgt)})
                continue
            if '=' in line and not line.startswith(' ') and 'rect=' not in line:
                k, v = line.strip().split('=', 1)
                fields[k] = v
                continue
            depth = (len(line) - len(line.lstrip(' '))) // 2
            words = line.split()
            node = {'class': words[0].rstrip('@'), 'children': [], 'modal': 'modal' in words[1:],
                    'chat': 'chat' in words[1:]}
            for w in words[1:]:
                if '=' in w:
                    k, v = w.split('=', 1)
                    node[k] = v
            x, y, wdt, hgt = (int(v) for v in node.pop('rect').split(','))
            node['rel'] = (x, y, wdt, hgt)
            node['id'] = int(node['id'])
            node['shown'] = node['shown'] == '1'
            for k in CONTROL_INTS:
                if k in node:
                    # a timed bar's position is written as "*" (parity_timed_bar_parent): None
                    node[k] = None if node[k] == '*' else int(node[k])
            if 'sel' in node:
                node['sel'] = tuple(int(v) for v in node['sel'].split(','))
            for k in ('text', 'args'):
                if k in node:
                    node[k] = _unescape(node[k])
            while len(stack) > depth:
                stack.pop()
            if stack:
                px, py = stack[-1]['rect'][:2]
                x, y = x + px, y + py
                stack[-1]['children'].append(node)
            else:
                roots.append(node)
            node['rect'] = (x, y, wdt, hgt)
            stack.append(node)
    _to_pixels(path, fields, roots)
    return fields, roots


def _to_pixels(path, fields, roots):
    """Under a UI scale the probe gives windows in UI units (its "ui=" line is the screen in those
    units) while input goes in the window's pixels: every absolute window rectangle is scaled by the
    client size (parity.dll's <state>.window line) over the UI size. The units' own rectangles
    ('rel') and the actors' (drawn in pixels already) are left as they are."""
    try:
        uw, uh = (int(v) for v in fields.get('ui', '').split(','))
        with open(path + '.window') as f:
            client = next(w for w in f.read().split() if w.startswith('client='))
        cw, ch = (int(v) for v in client.split('=', 1)[1].split('x'))
    except (OSError, ValueError, StopIteration):
        return
    if (cw, ch) == (uw, uh) or not uw or not uh:
        return
    sx, sy = cw / uw, ch / uh

    def scale(nodes):
        for n in nodes:
            x, y, w, h = n['rect']
            n['rect'] = (round(x * sx), round(y * sy), round(w * sx), round(h * sy))
            scale(n['children'])

    scale(roots)


def on_screen(node):
    """The client parks controls it does not use at a negative position in their parent (-100,
    -200, -1000) and leaves them 'shown'; those count as hidden, wherever the parent's offset
    puts them on the screen."""
    rx, ry = node['rel'][:2]
    x, y, w, h = node['rect']
    return rx > -100 and ry > -100 and x + w > 0 and y + h > 0 and x < 8192 and y < 8192


def _inside(node, parent):
    """The node's centre lies in its parent's rectangle (a row scrolled out of a list does not)."""
    x, y, w, h = node['rect']
    px, py, pw, ph = parent['rect']
    cx, cy = x + w // 2, y + h // 2
    return px <= cx < px + pw and py <= cy < py + ph


def find_ui(windows, path, shown_only=True):
    """The windows matching a path such as 'UIEquipWnd/UIBitmapButton#123', in tree order."""
    found = []

    def matches(node, seg):
        cls, _, wid = seg.partition('#')
        return (node['class'] == cls and (not wid or node['id'] == int(wid)) and
                ((node['shown'] and on_screen(node)) or not shown_only))

    def walk(nodes, segs, parent):
        for n in nodes:
            if shown_only and parent is not None and not _inside(n, parent):
                continue
            if matches(n, segs[0]):
                if len(segs) == 1:
                    found.append(n)
                else:
                    walk(n['children'], segs[1:], n)

    walk(windows, path.split('/'), None)
    return found


# ---- Seq -----------------------------------------------------------------------------------

class Seq:
    """Builds a scenario's steps one action after another; each action takes its frames and
    then waits `then` frames."""

    def __init__(self, start=0):
        self.frame = start
        self.steps = []

    def at(self, frame):
        self.frame = max(self.frame, frame)
        return self

    def wait(self, frames):
        self.frame += frames
        return self

    def _input(self, frames, then):
        self.steps.append((self.frame, 'input', frames))
        self.frame += len(frames) + then
        return self

    def key(self, spec, then=8):
        return self._input(key(spec), then)

    def text(self, s, then=4):
        return self._input(text(s), then)

    def type(self, s, then=4):
        return self._input(typed(s), then)

    def say(self, s, then=30):
        """A line typed into the focused chat input and sent with Enter."""
        return self.text(s, 2).key('enter', then)

    def click(self, x, y, then=10, button='left', double=False, mods=()):
        return self._input(click(x, y, button, double, mods), then)

    def rclick(self, x, y, then=10, mods=()):
        return self.click(x, y, then, 'right', mods=mods)

    def drag(self, x1, y1, x2, y2, then=10, button='left', steps=8, mods=()):
        return self._input(drag(x1, y1, x2, y2, button, steps, mods), then)

    def wheel(self, x, y, notches, then=10, mods=()):
        return self._input(wheel(x, y, notches, mods), then)

    def hover(self, x, y, then=30):
        return self._input(hover(x, y), then)

    def click_ui(self, path, then=10, button='left', double=False, at=(0.5, 0.5), index=0, mods=()):
        """A click on a window found in the state probe's tree: a state the frame before, then
        the click at the point `at` (fractions of its rectangle)."""
        self.steps.append((self.frame, 'click_ui', (path, button, double, at, index, tuple(mods))))
        self.frame += 1 + len(click(0, 0, button, double, mods)) + then
        return self

    def hover_ui(self, path, then=30, at=(0.5, 0.5), index=0):
        self.steps.append((self.frame, 'hover_ui', (path, at, index)))
        self.frame += 2 + then
        return self

    def click_each(self, path, prefix, at=((0.5, 0.5),), per=24, limit=40, where=None, snap=True):
        """Every window matching `path` when the step runs (at most `limit`), each clicked at
        each point of `at` (fractions of its rectangle), with a capture and a state after each
        window as '<prefix>_<n>'. `where(node, windows)` can leave windows out (by their rectangle,
        or by where other windows of the tree are).
        The step takes its whole budget of frames, however many windows it found."""
        self.steps.append((self.frame, 'click_each', (path, prefix, tuple(at), per, limit, where, snap)))
        self.frame += 1 + limit * per * len(at) + 4
        return self

    def converse(self, prefix, choices=(0,), rounds=12, per=40):
        """Talks an NPC dialog through: each round takes a state, clicks the say dialog's button
        (Next, Close) or answers a menu, and snaps '<prefix>_<n>', until no dialog is left or
        `rounds` are used. The n-th menu gets choices[n] (a row number, or 'cancel'); a menu
        beyond the list is cancelled. The step takes its whole budget of frames."""
        self.steps.append((self.frame, 'converse', (prefix, rounds, per, tuple(choices), 0, 0)))
        self.frame += 1 + rounds * (per + 1) + 4
        return self

    def click_actor(self, near, kind=0, job=None, then=10, button='left', double=False, mods=(), lead=0):
        """A click on a unit the last frame drew (the state probe's actors): of kind `kind` (0 an
        actor, 1 an item on the ground, 4 a companion) and job `job` when given, the one whose
        drawn rectangle's centre is nearest `near`. With `lead`, the pointer rests on it that many
        frames before the press, for paths that act on the actor the pointer already picked (an
        Alt press commanding a companion) rather than picking at the press."""
        self.steps.append((self.frame, 'click_actor', (near, kind, job, button, double, tuple(mods), lead)))
        self.frame += 1 + lead + len(click(0, 0, button, double, mods)) + then
        return self

    def baseline(self, then=1):
        """Remembers which top-level windows are shown now, for close_new."""
        self.steps.append((self.frame, 'baseline', None))
        self.frame += 1 + then
        return self

    def close_new(self, then=20):
        """Closes every top-level window shown since the last baseline: its close button
        (UIBitmapButton#193) when it has one, Escape when it has not."""
        self.steps.append((self.frame, 'close_new', None))
        self.frame += 1 + 4 * 6 + then
        return self

    def open_close(self, open_path, name, settle=40):
        """A click that opens something, its frame and state, and everything it opened closed."""
        self.baseline().click_ui(open_path, then=settle).snap(name).close_new()
        return self

    def expect(self, path, present=True, then=0):
        """Checks the state probe's tree for a window: a run that does not see it as expected
        reports the step (record mode stops being trustworthy there)."""
        self.steps.append((self.frame, 'expect', (path, present)))
        self.frame += 1 + then
        return self

    def call(self, op, *args, then=0, guard=False):
        """Has the client's probe carry out a call between this frame and the next (parity_world.h's
        ParityCallOp: op 1 makes window args[0] through the window manager, op 2 closes it); the next state
        lists it with its result. `guard` runs it under an exception handler, for a survey of which calls
        fault."""
        word = op | (PARITY_CALL_GUARD if guard else 0)
        return self._input([[(CMD_CALL, u32(word, *args))]], then)

    def item(self, words, then=0, hold=False, guard=False):
        """An item's description (parity_world.h's PARITY_CALL_ITEM_SHOW): `words` its ParityItemWord fields
        (item_words builds them); its line and, unless `hold`, its description windows go to the next
        state's records. A held window is recorded and closed by item_record."""
        flags = words[-1] | (PARITY_ITEM_HOLD if hold else 0)
        return self.call(PARITY_CALL_ITEM_SHOW, *words[:-1], flags, then=then, guard=guard)

    def item_record(self, then=0, guard=False):
        return self.call(PARITY_CALL_ITEM_RECORD, then=then, guard=guard)

    def window_sweep(self, ids, prefix, settle=4, after_close=3, guard=False):
        """Each window id in turn: made through the window manager, a state the next frame
        ('<prefix>_<id>_open') and `settle` frames later ('_later'), closed with PostQuit1, and a state
        `after_close` frames after that ('_closed'). The calls and their results are in the states."""
        for wid in ids:
            name = f'{prefix}_{wid:03x}'
            self.call(PARITY_CALL_MAKE_WINDOW, wid, guard=guard).state(name + '_open', settle)
            self.state(name + '_later').call(PARITY_CALL_CLOSE_WINDOW, wid, then=after_close, guard=guard)
            self.state(name + '_closed', 1)
        return self

    def capture(self, name, then=0):
        self.steps.append((self.frame, 'capture', name))
        self.frame += then
        return self

    def state(self, name, then=0):
        self.steps.append((self.frame, 'state', name))
        self.frame += then
        return self

    def snap(self, name, then=0):
        """A capture and a state of the same frame."""
        self.capture(name)
        return self.state(name, then)

    def quit(self):
        self.steps.append((self.frame + 1, 'quit', None))
        return self

    def exit_game(self, within=600):
        """The client left the way a player leaves it, Game Options' Exit to Windows, so its own
        exit runs (the window places and settings it saves then); the run fails when the client
        is still running `within` frames later."""
        self.click_ui('UIMenuIconWnd/UIMenuIcon#185', then=30)
        self.click_ui('UIEscOptionWnd/UIBitmapButton#243', then=10)
        self.steps.append((self.frame + within, 'exit_by', None))
        return self
