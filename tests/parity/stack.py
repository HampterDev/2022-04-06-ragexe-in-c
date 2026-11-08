"""The parity server stack (server/paritysrv_test.go in WSL): started, stopped, and restarted
so that a recording starts from the seeded database.

Every account a recording logs in with is noted in USED (beside the stack's log). A recording
with an account the running stack has already seen restarts the stack first: the character
would otherwise start where the last recording left it, holding what it was given then.
"""
import json
import os
import socket
import subprocess
import time

HERE = os.path.dirname(os.path.abspath(__file__))
STATE_DIR = os.environ.get('PARITY_RUNS', r'C:\parity\runs')
USED = os.path.join(STATE_DIR, 'stack_accounts.json')
LOG = os.path.join(STATE_DIR, 'stack.log')
START = '/mnt/' + HERE[0].lower() + HERE[2:].replace('\\', '/') + '/server/paritysrv_start.sh'
LOGIN_PORT = 16900


def _port_open(port=LOGIN_PORT):
    try:
        with socket.create_connection(('127.0.0.1', port), timeout=1):
            return True
    except OSError:
        return False


def _wsl(cmd):
    return subprocess.run(['wsl.exe', '-d', 'Debian', '--cd', '/tmp', '--', 'bash', '-c', cmd],
                          capture_output=True)


def stop(timeout=90):
    _wsl('touch /tmp/parity_stop')
    deadline = time.monotonic() + timeout
    while _port_open() and time.monotonic() < deadline:
        time.sleep(1)
    if _port_open():
        raise RuntimeError('the parity server stack did not stop')
    time.sleep(2)


def start(minutes=180, timeout=240):
    os.makedirs(STATE_DIR, exist_ok=True)
    log = open(LOG, 'w')
    # Detached from this process's console and handles, so the shell that ran the runner does
    # not wait for the stack, which outlives the runner.
    subprocess.Popen(['wsl.exe', '-d', 'Debian', '--cd', '/tmp', '--', 'bash', '-ic', f'bash {START} {minutes}'],
                     stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, close_fds=True,
                     creationflags=subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP)
    # The detached process's output does not always reach the log, so readiness is the three
    # servers listening, then a few seconds for the accounts to be seeded after them.
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        time.sleep(1)
        with open(LOG, errors='replace') as f:
            text = f.read()
        if '--- FAIL' in text or 'in use' in text:
            raise RuntimeError('the parity server stack did not start:\n' + text[-2000:])
        if 'PARITY READY' in text or all(_port_open(p) for p in (LOGIN_PORT, 16121, 16122)):
            time.sleep(0 if 'PARITY READY' in text else 8)
            _save([])
            return
    raise RuntimeError('the parity server stack did not start listening')


def _load():
    try:
        with open(USED) as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


def _save(accounts):
    with open(USED, 'w') as f:
        json.dump(accounts, f)


BOT_ACCOUNT = 'paritybot01'


def fresh_for(*accounts):
    """Makes sure the stack is up with every account in `accounts` as the seed made it, then
    notes them as used. A stack whose used accounts are unknown (started by hand) counts as used."""
    used = _load()
    if not _port_open():
        start()
    elif used is None or any(a in used for a in accounts) or BOT_ACCOUNT in used:
        # A bot that has played may still be in town (a vendor's stall stays for minutes), so a
        # recording after one starts from a fresh stack as well.
        stale = [a for a in accounts if used is None or a in used] or [BOT_ACCOUNT]
        print(f'restarting the server stack: {", ".join(stale)} not fresh')
        stop()
        start()
    used = _load() or []
    _save(used + list(accounts))


def start_bot(behaviour):
    """Has the parity server log ParityBot in and play `behaviour` (paritysrv_test.go's runBot)."""
    _wsl(f'printf %s {behaviour} > /tmp/parity_bot')
