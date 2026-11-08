"""The differential tester: generated map server streams (seqgen.py) played to herc and to ragexe
through the harness's in-process network, their states compared.

    python -I seqfuzz.py run OUT [--seeds A-B] [--jobs N] [--length N] [--keep]
    python -I seqfuzz.py case OUT SEED [--events FILE]       one case, its runs kept
    python -I seqfuzz.py minimize OUT SEED [--events FILE]   the shortest event list that still differs
    python -I seqfuzz.py regress OUT                         the fixed set in seqfuzz_regress.json
    python -I seqfuzz.py target OUT [--handlers A,B] [--scripts A,B]
                                                             seqtarget.py's scripts, one case each: those
                                                             that send a packet one of the handlers takes
                                                             (a dispatcher's handler function, a HEADER_
                                                             name, a layout name or an id), or the scripts
                                                             named; every script by default

Every mode takes --split SPEC (PARITY_RECV_SPLIT=SPEC for both clients: parity.c cuts the server's
stream into pieces, some of them a frame late) and --clients A,B (herc,ragexe by default; with
PARITY_SPLIT_CLIENTS set, herc_split and ragexe_split too, parity.py). A client reading a split
stream takes some bytes later than the recording had them, so its prefix sends are not held to
the recording: the two clients' whole send streams (parity.diff_streams) and the frames they send
in (parity.send_timeline) are compared with each other instead. --coverage DIR keeps each ragexe run's
coverage record (a coverage build's ragexe_coverage.txt) in DIR, for coverage_report.py.

A case is the recording seqgen.build makes: the prefix's login and map entry (its client sends
compared with the recording's, as a replay does), then the generated chunks, each due by frames
after the cut's byte count or after a map's load (FuzzReplay). What the client sends after the cut
is not held to anything recorded; it is compared between the two clients instead. FuzzReplay
answers what the server answers on its own: the keepalive (CZ_REQUEST_TIME2 0x0360, with the
server's tick, ZC_NOTIFY_TIME 0x007f) and CZ_BLOCKING_PLAY_CANCEL (0x0447, with ZC_LOAD_CONFIRM
0x0b1b).

The states (the generated checkpoints and one after the last event) are compared line by line.
ragexe differs from the original client on purpose in one place these streams reach: it lets a
pending-unit id go once its unit is gone (src/world/objects.c), where the original keeps it for the
rest of the map and destroys every later unit under that id. A difference is put down to that only
when the stock-pending build (RAGEXE_STOCK_EXE: ragexe with that one change undone) agrees with
herc on that state; that build is run only for a case whose ragexe states differ.

Run from tests/parity with PARITY_RUNS set to your own run root (as parity.py).
"""
import argparse
import concurrent.futures
import difflib
import json
import os
import re
import shutil
import struct
import sys
import threading
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import parity  # noqa: E402
import scenario as sc  # noqa: E402
import seqgen  # noqa: E402
import seqtarget  # noqa: E402
from inputs import Seq  # noqa: E402
from inputs import u32  # noqa: E402
from protocol import CMD_DELIVER, CMD_QUIT, CMD_STATE, SYNC_FRAME  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
REGRESS = os.path.join(HERE, 'seqfuzz_regress.json')
LOGIN_ACCOUNT = 'parity04'     # the PREFIX recording's account (scenarios/world.py)

STOCK_EXE = os.environ.get('RAGEXE_STOCK_EXE')
if STOCK_EXE:
    parity.CLIENTS['ragexe_stock'] = {'exe': STOCK_EXE, 'env': {}}
# Another ragexe build under test in ragexe's place (a build with a bug brought back, to show the
# tester finds it).
if os.environ.get('RAGEXE_TEST_EXE'):
    for _c in ('ragexe', 'ragexe_split'):
        if _c in parity.CLIENTS:
            parity.CLIENTS[_c]['exe'] = os.environ['RAGEXE_TEST_EXE']
# A fill sanitizer build (build.bat with RAGEXE_SANITIZE=fill) as two clients, fill00 and fillff, that hand out
# heap memory nobody wrote filled with 00 and with FF: --clients fill00,fillff differ only where the client
# reads such memory. Their folders' names are as long as each other's, so both runs get the same addresses.
if os.environ.get('RAGEXE_FILL_EXE'):
    for _b in ('00', 'ff'):
        parity.CLIENTS['fill' + _b] = {'exe': os.environ['RAGEXE_FILL_EXE'], 'env': {'RAGEXE_FILL_BYTE': _b}}
PAIR = ['herc', 'ragexe']       # the two clients a case compares (--clients)
COVERAGE = []                   # the folder ragexe runs' coverage records are kept in (--coverage)

CZ_REQUEST_TIME2, CZ_BLOCKING_PLAY_CANCEL = 0x0360, 0x0447
ZC_LOAD_CONFIRM = struct.pack('<H', 0x0b1b)
# Sizes of the client's private CZ_ ids, which the length table lacks: CZ_CLIENT_CAPS (id.W caps.L,
# src/parity/client_caps.c) and the memo query (id.W op.W, src/parity/memo.c).
CLIENT_SENDS = {0x0bf6: 6, 0x0c00: 4}
QUIT_AFTER = 5           # frames from the last state to the quit
# The script's own quit, a fallback for a client that never reaches the last state: frames past the
# frame the case takes it to be at
QUIT_SLACK = 1500


class FuzzReplay(sc.Replay):
    """sc.Replay for a generated case: the prefix played and compared as recorded, the generated
    chunks due as seqgen.py describes ('wait', 'state'), the server's own answers sent, and the
    client's sends after the cut kept in `tail` as (frame, packet) instead of compared."""

    def __init__(self, rec, prefix, split=False):
        self.split = split
        self.recorded = rec['connections']
        self.answer_quit = False
        self.next_conn = 0
        self.live = {}
        self.divergences = []
        self.stalled = 0
        self.sizes = sc.packet_sizes()
        self.shaped = {}
        self.tick_slack = []
        self.prefix = prefix
        self.cut_sent = rec['seqgen']['sent']
        self.tail = []
        self.loads = []          # the frame of each CZ_NOTIFY_ACTORINIT after the cut
        self.answers = []        # (socket id, packet) the server owes the client
        self.run = None
        self.quit_at = None      # the frame the client is told to quit at, once the last state is taken

    def _connect(self, e, frame):
        cmds = super()._connect(e, frame)
        return cmds

    def _compare(self, st, before, frame):
        # only what the prefix recorded is held to the recording, and that only on a whole stream
        if before >= len(st['expected']) or self.split:
            return
        keep = st['sent']
        st['sent'] = keep[:len(st['expected'])]
        try:
            super()._compare(st, before, frame)
        finally:
            st['sent'] = keep

    def on_events(self, run, events, reason, frame):
        self.run = run
        for e in events:
            if e[0] == 'send' and e[1] in self.live:
                self._sent_packets(e[1], self.live[e[1]], e[2], frame)
        cmds = super().on_events(run, events, reason, frame)
        while self.answers:
            sid, pkt = self.answers.pop(0)
            cmds.append((CMD_DELIVER, u32(sid) + pkt))
        if self.quit_at is not None and frame >= self.quit_at and reason == SYNC_FRAME:
            cmds.append((CMD_QUIT, b''))
            self.quit_at = None
        return cmds

    def _sent_packets(self, sid, st, data, frame):
        """The packets of one client send on a map connection, kept once past the cut, with the
        server's own answers queued. A send holds whole packets: one the length table does not
        know (ragexe's private CZ_ ids, CLIENT_SENDS) is the rest of the send."""
        if not st['framed']:
            return
        at = len(st['sent'])
        i = 0
        while i + 2 <= len(data):
            pid = data[i] | data[i + 1] << 8
            size = CLIENT_SENDS.get(pid, self.sizes.get(pid))
            if size == -1 and i + 4 <= len(data):
                size = data[i + 2] | data[i + 3] << 8
            if size is None or size < 2 or size == -1:
                size = len(data) - i
            pkt = data[i:i + size]
            if at + i >= self.cut_sent:
                self.tail.append((frame, pkt))
                if pid == seqgen.CZ_NOTIFY_ACTORINIT:
                    self.loads.append(frame)
                elif pid == CZ_REQUEST_TIME2:
                    self.answers.append((sid, seqgen.pack('ZC_NOTIFY_TIME', time=self.prefix.tick(frame))))
                elif pid == CZ_BLOCKING_PLAY_CANCEL:
                    self.answers.append((sid, ZC_LOAD_CONFIRM))
            i += size

    def _due(self, sid, st, frame, reason):
        cmds = []
        while st['server']:
            ev = st['server'][0]
            if 'wait' in ev:
                n, assumed = ev['wait']
                if len(self.loads) < n:
                    break
                if frame - self.loads[n - 1] < ev['s'] - assumed and (reason == SYNC_FRAME or 'state' in ev):
                    break
            else:
                now_anchor = self._reached(st['sends'], ev['after'])
                if now_anchor is None:
                    break
                was_anchor = self._reached(st['recorded_sends'], ev['after'])
                recorded_at = ev['s'] if 's' in ev else ev['peer_close']
                if (reason == SYNC_FRAME or 'state' in ev) and frame - now_anchor < recorded_at - was_anchor:
                    break
            st['server'].pop(0)
            if 'state' in ev:
                path = os.path.join(self.run.out_dir, f'{ev["state"]}.state.txt')
                self.run.states[ev['state']] = path
                cmds.append((CMD_STATE, sc._path(path)))
                if ev['state'] == seqgen.END_STATE:
                    self.quit_at = frame + QUIT_AFTER
            elif 's' in ev:
                data = bytes.fromhex(ev['data'])
                if st['framed']:
                    data = self._shape(st, data)
                if data:
                    cmds.append((CMD_DELIVER, u32(sid) + data))
            else:
                cmds.append((sc.CMD_PEER_CLOSE, u32(sid)))
        return cmds


def _scenarios_common():
    """scenarios/common.py, which logs a scenario in."""
    import importlib.util
    spec = importlib.util.spec_from_file_location('scenarios_common', os.path.join(HERE, 'scenarios', 'common.py'))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


COMMON = _scenarios_common()


def login_steps(quit_frame):
    s = COMMON.login(Seq(), LOGIN_ACCOUNT)
    s.at(quit_frame).quit()
    return s.steps


# ---- one case ---------------------------------------------------------------------------------

def split_client(client):
    return 'PARITY_RECV_SPLIT' in os.environ or 'PARITY_RECV_SPLIT' in parity.CLIENTS[client]['env']


def run_one(client, rec, prefix, out_dir):
    net = FuzzReplay(rec, prefix, split_client(client))
    script = sc.Script(login_steps(rec['seqgen']['quit'] + QUIT_SLACK))
    run = parity.run_client(client, sc.Scenario(script, net), out_dir, any_slot=True, timeout=60)
    return run, net


def masked_tail(net):
    """The client's sends after the cut, by frame, with CZ_REQUEST_TIME2's tick masked (the two
    clients read their clock a different number of times within a frame)."""
    out = []
    for frame, pkt in net.tail:
        if pkt[:2] == struct.pack('<H', CZ_REQUEST_TIME2):
            pkt = pkt[:2] + b'tick'
        out.append((frame, pkt.hex()))
    return out


def state_texts(run):
    return {n: parity.state_text(p) for n, p in run.states.items() if os.path.exists(p)}


# A checkpoint taken while a hit's short-lived objects are on screen (its damage digits and hit sprites,
# stepped by the clock) can find one client a frame ahead of the other in taking them down: the harness's
# virtual clock moves within a frame by how often a client reads it, which the original client and its
# C port do a different number of times. Such a checkpoint's object and effect counts are not compared;
# the last state, taken SETTLE_FRAMES after the last event, compares them.
WORLD_COUNTS = re.compile(r'^world objects=\d+ effects=\d+ ', re.M)


def settled(name):
    return name in (seqgen.START_STATE, seqgen.END_STATE)


def masked(text):
    return WORLD_COUNTS.sub('world objects=* effects=* ', text)


def diff_states(a, b, transient=None):
    """{state: [diff lines]} for the states two runs differ in (a missing state counts). A checkpoint
    that differs only in its object and effect counts is left out, and named in `transient`."""
    out = {}
    for name in sorted(set(a) | set(b), key=_state_order):
        if a.get(name) == b.get(name):
            continue
        if name in a and name in b and not settled(name) and masked(a[name]) == masked(b[name]):
            if transient is not None:
                transient.append(name)
            continue
        if name not in a or name not in b:
            out[name] = [f'state only in {"herc" if name in a else "ragexe"}']
            continue
        out[name] = [l for l in difflib.unified_diff(a[name].splitlines(), b[name].splitlines(), 'herc', 'ragexe',
                                                     n=0, lineterm='') if not l.startswith(('---', '+++', '@@'))]
    return out


def _state_order(name):
    return (name == seqgen.END_STATE, name)


def run_failures(run, net):
    out = [f'exit {run.exit_code}'] if run.exit_code != 0 else []
    out += [f'divergence: {d}' for d in net.divergences]
    out += [f'expectation: {d}' for d in run.expect_failures]
    return out


class Result:
    def __init__(self):
        self.diffs = {}           # state -> diff lines (herc against ragexe), unexplained ones
        self.intended = []        # states whose difference the stock-pending build explains
        self.transient = []       # checkpoints differing only in their object and effect counts
        self.sends = None         # the first tail send the clients part at
        self.streams = []         # with a split stream: where the send streams or send frames part
        self.failures = {}        # client -> what went wrong with its run itself
        self.n_states = 0

    @property
    def differs(self):
        return bool(self.diffs or self.sends or self.streams or self.failures)

    def signature(self):
        """The kinds of line that differ (the first word of each), for keeping a minimization on the
        same difference."""
        tags = set()
        for lines in self.diffs.values():
            for l in lines:
                words = l[1:].split()
                tags.add(words[0].split('@')[0] if words else '')
        if self.sends:
            tags.add('sends')
        if self.streams:
            tags.add('streams')
        for c in self.failures:
            tags.add('failure:' + c)
        return tags

    def to_json(self):
        return {'diffs': self.diffs, 'intended': self.intended, 'transient': self.transient, 'sends': self.sends,
                'streams': self.streams, 'failures': self.failures,
                'states': self.n_states}


def compare_case(rec, prefix, out_dir, keep=False):
    """Runs herc and ragexe on the case (and the stock-pending build where ragexe differs) and
    judges it."""
    res = Result()
    runs = {}

    def one(client):
        runs[client] = run_one(client, rec, prefix, os.path.join(out_dir, client))

    threads = [threading.Thread(target=one, args=(c,)) for c in PAIR]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    texts = {}
    for c, (run, net) in runs.items():
        f = run_failures(run, net)
        if f:
            res.failures[c] = f
        texts[c] = state_texts(run)
    a_name, b_name = PAIR
    res.n_states = len(texts[a_name])
    diffs = diff_states(texts[a_name], texts[b_name], res.transient)
    if diffs and 'ragexe_stock' in parity.CLIENTS:
        run, net = run_one('ragexe_stock', rec, prefix, os.path.join(out_dir, 'ragexe_stock'))
        f = run_failures(run, net)
        if f:
            res.failures['ragexe_stock'] = f
        stock = state_texts(run)
        for name in list(diffs):
            if stock.get(name) == texts[a_name].get(name):
                res.intended.append(name)
                del diffs[name]
    res.diffs = diffs
    a, b = masked_tail(runs[a_name][1]), masked_tail(runs[b_name][1])
    if a != b:
        i = next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), min(len(a), len(b)))
        res.sends = {'at': i, a_name: a[i:i + 3], b_name: b[i:i + 3], 'counts': [len(a), len(b)]}
    if any(split_client(c) for c in PAIR):
        ra, rb = runs[a_name][0], runs[b_name][0]
        res.streams = parity.diff_streams(parity.send_streams(ra), parity.send_streams(rb), rec['connections'])
        line = parity.diff_sends(parity.send_timeline(ra), parity.send_timeline(rb))
        if line:
            res.streams.append(line.replace('the golden run', a_name).replace('this run', b_name))
    if COVERAGE:
        for c in runs:
            record = os.path.join(out_dir, c, 'ragexe_coverage.txt')
            if os.path.exists(record):
                shutil.copy2(record, os.path.join(COVERAGE[0], f'{os.path.basename(out_dir)}_{c}.txt'))
    if not keep:
        shutil.rmtree(out_dir, ignore_errors=True)
    return res


# ---- many cases --------------------------------------------------------------------------------

def load_prefix():
    return seqgen.Prefix(sc.packet_sizes())


def case_for(seed, prefix, events=None, length=50):
    return seqgen.build(seed, prefix, events, length)


def sweep(out, seeds, jobs, length, keep):
    prefix = load_prefix()
    os.makedirs(out, exist_ok=True)
    log = open(os.path.join(out, 'sweep.jsonl'), 'a', encoding='utf-8')
    t0 = time.monotonic()
    done = [0]
    lock = threading.Lock()

    def one(seed):
        rec = case_for(seed, prefix, length=length)
        start = time.monotonic()
        res = compare_case(rec, prefix, os.path.join(out, f'case{seed}'), keep)
        took = time.monotonic() - start
        with lock:
            done[0] += 1
            line = {'seed': seed, 'seconds': round(took, 1), 'events': len(rec['seqgen']['events'])}
            line.update(res.to_json())
            log.write(json.dumps(line) + '\n')
            log.flush()
            mark = 'DIFFERS' if res.differs else 'same'
            extra = f' ({len(res.intended)} intended)' if res.intended else ''
            print(f'[{done[0]}/{len(seeds)}] seed {seed}: {mark}{extra} '
                  f'{sorted(res.diffs) if res.diffs else ""} {list(res.failures) if res.failures else ""}'
                  f'{" sends" if res.sends else ""} {took:.0f} s', flush=True)
            if res.differs:
                with open(os.path.join(out, f'case{seed}.json'), 'w') as f:
                    json.dump(rec, f)
        return res

    with concurrent.futures.ThreadPoolExecutor(jobs) as ex:
        results = list(ex.map(one, seeds))
    took = time.monotonic() - t0
    n = sum(r.differs for r in results)
    print(f'{len(seeds)} cases in {took:.0f} s ({took / max(len(seeds), 1):.1f} s a case wall, {jobs} at a time); '
          f'{n} differ')
    return results


def target(out, names, prefix, jobs, keep):
    """seqtarget.py's scripts `names`, each a case of its own (seed = its place in SCRIPTS)."""
    os.makedirs(out, exist_ok=True)
    order = list(seqtarget.SCRIPTS)
    lock = threading.Lock()

    def one(name):
        seed = order.index(name)
        rec = case_for(seed, prefix, seqtarget.events([name], prefix))
        start = time.monotonic()
        res = compare_case(rec, prefix, os.path.join(out, f'target_{name}'), keep)
        with lock:
            print(f'{name}: {"DIFFERS" if res.differs else "same"} ({res.n_states} states, '
                  f'{time.monotonic() - start:.0f} s)', flush=True)
            if res.differs:
                print('  ' + json.dumps(res.to_json())[:2000], flush=True)
                with open(os.path.join(out, f'target_{name}.json'), 'w') as f:
                    json.dump({'case': rec, 'result': res.to_json()}, f)
        return res

    with concurrent.futures.ThreadPoolExecutor(jobs) as ex:
        return list(ex.map(one, names))


# ---- minimizing ---------------------------------------------------------------------------------

def minimize(out, seed, events, prefix, jobs=3):
    """ddmin over the case's events: the shortest list (as rendered, every event the server could
    still send) whose runs still differ the same way (Result.signature)."""
    os.makedirs(out, exist_ok=True)
    counter = [0]
    lock = threading.Lock()

    def test(evs):
        with lock:
            counter[0] += 1
            k = counter[0]
        rec = case_for(seed, prefix, evs)
        res = compare_case(rec, prefix, os.path.join(out, f'try{k}'))
        return res, rec['seqgen']['events']

    base, events = test(events)
    if not base.differs:
        print('the case does not differ')
        return None, base
    want = base.signature()
    print(f'minimizing {len(events)} events; differs in {sorted(want)}', flush=True)

    def still(res):
        return res.differs and bool(res.signature() & want)

    n = 2
    best_res = base
    while len(events) >= 2:
        size = max(1, len(events) // n)
        parts = [events[i:i + size] for i in range(0, len(events), size)]
        cands = [[e for j, p in enumerate(parts) if j != i for e in p] for i in range(len(parts))]
        cands = [c for c in cands if c]
        found = None
        with concurrent.futures.ThreadPoolExecutor(jobs) as ex:
            for res, kept in ex.map(test, cands):
                if found is None and still(res) and len(kept) < len(events):
                    found = (res, kept)
        if found:
            best_res, events = found
            n = max(n - 1, 2)
            print(f'  {len(events)} events', flush=True)
        elif size == 1:
            break
        else:
            n = min(n * 2, len(events))
    # then each single event once more
    i = 0
    while i < len(events):
        cand = events[:i] + events[i + 1:]
        res, kept = test(cand)
        if still(res) and len(kept) < len(events):
            best_res, events = res, kept
        else:
            i += 1
    print(f'minimized to {len(events)} events after {counter[0]} runs')
    return events, best_res


def describe(events):
    out = []
    for ev in events:
        args = {k: v for k, v in ev.items() if k not in ('k', 'gap', 'attrs')}
        if 'attrs' in ev:
            a = ev['attrs']
            args.update({k: a[k] for k in ('job', 'utype', 'item', 'x', 'y') if k in a})
        out.append(f'+{ev["gap"]:<3} {ev["k"]:<8} ' + ' '.join(f'{k}={v}' for k, v in args.items()))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=['run', 'case', 'minimize', 'regress', 'target'])
    ap.add_argument('out')
    ap.add_argument('seed', nargs='?', type=int)
    ap.add_argument('--seeds', default='1-20')
    ap.add_argument('--jobs', type=int, default=4)
    ap.add_argument('--length', type=int, default=50)
    ap.add_argument('--events', help='a JSON file: a list of events, or a case with seqgen.events')
    ap.add_argument('--keep', action='store_true')
    ap.add_argument('--split', help='PARITY_RECV_SPLIT for both clients: SEED[:CHUNK[:FRAME]] (parity.c)')
    ap.add_argument('--clients', default='herc,ragexe', help='the two clients compared')
    ap.add_argument('--handlers', help='target: the handlers (or HEADER_ names, layouts, ids) to aim at')
    ap.add_argument('--scripts', help='target: seqtarget.py scripts by name')
    ap.add_argument('--coverage', help='a folder for the ragexe runs\' coverage records')
    args = ap.parse_args()
    if args.coverage:
        os.makedirs(args.coverage, exist_ok=True)
        COVERAGE[:] = [args.coverage]
    if args.split:
        os.environ['PARITY_RECV_SPLIT'] = args.split
    PAIR[:] = args.clients.split(',')
    if len(PAIR) != 2 or any(c not in parity.CLIENTS for c in PAIR):
        ap.error('--clients needs two of ' + ', '.join(sorted(parity.CLIENTS)))
    prefix = load_prefix()

    def events_arg():
        if not args.events:
            return None
        with open(args.events) as f:
            d = json.load(f)
        return d['seqgen']['events'] if isinstance(d, dict) else d

    if args.mode == 'run':
        lo, hi = (int(x) for x in args.seeds.split('-'))
        res = sweep(args.out, list(range(lo, hi + 1)), args.jobs, args.length, args.keep)
        sys.exit(1 if any(r.differs for r in res) else 0)
    if args.mode == 'case':
        rec = case_for(args.seed, prefix, events_arg(), args.length)
        res = compare_case(rec, prefix, os.path.join(args.out, f'case{args.seed}'), keep=True)
        with open(os.path.join(args.out, f'case{args.seed}', 'case.json'), 'w') as f:
            json.dump(rec, f)
        print(json.dumps(res.to_json(), indent=1))
        sys.exit(1 if res.differs else 0)
    if args.mode == 'minimize':
        evs = events_arg() or seqgen.generate(args.seed, prefix, args.length)
        events, res = minimize(os.path.join(args.out, f'min{args.seed}'), args.seed, evs, prefix, args.jobs)
        if events is None:
            sys.exit(0)
        path = os.path.join(args.out, f'min{args.seed}.json')
        with open(path, 'w') as f:
            json.dump({'seed': args.seed, 'events': events, 'result': res.to_json()}, f, indent=1)
        print('\n'.join(describe(events)))
        print(json.dumps(res.to_json(), indent=1))
        print('->', path)
        sys.exit(1)
    if args.mode == 'target':
        if args.scripts:
            names = args.scripts.split(',')
        elif args.handlers:
            names, unsent = seqtarget.scripts_for(args.handlers.split(','), parity.REPO, prefix)
            if unsent:
                print('no script sends ' + ', '.join(f'0x{i:04x}' for i in unsent))
        else:
            names = list(seqtarget.SCRIPTS)
        res = target(args.out, names, prefix, args.jobs, args.keep)
        sys.exit(1 if any(r.differs for r in res) else 0)
    if args.mode == 'regress':
        with open(REGRESS) as f:
            cases = json.load(f)['cases']
        failed = 0
        for c in cases:
            if 'bounds' in c:
                # the scripts with their text fields at a boundary (seqbounds.py), the 'only' fields
                # alone when given
                import seqbounds
                with seqbounds.Rewrite(c['bounds'], c.get('only')).active():
                    events = seqtarget.events(c['scripts'], prefix)
            else:
                events = seqtarget.events(c['scripts'], prefix) if 'scripts' in c else c.get('events')
            rec = case_for(c['seed'], prefix, events, c.get('length', args.length))
            res = compare_case(rec, prefix, os.path.join(args.out, f'reg{c["seed"]}'))
            ok = not res.differs
            failed += not ok
            print(f'{c["name"]}: {"same" if ok else "DIFFERS " + json.dumps(res.to_json())[:600]}')
        sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
