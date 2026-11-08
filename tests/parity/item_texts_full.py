"""Every item's description on herc and ragexe, compared: the item_texts scenario's way (the probe's
PARITY_CALL_ITEM_SHOW, one item a frame) over the whole item table the harness's data set loads.

    python -I item_texts_full.py [--out DIR] [--ids 501,502,...] [--item-db FILE] [--clients herc,ragexe]

The ids are the Lua item info's (DATA_ROOT\\System\\LuaFiles514\\itemInfo.lua and itemInfo_adv.lua) and the
server item_db's; each item is identified, unrefined, with no cards, of the type the server sends for it
(item_db's Type as clif.c's itemtype() converts it; 3, etc, for an item the server lacks). --item-db names
the server's item_db text (default: read through WSL from the parity stack's Hercules, pre-re and
item_db2). Both clients replay the item_texts recording (its login), the items following on; what the client
sends past the recording is not compared.

Writes DIR\\<client>\\ (the runs) and DIR\\items.txt: a line per item, its id and its hash on each client,
"same" or "DIFFERS", then the full text of each item that differs. Exit status 1 when any item differs or a
client did not finish.
"""
import argparse
import hashlib
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import inputs  # noqa: E402
import parity  # noqa: E402
import scenario as sc  # noqa: E402

PER_STATE = 200
ITEM_DB = ('~/ragexe/Hercules/db/pre-re/item_db.conf', '~/ragexe/Hercules/db/item_db2.conf')
TYPES = {'IT_HEALING': 0, 'IT_UNKNOWN': 1, 'IT_USABLE': 2, 'IT_ETC': 3, 'IT_WEAPON': 4, 'IT_ARMOR': 5,
         'IT_CARD': 6, 'IT_PETEGG': 7, 'IT_PETARMOR': 8, 'IT_UNKNOWN2': 9, 'IT_AMMO': 10, 'IT_DELAYCONSUME': 11,
         'IT_SELECTPACKAGE': 12, 'IT_CASH': 18}
# clif.c itemtype() for PACKETVER >= 20080827: a weapon goes as armour, armour, pet armour and eggs as weapons
CLIENT_TYPE = {4: 5, 5: 4, 7: 4, 8: 4}


def client_ids():
    ids = set()
    for name in ('itemInfo.lua', 'itemInfo_adv.lua'):
        path = os.path.join(parity.DATA_ROOT, 'System', 'LuaFiles514', name)
        if os.path.exists(path):
            text = open(path, encoding='latin-1').read()
            ids |= {int(m) for m in re.findall(r'^\s*\[(\d+)\]\s*=\s*\{', text, re.M)}
    return ids


def server_types(path):
    if path:
        text = open(path, encoding='latin-1').read()
    else:
        text = subprocess.run(['wsl.exe', '-d', 'Debian', '--cd', '/tmp', '--', 'bash', '-c',
                               'cat ' + ' '.join(ITEM_DB)], capture_output=True).stdout.decode('latin-1')
    types = {}
    for block in re.split(r'\n\s*\{\s*\n', text):
        m = re.search(r'^\s*Id:\s*(\d+)', block, re.M)
        if m:
            t = re.search(r'^\s*Type:\s*"(\w+)"', block, re.M)
            raw = TYPES.get(t.group(1), 3) if t else 3
            types[int(m.group(1))] = CLIENT_TYPE.get(raw, raw)
    return types


def steps(items):
    """item_texts' login, then each item a frame, a state every PER_STATE items."""
    import common
    s = common.scenario('parity59')
    s.wait(30)
    for n, (item_id, item_type) in enumerate(items):
        s.item(inputs.item_words(item_id, item_type))
        if n % PER_STATE == PER_STATE - 1 or n == len(items) - 1:
            s.state(f'full_{n // PER_STATE:03d}', 1)
    s.quit()
    return s.steps


def records(out_dir, count):
    """Each item's record text, in order: its line and its windows (the "call" line left out)."""
    found = []
    for k in range((count + PER_STATE - 1) // PER_STATE):
        path = os.path.join(out_dir, f'full_{k:03d}.state.txt')
        if not os.path.exists(path):
            break
        text = parity.state_text(path)
        cur = None
        for line in text.splitlines():
            if line.startswith('item id='):
                cur = [line]
                found.append(cur)
            elif line.startswith('call '):
                cur = None
            elif cur is not None:
                cur.append(line)
    return ['\n'.join(r) for r in found]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', default=os.path.join(parity.RUN_ROOT, 'item_texts_full'))
    ap.add_argument('--ids')
    ap.add_argument('--item-db')
    ap.add_argument('--clients', default='herc,ragexe')
    args = ap.parse_args()
    types = server_types(args.item_db)
    ids = [int(x, 0) for x in args.ids.split(',')] if args.ids else sorted(client_ids() | set(types))
    items = [(i, types.get(i, 3)) for i in ids]
    if parity.SCENARIOS not in sys.path:
        sys.path.insert(0, parity.SCENARIOS)
    recording = parity.recording_path('item_texts', 0)
    clients = args.clients.split(',')
    texts, ok = {}, True
    for client in clients:
        out = os.path.join(args.out, client)
        run = parity.run_client(client, sc.Scenario(sc.Script(steps(items)), sc.Replay(recording)), out,
                                timeout=300)
        parity.report(run)
        ok = ok and run.exit_code == 0
        texts[client] = records(out, len(items))
    a, b = (texts[c] for c in clients)
    differ = 0
    with open(os.path.join(args.out, 'items.txt'), 'w', encoding='utf-8', errors='replace') as f:
        full = []
        for n, (item_id, _) in enumerate(items):
            ta, tb = (t[n] if n < len(t) else None for t in (a, b))
            ha, hb = (hashlib.sha256(t.encode('utf-8', 'replace')).hexdigest()[:12] if t is not None else '-'
                      for t in (ta, tb))
            same = ta is not None and ta == tb
            differ += not same
            f.write(f'{item_id} {ha} {hb} {"same" if same else "DIFFERS"}\n')
            if not same:
                full.append(f'== {item_id}\n-- {clients[0]}\n{ta}\n-- {clients[1]}\n{tb}\n')
        f.write(''.join(full))
    print(f'{len(items)} items, {len(items) - differ} the same, {differ} differ: {os.path.join(args.out, "items.txt")}')
    sys.exit(0 if ok and not differ else 1)


if __name__ == '__main__':
    main()
