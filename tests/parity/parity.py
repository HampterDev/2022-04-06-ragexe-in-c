"""The parity runner: plays a scenario on a game client under parity.dll and keeps what it saw.

    python parity.py smoke --client herc|ragexe [--frames 30,60,120] [--quit N]
    python parity.py determinism --client herc|ragexe [--frames ...] [--quit N]
    python parity.py record SCENARIO --client herc|ragexe [--server HOST]
    python parity.py replay SCENARIO --client herc|ragexe
    python parity.py offline SCENARIO --client herc|ragexe
    python parity.py compare DIR_A DIR_B
    python parity.py bless SCENARIO...
    python parity.py check SCENARIO... --client herc|ragexe
    python parity.py expect SCENARIO RUN_DIR STATE... --why "RAGEXE_X: what changed and why"
    python parity.py pair SCENARIO... [--clients herc,ragexe]
    python parity.py upgrade SCENARIO... [--then SCENARIO] [--state DIR]

`smoke` runs the client offline (every connect is refused) and captures the listed frames;
`determinism` runs it twice and requires identical captures. `record` plays the scenario
scenarios/SCENARIO.py against a real server and keeps the session in recordings/SCENARIO.json;
`replay` plays that session to the client with no server and reports where the client's
packets differ from the recording; `offline` plays the scenario with every connect refused
(for its start-up steps). `compare` reports, for each capture two runs share, how
many pixels differ. `bless` replays the herc-client reference twice and, when the two runs
agree, keeps every capture's pixel hash and every state probe's text as the scenario's golden
file (recordings/SCENARIO.golden.json); `check` replays a client and compares it with that.
`pair` replays two clients and compares them with each other, for runs the golden file cannot
judge (PARITY_RECV_SPLIT, which moves server bytes to later frames).
`upgrade` is a player moving from herc to ragexe with the install herc left: the scenario's
first session runs on herc, and what it left (its folder's new and changed files, savedata
among them, and the registry) starts the later sessions on herc and on ragexe alike; the two are
compared with each other as `pair` compares them, the files each left included. The later
sessions are --then's when given (against the recording its RECORDING names), else the
scenario's own (its one session again for a scenario of one). --state DIR lays a
folder of files over what herc left (DIR\\savedata\\..., as in the client's folder) and
DIR\\registry.txt's lines (the virtual registry's KEY|NAME|TYPE|VALUE, or KEY|NAME|delete|)
over its registry: a saved state herc could have left, with values no scenario sets
(upgrade_states.py writes three; scenarios/upgrade_readback.py is the session that reads one
back).

ragexe is held to herc's golden but for the states an approved improvement changes on purpose:
improvements/SCENARIO.json maps each such state to ragexe's expected text and the reason (`expect`
writes it from a ragexe run's state files). Every other state must still be herc's.

A replay also holds what the client saved at quit (the registry and the savedata files) to the
golden's; saved_expected.json gives, per scenario, ragexe's own text for a saved file that differs
on purpose, and the reason. Any other difference is a divergence.

PARITY_VARIANT=<spec> lays a data variant (variants.py: messages emptied, Lua tables changed, files
emptied or removed) over every new run folder, and PARITY_SERVER_VARIANT=<spec> varies the texts a
replay's map server sends; `pair` then judges the two clients on the varied data. A variant scenario
(VARIANT and VARIANT_OF) keeps one in the gate, against its base scenario's recording.

Each run gets a fresh folder under RUN_ROOT: the client's exe, small files copied from
DATA_ROOT (a frozen copy of the client data), and the large read-only ones (GRFs, BGM, skin)
hard-linked. The user's own client folders are never used.
"""
import argparse
import hashlib
import importlib.util
import json
import msvcrt
import os
import re
import shutil
import struct
import subprocess
import sys
import threading
import time
import uuid
import _winapi

# Run with `python -I`, so this folder is not on the path by itself.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from protocol import (CMD_GO, EV_CLOSE, EV_CONNECT, EV_DIALOG, EV_LOG, EV_SEND, HELLO, SYNC)
import scenario as sc
import stack
import variants

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..'))
HARNESS = os.path.join(REPO, 'build', 'parity')
DATA_ROOT = os.environ.get('PARITY_DATA', r'C:\parity\data')
RUN_ROOT = os.environ.get('PARITY_RUNS', r'C:\parity\runs')
RECORDINGS = os.path.join(HERE, 'recordings')
IMPROVEMENTS = os.path.join(HERE, 'improvements')
SAVED_EXPECTED = os.path.join(HERE, 'saved_expected.json')
SCENARIOS = os.path.join(HERE, 'scenarios')

# PARITY_RAGEXE_EXE names another ragexe build to run as ragexe: a sanitizer build (build.bat,
# RAGEXE_SANITIZE) is build\asan\main_rebuild.exe or build\fill\main_rebuild.exe.
CLIENTS = {
    'herc': {'exe': os.environ.get('PARITY_HERC_EXE', r'C:\parity\reference\build\main_rebuild.exe'), 'env': {}},
    'ragexe': {'exe': os.environ.get('PARITY_RAGEXE_EXE', os.path.join(REPO, 'build', 'main_rebuild.exe')),
               'env': {}, 'aliases': {}},
}
# PARITY_SPLIT_CLIENTS=<spec> adds herc_split and ragexe_split: herc and ragexe with PARITY_RECV_SPLIT=<spec>
# set for them alone, run in herc's and ragexe's folders (`folder`), so that a pair can compare a client
# reading a split stream with one reading it whole.
if os.environ.get('PARITY_SPLIT_CLIENTS'):
    for _c in ('herc', 'ragexe'):
        CLIENTS[_c + '_split'] = dict(CLIENTS[_c], env={'PARITY_RECV_SPLIT': os.environ['PARITY_SPLIT_CLIENTS']},
                                      folder=_c)

# An AddressSanitizer build's runtime, which build.bat puts beside the exe; a run folder gets it with the exe.
ASAN_DLL = 'clang_rt.asan_dynamic-i386.dll'

# PARITY_ALIASES=1 copies data files under other names in a ragexe run folder (diagnostic: the
# kRO System file names the unported client asks for, served from the English data set).
if os.environ.get('PARITY_ALIASES'):
    CLIENTS['ragexe']['aliases'] = {
        'System/PetEvolutionCln_true.lub': 'System/PetEvolutionCln_true_E.lub',
        'System/Achievement_list.lub': 'System/achievement_list_EN.lub',
        'System/PrivateAirplane_True.lub': 'System/PrivateAirplane_T_EN.lub',
        'System/CheckAttendance.lub': 'System/CheckAttendance_EN.lub',
        'System/iteminfo_true.lub': 'System/itemInfo_EN.lua',
        'System/tipbox.lub': 'System/tipbox_EN.lub',
        'System/monster_size_effect_new.lub': 'System/monster_size_effect_EN.lub',
        'System/Towninfo.lub': 'System/Towninfo_EN.lub',
        'System/mapInfo_true.lub': 'System/mapInfo_true_EN.lub',
        'System/OngoingQuestInfoList_True.lub': 'System/OngoingQuestInfoList_True_EN.lub',
        'System/RecommendedQuestInfoList_True.lub': 'System/RecommendedQuestInfoList_True_EN.lub',
    }

# The data variant (variants.py) laid over each new run folder: the running scenario's VARIANT, else
# PARITY_VARIANT's. A variant scenario (VARIANT_OF names the scenario whose recording and steps it
# replays) keeps a data set the client must cope with in the gate.
VARIANT = [os.environ.get('PARITY_VARIANT')]

# The client's settings, served by parity.dll's virtual registry.
REGISTRY = [
    r'Software\Gravity Soft\Ragnarok|WIDTH|dword|1024',
    r'Software\Gravity Soft\Ragnarok|HEIGHT|dword|768',
    r'Software\Gravity Soft\Ragnarok|ISFULLSCREENMODE|dword|0',
    r'Software\Gravity Soft\Ragnarok|BITPERPIXEL|dword|32',
    r'Software\Gravity Soft\Ragnarok|SOUNDMODE|dword|0',
    r'Software\Gravity Soft\Ragnarok|SHOWTIPSATSTARTUP|dword|0',
]

# Files at least this large, and everything under these folders, are hard-linked; the rest is
# copied so that a client writing a file in place cannot change DATA_ROOT.
LINK_MIN_BYTES = 4 << 20
LINK_DIRS = ('BGM', 'skin')


def prepare_folder(client, run_dir):
    os.makedirs(run_dir)
    for dirpath, dirnames, files in os.walk(DATA_ROOT):
        rel = os.path.relpath(dirpath, DATA_ROOT)
        dst_dir = run_dir if rel == '.' else os.path.join(run_dir, rel)
        os.makedirs(dst_dir, exist_ok=True)
        top = rel.split(os.sep)[0]
        for name in files:
            src = os.path.join(dirpath, name)
            dst = os.path.join(dst_dir, name)
            if top in LINK_DIRS or os.path.getsize(src) >= LINK_MIN_BYTES:
                os.link(src, dst)
            else:
                shutil.copy2(src, dst)
    for alias, source in CLIENTS[client].get('aliases', {}).items():
        shutil.copy2(os.path.join(run_dir, source), os.path.join(run_dir, alias))
    if VARIANT[0]:
        print(variants.apply(run_dir, VARIANT[0], os.path.join(RUN_ROOT, 'variants'), DATA_ROOT))
    exe = os.path.join(run_dir, os.path.basename(CLIENTS[client]['exe']))
    shutil.copy2(CLIENTS[client]['exe'], exe)
    asan_dll = os.path.join(os.path.dirname(CLIENTS[client]['exe']), ASAN_DLL)
    if os.path.exists(asan_dll):
        shutil.copy2(asan_dll, os.path.join(run_dir, ASAN_DLL))
    reg = os.path.join(run_dir, 'parity_registry.txt')
    with open(reg, 'w') as f:
        f.write('\n'.join(REGISTRY) + '\n')
    return exe, reg


class Pipe:
    def __init__(self, name):
        self.h = _winapi.CreateNamedPipe(
            name, _winapi.PIPE_ACCESS_DUPLEX,
            _winapi.PIPE_WAIT,  # byte type and byte read mode are both 0
            1, 1 << 20, 1 << 20, 0, _winapi.NULL)
        self.last_io = time.monotonic()

    def accept(self):
        _winapi.ConnectNamedPipe(self.h, False)

    def read_exact(self, n):
        out = b''
        while len(out) < n:
            try:
                data, err = _winapi.ReadFile(self.h, n - len(out), False)
            except OSError:
                raise EOFError('pipe closed')
            if err or not data:
                raise EOFError('pipe closed')
            out += data
        self.last_io = time.monotonic()
        return out

    def read_msg(self):
        t, n = struct.unpack('<II', self.read_exact(8))
        return t, self.read_exact(n) if n else b''

    def write_msg(self, t, payload=b''):
        data = struct.pack('<II', t, len(payload)) + payload
        while data:
            try:
                written, err = _winapi.WriteFile(self.h, data, False)
            except OSError:
                raise EOFError('pipe closed')
            if err:
                raise EOFError('pipe write failed')
            data = data[written:]
        self.last_io = time.monotonic()

    def close(self):
        _winapi.CloseHandle(self.h)


class Run:
    """What one client run reported: its events in order, captures, states and dialogs."""

    def __init__(self, client, out_dir):
        self.client = client
        self.out_dir = out_dir
        self.events = []
        self.dialogs = []
        self.captures = {}
        self.states = {}
        self.divergences = []
        self.expect_failures = []
        self.frames = 0
        self.exit_code = None
        self.run_dir = None
        self.saved = None                # the registry the client left and its savedata files
        self.launcher_output = ''


def claim_run_folder(client, any_slot=False, wait=1800, keep=None):
    """A slot folder for the client (RUN_ROOT/herc-0, herc-1, ...), emptied, and the open lock
    file that keeps it ours until it is closed. The client keeps paths of its own folder in hash
    tables, whose order of teardown then sets the addresses of everything allocated after it:
    a folder named differently gives a run other addresses, and with them another order for
    objects drawn at the same depth. So every run whose pictures are compared uses the first
    slot, waiting up to `wait` seconds for it; `any_slot` (a recording, whose pictures nobody
    compares) takes the first free one. `keep` names the folder of the run before a restart,
    taken again as it is."""
    os.makedirs(RUN_ROOT, exist_ok=True)
    deadline = time.monotonic() + wait
    while True:
        for slot in range(8 if any_slot else 1):
            run_dir = keep or os.path.join(RUN_ROOT, f'{CLIENTS[client].get("folder", client)}-{slot}')
            lock = open(run_dir + '.lock', 'a+')
            try:
                msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
            except OSError:
                lock.close()
                continue
            if os.path.exists(run_dir) and not keep:
                shutil.rmtree(run_dir)
            return run_dir, lock
        if time.monotonic() > deadline:
            raise RuntimeError(f'no free run folder for {client} under {RUN_ROOT}')
        time.sleep(5)


def run_client(client, scenario, out_dir, timeout=90, any_slot=False, after=None):
    """One client run. `after` is the run before a restart: this one starts in its folder, as
    that run left it, with the registry it left."""
    run_dir, lock = claim_run_folder(client, any_slot, keep=after.run_dir if after else None)
    with lock:
        return _run_in(client, scenario, out_dir, timeout, run_dir, after)


def _run_in(client, scenario, out_dir, timeout, run_dir, after=None):
    if after:
        exe = os.path.join(run_dir, os.path.basename(CLIENTS[client]['exe']))
        reg = os.path.join(run_dir, 'parity_registry.txt')
        with open(reg, 'w', encoding='mbcs', errors='replace') as f:
            f.write(after.saved['registry'])
    else:
        exe, reg = prepare_folder(client, run_dir)
    os.makedirs(out_dir, exist_ok=True)
    run = Run(client, out_dir)
    run.run_dir = run_dir
    pipe_name = r'\\.\pipe\parity-' + uuid.uuid4().hex
    pipe = Pipe(pipe_name)
    env = dict(os.environ)
    env.update(CLIENTS[client]['env'])
    env.update({'PARITY_PIPE': pipe_name, 'PARITY_REG': reg,
                'PARITY_REG_END': os.path.join(out_dir, 'registry_end.txt'),
                'PARITY_LOG': os.path.join(out_dir, 'parity_dll.log')})
    if os.path.exists(os.path.join(run_dir, ASAN_DLL)):
        # ASan's reports go to the out folder (asan.<pid>). Its allocator fills new blocks with zeros, as
        # parity.dll's arena does for every other client, so a read of memory nobody wrote reads what it
        # reads in herc's run. The caller's ASAN_OPTIONS come after these and win.
        defaults = 'log_path="%s":malloc_fill_byte=0:max_malloc_fill_size=2147483647' % os.path.join(out_dir, 'asan')
        env['ASAN_OPTIONS'] = defaults + (':' + env['ASAN_OPTIONS'] if env.get('ASAN_OPTIONS') else '')
        # This MSVC's runtime cannot hook _except_handler4 in the current ucrtbase and can stop a process at
        # start-up for it. Going on costs only the unpoisoning of frames SEH unwinds.
        env.setdefault('ASAN_WIN_CONTINUE_ON_INTERCEPTION_FAILURE', '1')
    launcher = subprocess.Popen(
        [os.path.join(HARNESS, 'parity_launch.exe'), os.path.join(HARNESS, 'parity.dll'), run_dir, exe],
        env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    stalled = threading.Event()

    def release_pipe():
        # A client end opened and closed here ends a ConnectNamedPipe or ReadFile that would
        # otherwise wait for a client that is gone.
        try:
            h = _winapi.CreateFile(pipe_name, _winapi.GENERIC_READ | _winapi.GENERIC_WRITE, 0,
                                   _winapi.NULL, _winapi.OPEN_EXISTING, 0, _winapi.NULL)
            _winapi.CloseHandle(h)
        except OSError:
            pass

    def watchdog():
        while True:
            if launcher.poll() is not None:
                time.sleep(1)
                release_pipe()
                return
            if time.monotonic() - pipe.last_io > timeout:
                stalled.set()
                launcher.kill()
                release_pipe()
                return
            time.sleep(0.5)

    threading.Thread(target=watchdog, daemon=True).start()
    try:
        pipe.accept()
        t, body = pipe.read_msg()
        if t != HELLO:
            raise RuntimeError(f'expected HELLO, got {t}')
        batch = []
        while True:
            t, body = pipe.read_msg()
            if t == SYNC:
                reason, frame, clock_ms, index = struct.unpack('<IIII', body)
                run.frames = max(run.frames, frame)
                run.events.append(('sync', index, reason, frame, clock_ms))
                for cmd, payload in scenario.on_sync(run, reason, frame, clock_ms, batch):
                    pipe.write_msg(cmd, payload)
                batch = []
                pipe.write_msg(CMD_GO)
                continue
            if t == EV_CONNECT:
                e = ('connect',) + struct.unpack('<III', body)
            elif t == EV_SEND:
                e = ('send', struct.unpack('<I', body[:4])[0], body[4:])
            elif t == EV_CLOSE:
                e = ('close', struct.unpack('<I', body)[0])
            elif t == EV_DIALOG:
                e = ('dialog', body.decode('mbcs', 'replace'))
                run.dialogs.append(e[1])
            elif t == EV_LOG:
                e = ('log', body.decode('mbcs', 'replace'))
            else:
                raise RuntimeError(f'unknown message {t}')
            run.events.append(e)
            batch.append(e)
    except EOFError:
        pass
    finally:
        pipe.close()
    try:
        run.exit_code = launcher.wait(timeout=30)
    except subprocess.TimeoutExpired:
        launcher.kill()
        run.exit_code = launcher.wait()
    scenario.finish(run)
    out = launcher.stdout.read().decode('mbcs', 'replace')
    if stalled.is_set():
        out += f'\nstopped: no message from the client for {timeout} s'
    run.launcher_output = out.strip()
    with open(os.path.join(out_dir, 'events.txt'), 'w', encoding='utf-8') as f:
        for e in run.events:
            if e[0] == 'send':
                e = ('send', e[1], e[2].hex())
            f.write(repr(e) + '\n')
    run.saved = saved_state(run)
    # A coverage build (build.bat with RAGEXE_COVERAGE=1) leaves the functions that ran in its start folder,
    # which the next run in this slot empties: keep a copy with the run's output (coverage_report.py).
    record = os.path.join(run_dir, 'ragexe_coverage.txt')
    if os.path.exists(record):
        shutil.copy2(record, os.path.join(out_dir, 'ragexe_coverage.txt'))
    return run


# The client's own settings files: compared whole when this small and text, by hash otherwise.
SAVED_TEXT_MAX = 4096


def saved_state(run):
    """What the client keeps between sessions, as it left it: the registry (the harness's virtual
    one, dumped at quit) and every file under savedata but the logs."""
    reg_path = os.path.join(run.out_dir, 'registry_end.txt')
    registry = ''
    if os.path.exists(reg_path):
        with open(reg_path, encoding='mbcs', errors='replace') as f:
            registry = f.read()
    files = {}
    root = os.path.join(run.run_dir, 'savedata')
    for dirpath, _, names in os.walk(root):
        for name in names:
            if name.lower().endswith('.log'):
                continue                 # a diagnostic log, not a setting (some print pointers)
            path = os.path.join(dirpath, name)
            rel = os.path.relpath(path, root).replace(os.sep, '/')
            data = open(path, 'rb').read()
            try:
                text = data.decode('cp949') if len(data) <= SAVED_TEXT_MAX else None
            except UnicodeDecodeError:
                text = None
            files[rel] = text if text is not None else 'sha256:' + hashlib.sha256(data).hexdigest()
    return {'registry': registry, 'files': dict(sorted(files.items()))}


def read_bmp(path):
    with open(path, 'rb') as f:
        b = f.read()
    off = struct.unpack_from('<I', b, 10)[0]
    w, h = struct.unpack_from('<ii', b, 18)
    return abs(w), abs(h), b[off:]


def diff_images(a, b):
    """(differing pixels, total pixels, largest channel difference), alpha ignored."""
    wa, ha, pa = read_bmp(a)
    wb, hb, pb = read_bmp(b)
    if (wa, ha) != (wb, hb):
        return None
    if pa == pb:
        return 0, wa * ha, 0
    differ = 0
    worst = 0
    for i in range(0, wa * ha * 4, 4):
        if pa[i:i + 3] != pb[i:i + 3]:
            differ += 1
            worst = max(worst, max(abs(pa[i + k] - pb[i + k]) for k in range(3)))
    return differ, wa * ha, worst


def report(run):
    print(f'{run.client}: exit {run.exit_code}, {run.frames} frames, {len(run.captures)} captures, '
          f'{len(run.dialogs)} dialogs, folder {run.run_dir}')
    for d in run.dialogs[:20]:
        print('  dialog:', d)
    for d in run.divergences:
        print('  divergence:', d)
    for d in run.expect_failures:
        print('  expectation:', d)
    if getattr(run, 'tick_slack', None):
        print(f'  ticks within a frame of the recorded ones: {len(run.tick_slack)} ({run.tick_slack[0]}, ...)')
    if getattr(run, 'shaped', None):
        print('  shaped to its caps:', ', '.join(f'0x{k:04x} x{v}' for k, v in run.shaped.items()))
    if run.launcher_output:
        print('  launcher:', run.launcher_output)


def compare_dirs(a, b):
    names = sorted(n for n in os.listdir(a) if n.endswith('.bmp') and os.path.exists(os.path.join(b, n)))
    same = True
    for n in names:
        r = diff_images(os.path.join(a, n), os.path.join(b, n))
        if r is None:
            print(f'{n}: sizes differ')
            same = False
        else:
            differ, total, worst = r
            print(f'{n}: {differ} of {total} pixels differ (largest channel difference {worst})')
            same = same and differ == 0
    return same


def scenario_meta(name):
    """The account the scenario logs in with (scenarios/common.py notes it, None for a scenario
    that logs in by hand) and the bot behaviour it declares (BOT), if any."""
    load_script(name)
    import common
    return common.CURRENT_ACCOUNT, getattr(sys.modules.get(f'scenario_{name}'), 'BOT', None)


def load_script(name):
    return load_phases(name)[0]


def load_phases(name):
    """A scenario's steps, one Script per client session: PHASES when the scenario restarts the
    client (each session starts in the folder the last one left, with its registry), else STEPS."""
    if SCENARIOS not in sys.path:
        sys.path.insert(0, SCENARIOS)
    if 'common' in sys.modules:
        sys.modules['common'].CURRENT_ACCOUNT = None
    spec = importlib.util.spec_from_file_location(name, os.path.join(SCENARIOS, name + '.py'))
    mod = importlib.util.module_from_spec(spec)
    sys.modules[f'scenario_{name}'] = mod
    spec.loader.exec_module(mod)
    return [sc.Script(steps) for steps in getattr(mod, 'PHASES', None) or [mod.STEPS]]


def recording_path(name, phase):
    mod = sys.modules.get(f'scenario_{name}')
    name = getattr(mod, 'VARIANT_OF', None) or name
    return os.path.join(RECORDINGS, name + ('.json' if phase == 0 else f'.p{phase}.json'))


# Before a later session of a recording: the map server keeps an account logged in for a few
# seconds after its client is gone, and refuses a login meanwhile ("already logged in").
RESTART_PAUSE = 10


def run_phases(client, name, network, out_dir, any_slot=False, live=False, strict=True):
    """Every session of a scenario in turn, each after the last in the same folder; `network(k)`
    is session k's network. `live` (a recording) waits RESTART_PAUSE seconds before each later
    session. Stops after a session that failed (with `strict` off, only after one whose client
    did not exit cleanly). Returns the runs."""
    runs = []
    phases = load_phases(name)
    VARIANT[0] = getattr(sys.modules[f'scenario_{name}'], 'VARIANT', None) or os.environ.get('PARITY_VARIANT')
    for k, script in enumerate(phases):
        if k and live:
            time.sleep(RESTART_PAUSE)
        run = run_client(client, sc.Scenario(script, network(k)),
                         out_dir if k == 0 else os.path.join(out_dir, f'phase{k}'), any_slot=any_slot,
                         after=runs[-1] if runs else None)
        report(run)
        runs.append(run)
        if run.exit_code != 0 or strict and (run.divergences or run.expect_failures):
            break
    return runs


class Phases:
    """The runs of a scenario's sessions seen as one: their captures and states together (their
    names are the scenario's own, unique across sessions), what the last one saved."""

    def __init__(self, runs):
        self.runs = runs
        self.captures, self.states = {}, {}
        for run in runs:
            self.captures.update(run.captures)
            self.states.update(run.states)
        self.saved = runs[-1].saved


KEEP_OUTPUTS = 30    # newest folders kept under RUN_ROOT/out: a bless of a long scenario writes gigabytes
KEEP_RECENT_S = 3600  # and any written in the last hour: a run going on at the same time may still use it


def last_written(path):
    """When the folder or one of its direct subfolders (a bless's run1 and run2) last changed."""
    try:
        times = [os.path.getmtime(path)]
        times += [e.stat().st_mtime for e in os.scandir(path) if e.is_dir()]
    except OSError:
        return time.time()
    return max(times)


def out_dir_for(args, label):
    if args.out:
        return args.out
    root = os.path.join(RUN_ROOT, 'out')
    if os.path.isdir(root):
        cutoff = time.time() - KEEP_RECENT_S
        for old in sorted(os.listdir(root))[:-KEEP_OUTPUTS or None]:
            if last_written(os.path.join(root, old)) < cutoff:
                shutil.rmtree(os.path.join(root, old), ignore_errors=True)
    return os.path.join(root, time.strftime('%Y%m%d-%H%M%S') + '-' + label)


def capture_hash(path):
    """The pixels of a capture (the header and the alpha bytes left out)."""
    _, _, px = read_bmp(path)
    rgb = bytearray(px)
    del rgb[3::4]
    return hashlib.sha256(rgb).hexdigest()


def send_timeline(run):
    """When the client sent what: [frame, connection, bytes sent on it so far, the packet id the
    frame's first send starts with] for every frame in which it sent anything. A replay already
    requires the bytes to match the recording, so this adds their timing: a packet sent a frame
    sooner or later (a walk sent during a swing, a skill pressed during its delay) shows here
    when nothing on screen differs. Connections are numbered in the order the client opened them;
    sends are counted per frame, so how a client splits a frame's bytes into calls does not matter."""
    out, conn, total, pending = [], {}, [], []
    for e in run.events:
        if e[0] == 'connect':
            conn[e[1]] = len(total)         # a socket handle reused for a later connection gets a new number
            total.append(0)
        elif e[0] == 'send' and e[1] in conn:
            pending.append(e)
        elif e[0] == 'sync' and pending:
            frame, first = e[3], {}
            for _, sid, data in pending:
                index = conn[sid]
                first.setdefault(index, data[:2][::-1].hex())
                total[index] += len(data)
            out += [[frame, index, total[index], first[index]] for index in sorted(first)]
            pending = []
    return out


# A window's id slot that holds more than an id: some controls keep a pointer there (the replay
# window's bitmaps point into a file buffer the client allocates outside the harness's arena, at an
# address that changes from run to run). Ids are small numbers, so a large one is left out.
POINTER_ID = re.compile(r'\bid=(\d{6,})\b')


def state_text(path):
    with open(path, encoding='mbcs', errors='replace') as f:
        return ''.join(POINTER_ID.sub('id=*', line) if '@@' in line else line for line in f)


def side_file(path, suffix):
    if os.path.exists(path + suffix):
        with open(path + suffix) as f:
            return f.read().strip()
    return None


def run_summary(phases):
    """What a replay is judged by: every capture's pixel hash, every state's text, the frames the
    client sent its packets in (a later session's frames counted from 100000 times its number),
    the main window's style and the work counters at each state, and what the client saved."""
    sends = []
    for k, run in enumerate(phases.runs):
        sends += [[e[0] + 100000 * k] + e[1:] for e in send_timeline(run)]
    window, counters = {}, {}
    for name, path in phases.states.items():
        if (w := side_file(path, '.window')) is not None:
            window[name] = w
        if (c := side_file(path, '.counters')) is not None:
            counters[name] = {k: int(v) for k, v in (item.split('=') for item in c.split())}
    return {'captures': {n: capture_hash(p) for n, p in sorted(phases.captures.items())},
            'states': dict(sorted((n, state_text(p)) for n, p in phases.states.items())), 'sends': sends,
            'window': dict(sorted(window.items())), 'counters': dict(sorted(counters.items())),
            'saved': phases.saved}


# A work counter may move this much (a fraction, or COUNTER_SLACK, whichever is larger) before it
# counts as different: a port is not expected to make exactly the same calls, only about as many.
COUNTER_TOLERANCE = 0.10
COUNTER_SLACK = 16


def diff_counters(want, got):
    """For each work counter, the first state at which it moved past the tolerance."""
    lines, seen = [], set()
    for name in sorted(set(want) & set(got)):
        for key in sorted(set(want[name]) & set(got[name])):
            a, b = want[name][key], got[name][key]
            if key not in seen and abs(b - a) > max(COUNTER_SLACK, COUNTER_TOLERANCE * a):
                seen.add(key)
                lines.append(f'counter {key} at {name}: {a} in the golden run, {b} in this one')
    return lines


def diff_saved(want, got):
    lines = []
    if want['registry'] != got['registry']:
        a, b = set(want['registry'].splitlines()), set(got['registry'].splitlines())
        lines.append(f'saved registry differs: only in the golden run {sorted(a - b)[:4]}, '
                     f'only in this one {sorted(b - a)[:4]}')
    for name in sorted(set(want['files']) | set(got['files'])):
        if want['files'].get(name) != got['files'].get(name):
            lines.append(f'saved file {name} differs' if name in want['files'] and name in got['files']
                         else f'saved file {name} only in {"the golden run" if name in want["files"] else "this run"}')
    return lines


def saved_divergences(name, got):
    """What a replay of `name` saved that differs from the golden's, as divergence lines; a file
    saved_expected.json names for the scenario is held to the text given there instead."""
    path = os.path.join(RECORDINGS, name + '.golden.json')
    if not got or not os.path.exists(path):
        return []
    with open(path, encoding='utf-8') as f:
        want = json.load(f).get('saved')
    if not want:
        return []
    files = dict(want['files'])
    if os.path.exists(SAVED_EXPECTED):
        with open(SAVED_EXPECTED, encoding='utf-8') as f:
            for file, e in json.load(f).get(name, {}).items():
                files[file] = e['text']
    return ['saved-file divergence: ' + line
            for line in diff_saved({'registry': want['registry'], 'files': files}, got)]


def diff_sends(want, got):
    """The first frame two send timelines part at, as a line, or None."""
    for a, b in zip(want, got):
        if a != b:
            return (f'sends differ from frame {min(a[0], b[0])}: the golden run sent packet {a[3]} on connection '
                    f'{a[1]} at frame {a[0]} (bytes to {a[2]}), this run {b[3]} on connection {b[1]} at frame '
                    f'{b[0]} (bytes to {b[2]})')
    if len(want) != len(got):
        return f'sends differ: {len(want)} frames with sends in the golden run, {len(got)} in this one'
    return None


def replay_summary(client, name, out_dir):
    runs = run_phases(client, name, lambda k: sc.Replay(recording_path(name, k)), out_dir)
    phases = Phases(runs)
    last = runs[-1]
    if last.exit_code != 0 or last.divergences or last.expect_failures or len(runs) < len(load_phases(name)):
        return phases, None
    return phases, run_summary(phases)


def diff_summaries(want, got, noisy=()):
    """Lines naming every capture and state that differ; captures named in `noisy` are left to
    the caller (a pixel comparison)."""
    lines = []
    for kind in ('captures', 'states'):
        for name in sorted(set(want[kind]) | set(got[kind])):
            if kind == 'captures' and name in noisy and name in want[kind] and name in got[kind]:
                continue
            if want[kind].get(name) != got[kind].get(name):
                lines.append(f'{kind[:-1]} {name} differs' if name in want[kind] and name in got[kind]
                             else f'{kind[:-1]} {name} only in {"the golden run" if name in want[kind] else "this run"}')
    if 'sends' in want and 'sends' in got:
        line = diff_sends(want['sends'], got['sends'])
        if line:
            lines.append(line)
    if 'window' in want and 'window' in got:
        for name in sorted(set(want['window']) | set(got['window'])):
            if want['window'].get(name) != got['window'].get(name):
                lines.append(f'window at {name}: {want["window"].get(name)} in the golden run, '
                             f'{got["window"].get(name)} in this one')
    if 'counters' in want and 'counters' in got:
        lines += diff_counters(want['counters'], got['counters'])
    if want.get('saved') and got.get('saved'):
        lines += diff_saved(want['saved'], got['saved'])
    return lines


# Two reference runs can disagree in a few pixels at sprite edges: the sprite atlas places
# textures by the order of Direct3D objects the driver allocates, which the harness does not
# control. A capture that differs in no more than this many pixels counts as the same.
NOISE_PIXELS = 64


def pixel_noise(a, b):
    """True when two captures differ in at most NOISE_PIXELS pixels."""
    r = diff_images(a, b)
    return r is not None and r[0] <= NOISE_PIXELS


def bless(name, out_base):
    """Replays the reference twice; two runs that agree become the scenario's golden file. A
    capture the two runs draw a few pixels apart is kept, marked noisy."""
    a_run, a = replay_summary('herc', name, os.path.join(out_base, 'run1'))
    b_run, b = replay_summary('herc', name, os.path.join(out_base, 'run2'))
    if a is None or b is None:
        print('NOT blessed: a reference replay failed')
        return False
    noisy = [n for n in a['captures'] if n in b['captures'] and a['captures'][n] != b['captures'][n]
             and pixel_noise(a_run.captures[n], b_run.captures[n])]
    differ = diff_summaries(a, b, noisy)
    if differ:
        print('NOT blessed: the reference is not deterministic here:')
        for line in differ:
            print('  ' + line)
        return False
    a['noisy'] = sorted(noisy)
    path = os.path.join(RECORDINGS, name + '.golden.json')
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(a, f, indent=1, ensure_ascii=False)
    print(f'blessed: {len(a["captures"])} captures ({len(noisy)} noisy), {len(a["states"])} states, '
          f'{len(a["sends"])} frames with sends, {len(a["saved"]["files"])} saved files -> {path}')
    return True


def improvements(name):
    """{state: {'text': ragexe's expected text, 'why': the approved improvement}} for the scenario."""
    path = os.path.join(IMPROVEMENTS, name + '.json')
    if not os.path.exists(path):
        return {}
    with open(path, encoding='utf-8') as f:
        return json.load(f)


def golden(name, client='ragexe'):
    """The scenario's golden file as `client` is judged by it: herc's, with ragexe's approved improvements
    laid over the states they change."""
    with open(os.path.join(RECORDINGS, name + '.golden.json'), encoding='utf-8') as f:
        want = json.load(f)
    if client == 'ragexe':
        for state, e in improvements(name).items():
            if state not in want['states']:
                raise KeyError(f'improvements/{name}.json names {state}, which the golden does not have')
            want['states'][state] = e['text']
    return want


def expect(name, run_dir, states, why):
    """Keeps the named states of a ragexe run as the scenario's expected text, for the reason given."""
    want = golden(name, 'herc')
    known = improvements(name)
    for state in states:
        if state not in want['states']:
            raise KeyError(f'{name} has no state {state}')
        path = os.path.join(run_dir, state + '.state.txt')
        if not os.path.exists(path):
            raise FileNotFoundError(path)
        text = state_text(path)
        if text == want['states'][state]:
            known.pop(state, None)
            print(f'  {state}: the same as herc\'s, no expectation kept')
        else:
            known[state] = {'text': text, 'why': why}
            print(f'  {state}: ragexe\'s text kept')
    path = os.path.join(IMPROVEMENTS, name + '.json')
    if known:
        os.makedirs(IMPROVEMENTS, exist_ok=True)
        with open(path, 'w', encoding='utf-8') as f:
            json.dump(dict(sorted(known.items())), f, indent=1, ensure_ascii=False)
    elif os.path.exists(path):
        os.remove(path)


def check(client, name, out_dir):
    """Replays `client` and compares it with the golden file. A capture whose hash differs is
    compared pixel by pixel with a fresh reference replay's: within NOISE_PIXELS it matches."""
    want = golden(name, client)
    run, got = replay_summary(client, name, out_dir)
    if got is None:
        print(f'{name}: FAILS (the replay itself)')
        return False
    mismatched = [n for n in want['captures'] if n in got['captures'] and want['captures'][n] != got['captures'][n]]
    differ = diff_summaries(want, got, mismatched)
    if mismatched:
        ref_run, _ = replay_summary('herc', name, out_dir + '-reference')
        for n in mismatched:
            if n not in ref_run.captures or not pixel_noise(ref_run.captures[n], run.captures[n]):
                differ.append(f'capture {n} differs')
            else:
                print(f'  capture {n}: within {NOISE_PIXELS} pixels of the reference')
    for line in differ:
        print('  ' + line)
    print(f'{name}: ' + ('matches the reference' if not differ else f'FAILS ({len(differ)} differences)'))
    return not differ


def send_streams(run):
    """Each connection's sent bytes, the connections in the order the client opened them."""
    conn, out = {}, []
    for e in run.events:
        if e[0] == 'connect':
            conn[e[1]] = len(out)
            out.append(bytearray())
        elif e[0] == 'send' and e[1] in conn:
            out[conn[e[1]]] += e[2]
    return out


def diff_streams(a, b, recorded):
    """Where two runs' send streams part, a line per connection. The bytes a replay leaves out of
    its comparison with the recording (Replay._mask, by the `recorded` connections) are left out
    here too, and a keepalive's tick under a frame from the other run's is taken as it."""
    lines = [f'{len(a)} connections against {len(b)}'] if len(a) != len(b) else []
    for k, (x, y) in enumerate(zip(a, b)):
        x, y = bytearray(x), bytearray(y)
        if k < len(recorded) and 'refused' not in recorded[k]:
            for lo, hi in sc.Replay._mask([bytes.fromhex(ev['data']) for ev in recorded[k]['events'] if 'c' in ev]):
                x[lo:hi] = bytes(len(x[lo:hi]))
                y[lo:hi] = bytes(len(y[lo:hi]))
        n = min(len(x), len(y))
        i = x.find(sc.KEEPALIVE, 0, n)
        while i != -1 and i + 6 <= n:
            if y[i:i + 2] == sc.KEEPALIVE and abs(struct.unpack_from('<I', x, i + 2)[0] -
                                                   struct.unpack_from('<I', y, i + 2)[0]) < sc.FRAME_MS:
                y[i + 2:i + 6] = x[i + 2:i + 6]
            i = x.find(sc.KEEPALIVE, i + 1, n)
        if x[:n] != y[:n]:
            at = next(i for i in range(n) if x[i] != y[i])
            lines.append(f'connection {k + 1}: byte {at} sent as {x[at:at + 16].hex()} and as {y[at:at + 16].hex()}')
        elif len(x) != len(y):
            lines.append(f'connection {k + 1}: {len(x)} bytes sent against {len(y)}')
    return lines


def replay_notes(run):
    """A run's divergences from the recording but its byte comparisons: what went wrong with the replay."""
    return [d for d in run.divergences if ' sent byte ' not in d and ' sent more than recorded ' not in d]


def pair(name, clients, out_dir):
    """Replays the scenario on two clients (two runs of one client, or herc and ragexe) and judges
    them by each other rather than by the golden file: under PARITY_RECV_SPLIT a client takes some
    server bytes a frame later than the recording had them, so its states, its send frames and the
    ticks in what it sends move off the recording's, the same way in two clients that read the
    stream alike. The replay still plays: each server chunk is due by the client's sent byte count,
    which timing does not change, and frames after it. Compared: the send streams (diff_streams),
    every capture (within NOISE_PIXELS), state, send frame, window, counter and saved file
    (diff_summaries), and every divergence and expectation failure but the recording's byte
    comparisons. When the two are different builds, the captures, the work counters and the states
    ragexe changes on purpose (improvements/SCENARIO.json) are left out, and the saved files are compared in
    the forms SAVED_FORMS makes one. The message boxes each run showed are compared too."""
    dirs = [os.path.join(out_dir, f'{c}{i + 1}') for i, c in enumerate(clients)]
    runs = [None, None]

    def one(i):
        runs[i] = run_phases(clients[i], name, lambda k: sc.Replay(recording_path(name, k)), dirs[i], strict=False)

    if CLIENTS[clients[0]].get('folder', clients[0]) == CLIENTS[clients[1]].get('folder', clients[1]):
        one(0)
        one(1)
    else:
        threads = [threading.Thread(target=one, args=(i,)) for i in (0, 1)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
    phases = [Phases(r) for r in runs]
    summaries = [run_summary(p) for p in phases]
    lines = []
    if len(runs[0]) != len(runs[1]):
        lines.append(f'{len(runs[0])} sessions against {len(runs[1])}')
    for c, r in zip(clients, runs):
        if r[-1].exit_code != 0:
            lines.append(f'{c}: exit {r[-1].exit_code}')
    if CLIENTS[clients[0]].get('folder', clients[0]) != CLIENTS[clients[1]].get('folder', clients[1]):
        for s in summaries:
            for state in improvements(name):
                s['states'].pop(state, None)
            # pictures and work counters differ between two builds by design: the gate judges sends and states
            s['captures'], s['counters'] = {}, {}
            if s.get('saved'):
                s['saved'] = saved_forms(s['saved'])
    mismatched = [n for n in summaries[0]['captures'] if summaries[0]['captures'][n] != summaries[1]['captures'].get(n)
                  and n in summaries[1]['captures']]
    lines += diff_summaries(summaries[0], summaries[1], mismatched)
    for n in mismatched:
        if not pixel_noise(phases[0].captures[n], phases[1].captures[n]):
            lines.append(f'capture {n} differs')
    for k, (ra, rb) in enumerate(zip(runs[0], runs[1])):
        with open(recording_path(name, k)) as f:
            recorded = json.load(f)['connections']
        lines += [f'session {k}: {l}' for l in diff_streams(send_streams(ra), send_streams(rb), recorded)]
        if replay_notes(ra) != replay_notes(rb):
            lines.append(f'session {k}: replay {replay_notes(ra)[:3]} against {replay_notes(rb)[:3]}')
        # a box naming the run folder names each client's own
        da, db = ([d.replace(r.run_dir, '<folder>') for d in r.dialogs] for r in (ra, rb))
        if da != db:
            lines.append(f'session {k}: message boxes {da[:3]} against {db[:3]}')
        if ra.expect_failures != rb.expect_failures:
            lines.append(f'session {k}: expectations {ra.expect_failures[:3]} against {rb.expect_failures[:3]}')
    left_out = [d for r in runs[0] for d in replay_notes(r)]
    for line in lines:
        print('  ' + line.replace('the golden run', clients[0]).replace('this one', clients[1])
              .replace('this run', clients[1]))
    print(f'{name}: ' + (f'{clients[0]} and {clients[1]} agree' if not lines else f'DIFFER ({len(lines)} differences)')
          + (f'; the replay itself: {left_out[:2]}' if left_out else ''))
    return not lines


# ---- upgrade: herc's saved state taken over by ragexe --------------------------------------

# Files of a run folder never carried to a later session on another client: the client itself and the
# harness's own files. Logs are diagnostics of the run that wrote them.
NOT_CARRIED = {'parity_registry.txt', ASAN_DLL.lower()}


def folder_changes(run_dir, exe):
    """The files of a run folder its client made or changed: every one DATA_ROOT lacks or holds with other
    bytes (relative paths, '/'-separated), but the exe, NOT_CARRIED and logs. A hard-linked file (LINK_DIRS,
    LINK_MIN_BYTES) is the data's own unless its size or time moved."""
    out = []
    for dirpath, _, names in os.walk(run_dir):
        rel_dir = os.path.relpath(dirpath, run_dir)
        top = rel_dir.split(os.sep)[0]
        for name in names:
            rel = name if rel_dir == '.' else os.path.join(rel_dir, name)
            low = name.lower()
            if (rel_dir == '.' and (low == exe.lower() or low in NOT_CARRIED)) or low.endswith('.log'):
                continue
            path, src = os.path.join(dirpath, name), os.path.join(DATA_ROOT, rel)
            if os.path.exists(src):
                a, b = os.stat(path), os.stat(src)
                if top in LINK_DIRS or b.st_size >= LINK_MIN_BYTES:
                    if a.st_size == b.st_size and int(a.st_mtime) == int(b.st_mtime):
                        continue
                elif a.st_size == b.st_size and open(path, 'rb').read() == open(src, 'rb').read():
                    continue
            out.append(rel.replace(os.sep, '/'))
    return sorted(out)


def registry_lines(text):
    """The virtual registry's dump as {(key, value name): line}, the names compared as the client's reads
    compare them (without case)."""
    out = {}
    for line in text.splitlines():
        parts = line.split('|')
        if len(parts) >= 4:
            out[(parts[0].lower(), parts[1].lower())] = line
    return out


def take_state(run, out_dir, state=None):
    """What a run left for the next session, kept in out_dir/carried: the files folder_changes names (the
    folder's own layout) and registry.txt, the registry it left. `state` is a folder laid out the same way
    whose files replace or join them; its registry.txt lines replace the values they name, and a line
    'KEY|NAME|delete|' removes one. Returns (the carried files, the registry text)."""
    carried = os.path.join(out_dir, 'carried')
    if os.path.exists(carried):
        shutil.rmtree(carried)
    os.makedirs(carried)
    files = folder_changes(run.run_dir, os.path.basename(CLIENTS[run.client]['exe']))
    for rel in files:
        os.makedirs(os.path.dirname(os.path.join(carried, rel)), exist_ok=True)
        shutil.copy2(os.path.join(run.run_dir, rel), os.path.join(carried, rel))
    reg = registry_lines(run.saved['registry'])
    if state:
        for dirpath, _, names in os.walk(state):
            for name in names:
                rel = os.path.relpath(os.path.join(dirpath, name), state).replace(os.sep, '/')
                if rel == 'registry.txt':
                    with open(os.path.join(dirpath, name), encoding='mbcs') as f:
                        for key, line in registry_lines(f.read()).items():
                            if line.split('|')[2] == 'delete':
                                reg.pop(key, None)
                            else:
                                reg[key] = line
                    continue
                os.makedirs(os.path.dirname(os.path.join(carried, rel)), exist_ok=True)
                shutil.copyfile(os.path.join(dirpath, name), os.path.join(carried, rel))
                if rel not in files:
                    files.append(rel)
    registry = ''.join(line + '\n' for line in reg.values())
    with open(os.path.join(carried, 'registry.txt'), 'w', encoding='mbcs', errors='replace') as f:
        f.write(registry)
    return sorted(files), registry


class Carried:
    """Stands for the run before a restart (run_client's `after`) when the next session starts in a new
    folder holding what another client left."""

    def __init__(self, run_dir, registry):
        self.run_dir = run_dir
        self.saved = {'registry': registry}


def run_carried(client, script, network, out_dir, carried, files, registry):
    """A session of `client` in a new folder (prepare_folder) that holds `files` from the folder
    `carried` and starts with `registry`."""
    run_dir, lock = claim_run_folder(client)
    with lock:
        prepare_folder(client, run_dir)
        for rel in files:
            dst = os.path.join(run_dir, rel)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            if os.path.exists(dst):
                os.remove(dst)              # a hard link to DATA_ROOT is replaced, never written through
            shutil.copy2(os.path.join(carried, rel), dst)
        return _run_in(client, sc.Scenario(script, network), out_dir, 90, run_dir,
                       Carried(run_dir, registry))


# A saved file ragexe writes in another form on purpose, read the same by both clients: each form is turned
# into one text before the two are compared. The reasons are saved_expected.json's.
def _lua_long_or_quoted(m):
    """A chat tab's name, a [[...]] long string or a quoted string with ragexe's escapes (\\ddd, a byte in
    decimal), as the name itself."""
    if m.group(2) is not None:
        return f'TabName = {m.group(2)!r}'
    raw = re.sub(r'\\(\d{1,3})', lambda e: chr(int(e.group(1))), m.group(3).encode('cp949').decode('latin-1'))
    raw = re.sub(r'\\(.)', lambda e: {'n': '\n', 'r': '\r', 't': '\t'}.get(e.group(1), e.group(1)), raw)
    return 'TabName = ' + repr(raw.encode('latin-1').decode('cp949', 'replace'))


SAVED_FORMS = {
    # chat tab names are quoted Lua strings with escapes in ragexe's file, [[...]] long strings in herc's
    'ChatWndInfo_U.lua': lambda t: re.sub(r'TabName = (\[\[(.*?)\]\]|"((?:[^"\\]|\\.)*)")', _lua_long_or_quoted, t),
    # /roulette is no ON/OFF command in ragexe (the server has roulette off), so its line is not written
    'OptionInfo.lua': lambda t: re.sub(r'CmdOnOffList\["/roulette"\] = \d+\r?\n', '', t),
}


def saved_forms(saved):
    files = {n: SAVED_FORMS[n](t) if n in SAVED_FORMS and not t.startswith('sha256:') else t
             for n, t in saved['files'].items()}
    return {'registry': saved['registry'], 'files': files}


def upgrade(name, then, out_dir, state=None):
    """herc's saved state taken over by ragexe, as a player's install is: the scenario's first session runs on
    herc; what it left (take_state: its folder's new and changed files and the registry, with `state` laid
    over them) starts the later sessions on herc and on ragexe alike, each in a new folder, and the two are
    judged by each other as `pair` judges them. The later sessions are `then`'s (all of them) when given,
    else the scenario's own after the first, or for a scenario of one session that session again. Compared
    besides: the files each later run left in its folder, saved files in the forms SAVED_FORMS makes one."""
    first = load_phases(name)[0]
    later_name = then or name
    skip = 1 if later_name == name and len(load_phases(name)) > 1 else 0

    def later_sessions():
        # a Script is used up as it plays, so each client gets its own
        scripts = load_phases(later_name)
        mod = sys.modules[f'scenario_{later_name}']
        rec = getattr(mod, 'RECORDING', later_name)
        return [(s, recording_path(rec, k)) for k, s in enumerate(scripts)][skip:]

    sessions = {c: later_sessions() for c in ('herc', 'ragexe')}
    answer_quit = getattr(sys.modules[f'scenario_{later_name}'], 'ANSWER_QUIT', False)
    base = run_client('herc', sc.Scenario(first, sc.Replay(recording_path(name, 0))), os.path.join(out_dir, 'herc0'))
    report(base)
    if base.exit_code != 0:
        print(f'{name}: FAILS (herc\'s first session did not exit cleanly)')
        return False
    files, registry = take_state(base, out_dir, state)
    print(f'  carried: {len(files)} files ({", ".join(files[:12])}{", ..." if len(files) > 12 else ""}), '
          f'{len(registry.splitlines())} registry values')
    clients = ('herc', 'ragexe')
    runs = {c: [] for c in clients}

    def chain(client):
        for i, (script, rec) in enumerate(sessions[client]):
            out = os.path.join(out_dir, f'{client}1' if i == 0 else f'{client}{i + 1}')
            if i == 0:
                run = run_carried(client, script, sc.Replay(rec, answer_quit), out, os.path.join(out_dir, 'carried'), files,
                                  registry)
            else:
                run = run_client(client, sc.Scenario(script, sc.Replay(rec, answer_quit)), out, after=runs[client][-1])
            report(run)
            runs[client].append(run)
            with open(os.path.join(out, 'saved.json'), 'w', encoding='utf-8') as f:
                json.dump(run.saved, f, indent=1, ensure_ascii=False)
            if run.exit_code != 0:
                return

    threads = [threading.Thread(target=chain, args=(c,)) for c in clients]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    lines = []
    if len(runs['herc']) != len(runs['ragexe']):
        lines.append(f'{len(runs["herc"])} sessions against {len(runs["ragexe"])}')
    for c in clients:
        if not runs[c] or runs[c][-1].exit_code != 0:
            lines.append(f'{c}: exit {runs[c][-1].exit_code if runs[c] else None}')
    if not runs['herc'] or not runs['ragexe']:
        print(f'{name}: FAILS ({"; ".join(lines)})')
        return False
    summaries = [run_summary(Phases(runs[c])) for c in clients]
    # STATE_FORM (a scenario without a golden, so without improvements/SCENARIO.json) turns each state into
    # the text compared, leaving out what an approved improvement changes
    form = getattr(sys.modules[f'scenario_{later_name}'], 'STATE_FORM', None)
    for s in summaries:
        for st in improvements(later_name):
            s['states'].pop(st, None)
        if form:
            s['states'] = {n: form(n, t) for n, t in s['states'].items()}
        s['captures'], s['counters'] = {}, {}
        s['saved'] = saved_forms(s['saved'])
    lines += diff_summaries(summaries[0], summaries[1])
    for i, (ra, rb) in enumerate(zip(runs['herc'], runs['ragexe'])):
        with open(sessions['herc'][i][1]) as f:
            recorded = json.load(f)['connections']
        lines += [f'session {i + 1}: {l}' for l in diff_streams(send_streams(ra), send_streams(rb), recorded)]
        if replay_notes(ra) != replay_notes(rb):
            lines.append(f'session {i + 1}: replay {replay_notes(ra)[:3]} against {replay_notes(rb)[:3]}')
        if ra.expect_failures != rb.expect_failures:
            lines.append(f'session {i + 1}: expectations {ra.expect_failures[:3]} against {rb.expect_failures[:3]}')
    left = [folder_changes(runs[c][-1].run_dir, os.path.basename(CLIENTS[c]['exe'])) for c in clients]
    outside = [{rel: open(os.path.join(runs[c][-1].run_dir, rel), 'rb').read() for rel in names
                if not rel.lower().startswith('savedata/')} for c, names in zip(clients, left)]
    for rel in sorted(set(outside[0]) | set(outside[1])):
        if outside[0].get(rel) != outside[1].get(rel):
            lines.append(f'left file {rel} differs' if rel in outside[0] and rel in outside[1]
                         else f'left file {rel} only in {"herc" if rel in outside[0] else "ragexe"}\'s folder')
    left_out = [d for r in runs['herc'] for d in replay_notes(r)]
    for line in lines:
        print('  ' + line.replace('the golden run', 'herc').replace('this one', 'ragexe').replace('this run', 'ragexe'))
    print(f'{name}' + (f' then {later_name}' if later_name != name else '') + (f' from {state}' if state else '') + ': '
          + ('ragexe takes over herc\'s saved state as herc does' if not lines else f'DIFFER ({len(lines)} differences)')
          + (f'; the replay itself: {left_out[:2]}' if left_out else ''))
    return not lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=['smoke', 'determinism', 'record', 'replay', 'offline', 'compare', 'bless',
                                     'check', 'expect', 'pair', 'upgrade'])
    ap.add_argument('args', nargs='*')
    ap.add_argument('--client', choices=sorted(CLIENTS))
    ap.add_argument('--clients', default='herc,ragexe', help='pair: the two clients compared')
    ap.add_argument('--out')
    ap.add_argument('--frames', default='30,60,120,240')
    ap.add_argument('--quit', type=int)
    ap.add_argument('--server', default='127.0.0.1', help='record: the server host every connection goes to')
    ap.add_argument('--login-port', type=int, default=16900,
                    help='record: the port the login connection (6900 in clientinfo.xml) goes to')
    ap.add_argument('--why', help='expect: the approved improvement that changes these states')
    ap.add_argument('--then', help='upgrade: the scenario whose sessions follow the first one')
    ap.add_argument('--state', help='upgrade: a folder of saved files (and registry.txt) laid over herc\'s')
    args = ap.parse_args()

    if args.mode == 'expect':
        if len(args.args) < 3 or not args.why:
            ap.error('expect needs SCENARIO RUN_DIR STATE... and --why')
        expect(args.args[0], args.args[1], args.args[2:], args.why)
        sys.exit(0)

    if args.mode == 'compare':
        sys.exit(0 if compare_dirs(*args.args) else 1)
    if args.mode == 'bless':
        ok = all(bless(n, out_dir_for(args, f'bless-{n}')) for n in args.args)
        sys.exit(0 if ok else 1)
    if args.mode == 'check':
        ok = all([check(args.client or 'ragexe', n, out_dir_for(args, f'check-{n}-{args.client}')) for n in args.args])
        sys.exit(0 if ok else 1)
    if args.mode == 'pair':
        clients = args.clients.split(',')
        if len(clients) != 2 or any(c not in CLIENTS for c in clients):
            ap.error('pair needs --clients A,B, two of ' + ', '.join(sorted(CLIENTS)))
        base = out_dir_for(args, 'pair')
        ok = all([pair(n, clients, os.path.join(base, n)) for n in args.args])
        sys.exit(0 if ok else 1)
    if args.mode == 'upgrade':
        base = out_dir_for(args, 'upgrade')
        state = os.path.abspath(args.state) if args.state else None
        ok = all([upgrade(n, args.then, os.path.join(base, n), state) for n in args.args])
        sys.exit(0 if ok else 1)

    if args.mode in ('record', 'replay', 'offline'):
        name = args.args[0]
        if args.mode == 'record':
            os.makedirs(RECORDINGS, exist_ok=True)
            account, bot = scenario_meta(name)
            if account and args.server == '127.0.0.1':
                stack.fresh_for(account, *([stack.BOT_ACCOUNT] if bot else []))
                if bot:
                    stack.start_bot(bot)

            def network(k):
                return sc.Record(recording_path(name, k), server_host=args.server,
                                 port_map={6900: args.login_port})
        elif args.mode == 'replay':
            def network(k):
                return sc.Replay(recording_path(name, k))
        else:
            def network(k):
                return sc.Offline()
        runs = run_phases(args.client, name, network, out_dir_for(args, f'{args.mode}-{name}-{args.client}'),
                          any_slot=args.mode == 'record', live=args.mode == 'record')
        last = runs[-1]
        ok = (last.exit_code == 0 and not last.divergences and not last.expect_failures
              and len(runs) == len(load_phases(name)))
        if args.mode == 'replay' and args.client == 'ragexe':
            saved = saved_divergences(name, last.saved)
            for line in saved:
                print(line)
            ok = ok and not saved
        if args.mode != 'offline':
            print(f'recorded to {recording_path(name, 0)}' + (f' and {len(runs) - 1} more session(s)' if len(runs) > 1 else '')
                  if args.mode == 'record' else ('matches the recording' if ok else 'DIVERGES'))
        sys.exit(0 if ok else 1)

    frames = [int(x) for x in args.frames.split(',')]
    quit_frame = args.quit or max(frames)
    steps = [(f, 'capture', f'frame_{f:05d}') for f in frames] + [(quit_frame, 'quit', None)]
    base = out_dir_for(args, f'{args.mode}-{args.client}')
    if args.mode == 'smoke':
        run = run_client(args.client, sc.Scenario(sc.Script(steps), sc.Offline()), base)
        report(run)
        sys.exit(0 if run.exit_code == 0 and len(run.captures) == len(frames) else 1)
    runs = []
    for i in (1, 2):
        run = run_client(args.client, sc.Scenario(sc.Script(steps), sc.Offline()), os.path.join(base, f'run{i}'))
        report(run)
        runs.append(run)
    if any(r.exit_code != 0 or len(r.captures) != len(frames) for r in runs):
        print('NOT deterministic: a run did not finish with every capture')
        sys.exit(1)
    same = compare_dirs(runs[0].out_dir, runs[1].out_dir)
    print('deterministic' if same else 'NOT deterministic')
    sys.exit(0 if same else 1)


if __name__ == '__main__':
    main()
