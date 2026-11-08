"""Interrupted sessions: a scenario replayed up to one of its moments, the session interrupted there, and
herc's and ragexe's aftermath compared.

    python -I teardown.py moments SCENARIO                     the scenario's moments and the kinds each takes
    python -I teardown.py case OUT SCENARIO MOMENT KIND [--keep]
    python -I teardown.py run OUT --plan FILE [--jobs N] [--keep]
                                                               a plan's cases (plan writes one)
    python -I teardown.py plan FILE SCENARIO... [--kinds A,B] [--sample N] [--seed S]
    python -I teardown.py regress OUT                          the cases in teardown_regress.json, ragexe (and
                                                               ragexe_asan) against herc's blessed summaries
    python -I teardown.py bless OUT                            herc's summaries of those cases, twice, into
                                                               recordings/teardown_regress.golden.json

A moment is one of the scenario's state checkpoints (its golden file's states; a converse's or a
click_each's states included), or connN: the client's first send on the recording's connection N (a
login, character or map server connection waiting for its answer), or connN.PPPP: its first send of
packet PPPP (conn2.007d: the map-loaded packet, with the login's same-map warp queued). The replay
is the scenario's own up to the moment: its steps and the recording's server side, the client's sends held to the
recording as in a replay. At the moment the script's later steps and the recording's later server
chunks are dropped, and the interruption takes their place the next frame (KINDS):

    close            the server closes the live connection (recv returns 0)
    reset            the connection is reset: what was buffered is lost, recv and send fail
                     (WSAECONNRESET; parity.c's PARITY_CMD_PEER_RESET)
    reset_send       the same, met first by the client's next send (the reset arrived after the frame's
                     receive): recv and select see nothing until a send failed
    banN             SC_NOTIFY_BAN (0x0081) with code N, and the server closes the connection
                     (Hercules' clif_authfail_fd, chr->authfail_fd, the login server's)
    warp_close       a warp to the map the client is on, and the server closes the connection behind it
    warp_same        ZC_NPCACK_MAPMOVE (0x0091) to the map and cell the client is on; the map-entry
                     burst after the client's CZ_NOTIFY_ACTORINIT (0x007d)
    warp_other       the same to another map (geffen)
    warp_twice_load  a warp to another map, and a second warp back two frames later, while the first map loads
                     (read right after its load, ahead of its burst)
    warp_twice_chunk two warps back to back in one chunk
    servermove       ZC_NPCACK_SERVERMOVE (0x0092): the client reconnects to a map server
    restart_ack      ZC_RESTART_ACK (0x00b3) 1, the character server's yes to a char select nobody asked for
    quit_ack         ZC_ACK_REQ_DISCONNECT (0x018b) 0, the yes to a quit nobody asked for
    ui_charselect    Game Options' Character Select (CZ_RESTART 1), answered ZC_RESTART_ACK 1
    ui_charselect_no the same answered ZC_ACK_REQ_DISCONNECT 1 (clif_parse_Restart's refusal)
    ui_quit          Game Options' Exit to Windows (CZ_QUIT_GAME), answered ZC_ACK_REQ_DISCONNECT 0
    ui_quit_no       the same answered ZC_ACK_REQ_DISCONNECT 1
    ui_select_cancel the character selection's close button (back to the login window)
    ui_login_quit    the login window's close button

The aftermath is scripted the same for every kind: states td_0..td_3 (2, 12, 60 and 200 frames after
the moment), then a drive back into the game (td_drive_NN: a message box closed with Enter, Select
Service passed, the login typed again with the recording's account and password, Game Start), states
td_back and td_back2 once the client is in the game, and Exit to Windows. Whatever connection the
client opens after the moment is answered from the recording's first connection to that port (the
login, the character list, a map server's entry and its burst), the map server's entry cut after its
map-entry burst; the server's own answers (keepalive, CZ_BLOCKING_PLAY_CANCEL, a quit, a char select,
a warp's burst after each CZ_NOTIFY_ACTORINIT) are made here.

Compared, herc with ragexe and with the AddressSanitizer build (TD_ASAN_EXE, run as ragexe_asan): every
state (pointer ids masked), every packet sent after the moment (by frame and connection, with the bytes
a replay masks and the keepalive's tick masked), the connections opened after it, the message boxes,
how the run ended (exit code, stalled for TIMEOUT s with no message from the client, the drive's and
the clicks' failures, the replay's divergences before the moment), what the client saved at quit, and
any AddressSanitizer report. A difference is a finding.

Run from tests/parity with PARITY_RUNS set to your own run root and PARITY_HERC_EXE to the herc
reference (as parity.py).
"""
import argparse
import concurrent.futures
import difflib
import glob
import json
import os
import random
import re
import shutil
import struct
import sys
import threading
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import inputs  # noqa: E402
import parity  # noqa: E402
import scenario as sc  # noqa: E402
from inputs import Seq, u32  # noqa: E402
from protocol import CMD_DELIVER, CMD_KEY, CMD_PEER_CLOSE, CMD_PEER_RESET, SYNC_FRAME  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
REGRESS = os.path.join(HERE, 'teardown_regress.json')
REGRESS_GOLDEN = os.path.join(HERE, 'recordings', 'teardown_regress.golden.json')
if os.environ.get('TD_ASAN_EXE'):
    parity.CLIENTS['ragexe_asan'] = {'exe': os.environ['TD_ASAN_EXE'], 'env': {}}
CLIENTS = ['herc', 'ragexe'] + (['ragexe_asan'] if 'ragexe_asan' in parity.CLIENTS else [])
TIMEOUT = {'herc': 60, 'ragexe': 60, 'ragexe_asan': 180}

LOGIN, CHAR, MAP = 'login', 'char', 'map'
CZ_ENTER, CH_ENTER = 0x0436, 0x0065
CZ_NOTIFY_ACTORINIT, CZ_REQUEST_TIME2, CZ_BLOCKING_PLAY_CANCEL = 0x007d, 0x0360, 0x0447
CZ_RESTART, CZ_QUIT_GAME, CZ_PING = 0x00b2, 0x018a, 0x0187
ZC_AID, ZC_ACCEPT_ENTER, ZC_NOTIFY_TIME = 0x0283, 0x02eb, 0x007f
# Sizes of the client's private CZ_ ids, which the length table lacks: CZ_CLIENT_CAPS (id.W caps.L,
# src/parity/client_caps.c) and the memo query (id.W op.W, src/parity/memo.c).
CLIENT_SENDS = {0x0bf6: 6, 0x0c00: 4}
GAME_START = (910, 545)
OTHER_MAP = ('geffen', 119, 59)
# A recorded map server's answer to the client's map-loaded packet after a warp: the @warp of the
# recording 'world' (account parity04), its own character's id swapped for the scenario's.
BURST_RECORDING, BURST_FRAME = 'world', 729

MAP_BANS = [0, 1, 2, 3, 4, 5, 9, 10, 15, 110, 113, 115, 99]
LOGIN_BANS = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 18, 100, 101, 102, 103, 104, 105, 107,
              108, 109, 110, 113, 114, 116, 99]
NET_KINDS = ['close', 'reset', 'reset_send']
MAP_KINDS = ['warp_close', 'warp_same', 'warp_other', 'warp_twice_load', 'warp_twice_chunk', 'servermove',
             'restart_ack', 'quit_ack', 'ui_charselect', 'ui_charselect_no', 'ui_quit', 'ui_quit_no']


def kinds_for(conn, all_bans=False, moment=''):
    """The kinds a moment whose live connection is `conn` (None: none is open) takes. At a map
    connection's first send (connN) the client still waits in the character selection for
    ZC_ACCEPT_ENTER: the map kinds, game packets the selection has no case for, are left out there (the
    server sends none of them before the client is in the game)."""
    if conn is None:
        return ['ui_login_quit']
    bans = (MAP_BANS if conn == MAP else LOGIN_BANS) if all_bans else [0, 15 if conn == MAP else 8, 99]
    kinds = NET_KINDS + [f'ban{c}' for c in bans]
    if conn == MAP and not re.fullmatch(r'(p\d+\.)?conn\d+', moment):
        kinds += MAP_KINDS
    elif conn == CHAR:
        kinds += ['ui_select_cancel']
    return kinds


def conn_kind(first):
    pid = struct.unpack_from('<H', first)[0] if len(first) >= 2 else None
    return MAP if pid == CZ_ENTER else CHAR if pid == CH_ENTER else LOGIN


def split_packets(data, sizes, client=False):
    """A client's or server's bytes as whole packets. An id the length table lacks carries its length
    at +2 from the server (both clients frame it so), and is the rest of the send from the client
    (but for CLIENT_SENDS)."""
    out, i = [], 0
    while i + 2 <= len(data):
        pid = data[i] | data[i + 1] << 8
        size = CLIENT_SENDS.get(pid, sizes.get(pid)) if client else sizes.get(pid, -1)
        if size == -1 and i + 4 <= len(data):
            size = data[i + 2] | data[i + 3] << 8
        if size is None or size < 2 or size == -1:
            size = len(data) - i
        out.append(data[i:i + size])
        i += size
    return out


def mask_packet(pkt):
    """A sent packet with the bytes a replay does not compare (scenario.Replay._mask) and the
    keepalive's tick blanked."""
    p = bytearray(pkt)
    pid = p[:2]
    if pid == b'\x04\x02':
        p[2:18] = bytes(len(p[2:18]))
    elif pid == b'\x36\x04':
        p[14:18] = bytes(len(p[14:18]))
    elif pid == b'\x60\x03':
        p[2:6] = bytes(len(p[2:6]))
    for head, at, n in ((b'\x6e\x0a', 4, 24), (b'\x6e\x0a', 28, 24), (b'\x02\x02', 2, 24), (b'\x67\x03', 10, 21),
                        (b'\xb2\x01', 4, 80), (b'\x11\x08', 9, 80)):
        if pid == head and len(p) >= at + n:
            end = p.find(b'\0', at, at + n)
            if end >= 0:
                p[end + 1:at + n] = bytes(at + n - end - 1)
    if pid == b'\x02\x02':
        p[26:] = bytes(len(p[26:]))
    return bytes(p)


# ---- what a case knows about its scenario ----------------------------------------------------

class Session:
    """What the interruptions need from the scenario's recording: the account and password its
    CA_LOGIN carries, the character's account id, the server tick at a known frame, each port's
    first connection, the map server's address, the map the client is on, and the warp burst."""

    def __init__(self, name, sizes, phase=0):
        _module(name)
        with open(parity.recording_path(name, phase)) as f:
            self.connections = json.load(f)['connections']
        self.sizes = sizes
        self.templates = {}
        self.account = self.password = None
        self.aid = None
        self.tick0 = self.tick_frame = None
        self.map_addr = None
        for rec in self.connections:
            if 'refused' in rec:
                continue
            sends = [bytes.fromhex(e['data']) for e in rec['events'] if 'c' in e]
            if not sends:
                continue
            kind = conn_kind(sends[0])
            self.templates.setdefault(rec['port'], (kind, rec))
            for data in sends:
                for p in split_packets(data, sizes, client=True):
                    if p[:2] == b'\x64\x00' and len(p) >= 54 and self.account is None:
                        self.account = p[6:30].split(b'\0')[0].decode('mbcs')
                        self.password = p[30:54].split(b'\0')[0].decode('mbcs')
            if kind == MAP and self.map_addr is None:
                self.map_addr = (rec.get('ip', '127.0.0.1'), rec['port'])
                for e in rec['events']:
                    if 's' not in e:
                        continue
                    for p in split_packets(bytes.fromhex(e['data']), sizes):
                        if p[:2] == struct.pack('<H', ZC_AID) and self.aid is None:
                            self.aid = struct.unpack_from('<I', p, 2)[0]
                        elif p[:2] == struct.pack('<H', ZC_ACCEPT_ENTER) and self.tick0 is None:
                            self.tick0, self.tick_frame = struct.unpack_from('<I', p, 2)[0], e['s']
        self.burst = self._burst()

    def tick(self, frame):
        if self.tick0 is None:
            return 0
        return (self.tick0 + (frame - self.tick_frame) * sc.FRAME_MS) & 0xffffffff

    def _burst(self):
        with open(os.path.join(parity.RECORDINGS, BURST_RECORDING + '.json')) as f:
            rec = json.load(f)['connections']
        mapconn = next(c for c in rec if c['events'] and c['events'][0].get('data', '').startswith('3604'))
        aid = None
        for e in mapconn['events']:
            if 's' in e:
                for p in split_packets(bytes.fromhex(e['data']), self.sizes):
                    if p[:2] == struct.pack('<H', ZC_AID):
                        aid = struct.unpack_from('<I', p, 2)[0]
                        break
            if aid is not None:
                break
        event = next(e for e in mapconn['events'] if e.get('s') == BURST_FRAME)
        out = []
        for p in split_packets(bytes.fromhex(event['data']), self.sizes):
            if p[:2] == struct.pack('<H', ZC_NOTIFY_TIME):
                continue
            if self.aid is not None and aid is not None:
                p = p.replace(struct.pack('<I', aid), struct.pack('<I', self.aid))
            out.append(p)
        return out

    def burst_for(self, same_map):
        """The burst after a warp: the recorded one, its NPC entries (prontera's) only on the same map."""
        return b''.join(p for p in self.burst if same_map or p[:2] != b'\xff\x09')

    def map_entry_events(self, rec):
        """A map server connection's recorded server events up to its first map-entry burst: those
        within ENTRY_FRAMES of the client's first CZ_NOTIFY_ACTORINIT, and the client bytes they answer."""
        first = None
        for e in rec['events']:
            if 'c' in e and any(p[:2] == b'\x7d\x00'
                                for p in split_packets(bytes.fromhex(e['data']), self.sizes, client=True)):
                first = e['c']
                break
        if first is None:
            return [], 0
        keep = [e for e in rec['events'] if ('s' in e or 'peer_close' in e) and e.get('s', e.get('peer_close')) <=
                first + ENTRY_FRAMES]
        return keep, max((e['after'] for e in keep), default=0)


ENTRY_FRAMES = 60


# ---- the network --------------------------------------------------------------------------------

class TeardownReplay(sc.Replay):
    """sc.Replay up to the moment, then the interruption and the server's own answers."""

    def __init__(self, name, session, kind, moment, phase=0):
        super().__init__(parity.recording_path(name, phase))
        self.session = session
        self.kind = kind
        self.moment = moment
        m = re.fullmatch(r'conn(\d+)(?:\.([0-9a-f]{4}))?', moment)
        self.conn_moment = int(m.group(1)) if m else None
        self.conn_packet = int(m.group(2), 16) if m and m.group(2) else None
        self.script = None
        self.cut_frame = None
        self.cut_pending = None
        self.done = False         # the interruption has been made
        self.order = []           # client socket ids in connect order
        self.tail = []            # (frame, connection number, masked packet hex) sent after the moment
        self.connects = []        # (frame, port, kind) connections opened after the moment
        self.answers = []         # (frame due, sid, command, payload, the connection's state)
        self.warps = {}           # sid -> [bursts owed, one per CZ_NOTIFY_ACTORINIT]
        self.refused = set()      # the requests already refused once (ui_*_no)
        self.notes = []

    # -- before the moment, a replay; at it, the cut
    def _connect(self, e, frame):
        _, sid, ip, port = e
        self.order.append(sid)
        if self.cut_frame is None:
            cmds = super()._connect(e, frame)
            st = self.live.get(sid)
            if st is not None:
                st['number'] = len(self.order) - 1
                st['kind'] = conn_kind(st['expected'])
            return cmds
        tpl = self.session.templates.get(port)
        self.connects.append((frame, port, tpl[0] if tpl else None))
        if tpl is None:
            self.notes.append(f'frame {frame}: a connection to port {port}, which the recording never opened')
            return [(sc.CMD_CONNECTED, u32(sid, 0))]
        kind, rec = tpl
        if kind == MAP:
            server, _ = self.session.map_entry_events(rec)
        else:
            server = [ev for ev in rec['events'] if 's' in ev or 'peer_close' in ev]
        # the template's server events anchored by the bytes the client had sent but its keepalives,
        # which come when the clock says, not where the recording had them
        eff, eff_sends, anchored = 0, [(0, rec['connect_frame'])], []
        for ev in rec['events']:
            if 'c' in ev:
                eff += self._effective(kind, bytes.fromhex(ev['data']))
                eff_sends.append((eff, ev['c']))
            elif ev in server:
                anchored.append(dict(ev, after=eff))
        limit = max((ev['after'] for ev in anchored), default=0) if kind == MAP else eff
        self.live[sid] = {'expected': b'', 'sent': b'', 'sends': [(0, frame)], 'mask': [],
                          'recorded_sends': eff_sends, 'index': len(self.order), 'server': anchored,
                          'framed': kind == MAP, 'carry': b'', 'carry_sent': False, 'caps': None,
                          'number': len(self.order) - 1, 'kind': kind, 'cut_at': 0, 'answer_from': limit,
                          'template': True, 'eff': 0, 'eff_sends': [(0, frame)]}
        return [(sc.CMD_CONNECTED, u32(sid, 1))]

    def _effective(self, kind, data):
        """How many of a send's bytes a template connection's anchors count: all but the keepalives."""
        if kind == LOGIN:
            return len(data)
        skip = CZ_PING if kind == CHAR else CZ_REQUEST_TIME2
        return sum(len(p) for p in split_packets(data, self.sizes, client=True)
                   if len(p) < 2 or struct.unpack_from('<H', p)[0] != skip)

    def _compare(self, st, before, frame):
        if 'cut_at' in st:
            if st.get('template'):
                return
            keep = st['sent']
            st['sent'] = keep[:st['cut_at']]
            try:
                if before < len(st['sent']):
                    super()._compare(st, before, frame)
            finally:
                st['sent'] = keep
            return
        super()._compare(st, before, frame)

    def cut(self, frame):
        """The moment: what the recording still had for the client is dropped."""
        if self.cut_frame is not None:
            return
        self.cut_frame = frame
        for st in self.live.values():
            st['server'] = []
            st['cut_at'] = len(st['sent'])
            st['answer_from'] = len(st['sent'])
            st['carry'] = b''
        if self.script is not None:
            self.script.cut(frame)

    def live_conn(self):
        """The connection the client opened last and still has open: (sid, state)."""
        for sid in reversed(self.order):
            if sid in self.live:
                return sid, self.live[sid]
        return None, None

    def on_events(self, run, events, reason, frame):
        self.run = run
        # where each send starts in its connection's stream, before the replay adds it
        starts, at = [], {}
        for e in events:
            if e[0] == 'send' and e[1] in self.live:
                st = self.live[e[1]]
                pos = at.get(e[1], len(st['sent']))
                starts.append((e[1], st, pos, e[2], st.get('eff', 0)))
                at[e[1]] = pos + len(e[2])
                if st.get('template'):
                    st['eff'] += self._effective(st['kind'], e[2])
                    st['eff_sends'].append((st['eff'], frame))
                if (self.conn_moment is not None and self.cut_frame is None and self.cut_pending is None
                        and st.get('number') == self.conn_moment and (
                            self.conn_packet is None or any(
                                len(p) >= 2 and struct.unpack_from('<H', p)[0] == self.conn_packet
                                for p in split_packets(e[2], self.sizes, client=True)))):
                    self.cut_pending = frame
        cmds = super().on_events(run, events, reason, frame)
        if self.cut_pending is not None and self.cut_frame is None:
            self.cut(frame)
        if self.cut_frame is not None:
            for sid, st, pos, data, eff in starts:
                self._after_cut_send(sid, st, pos, eff, data, frame)
            if not self.done and frame > self.cut_frame and reason == SYNC_FRAME:
                self.done = True
                cmds += self._interrupt(frame)
            due = [a for a in self.answers if a[0] <= frame and (reason == SYNC_FRAME or a[0] < frame)]
            for a in due:
                self.answers.remove(a)
                if self.live.get(a[1]) is a[4]:
                    cmds.append((a[2], a[3]))
        return cmds

    def _due(self, sid, st, frame, reason):
        # the client's send at a connN moment is the last thing the recording answers
        if self.cut_pending is not None and not st.get('template'):
            return []
        if not st.get('template'):
            return super()._due(sid, st, frame, reason)
        keep = st['sends']
        st['sends'] = st['eff_sends']
        try:
            return super()._due(sid, st, frame, reason)
        finally:
            st['sends'] = keep

    def _after_cut_send(self, sid, st, start, eff, data, frame):
        """The packets of one client send (at `start` in its stream, `eff` in a template connection's
        count) past the moment, kept, with the server's own answers queued: on a template connection
        once the client is past what the template answers."""
        pkts = split_packets(data, self.sizes, client=True) if st['kind'] == MAP else [data]
        at = start
        for pkt in pkts:
            pos, at = at, at + len(pkt)
            pid = struct.unpack_from('<H', pkt)[0] if len(pkt) >= 2 else None
            where = eff if st.get('template') else pos
            if st.get('template') and pid != CZ_REQUEST_TIME2:
                eff += len(pkt)
            if pos + len(pkt) <= st['cut_at']:
                continue
            self.tail.append((frame, st['number'], mask_packet(pkt).hex()))
            if where < st['answer_from'] or st['kind'] != MAP:
                continue
            if pid == CZ_REQUEST_TIME2:
                self._answer(frame, sid, struct.pack('<HI', ZC_NOTIFY_TIME, self.session.tick(frame)))
            elif pid == CZ_BLOCKING_PLAY_CANCEL:
                self._answer(frame, sid, struct.pack('<H', 0x0b1b))
            elif pid == CZ_NOTIFY_ACTORINIT and self.warps.get(sid):
                self._answer(frame + 3, sid, self.warps[sid].pop(0))
            elif pid == CZ_RESTART and len(pkt) >= 3 and pkt[2] == 1:
                if self.kind == 'ui_charselect_no' and 'restart' not in self.refused:
                    self.refused.add('restart')
                    self._answer(frame + 1, sid, struct.pack('<Hh', 0x018b, 1))
                else:
                    self._answer(frame + 1, sid, struct.pack('<HB', 0x00b3, 1))
            elif pid == CZ_QUIT_GAME:
                if self.kind == 'ui_quit_no' and 'quit' not in self.refused:
                    self.refused.add('quit')
                    self._answer(frame + 1, sid, struct.pack('<Hh', 0x018b, 1))
                else:
                    self._answer(frame + 1, sid, struct.pack('<Hh', 0x018b, 0))
                    self._answer(frame + 3, sid, b'', CMD_PEER_CLOSE)

    def _answer(self, frame, sid, data, cmd=CMD_DELIVER):
        self.answers.append((frame, sid, cmd, u32(sid) + data, self.live.get(sid)))

    def _interrupt(self, frame):
        """The interruption itself, the frame after the moment."""
        k = self.kind
        sid, st = self.live_conn()
        if k.startswith('ui_'):
            return []
        if sid is None:
            self.notes.append(f'frame {frame}: {k} with no connection open')
            return []
        if k == 'close':
            return [(CMD_PEER_CLOSE, u32(sid))]
        if k == 'reset':
            return [(CMD_PEER_RESET, u32(sid))]
        if k == 'reset_send':
            return [(CMD_PEER_RESET, u32(sid, 1))]
        if k.startswith('ban'):
            return [(CMD_DELIVER, u32(sid) + struct.pack('<HB', 0x0081, int(k[3:]) & 0xff)),
                    (CMD_PEER_CLOSE, u32(sid))]
        if st['kind'] != MAP:
            self.notes.append(f'frame {frame}: {k} on a {st["kind"]} connection')
            return []
        here = self._map_here()
        if k == 'restart_ack':
            return [(CMD_DELIVER, u32(sid) + struct.pack('<HB', 0x00b3, 1))]
        if k == 'quit_ack':
            return [(CMD_DELIVER, u32(sid) + struct.pack('<Hh', 0x018b, 0))]
        if k == 'servermove':
            ip, port = self.session.map_addr
            packed_ip = bytes(int(x) for x in ip.split('.'))
            return [(CMD_DELIVER, u32(sid) + struct.pack('<H16shh', 0x0092, (here[0] + '.gat').encode(), *here[1:])
                     + packed_ip + struct.pack('<H', port))]
        if k == 'warp_close':
            return [(CMD_DELIVER, u32(sid) + mapmove(*here)), (CMD_PEER_CLOSE, u32(sid))]
        if k == 'warp_same':
            self.warps[sid] = [self.session.burst_for(True)]
            return [(CMD_DELIVER, u32(sid) + mapmove(*here))]
        if k == 'warp_other':
            self.warps[sid] = [self.session.burst_for(False)]
            return [(CMD_DELIVER, u32(sid) + mapmove(*OTHER_MAP))]
        if k == 'warp_twice_chunk':
            self.warps[sid] = [self.session.burst_for(True), self.session.burst_for(True)]
            return [(CMD_DELIVER, u32(sid) + mapmove(*OTHER_MAP) + mapmove(*here))]
        if k == 'warp_twice_load':
            # Neither client reads its socket from the frame after it takes a map move to the first read after
            # the new map's load (the old mode's fade-out and the load make no Winsock call), so the second warp,
            # two frames on, is read as one sent at any frame of the load would be: right after the first map's
            # CZ_NOTIFY_ACTORINIT, ahead of that map's burst.
            self.warps[sid] = [self.session.burst_for(False), self.session.burst_for(True)]
            self._answer(frame + 2, sid, mapmove(*here))
            return [(CMD_DELIVER, u32(sid) + mapmove(*OTHER_MAP))]
        raise ValueError(k)

    def _map_here(self):
        """The map the server last put the client on and its cell, from the recording's warps."""
        here = ('prontera', 155, 185)
        sid, st = self.live_conn()
        stream = b''
        for e in self.recorded[st['index'] - 1]['events'] if st and not st.get('template') else []:
            if 's' in e and (self.cut_frame is None or e['s'] <= self.cut_frame):
                stream += bytes.fromhex(e['data'])
        for p in split_packets(stream, self.sizes):
            if p[:2] == b'\x91\x00' and len(p) >= 22:
                name = p[2:18].split(b'\0')[0].decode('latin-1').rsplit('.', 1)[0]
                here = (name,) + struct.unpack_from('<hh', p, 18)
        return here

    def finish(self, run):
        super().finish(run)
        # a connection cut at the moment sends less than the recording has: that is the point
        run.divergences = [d for d in run.divergences
                           if not re.match(r'connection \d+: sent \d+ of \d+ recorded', d)]
        run.divergences += self.notes


def mapmove(name, x, y):
    return struct.pack('<H16shh', 0x0091, (name + '.gat').encode(), x, y)


# ---- the script ---------------------------------------------------------------------------------

DRIVE_ROUNDS, DRIVE_PER = 12, 90
ID_FIELD, PASSWORD_FIELD = 1, 0       # the login window's two UIEditCtrl_Custom children, in its order


class TeardownScript(sc.Script):
    """The scenario's steps up to the moment, then the aftermath."""

    def __init__(self, steps, moment, kind, session, net):
        super().__init__(steps)
        self.moment = moment
        self.kind = kind
        self.session = session
        self.net = net
        self.cut_frame = None
        self.drive_failures = []
        self.held = set()         # virtual keys the script's commands left down

    def commands(self, run, frame):
        cmds = super().commands(run, frame)
        for c, payload in cmds:
            if c == CMD_KEY and len(payload) >= 8:
                vk, down = struct.unpack_from('<II', payload)
                if down:
                    self.held.add(vk)
                else:
                    self.held.discard(vk)
        if self.cut_frame is None and self.moment in run.states and self.net.conn_moment is None:
            self.net.cut(frame)
        return cmds

    def cut(self, frame):
        self.cut_frame = frame
        self.pending = {f: c for f, c in self.pending.items() if f <= frame}
        self.actions = {f: a for f, a in self.actions.items() if f <= frame}
        self.quit_frame = self.exit_by = None
        # a key the scenario was pressing when its steps were dropped is let go (its key-up was a later step)
        self.pending.setdefault(frame + 1, []).extend((CMD_KEY, u32(vk, 0)) for vk in sorted(self.held))
        s = Seq(frame + 2)
        s.state('td_0', 10).state('td_1', 48).state('td_2', 140).state('td_3', 4)
        k = self.kind
        if k in ('ui_charselect', 'ui_charselect_no'):
            s.click_ui('UIMenuIconWnd/UIMenuIcon#185', then=30).click_ui('UIEscOptionWnd/UIBitmapButton#363', then=40)
        elif k in ('ui_quit', 'ui_quit_no'):
            s.click_ui('UIMenuIconWnd/UIMenuIcon#185', then=30).click_ui('UIEscOptionWnd/UIBitmapButton#243', then=40)
        elif k == 'ui_select_cancel':
            s.click_ui('UINewSelectCharWnd/UIBitmapButton#177', then=40)
        elif k == 'ui_login_quit':
            s.click_ui('UILoginWnd/UIBitmapButton#213', then=40)
        s.state('td_4', 2)
        merged = sc.Script(s.steps)
        for f, c in merged.pending.items():
            self.pending.setdefault(f, []).extend(c)
        for f, a in merged.actions.items():
            self.actions.setdefault(f, []).extend(a)
        self.actions.setdefault(s.frame, []).append(('drive', (0,)))
        self.quit_frame = s.frame + DRIVE_ROUNDS * (DRIVE_PER + 40) + 1200

    def _resolve(self, run, frame, kind, taken, arg):
        if kind != 'drive':
            return super()._resolve(run, frame, kind, taken, arg)
        n = arg[0]
        path = self._probe_path(run, taken)
        if not os.path.exists(path):
            self.drive_failures.append(f'frame {taken}: no state for the drive')
            return
        fields, windows = inputs.parse_state(path)
        shown = {w['class'] for w in windows if w['shown']}
        act = None
        if 'UIMessageBox' in shown:
            act = inputs.key('enter')
        elif 'UIWaitWnd' in shown:
            act = []
        elif 'UISelectServerWnd' in shown:
            act = inputs.key('enter')
        elif 'UILoginWnd' in shown:
            act = self._login(windows)
        elif 'UINewSelectCharWnd' in shown:
            act = inputs.click(*GAME_START)
        elif fields.get('mode') == 'game':
            self._end(frame)
            return
        else:
            act = []
        if n >= DRIVE_ROUNDS:
            self.drive_failures.append(f'frame {taken}: still not in the game after {n} rounds ({sorted(shown)[:6]})')
            self._end(frame, exit_game=False)
            return
        self._schedule(frame, act)
        done = frame + len(act) + DRIVE_PER
        self.actions.setdefault(done, []).append(('state', f'td_drive_{n:02d}'))
        self.actions.setdefault(done + 1, []).append(('drive', (n + 1,)))

    def _login(self, windows):
        login = inputs.find_ui(windows, 'UILoginWnd/UIEditCtrl_Custom')
        if len(login) < 2 or not self.session.account:
            return inputs.key('enter')
        frames = []
        for field, text in ((ID_FIELD, self.session.account), (PASSWORD_FIELD, self.session.password)):
            x, y, w, h = login[field]['rect']
            frames += inputs.click(x + w // 2, y + h // 2) + [[] for _ in range(5)]
            frames += inputs.key('end') + [[], []]
            for _ in range(26):
                frames += inputs.key('back') + [[]]
            frames += inputs.text(text) + [[] for _ in range(4)]
        return frames + inputs.key('enter')

    def _end(self, frame, exit_game=True):
        s = Seq(frame + 1)
        s.state('td_back', 120).state('td_back2', 2)
        if exit_game:
            s.click_ui('UIMenuIconWnd/UIMenuIcon#185', then=30).click_ui('UIEscOptionWnd/UIBitmapButton#243', then=10)
        merged = sc.Script(s.steps)
        for f, c in merged.pending.items():
            self.pending.setdefault(f, []).extend(c)
        for f, a in merged.actions.items():
            self.actions.setdefault(f, []).extend(a)
        self.quit_frame = s.frame + 400


# ---- one case -----------------------------------------------------------------------------------

def run_one(client, name, moment, kind, out_dir, session):
    """One client's run of a case. A moment of a later session is reached by replaying the sessions before
    it whole in the same folder, each starting with what the last one left (as a replay of the scenario
    does); what went wrong in those is kept in the run's `before`."""
    phase, inner = split_moment(name, moment)
    phases = _phases(name)
    variant = getattr(_module(name), 'VARIANT', None)
    net = TeardownReplay(name, session, kind, inner, phase)
    script = TeardownScript(phases[phase], inner, kind, session, net)
    net.script = script
    run_dir, lock = claim_folder(client, variant)
    with lock:
        after = parity.Carried(run_dir, INITIAL_REGISTRY)
        before = []
        for k in range(phase):
            prev = parity._run_in(client, sc.Scenario(sc.Script(phases[k]), sc.Replay(parity.recording_path(name, k))),
                                  os.path.join(out_dir, f'phase{k}'), TIMEOUT.get(client, 60), run_dir, after)
            if prev.exit_code:
                before.append(f'session {k}: exit {prev.exit_code}')
            before += [f'session {k}: {d}' for d in prev.divergences + prev.expect_failures]
            after = prev
        run = parity._run_in(client, sc.Scenario(script, net), out_dir, TIMEOUT.get(client, 60), run_dir, after)
    run.before = before
    return run, net, script


# ---- run folders --------------------------------------------------------------------------------
# A run folder is made once per slot (parity.prepare_folder: some 21000 files, most of them hard links,
# which take longer to make and remove than a case takes to run) and put back as DATA_ROOT has it before
# each later run: every file the client made removed, every copied file it changed or removed copied
# again, the exe copied again.

READY = '.teardown_ready'
INITIAL_REGISTRY = '\n'.join(parity.REGISTRY) + '\n'
_DATA_FILES = []


def data_files():
    if not _DATA_FILES:
        for dirpath, _, names in os.walk(parity.DATA_ROOT):
            rel_dir = os.path.relpath(dirpath, parity.DATA_ROOT)
            for name in names:
                _DATA_FILES.append(name if rel_dir == '.' else os.path.join(rel_dir, name))
    return _DATA_FILES


def reset_folder(client, run_dir):
    known = {r.lower() for r in data_files()}
    exe = os.path.basename(parity.CLIENTS[client]['exe']).lower()
    keep = {exe, READY.lower(), parity.ASAN_DLL.lower()}
    for dirpath, dirs, names in os.walk(run_dir, topdown=False):
        rel_dir = os.path.relpath(dirpath, run_dir)
        for name in names:
            rel = name if rel_dir == '.' else os.path.join(rel_dir, name)
            if rel.lower() not in known and not (rel_dir == '.' and name.lower() in keep):
                os.remove(os.path.join(dirpath, name))
        if rel_dir != '.' and not os.listdir(dirpath) and not os.path.isdir(os.path.join(parity.DATA_ROOT, rel_dir)):
            os.rmdir(dirpath)
    for rel in data_files():
        src, dst = os.path.join(parity.DATA_ROOT, rel), os.path.join(run_dir, rel)
        top = rel.split(os.sep)[0]
        linked = top in parity.LINK_DIRS or os.path.getsize(src) >= parity.LINK_MIN_BYTES
        if os.path.exists(dst):
            if linked:
                continue
            a, b = os.stat(dst), os.stat(src)
            if a.st_size == b.st_size and int(a.st_mtime) == int(b.st_mtime):
                continue
            os.remove(dst)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        if linked:
            os.link(src, dst)
        else:
            shutil.copy2(src, dst)
    shutil.copy2(parity.CLIENTS[client]['exe'], os.path.join(run_dir, exe))
    asan_dll = os.path.join(os.path.dirname(parity.CLIENTS[client]['exe']), parity.ASAN_DLL)
    if os.path.exists(asan_dll):
        shutil.copy2(asan_dll, os.path.join(run_dir, parity.ASAN_DLL))
    with open(os.path.join(run_dir, 'parity_registry.txt'), 'w') as f:
        f.write(INITIAL_REGISTRY)


def claim_folder(client, variant=None):
    """A slot folder for the client (RUN_ROOT/td-<client>-N), as DATA_ROOT has it, and its lock. With a
    data variant (a variant scenario's VARIANT) the folder (td-<client>-v-N) is made anew each time with
    the variant laid over it, since putting a folder back as DATA_ROOT has it would undo the variant."""
    import msvcrt
    os.makedirs(parity.RUN_ROOT, exist_ok=True)
    while True:
        for slot in range(4):
            run_dir = os.path.join(parity.RUN_ROOT, f'td-{client}-v-{slot}' if variant else f'td-{client}-{slot}')
            lock = open(run_dir + '.lock', 'a+')
            try:
                msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
            except OSError:
                lock.close()
                continue
            if not variant and os.path.exists(os.path.join(run_dir, READY)):
                reset_folder(client, run_dir)
            else:
                if os.path.exists(run_dir):
                    shutil.rmtree(run_dir)
                # prepare_folder lays parity.VARIANT[0] over the folder: one folder made at a time
                with _PREPARE:
                    parity.VARIANT[0] = variant
                    try:
                        parity.prepare_folder(client, run_dir)
                    finally:
                        parity.VARIANT[0] = None
                if not variant:
                    open(os.path.join(run_dir, READY), 'w').close()
            return run_dir, lock
        time.sleep(2)


_LOAD = threading.Lock()
_PREPARE = threading.Lock()


def _module(name):
    """The scenario's module (loaded once; recording_path reads a variant scenario's VARIANT_OF from it)."""
    with _LOAD:
        if f'scenario_{name}' not in sys.modules:
            parity.load_phases(name)
        return sys.modules[f'scenario_{name}']


def _phases(name):
    """The steps of each of the scenario's sessions (its PHASES, else its STEPS). A scenario file is
    loaded as a module of its own name: one load at a time."""
    with _LOAD:
        parity.load_phases(name)
        mod = sys.modules[f'scenario_{name}']
        return [list(p) for p in getattr(mod, 'PHASES', None) or [mod.STEPS]]


def _phase_states(name):
    """{state name: the session that makes it} for the states the scenario's steps name."""
    out = {}
    for k, steps in enumerate(_phases(name)):
        for acts in sc.Script(steps).actions.values():
            for a in acts:
                if a[0] == 'state':
                    out.setdefault(a[1], k)
    return out


def split_moment(name, moment):
    """(session, the moment within it): pK.connN is session K's connection N; a state belongs to the
    session whose steps make it (a converse's or a click_each's states, which no step names, to the first)."""
    m = re.fullmatch(r'p(\d+)\.(.+)', moment)
    if m:
        return int(m.group(1)), m.group(2)
    return _phase_states(name).get(moment, 0), moment


def asan_reports(out_dir):
    out = []
    for path in glob.glob(os.path.join(out_dir, 'asan.*')):
        with open(path, encoding='utf-8', errors='replace') as f:
            text = f.read()
        m = re.search(r'ERROR: AddressSanitizer: (.*)', text)
        frames = re.findall(r'#\d+ 0x[0-9a-f]+ in (\S+) (\S+)', text)
        out.append({'error': m.group(1).strip() if m else text[:200],
                    'top': [f'{fn} {loc.split(chr(92))[-1]}' for fn, loc in frames[:6]], 'file': path})
    return out


def summary(run, net, script):
    states = {n: parity.state_text(p) for n, p in run.states.items() if os.path.exists(p)}
    return {
        'exit': run.exit_code,
        'stalled': 'stopped: no message from the client' in run.launcher_output,
        'frames': run.frames,
        'cut': net.cut_frame,
        'states': states,
        # lists, as a blessed summary read back from JSON has them
        'tail': [list(t) for t in net.tail],
        'connects': [list(c) for c in net.connects],
        'dialogs': [d.replace(run.run_dir, '<folder>') for d in run.dialogs],
        'divergences': [d for d in run.divergences if ' sent byte ' not in d],
        'prefix_bytes': [d for d in run.divergences if ' sent byte ' in d],
        'expect': run.expect_failures,
        'drive': script.drive_failures,
        'before': getattr(run, 'before', []),
        'saved': parity.saved_forms(run.saved) if run.saved else None,
        'asan': asan_reports(run.out_dir),
    }


ACCESS_VIOLATION = 0xc0000005
PALETTE_BAR = re.compile(r'(  UIScrollBar@@ .* max=)(\d+)( page=\d+ total=)(\d+)$')


def palette_row_less(parent, x, y):
    """Whether herc's line x and ragexe's y are the command palette's scroll bar (a plain UIWindow's,
    src/parity/command_palette.c) with one row less on ragexe: the roulette icon's ON/OFF row, gone with
    the roulette (talk_option_find has no case 0xda). A cut can leave a bare '/' on the chat line, which
    lists every '/' command; the scenarios always type a whole command, so their states never show it."""
    mx, my = PALETTE_BAR.match(x), PALETTE_BAR.match(y)
    return (parent.startswith('UIWindow@@ ') and mx and my and mx.group(1) == my.group(1)
            and mx.group(3) == my.group(3) and int(mx.group(2)) == int(my.group(2)) + 1
            and int(mx.group(4)) == int(my.group(4)) + 1)


def state_diff(x, y):
    """The lines of state y (ragexe's) that differ from state x (herc's), as '-'/'+' lines, without the
    differences ragexe makes on purpose that a cut can bring to light (palette_row_less)."""
    out = []
    for op, i1, i2, j1, j2 in difflib.SequenceMatcher(None, x, y, autojunk=False).get_opcodes():
        if op == 'equal':
            continue
        if op == 'replace' and i2 - i1 == j2 - j1:
            keep = []
            for i, j in zip(range(i1, i2), range(j1, j2)):
                parent = next((x[k] for k in range(i - 1, -1, -1) if not x[k].startswith(' ')), '')
                if not palette_row_less(parent, x[i], y[j]):
                    keep.append((i, j))
            out += [f'-{x[i]}' for i, _ in keep] + [f'+{y[j]}' for _, j in keep]
            continue
        out += [f'-{l}' for l in x[i1:i2]] + [f'+{l}' for l in y[j1:j2]]
    return out


def deliberate_fault(s):
    """The AddressSanitizer build's reports, but the fault RAGEXE_FEATURE_CRASH_ON_UNKNOWN_PACKET makes on
    purpose (ragexe_unknown_packet reads through the packet id); with that fault the exit code the release
    builds have for it (ASan ends the process with 1 where they fault)."""
    reports = [r for r in s.get('asan', []) if not (r['error'].startswith('access-violation')
                                                    and r['top'] and r['top'][0].startswith('ragexe_unknown_packet '))]
    exit_code = ACCESS_VIOLATION if len(reports) < len(s.get('asan', [])) and s['exit'] == 1 else s['exit']
    return reports, exit_code


_IMPROVED = {}


def improved_lines(name):
    """The state lines the scenario's approved improvements change ('-' herc's, '+' ragexe's), as a set:
    a window an improvement changes can stay open past the moment, into the aftermath's states."""
    if name not in _IMPROVED:
        lines = set()
        imp = parity.improvements(name)
        if imp:
            with open(os.path.join(parity.RECORDINGS, name + '.golden.json'), encoding='utf-8') as f:
                states = json.load(f)['states']
            for state, e in imp.items():
                if state in states:
                    lines |= set(state_diff(states[state].splitlines(), e['text'].splitlines()))
        _IMPROVED[name] = lines
    return _IMPROVED[name]


def compare(a, b, an='herc', bn='ragexe', name=None, expected=()):
    """The differences of run b from run a, as lines; with the scenario's `name`, without the state lines its
    approved improvements change, nor the `expected` ones (a regression case's: ragexe's fix of a bug herc
    keeps)."""
    lines = []
    improved = (improved_lines(name) | set(expected)) if name and bn != f'{an} again' else set()
    b_asan, b_exit = deliberate_fault(b)
    for k in ('exit', 'stalled', 'cut'):
        x, y = a[k], b_exit if k == 'exit' else b[k]
        if x != y:
            lines.append(f'{k}: {an} {x}, {bn} {y}')
    for k in ('connects', 'dialogs', 'divergences', 'expect', 'drive', 'before'):
        if a.get(k, []) != b.get(k, []):
            lines.append(f'{k}: {an} {a.get(k, [])[:4]}, {bn} {b.get(k, [])[:4]}')
    if b['prefix_bytes']:
        lines.append(f'prefix sends: {bn} {b["prefix_bytes"][:2]}')
    names = sorted(set(a['states']) | set(b['states']), key=lambda n: (not n.startswith('td_'), n))
    for n in names:
        x, y = a['states'].get(n), b['states'].get(n)
        if x == y:
            continue
        if x is None or y is None:
            lines.append(f'state {n} only in {an if y is None else bn}')
            continue
        diff = [l for l in state_diff(x.splitlines(), y.splitlines()) if l not in improved]
        if diff:
            lines.append(f'state {n}: ' + ' | '.join(diff[:6]) + (f' (+{len(diff) - 6})' if len(diff) > 6 else ''))
    ta, tb = a['tail'], b['tail']
    if ta != tb:
        i = next((i for i, (x, y) in enumerate(zip(ta, tb)) if x != y), min(len(ta), len(tb)))
        lines.append(f'sends after the moment part at {i} of {len(ta)}/{len(tb)}: '
                     f'{an} {ta[i:i + 2]}, {bn} {tb[i:i + 2]}')
    if a['saved'] != b['saved'] and a['saved'] and b['saved']:
        lines += [f'saved: {l}' for l in parity.diff_saved(a['saved'], b['saved'])]
    for r in b_asan:
        lines.append(f'asan: {r["error"]} at {" < ".join(r["top"][:4])}')
    return lines


def improvements_for(name):
    return parity.improvements(name)


def case_key(name, moment, kind):
    return f'{name}__{moment}__{kind}'


def run_case(out, name, moment, kind, keep=False, session=None, golden=None, expected=()):
    """One case on every client in CLIENTS, in turn; its result as a dict. With `golden` (herc's summary
    as bless kept it) herc is not run."""
    session = session or Session(name, sc.packet_sizes(), split_moment(name, moment)[0])
    case_dir = os.path.join(out, case_key(name, moment, kind))
    sums = {}
    start = time.monotonic()
    if golden is not None:
        sums['herc'] = golden
    for c in CLIENTS:
        if c in sums:
            continue
        run, net, script = run_one(c, name, moment, kind, os.path.join(case_dir, c), session)
        sums[c] = summary(run, net, script)
    # the states an approved improvement changes are ragexe's own (the prefix's)
    for s in [sums[c] for c in CLIENTS[1:]]:
        for state in improvements_for(name):
            s['states'].pop(state, None)
    for state in improvements_for(name):
        sums['herc']['states'].pop(state, None)
    res = {'scenario': name, 'moment': moment, 'kind': kind, 'seconds': round(time.monotonic() - start, 1),
           'cut': sums['herc']['cut'], 'frames': {c: sums[c]['frames'] for c in CLIENTS},
           'exit': {c: sums[c]['exit'] for c in CLIENTS}, 'states': len(sums['herc']['states']),
           'tail': len(sums['herc']['tail']), 'connects': sums['herc']['connects'],
           'herc_problems': {k: sums['herc'][k] for k in ('divergences', 'prefix_bytes', 'expect', 'drive')
                             if sums['herc'][k]},
           'diffs': {c: compare(sums['herc'], sums[c], 'herc', c, name, expected) for c in CLIENTS[1:]}}
    res['differs'] = any(res['diffs'].values())
    if res['differs'] or keep:
        os.makedirs(case_dir, exist_ok=True)
        with open(os.path.join(case_dir, 'summaries.json'), 'w', encoding='utf-8') as f:
            json.dump(sums, f, indent=1)
    else:
        shutil.rmtree(case_dir, ignore_errors=True)
    return res


def bless(out):
    """herc's summary of every regression case, run twice: kept in REGRESS_GOLDEN when the two runs agree,
    so that a gate's `regress` runs only the ragexe builds."""
    with open(REGRESS) as f:
        cases = json.load(f)['cases']
    goldens, bad = {}, []
    for c in cases:
        name, moment, kind = c['scenario'], c['moment'], c['kind']
        session = Session(name, sc.packet_sizes(), split_moment(name, moment)[0])
        twice = []
        for k in range(2):
            run, net, script = run_one('herc', name, moment, kind, os.path.join(out, case_key(name, moment, kind),
                                                                                f'herc{k}'), session)
            twice.append(summary(run, net, script))
        lines = compare(twice[0], twice[1], 'herc', 'herc again')
        print(f'{case_key(name, moment, kind)}: {"blessed" if not lines else "NOT the same twice: " + lines[0]}',
              flush=True)
        if lines:
            bad.append(case_key(name, moment, kind))
            continue
        for state in improvements_for(name):
            twice[0]['states'].pop(state, None)
        twice[0]['asan'] = []
        goldens[case_key(name, moment, kind)] = twice[0]
    with open(REGRESS_GOLDEN, 'w', encoding='utf-8') as f:
        json.dump(goldens, f, indent=0, sort_keys=True)
    if bad:
        sys.exit(1)


# ---- moments and plans --------------------------------------------------------------------------

def moments(name):
    """The scenario's moments, each with the kind of connection open there (None: none is), from its
    golden states and its recordings' connections (a later session's as pK.connN)."""
    path = os.path.join(parity.RECORDINGS, name + '.golden.json')
    with open(path, encoding='utf-8') as f:
        golden = json.load(f)
    out = []
    _module(name)
    for phase in range(len(_phases(name))):
        with open(parity.recording_path(name, phase)) as f:
            rec = json.load(f)['connections']
        prefix, seen_007d = f'p{phase}.' if phase else '', False
        for k, c in enumerate(rec):
            sends = [bytes.fromhex(e['data']) for e in c['events'] if 'c' in e]
            if 'refused' not in c and sends:
                out.append((f'{prefix}conn{k}', conn_kind(sends[0])))
                if conn_kind(sends[0]) == MAP and not seen_007d and any(d[:2] == b'\x7d\x00' for d in sends):
                    seen_007d = True
                    out.append((f'{prefix}conn{k}.007d', MAP))
    for n, text in golden['states'].items():
        mode = re.search(r'^mode=(\w+)', text, re.M)
        mode = mode.group(1) if mode else ''
        if mode == 'game':
            conn = MAP
        elif re.search(r'^UINewSelectCharWnd@@ .* shown=1', text, re.M) or re.search(
                r'^UINewMakeCharWnd@@ .* shown=1', text, re.M):
            conn = CHAR
        else:
            conn = None
        out.append((n, conn))
    return out


def moment_frames(name):
    """{moment: the frame it falls at in its session, as the script schedules it}: a converse's or a
    click_each's states (made while the scenario runs) at their step's frame plus their budget's share."""
    frames, dynamic = {}, []
    for steps in _phases(name):
        for f, acts in sc.Script(steps).actions.items():
            for kind, arg in acts:
                if kind == 'state':
                    frames.setdefault(arg, f)
                elif kind == 'converse':
                    dynamic.append((arg[0], f, arg[2] + 1))
                elif kind == 'click_each':
                    dynamic.append((arg[1], f, arg[3] * len(arg[2])))
    for path in glob.glob(os.path.join(parity.RECORDINGS, name + '.golden.json')):
        with open(path, encoding='utf-8') as fh:
            for n in json.load(fh)['states']:
                if n in frames:
                    continue
                m = re.fullmatch(r'(.*)_(\d+)', n)
                for prefix, f, per in dynamic:
                    if m and m.group(1) == prefix:
                        frames[n] = f + per * (int(m.group(2)) + 1)
                    elif n == prefix + '_done':
                        frames[n] = f + per * 12
    return frames


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=['moments', 'case', 'run', 'plan', 'regress', 'bless'])
    ap.add_argument('args', nargs='*')
    ap.add_argument('--keep', action='store_true')
    ap.add_argument('--jobs', type=int, default=1)
    ap.add_argument('--plan')
    ap.add_argument('--kinds')
    ap.add_argument('--sample', type=int)
    ap.add_argument('--seed', type=int, default=24)
    ap.add_argument('--all-bans', action='store_true')
    a = ap.parse_args()
    if a.mode == 'moments':
        for n, conn in moments(a.args[0]):
            print(f'{n:40} {conn or "-":6} {" ".join(kinds_for(conn, moment=n))}')
        return
    if a.mode == 'case':
        out, name, moment, kind = a.args
        res = run_case(out, name, moment, kind, keep=a.keep)
        print(json.dumps(res, indent=1))
        sys.exit(1 if res['differs'] else 0)
    if a.mode == 'plan':
        path, names = a.args[0], a.args[1:]
        rnd = random.Random(a.seed)
        cases = []
        for name in names:
            ms = moments(name)
            if a.sample and len(ms) > a.sample:
                ms = sorted(rnd.sample(ms, a.sample), key=ms.index)
            for m, conn in ms:
                kinds = kinds_for(conn, a.all_bans, m)
                if a.kinds:
                    kinds = [k for k in kinds if k in a.kinds.split(',')]
                cases += [[name, m, k] for k in kinds]
        with open(path, 'w') as f:
            json.dump(cases, f, indent=0)
        print(f'{len(cases)} cases -> {path}')
        return
    if a.mode == 'bless':
        bless(a.args[0])
        return
    goldens, expected = {}, {}
    if a.mode in ('run', 'regress'):
        out = a.args[0]
        if a.mode == 'run':
            with open(a.plan) as f:
                cases = [c if isinstance(c, list) else [c['scenario'], c['moment'], c['kind']] for c in json.load(f)]
        else:
            with open(REGRESS) as f:
                regress = json.load(f)['cases']
            cases = [[c['scenario'], c['moment'], c['kind']] for c in regress]
            expected = {case_key(c['scenario'], c['moment'], c['kind']): c.get('ragexe_differs', {}).get('lines', [])
                        for c in regress}
            if os.path.exists(REGRESS_GOLDEN):
                with open(REGRESS_GOLDEN, encoding='utf-8') as f:
                    goldens = json.load(f)
        os.makedirs(out, exist_ok=True)
        log_path = os.path.join(out, 'teardown.jsonl')
        done = set()
        if os.path.exists(log_path) and a.mode == 'run':
            with open(log_path, encoding='utf-8') as f:
                for line in f:
                    r = json.loads(line)
                    done.add((r['scenario'], r['moment'], r['kind']))
        todo = [c for c in cases if tuple(c) not in done]
        lock = threading.Lock()
        sessions = {}
        counter = [len(cases) - len(todo)]
        t0 = time.monotonic()
        failed = [0]

        def one(c):
            name, moment, kind = c
            with lock:
                key = (name, split_moment(name, moment)[0])
                if key not in sessions:
                    sessions[key] = Session(name, sc.packet_sizes(), key[1])
            try:
                res = run_case(out, name, moment, kind, a.keep, sessions[key],
                               goldens.get(case_key(name, moment, kind)),
                               expected.get(case_key(name, moment, kind), ()))
            except Exception as e:      # a harness error is a result of its own
                res = {'scenario': name, 'moment': moment, 'kind': kind, 'error': repr(e), 'differs': True,
                       'diffs': {}}
            with lock:
                counter[0] += 1
                failed[0] += bool(res['differs'])
                with open(log_path, 'a', encoding='utf-8') as f:
                    f.write(json.dumps(res) + '\n')
                tag = 'DIFFERS' if res['differs'] else 'same'
                first = next((l for v in res['diffs'].values() for l in v), res.get('error', ''))
                print(f'[{counter[0]}/{len(cases)}] {name} {moment} {kind}: {tag} {first[:200]} '
                      f'({res.get("seconds", 0):.0f} s)', flush=True)

        with concurrent.futures.ThreadPoolExecutor(a.jobs) as ex:
            list(ex.map(one, todo))
        print(f'{len(todo)} cases in {time.monotonic() - t0:.0f} s; {failed[0]} differ')
        sys.exit(1 if failed[0] else 0)


if __name__ == '__main__':
    main()
