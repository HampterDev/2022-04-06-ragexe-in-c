"""Compares the packet dispatcher of the original with this tree's and pairs their handlers.

    python -I tools/funcmap/dispatch_check.py [--exe PATH] [--dump PATH] [--write]

CGameMode::PollNetworkStatus at 0x7a0570 dispatches a received packet by its id through a jump
table of 2881 entries (ids 0x73 to 0xbb3) at 0x7aab40; this tree does the same in
src/game/packet_dispatch.c and packet_dispatch2.c. The script reads the table from the
executable, lists the ids one side handles and the other does not, and, for every arm of the
form `push packet; mov ecx, mode; call handler; jmp loop` whose case here calls exactly one
function, pairs that handler's address with the function's name. With --write the pairs go to
dispatch_pairs.tsv, which verify_map.py folds into function_map.tsv with method `dispatch`."""
import argparse
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..'))
DEFAULT_EXE = os.path.normpath(os.path.join(REPO, '..', 'client-files', '2022-04-06_Ragexe_1648707856.exe'))
DEFAULT_DUMP = os.path.normpath(os.path.join(REPO, '..', 'client-disassembly',
                                             '2022-04-06_Ragexe_1648707856.exe.bndb_disassembly.txt'))
PAIRS = os.path.join(HERE, 'dispatch_pairs.tsv')
IMAGE_BASE = 0x400000
TABLE, FIRST_ID, ENTRIES = 0x7aab40, 0x73, 0xb41
PACKET_BUFFER = 0x1121a10
LOOP = 0x7a0818


def read_table(exe_path):
    data = open(exe_path, 'rb').read()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    nsec = struct.unpack_from('<H', data, pe + 6)[0]
    opt = struct.unpack_from('<H', data, pe + 20)[0]
    for i in range(nsec):
        s = pe + 24 + opt + i * 40
        vsize, va, rsize, raw = struct.unpack_from('<IIII', data, s + 8)
        rva = TABLE - IMAGE_BASE
        if va <= rva < va + max(vsize, rsize):
            off = raw + rva - va
            return struct.unpack_from('<%dI' % ENTRIES, data, off)
    raise SystemExit('the jump table is not in a section of %s' % exe_path)


def read_lift():
    ids = {m.group(1): int(m.group(2), 0) for m in
           re.finditer(r'(HEADER_\w+)\s*=\s*(0x[0-9a-fA-F]+|\d+)', open(os.path.join(REPO, 'src', 'game', 'packet_ids.h'), encoding='utf-8').read())}
    handlers = {}
    for name in ('packet_dispatch.c', 'packet_dispatch2.c'):
        src = open(os.path.join(REPO, 'src', 'game', name), encoding='utf-8').read()
        parts = re.split(r'\n\s*case (HEADER_\w+):', src)
        pending = []
        for i in range(1, len(parts), 2):
            pending.append(parts[i])
            body = parts[i + 1].split('break;')[0]
            calls = [c for c in re.findall(r'\b([a-z_][a-z0-9_]*)\(', body)
                     if c not in ('sizeof', 'if', 'return', 'switch', 'while', 'for')]
            if calls or 'return' in body or 'break;' in parts[i + 1]:
                for label in pending:
                    handlers[ids[label]] = calls
                pending = []
    return handlers, {v: k for k, v in ids.items()}


def read_arms(dump_path, targets):
    wanted = set(targets)
    arms = {}
    current = None
    with open(dump_path, encoding='utf-8', errors='replace') as f:
        for line in f:
            m = re.match(r'^([0-9a-f]{8})  ', line)
            if not m:
                continue
            addr = int(m.group(1), 16)
            if addr in wanted and addr not in arms:
                current = addr
                arms[addr] = []
            if current is not None and line.strip():
                arms[current].append(line.rstrip('\n'))
                if len(arms[current]) >= 4:
                    current = None
    simple = {}
    for addr, seq in arms.items():
        if len(seq) == 4 and ('push    0x%x' % PACKET_BUFFER) in seq[0] and 'mov     ecx, edi' in seq[1] \
                and re.search(r'call    sub_[0-9a-f]+', seq[2]) and ('jmp     0x%x' % LOOP) in seq[3]:
            simple[addr] = int(re.search(r'call    sub_([0-9a-f]+)', seq[2]).group(1), 16)
    return simple


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', default=DEFAULT_EXE)
    ap.add_argument('--dump', default=DEFAULT_DUMP)
    ap.add_argument('--write', action='store_true', help='write dispatch_pairs.tsv (else report only)')
    args = ap.parse_args()

    table = read_table(args.exe)
    default = max(set(table), key=table.count)
    original = {FIRST_ID + i: t for i, t in enumerate(table) if t != default}
    handlers, names = read_lift()

    only_original = sorted(set(original) - set(handlers))
    only_lift = sorted(set(handlers) - set(original))
    print('%d ids dispatched by the original, %d by this tree' % (len(original), len(handlers)))
    print('dispatched by the original only: %s' % (' '.join('0x%x' % i for i in only_original) or 'none'))
    print('dispatched by this tree only: %s' % (' '.join('0x%x' % i for i in only_lift) or 'none'))

    simple = read_arms(args.dump, original.values())
    pairs = {}
    for pid in sorted(original):
        handler = simple.get(original[pid])
        calls = handlers.get(pid, [])
        if handler is None or len(set(calls)) != 1:
            continue
        pairs.setdefault((handler, calls[0]), []).append(pid)
    by_addr = {}
    for (addr, name), pids in pairs.items():
        by_addr.setdefault(addr, set()).add(name)
    clashes = {a: n for a, n in by_addr.items() if len(n) > 1}
    print('%d arms call one handler with the packet; %d handler/function pairs, %d addresses with two names'
          % (len(simple), len(pairs), len(clashes)))
    for a, n in sorted(clashes.items()):
        print('  0x%x: %s' % (a, ', '.join(sorted(n))))
    if args.write:
        with open(PAIRS, 'w', encoding='utf-8', newline='\n') as f:
            f.write('addr\tfinal_name\tnote\n')
            for (addr, name), pids in sorted(pairs.items()):
                if addr in clashes:
                    continue
                f.write('%x\t%s\tpacket dispatcher arm for %s\n' % (addr, name, ', '.join('0x%x %s' % (p, names.get(p, '')) for p in pids)))
        print('%d pairs written to %s' % (len(pairs) - sum(1 for k in pairs if k[0] in clashes), PAIRS))


if __name__ == '__main__':
    sys.exit(main())
