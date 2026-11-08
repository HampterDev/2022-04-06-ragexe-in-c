"""The login and character servers' text at its limits: a scenario's recording with the names its login and
character servers sent rewritten to one of seqbounds.py's boundary cases, played to two clients as
`parity.py pair` plays a recording (their send streams, states, windows and saved files judged by each
other). The map server's text is seqbounds.py's: these packets come before the map, in the part every
generated case keeps as recorded.

    python -I recbounds.py pair VARIANT SCENARIO... [--clients herc,ragexe] [--out DIR]
    python -I recbounds.py fields SCENARIO...     the fields each scenario's recording holds

The fields, by packet (Hercules at PACKETVER 20220406, the offsets as the recordings hold them):
    0x0ac4 AC_ACCEPT_LOGIN        each server's name (20 bytes) in the server list
    0x006b HC_ACCEPT_ENTER        each character's name (24) and map (16) in the character list
    0x099d HC_ACK_CHARINFO_PER_PAGE   the same, a page of the list
    0x0b6f HC_ACCEPT_MAKECHAR     the new character's name and map
VARIANT is one of seqbounds.FIXED_VARIANTS. The rewritten recording is written under the run's folder and
read in place of the scenario's own.

Run from tests/parity with PARITY_RUNS set to your own run root.
"""
import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import parity  # noqa: E402
import scenario as sc  # noqa: E402
import seqbounds  # noqa: E402

CHAR_INFO = 175                    # CHARACTER_INFO at this PACKETVER (char_mmo_char_tobuf)
CHAR_NAME, CHAR_MAP = 108, 142     # its name and map name


def _char_list(head):
    def fields(n):
        return [(head + k * CHAR_INFO + off, size) for k in range((n - head) // CHAR_INFO)
                for off, size in ((CHAR_NAME, 24), (CHAR_MAP, 16))]
    return fields


# packet id -> fields of a packet of length n: [(offset, size)]
FIELDS = {
    0x0ac4: lambda n: [(64 + k * 160 + 6, 20) for k in range((n - 64) // 160)],
    0x006b: _char_list(27),
    0x099d: _char_list(4),
    0x0b6f: lambda n: [(2 + CHAR_NAME, 24), (2 + CHAR_MAP, 16)],
}


def rewrite(rec, variant, seen):
    """The recording with every FIELDS field of the server's packets made `variant`; `seen` collects
    (id, offset, size). A packet split across two recorded chunks is left as it is."""
    sizes = sc.packet_sizes()
    out = json.loads(json.dumps(rec))
    for conn in out['connections']:
        if conn.get('port') not in (6900, 16121) and 'refused' not in conn:
            continue
        for ev in conn.get('events', []):
            if 'data' not in ev or 'c' in ev:
                continue
            data = bytearray.fromhex(ev['data'])
            i = 0
            while i + 4 <= len(data):
                pid = data[i] | data[i + 1] << 8
                n = sizes.get(pid)
                if n == -1:
                    n = data[i + 2] | data[i + 3] << 8
                if not n or i + n > len(data):
                    break
                for off, size in FIELDS.get(pid, lambda n: [])(n):
                    old = bytes(data[i + off:i + off + size])
                    data[i + off:i + off + size] = seqbounds.fixed_value(variant, old, size)
                    seen.append((pid, off, size))
                i += n
            ev['data'] = data.hex()
    return out


def recordings_for(name, variant, base, seen):
    """Rewritten copies of the scenario's recordings, by session, under `base`."""
    paths = {}
    k = 0
    while os.path.exists(parity.recording_path(name, k)):
        with open(parity.recording_path(name, k)) as f:
            rec = json.load(f)
        path = os.path.join(base, 'rec', os.path.basename(parity.recording_path(name, k)))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w') as f:
            json.dump(rewrite(rec, variant, seen), f)
        paths[k] = path
        k += 1
    return paths


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=['pair', 'fields'])
    ap.add_argument('args', nargs='+')
    ap.add_argument('--clients', default='herc,ragexe')
    ap.add_argument('--out')
    args = ap.parse_args()
    if args.mode == 'fields':
        for name in args.args:
            seen = []
            with open(parity.recording_path(name, 0)) as f:
                rewrite(json.load(f), 'full', seen)
            print(f'{name}: ' + ', '.join(f'0x{p:04x}+{o}:{s}' for p, o, s in seen))
        return
    variant, names = args.args[0], args.args[1:]
    clients = args.clients.split(',')
    base = args.out or os.path.join(parity.RUN_ROOT, f'rec-{variant}')
    own = parity.recording_path
    ok = True
    for name in names:
        seen = []
        paths = recordings_for(name, variant, os.path.join(base, name), seen)
        print(f'{name}: {len(seen)} fields as {variant}', flush=True)
        parity.recording_path = lambda n, k, _p=paths: _p[k] if n == name else own(n, k)
        try:
            ok &= parity.pair(name, clients, os.path.join(base, name))
        finally:
            parity.recording_path = own
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
