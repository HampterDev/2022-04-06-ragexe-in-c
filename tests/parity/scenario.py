"""Scenarios for the parity runner: scripted input plus one of three networks.

A scenario is a list of steps, each run at a frame number (inputs.Seq builds them): input,
clicks on windows the state probe finds, expectations, captures, state dumps and the quit. The
network decides where the client's connections go:

    Offline   every connect is refused.
    Record    the connections are relayed to a real server and everything both sides sent is
              kept, frame by frame. The client is paced to real time so the server's timers and
              the client's clock agree.
    Replay    nothing leaves the machine: a recorded server answer is delivered once the client
              has sent as many bytes on that connection as when the server answered, and as
              many frames later. What the client sends is compared with the recording.
"""
import json
import os
import selectors
import socket
import struct
import time

import inputs
from inputs import u32 as _u32
from protocol import (CMD_CAPTURE, CMD_CONNECTED, CMD_DELIVER, CMD_PEER_CLOSE, CMD_QUIT, CMD_REGDUMP, CMD_STATE,
                      SYNC_FRAME, SYNC_NETWAIT)

FRAME_MS = 16

def _path(p):
    return p.encode('mbcs') + b'\0'


# ---- input ---------------------------------------------------------------------------------

# The close button of a window whose title bar has none (UIBitmapButton#193 elsewhere).
CLOSE_BUTTONS = {
    'UIReplayRECControlWnd': 417,
    'UIEscOptionWnd': 177,          # Return to game
    'UIGuildHelperWnd': 176,        # OK
    'UIChooseSellBuyWnd': 177,      # Cancel
    'UIItemPurchaseWnd': 177,       # Cancel (the shop's list closes with it)
    'UIItemSellWnd': 177,           # Cancel (likewise)
    'UIMerchantItemPurchaseWnd': 177,   # Cancel (the vendor's list closes with it)
    'UIMerchantShopMakeWnd': 177,   # Cancel (the cart's mirror closes with it)
}
# An NPC's menu: the window, the list in it whose rows are the choices, a row's height.
MENU_WINDOWS = {'UIChooseWnd', 'UIChooseWarpWnd'}
MENU_LISTS = {'UIListBox', 'UIChooseListBox'}
MENU_ROW_HEIGHT = 18
MENU_CANCEL = 177             # the menu's Cancel button
# Top-level windows that are not closed (overlays drawn over actors, not dialogs).
NOT_CLOSED = {'UITransBalloonText', 'UINameBalloonText', 'UIPlayerGage', 'UIQuestDisplay',
              'UIItemShopWnd',       # the shop's item list closes with its cart
              'UIMerchantItemShopWnd', 'UIMerchantMirrorItemWnd',  # likewise for vending
              'UIMerchantShopTitle',  # a stall's sign stays while the stall is open
              'UIMiniPartyWnd',      # a party member's plate stays where it was dropped
              'UINotifyItemObtainWnd'}  # the 'item obtained' notice fades by itself


class Script:
    """Plays a scenario's steps: (frame, kind, argument) as Seq builds them (input, click_ui,
    hover_ui, expect, capture, state, quit), and the older literal kinds key, text and click."""

    def __init__(self, steps):
        self.pending = {}           # frame -> commands
        self.actions = {}           # frame -> [(kind, argument)] carried out while running
        self.quit_frame = None
        self.exit_by = None         # the client is to have ended itself by this frame
        for frame, kind, arg in steps:
            if kind == 'input':
                self._schedule(frame, arg)
            elif kind == 'key':
                self._schedule(frame, inputs.key(arg))
            elif kind == 'text':
                self._schedule(frame, inputs.text(arg))
            elif kind == 'click':
                self._schedule(frame, inputs.click(*arg))
            elif kind == 'quit':
                self.quit_frame = frame
            elif kind == 'exit_by':
                self.exit_by = frame
            elif kind in ('capture', 'state', 'click_ui', 'hover_ui', 'expect', 'baseline', 'close_new', 'click_each',
                          'converse', 'click_actor'):
                self.actions.setdefault(frame, []).append((kind, arg))
            else:
                raise ValueError(kind)

    def _schedule(self, frame, frames):
        for i, cmds in enumerate(frames):
            self.pending.setdefault(frame + i, []).extend(cmds)

    def _probe_path(self, run, frame):
        return os.path.join(run.out_dir, f'_ui_{frame:06d}.txt')

    def commands(self, run, frame):
        cmds = []
        due = self.actions.pop(frame, [])
        for kind, (taken, arg) in [a for a in due if a[0].startswith('resolve_')]:
            self._resolve(run, frame, kind[len('resolve_'):], taken, arg)
        for kind, arg in [a for a in due if not a[0].startswith('resolve_')]:
            if kind == 'capture':
                path = os.path.join(run.out_dir, f'{arg}.bmp')
                run.captures[arg] = path
                cmds.append((CMD_CAPTURE, _path(path)))
            elif kind == 'state':
                path = os.path.join(run.out_dir, f'{arg}.state.txt')
                run.states[arg] = path
                cmds.append((CMD_STATE, _path(path)))
            else:
                # Resolved in the next frame from a state taken now.
                cmds.append((CMD_STATE, _path(self._probe_path(run, frame))))
                self.actions.setdefault(frame + 1, []).append(('resolve_' + kind, (frame, arg)))
        cmds += self.pending.pop(frame, [])
        if self.exit_by is not None and frame >= self.exit_by:
            run.expect_failures.append(f'frame {frame}: the client is still running after its exit')
            self.quit_frame = frame
        if self.quit_frame is not None and frame >= self.quit_frame:
            cmds.append((CMD_REGDUMP, _path(os.path.join(run.out_dir, 'registry_end.txt'))))
            cmds.append((CMD_QUIT, b''))
        return cmds

    def _find(self, windows, path, **kw):
        """find_ui, where a path starting with '@new/' looks only at top-level windows shown
        since the last baseline (the custom windows all share one class)."""
        if path.startswith('@new/'):
            base = getattr(self, 'baseline', set())
            fresh = [w for w in windows if (w['class'], w['id']) not in base]
            return inputs.find_ui(fresh, path[len('@new/'):], **kw)
        return inputs.find_ui(windows, path, **kw)

    def _resolve(self, run, frame, kind, taken, arg):
        state = self._probe_path(run, taken)
        if not os.path.exists(state):
            run.expect_failures.append(f'frame {taken}: no state probe in this client for {kind} {arg[0]}')
            return
        fields, windows = inputs.parse_state(state)
        if kind == 'click_actor':
            near, want_kind, job, button, double, mods, lead = arg
            actors = [a for a in fields.get('actors', []) if a['kind'] == want_kind and (job is None or a['job'] == job)]
            if not actors:
                run.expect_failures.append(f'frame {taken}: no actor of kind {want_kind} job {job} to click')
                return

            def dist(a):
                x, y, w, h = a['rect']
                return (x + w / 2 - near[0]) ** 2 + (y + h / 2 - near[1]) ** 2

            x, y, w, h = min(actors, key=dist)['rect']
            px, py = x + w // 2, y + h * 2 // 3
            hold = inputs.hover(px, py) + [[] for _ in range(lead - 1)] if lead else []
            self._schedule(frame, hold + inputs.click(px, py, button, double, mods))
            return
        if kind == 'baseline':
            self.baseline = {(w['class'], w['id']) for w in windows if w['shown']}
            return
        if kind == 'close_new':
            when = frame
            for w in windows:
                if (not w['shown'] or w['class'] in NOT_CLOSED or
                        (w['class'], w['id']) in getattr(self, 'baseline', set())):
                    continue
                want = CLOSE_BUTTONS.get(w['class'], 193)
                close = [c for c in w['children'] if c['class'] == 'UIBitmapButton' and c['id'] == want and c['shown']
                         and inputs.on_screen(c)]
                if not close:
                    run.expect_failures.append(f'frame {taken}: no close button found on {w["class"]}#{w["id"]}')
                    continue
                x, y, cw, ch = close[0]['rect']
                self._schedule(when, inputs.click(x + cw // 2, y + ch // 2))
                when += 4
            return
        if kind == 'converse':
            prefix, rounds, per, choices, n, menus_seen = arg
            say = inputs.find_ui(windows, 'UISayDialogWnd')
            menus = [w for w in windows if w['shown'] and w['class'] in MENU_WINDOWS and inputs.on_screen(w)]
            asks = [w for w in windows if w['shown'] and ('Input' in w['class'] or 'EditDialog' in w['class'])
                    and inputs.on_screen(w)]
            target = None
            if asks:
                # An NPC's input box (UINpcEditDialogWnd asks for numbers and words alike): 42
                # answers both.
                target = inputs.text('42') + inputs.key('enter')
            elif menus:
                choice = choices[menus_seen] if menus_seen < len(choices) else 'cancel'
                menus_seen += 1
                items = [c for c in menus[0]['children'] if c['shown'] and inputs.on_screen(c)]
                lists = [c for c in items if c['class'] in MENU_LISTS]
                cancel = [c for c in items if c['class'] == 'UIBitmapButton' and c['id'] == MENU_CANCEL]
                if choice == 'cancel' and cancel:
                    x, y, w, h = cancel[0]['rect']
                    target = inputs.click(x + w // 2, y + h // 2)
                elif lists:
                    x, y, w, h = lists[0]['rect']
                    row = MENU_ROW_HEIGHT * int(choice if choice != 'cancel' else 0) + MENU_ROW_HEIGHT // 2
                    target = inputs.click(x + w // 3, y + min(row, h - 2), double=True)
            elif say:
                # The say dialog's one button (Next, Close) is drawn at its bottom right; the
                # position the button keeps can be the dialog's size instead of where it is drawn.
                x, y, w, h = say[0]['rect']
                target = inputs.click(x + w - 28, y + h - 17)
            if target is None or n >= rounds:
                return
            self._schedule(frame, target)
            done = frame + len(target) + per - 4
            self.actions.setdefault(done, []).extend([('capture', f'{prefix}_{n:02d}'), ('state', f'{prefix}_{n:02d}')])
            self.actions.setdefault(done + 1, []).append(('converse', (prefix, rounds, per, choices, n + 1, menus_seen)))
            return
        if kind == 'click_each':
            path, prefix, points, per, limit, where, snap = arg
            found = [n for n in self._find(windows, path) if not where or where(n, windows)][:limit]
            when = frame
            for i, node in enumerate(found):
                x, y, w, h = node['rect']
                for fx, fy in points:
                    self._schedule(when, inputs.click(x + int(w * fx), y + int(h * fy)))
                    when += per
                if snap:
                    self.actions.setdefault(when - 2, []).extend(
                        [('capture', f'{prefix}_{i:02d}'), ('state', f'{prefix}_{i:02d}')])
            return
        if kind == 'expect':
            path, present = arg
            if bool(self._find(windows, path)) != present:
                run.expect_failures.append(f'frame {taken}: {path} {"missing" if present else "still shown"}')
            return
        if kind == 'click_ui':
            path, button, double, at, index, mods = arg
        else:
            path, at, index = arg
        found = self._find(windows, path)
        if len(found) <= index:
            run.expect_failures.append(f'frame {taken}: {path} not found for {kind}')
            return
        x, y, w, h = found[index]['rect']
        px, py = x + int(w * at[0]), y + int(h * at[1])
        frames = inputs.click(px, py, button, double, mods) if kind == 'click_ui' else inputs.hover(px, py)
        self._schedule(frame, frames)


# ---- networks ------------------------------------------------------------------------------

class Offline:
    def on_events(self, run, events, reason, frame):
        return [(CMD_CONNECTED, _u32(e[1], 0)) for e in events if e[0] == 'connect']

    def finish(self, run):
        pass


class Record:
    """Relays the client's connections to a real server and keeps both directions.

    `server_host` replaces every address the client dials and `port_map` the ports it names
    (the login port from clientinfo.xml), for a client whose configured server is not the test
    server. The recording keeps the address and port the client asked for."""

    def __init__(self, path, server_host=None, port_map=None, netwait_ms=300):
        self.path = path
        self.server_host = server_host
        self.port_map = port_map or {}
        self.netwait_ms = netwait_ms
        self.sel = selectors.DefaultSelector()
        self.conns = {}          # client socket id -> connection record
        self.order = []
        self.t0 = None

    def _ip(self, ip):
        return socket.inet_ntoa(struct.pack('<I', ip))

    def on_events(self, run, events, reason, frame):
        cmds = []
        if self.t0 is None:
            self.t0 = time.monotonic()
        for e in events:
            if e[0] == 'connect':
                _, sid, ip, port = e
                host = self.server_host or self._ip(ip)
                rec = {'ip': self._ip(ip), 'port': port, 'connect_frame': frame, 'events': [], 'sent': 0}
                try:
                    s = socket.create_connection((host, self.port_map.get(port, port)), timeout=5)
                    s.setblocking(False)
                    s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                    self.sel.register(s, selectors.EVENT_READ, sid)
                    rec['sock'] = s
                    cmds.append((CMD_CONNECTED, _u32(sid, 1)))
                except OSError as err:
                    rec['refused'] = str(err)
                    cmds.append((CMD_CONNECTED, _u32(sid, 0)))
                self.conns[sid] = rec
                self.order.append(rec)
            elif e[0] == 'send':
                rec = self.conns.get(e[1])
                if rec and 'sock' in rec:
                    rec['sock'].sendall(e[2])
                    rec['events'].append({'c': frame, 'data': e[2].hex()})
                    rec['sent'] += len(e[2])
            elif e[0] == 'close':
                rec = self.conns.pop(e[1], None)
                if rec and 'sock' in rec:
                    self.sel.unregister(rec['sock'])
                    rec['sock'].close()
                    rec['events'].append({'close': frame})
                    del rec['sock']
        if reason == SYNC_FRAME:
            # Real time catches up with the client's clock before the frame's deliveries.
            lag = self.t0 + frame * FRAME_MS / 1000.0 - time.monotonic()
            if lag > 0:
                time.sleep(lag)
        wait = self.netwait_ms / 1000.0 if reason == SYNC_NETWAIT else 0
        cmds += self._collect(frame, wait)
        return cmds

    def _collect(self, frame, wait):
        cmds = []
        deadline = time.monotonic() + wait
        while True:
            if not self.sel.get_map():
                return cmds         # Windows' select refuses an empty set
            ready = self.sel.select(timeout=max(0.0, deadline - time.monotonic()))
            for keyobj, _ in ready:
                sid = keyobj.data
                rec = self.conns[sid]
                try:
                    data = keyobj.fileobj.recv(1 << 16)
                except BlockingIOError:
                    continue
                if not data:
                    self.sel.unregister(keyobj.fileobj)
                    rec['events'].append({'peer_close': frame, 'after': rec['sent']})
                    cmds.append((CMD_PEER_CLOSE, _u32(sid)))
                    continue
                rec['events'].append({'s': frame, 'after': rec['sent'], 'data': data.hex()})
                cmds.append((CMD_DELIVER, _u32(sid) + data))
            if cmds or time.monotonic() >= deadline:
                return cmds

    def finish(self, run):
        for rec in self.order:
            sock = rec.pop('sock', None)
            if sock is not None:
                # The client is gone: the server must see its connections end, or it keeps the
                # account logged in and refuses the next session's login.
                try:
                    sock.close()
                except OSError:
                    pass
            rec.pop('sent', None)
        with open(self.path, 'w') as f:
            json.dump({'connections': self.order}, f, indent=1)


# The map server's optional packets. A recording holds what the map server sent the recording client,
# whose CZ_CLIENT_CAPS (0x0bf6, id.W caps.L) asks for every one of them; the server sends each only to
# a client whose caps carry its bit, so a replayed client whose caps lack the bit is not sent it. Each
# of these is an addition: the server sends nothing else in its place.
CAPS_ADDITIONS = {
    0x0aaa: 0,      # ZC_CAST_PROGRESS
    0x0b85: 4,      # ZC_HOMUN_STATS
    0x0be4: 8,      # ZC_BANK_MASTER
    0x0be5: 9,      # ZC_STYLIST
    0x0be8: 10,     # ZC_COMPANION_PALETTES
    0x0be9: 10,     # ZC_FALCON_PALETTE
    0x0bea: 10,     # ZC_MOUNT_PALETTE
    0x0b8b: 11,     # ZC_WALK_STYLE
    0x0bec: 15,     # ZC_WALK_ROUTE
    0x0bed: 17,     # ZC_SERVER_SETTINGS
    0x0bee: 18,     # ZC_SHOP_WEIGHTS
    0x0bef: 19,     # ZC_STALL_SETUPS
    0x0bdf: 20,     # ZC_COMMANDS
}
# ZC_WALK_STOP (id.W len.W aid.L x.W y.W offx.B offy.B) stands in for the ZC_STOPMOVE (0x0088, id.W
# aid.L x.W y.W) of the same stop, which a client without the bit is sent instead. The other caps
# bits (color chat, dual beams, homunculus averages, beam flags, whisper replay) change what the
# server sends in ways a recording cannot be turned back from, so their packets are left as they
# were recorded.
WALK_STOP, WALK_STOP_BIT, STOPMOVE = 0x0beb, 14, 0x0088
SHAPED_BITS = sum(1 << b for b in set(CAPS_ADDITIONS.values()) | {WALK_STOP_BIT})
CZ_ENTER, CZ_CLIENT_CAPS = 0x0436, 0x0bf6
KEEPALIVE = struct.pack('<H', 0x0360)    # CZ_REQUEST_TIME2: id.W tick.L
CZ_QUIT_GAME = struct.pack('<HH', 0x018a, 0)              # id.W type.W, Exit to Windows
ZC_ACK_REQ_DISCONNECT = struct.pack('<HH', 0x018b, 0)     # id.W result.W, 0: the client may go
PACKET_TABLE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src', 'core',
                            'packet_table.c')


def packet_sizes():
    """The client's packet length table (ragexe's src/core/packet_table.c, the original client's
    table): id -> its size, -1 for one that carries its length at +2. Both clients frame an id the
    table lacks by that length word too."""
    import re
    with open(PACKET_TABLE, encoding='utf-8', errors='replace') as f:
        text = f.read()
    sizes = {}
    for m in re.finditer(r'\{\s*0x([0-9a-fA-F]+),\s*(-?\d+),\s*-?\d+,\s*\d+\s*\}', text):
        sizes.setdefault(int(m.group(1), 16), int(m.group(2)))
    return sizes


class Replay:
    """Plays a recording's server side to a client and compares what the client sends.

    A server chunk is due once the client has sent as many bytes on its connection as it had
    when the server sent the chunk, and as many frames have passed since the client reached
    that byte count (or connected) as passed in the recording. At a network wait the frame
    count is not waited for: the client is blocked until something arrives.

    On a map server connection the chunks are framed into packets and shaped to the client's own
    caps (CAPS_ADDITIONS, WALK_STOP): the server's answer to this client, not to the recording's.
    A packet split across chunks goes out whole with the chunk that completes it, which the client
    could not have read sooner."""

    STALL_SYNCS = 200

    def __init__(self, path, answer_quit=False):
        """`answer_quit`: a CZ_QUIT_GAME the recording does not have where the client sends it is answered
        at once with the map server's ZC_ACK_REQ_DISCONNECT, so a session played against another
        scenario's recording can leave through Exit to Windows, which saves what the client keeps."""
        with open(path) as f:
            self.recorded = json.load(f)['connections']
        if os.environ.get('PARITY_SERVER_VARIANT'):
            import variants
            self.recorded = variants.server(self.recorded, os.environ['PARITY_SERVER_VARIANT'], packet_sizes())
        self.answer_quit = answer_quit
        self.next_conn = 0
        self.live = {}           # client socket id -> replay state
        self.divergences = []
        self.stalled = 0
        self.sizes = packet_sizes()
        self.shaped = {}         # what the shaping left out or turned back, by packet id: count
        self.tick_slack = []     # the keepalive ticks taken within a frame of the recording's

    def on_events(self, run, events, reason, frame):
        cmds = []
        for e in events:
            if e[0] == 'connect':
                cmds += self._connect(e, frame)
            elif e[0] == 'send':
                st = self.live.get(e[1])
                if st:
                    before = len(st['sent'])
                    st['sent'] += e[2]
                    st['sends'].append((len(st['sent']), frame))
                    self._compare(st, before, frame)
                    if e[2][:2] == struct.pack('<H', CZ_CLIENT_CAPS) and len(e[2]) >= 6:
                        st['caps'] = struct.unpack_from('<I', e[2], 2)[0]
                    if (self.answer_quit and st['framed'] and e[2][:4] == CZ_QUIT_GAME
                            and st['expected'][before:before + 4] != CZ_QUIT_GAME):
                        cmds.append((CMD_DELIVER, _u32(e[1]) + ZC_ACK_REQ_DISCONNECT))
            elif e[0] == 'close':
                self.live.pop(e[1], None)
        for sid, st in self.live.items():
            cmds += self._due(sid, st, frame, reason)
        if reason == SYNC_NETWAIT and not cmds:
            self.stalled += 1
            if self.stalled == self.STALL_SYNCS:
                self.divergences.append(f'frame {frame}: the client waits for server data the recording '
                                        f'only has after bytes it has not sent')
                cmds.append((CMD_QUIT, b''))
        else:
            self.stalled = 0
        return cmds

    def _connect(self, e, frame):
        _, sid, ip, port = e
        if self.next_conn >= len(self.recorded):
            self.divergences.append(f'frame {frame}: an extra connection to port {port}')
            return [(CMD_CONNECTED, _u32(sid, 0))]
        rec = self.recorded[self.next_conn]
        self.next_conn += 1
        if rec['port'] != port:
            self.divergences.append(f'frame {frame}: connection {self.next_conn} to port {port}, '
                                    f'recorded {rec["port"]}')
        if 'refused' in rec:
            return [(CMD_CONNECTED, _u32(sid, 0))]
        sends = [bytes.fromhex(ev['data']) for ev in rec['events'] if 'c' in ev]
        expected = b''.join(sends)
        recorded_sends = [(0, rec['connect_frame'])]
        total = 0
        for ev in rec['events']:
            if 'c' in ev:
                total += len(ev['data']) // 2
                recorded_sends.append((total, ev['c']))
        self.live[sid] = {'expected': expected, 'sent': b'', 'sends': [(0, frame)], 'mask': self._mask(sends),
                          'recorded_sends': recorded_sends, 'index': self.next_conn,
                          'server': [ev for ev in rec['events'] if 's' in ev or 'peer_close' in ev],
                          # a map server connection opens with CZ_ENTER; its stream is all packets
                          'framed': expected[:2] == struct.pack('<H', CZ_ENTER), 'carry': b'', 'carry_sent': False,
                          'caps': None}
        return [(CMD_CONNECTED, _u32(sid, 1))]

    @staticmethod
    def _reached(sends, after):
        """The frame at which a connection's sent byte count first reached `after`."""
        for total, frame in sends:
            if total >= after:
                return frame
        return None

    @staticmethod
    def _mask(sends):
        """Byte ranges of a connection whose content is not compared, all of them bytes the server
        does not read: the MD5 of the client's own exe that a login connection opens with
        (CA_EXE_HASHCHECK 0x0204), different for every build; the four bytes the client leaves unset
        in the CZ_ENTER (0x0436, 23 bytes) a map connection opens with, which hold whatever its stack
        held; in a CZ_REQ_WRITE_MAIL (0x0a6e) the bytes after the receiver's and the sender's names
        (24 bytes each from +4), which hold whatever its buffer held past each NUL; and in a
        CZ_ADD_FRIENDS (0x0202) the bytes after the name's NUL (24 from +2) and the bytes the client's
        length table sends past the packet's 26, which hold whatever its stack held; and in a
        CZ_USE_SKILL_TOGROUND_WITHTALKBOX2 (0x0367, 31 bytes: a Talkie Box's or a graffiti's text from +10)
        the bytes after the text's NUL, which hold whatever its stack held; and in a CZ_REQ_OPENSTORE2
        (0x01b2, the shop's title from +4, 80 bytes) the bytes after the title's NUL, which hold whatever
        its buffer held (a shop window cancelled sends an empty title), and likewise in a
        CZ_REQ_OPEN_BUYING_STORE (0x0811, the title from +9, 80 bytes); and in a CA_LOGIN (0x0064, 55
        bytes: the user id from +6 and the password from +30, 24 bytes each) the bytes after each one's
        NUL, which hold whatever the login mode's heap object held (the original's CLoginMode never
        clears them). sends are the connection's recorded sends, in order."""
        expected = b''.join(sends)
        ranges = []
        if expected[:2] == b'\x04\x02' and len(expected) >= 18:
            ranges.append((2, 18))
        if expected[:2] == b'\x36\x04' and len(expected) >= 23:
            ranges.append((14, 18))
        pos = 0
        for data in sends:
            if data[:2] == b'\x64\x00' and len(data) >= 55:
                for at in (6, 30):
                    end = data.find(b'\0', at, at + 24)
                    if end >= 0:
                        ranges.append((pos + end + 1, pos + at + 24))
            if data[:2] == b'\x6e\x0a' and len(data) >= 52:
                for at in (4, 28):
                    end = data.find(b'\0', at, at + 24)
                    if end >= 0:
                        ranges.append((pos + end + 1, pos + at + 24))
            if data[:2] == b'\x02\x02' and len(data) >= 26:
                end = data.find(b'\0', 2, 26)
                if end >= 0:
                    ranges.append((pos + end + 1, pos + 26))
                ranges.append((pos + 26, pos + len(data)))
            if data[:2] == b'\x67\x03' and len(data) >= 31:
                end = data.find(b'\0', 10, 31)
                if end >= 0:
                    ranges.append((pos + end + 1, pos + 31))
            if data[:2] == b'\xb2\x01' and len(data) >= 84:
                end = data.find(b'\0', 4, 84)
                if end >= 0:
                    ranges.append((pos + end + 1, pos + 84))
            if data[:2] == b'\x11\x08' and len(data) >= 89:
                end = data.find(b'\0', 9, 89)
                if end >= 0:
                    ranges.append((pos + end + 1, pos + 89))
            pos += len(data)
        return ranges

    def _compare(self, st, before, frame):
        got, want = bytearray(st['sent']), st['expected']
        n = min(len(got), len(want))
        for lo, hi in st['mask']:
            got[max(lo, before):min(hi, n)] = want[max(lo, before):min(hi, n)]
        self._tick_slack(got, want, before, n, frame)
        if got[before:n] != want[before:n]:
            at = next(i for i in range(before, n) if got[i] != want[i])
            self.divergences.append(f'frame {frame}: connection {st["index"]} sent byte {at} as '
                                    f'{got[at:at + 16].hex()}, recorded {want[at:at + 16].hex()}')
        elif len(got) > len(want) and before <= len(want):
            self.divergences.append(f'frame {frame}: connection {st["index"]} sent more than recorded '
                                    f'({len(got)} > {len(want)} bytes)')

    def _tick_slack(self, got, want, before, n, frame):
        """A keepalive (CZ_REQUEST_TIME2, id.W tick.L) whose tick is under a frame from the recorded
        one is taken as it: within a frame the virtual clock moves only by the reads that spin it
        (parity.dll), and the original client and its C port read the clock a different number of
        times. A tick a frame or more off still differs."""
        i = want.find(KEEPALIVE, max(before - 5, 0), n)
        while i != -1 and i + 6 <= n:
            if got[i:i + 2] == KEEPALIVE and got[i + 2:i + 6] != want[i + 2:i + 6]:
                a, b = struct.unpack_from('<I', want, i + 2)[0], struct.unpack_from('<I', got, i + 2)[0]
                if abs(a - b) < FRAME_MS:
                    got[i + 2:i + 6] = want[i + 2:i + 6]
                    self.tick_slack.append(f'frame {frame}: tick {b} for {a}')
            i = want.find(KEEPALIVE, i + 1, n)

    def _due(self, sid, st, frame, reason):
        cmds = []
        while st['server']:
            ev = st['server'][0]
            now_anchor = self._reached(st['sends'], ev['after'])
            if now_anchor is None:
                break
            was_anchor = self._reached(st['recorded_sends'], ev['after'])
            recorded_at = ev['s'] if 's' in ev else ev['peer_close']
            if reason == SYNC_FRAME and frame - now_anchor < recorded_at - was_anchor:
                break
            st['server'].pop(0)
            if 's' in ev:
                data = bytes.fromhex(ev['data'])
                if st['framed']:
                    data = self._shape(st, data)
                if data:
                    cmds.append((CMD_DELIVER, _u32(sid) + data))
            else:
                cmds.append((CMD_PEER_CLOSE, _u32(sid)))
        return cmds

    def _shape(self, st, data):
        """A map server chunk as the server would have sent it to this client: each of
        CAPS_ADDITIONS left out and each ZC_WALK_STOP turned back into its ZC_STOPMOVE while the
        client's caps lack the bit, its packets whole. Every chunk is framed, to keep the packet
        boundaries, but goes out as recorded before the client's first caps (the server's flags
        start clear) and while its caps carry all of the bits; a packet whose start went out so is
        finished as recorded. A stream that cannot be framed goes out as recorded from there on."""
        buf = st['carry'] + data
        sent = len(st['carry']) if st['carry_sent'] else 0     # bytes of buf the client already has
        caps = st['caps']
        shaping = caps is not None and caps & SHAPED_BITS != SHAPED_BITS
        out, i = bytearray(), 0
        while i + 2 <= len(buf):
            pid = buf[i] | buf[i + 1] << 8
            size = self.sizes.get(pid, -1)
            if size == -1:
                if i + 4 > len(buf):
                    break                       # its length word has not come yet
                size = buf[i + 2] | buf[i + 3] << 8
            if size < 2:
                st['framed'] = False
                break
            if i + size > len(buf):
                break
            pkt, at = buf[i:i + size], i
            i += size
            if at < sent:
                out += pkt[sent - at:] if i > sent else b''
            elif not shaping:
                out += pkt
            elif CAPS_ADDITIONS.get(pid) is not None and not caps >> CAPS_ADDITIONS[pid] & 1:
                self.shaped[pid] = self.shaped.get(pid, 0) + 1
            elif pid == WALK_STOP and not caps >> WALK_STOP_BIT & 1 and size >= 12:
                self.shaped[pid] = self.shaped.get(pid, 0) + 1
                out += struct.pack('<H', STOPMOVE) + pkt[4:12]
            else:
                out += pkt
        rest = buf[i:]
        if not st['framed'] or not shaping or i < sent:
            # the rest goes out now: unframeable, not held back, or the tail of a packet already started
            out += rest[max(sent - i, 0):]
            st['carry'], st['carry_sent'] = (rest, True) if st['framed'] else (b'', False)
        else:
            st['carry'], st['carry_sent'] = rest, False
        return bytes(out)

    def finish(self, run):
        for st in self.live.values():
            if len(st['sent']) < len(st['expected']):
                self.divergences.append(f'connection {st["index"]}: sent {len(st["sent"])} of '
                                        f'{len(st["expected"])} recorded bytes')
        run.divergences = self.divergences
        run.shaped = dict(sorted(self.shaped.items()))
        run.tick_slack = self.tick_slack


class Scenario:
    def __init__(self, script, net):
        self.script = script
        self.net = net

    def on_sync(self, run, reason, frame, clock_ms, events):
        cmds = self.net.on_events(run, events, reason, frame)
        if reason == SYNC_FRAME:
            cmds += self.script.commands(run, frame)
        return cmds

    def finish(self, run):
        self.net.finish(run)
