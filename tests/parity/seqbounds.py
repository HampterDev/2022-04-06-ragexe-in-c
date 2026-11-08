"""Server text at its limits, for the differential tester: seqtarget.py's scripts (and seqgen.py's units)
played to herc and to ragexe with every text field of every packet they send rewritten to one boundary
case, the two clients' states and sends compared as seqfuzz.py compares them.

    python -I seqbounds.py run OUT [--scripts A,B] [--variants A,B] [--keep]
                                        each script under each variant, one case each
    python -I seqbounds.py case OUT SCRIPT VARIANT [--only LAYOUT.FIELD,...]
                                        one case, its runs kept; --only rewrites just those fields
    python -I seqbounds.py fields       the text fields covered, by layout, with the scripts that send them
    python -I seqbounds.py summary OUT  the cases of a run that differ, by the kinds of line that differ

A text field is a fixed char array of a layout (a name, a title, a map name: the 's' fields but the
binary ones in BINARY), a variable packet's text tail (TEXT_TAILS) or each name of a tail of names
(NAME_TAILS, rewritten as fixed fields). Every one a script sends is
rewritten (a field the script leaves out too), from its own value, so that two fields that held the same
text still hold the same text:

  fixed field of N bytes                      text tail (the server's longest: TAIL_MAX)
  full      N bytes, no NUL                   the text without its NUL
  short     N - 1 bytes and the NUL           the text with its NUL (unchanged)
  empty     N NULs                            no bytes at all
  lead      N - 1 bytes, a CP949 lead byte    the text, then a lead byte, no NUL
  lead_nul  N - 2 bytes, a lead byte, NUL     the text, a lead byte, NUL
  caret     N - 2 bytes, '^', NUL             the text, '^', NUL
  color     N - 5 bytes, '^FF0' (cut), NUL    the text, '^FF0', NUL
  max       as full                           the server's longest text, NUL-terminated where it sends one
  over      as full                           one byte past the longest, or LENGTH_WORD_MAX's reach without
                                              a NUL where the server bounds it no further

The scripts' packets are Hercules' (seqgen.LAYOUTS, checked by tests/ragexe/seqgen), so every case is
something the server's framing allows; a field the server itself never fills to its end (a map name it
copies from its own table, say) is still sent full, since a modified or other server could. Which of
those a finding needs is said in the report, not here.

Run from tests/parity with PARITY_RUNS set to your own run root (as seqfuzz.py). A case runs
herc and ragexe at once (two client slots).
"""
import argparse
import contextlib
import json
import os
import re
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import parity  # noqa: E402
import seqfuzz  # noqa: E402
import seqgen  # noqa: E402
import seqtarget  # noqa: E402

VARIANTS = ['full', 'short', 'empty', 'lead', 'lead_nul', 'caret', 'color', 'max', 'over']
# the variants of a fixed field: max and over are full there
FIXED_VARIANTS = VARIANTS[:7]
LEAD = 0xb0           # a CP949 lead byte ('가' is b0 a1)
FILL = b'Abcdefghij0123456789klmnopqrstuvwxyz'
# 's' fields that are not text
BINARY = {'PosDir', 'MoveData', 'ratios', 'unused', 'rest', 'objectives', 'captchaKey'}
# Variable packets' tails that are text (the others are arrays of records or binary)
TEXT_TAILS = {'title', 'chatMsg', 'MessageString', 'messageString', 'message', 'Message', 'position'}
# Tails that are a run of fixed-size names (the whisper ignore list, a clan's allies and antagonists):
# each name is rewritten as a fixed field
NAME_TAILS = {'names': 24}
# The longest tail Hercules sends (src/map/clif.c), as (bytes, ends in a NUL); a tail not listed is as
# long as LENGTH_WORD_MAX allows.
TAIL_MAX = {
    0x00d7: (36, False),      # clif_dispchat: strlen of a CHATROOM_TITLE_SIZE (37) title, no NUL
    0x00df: (36, False),      # clif_changechatstatus: the same
    0x0109: (257, True),      # clif_party_message: CHAT_SIZE_MAX (256) characters and the NUL
    0x0adb: (512, True),      # clif_serviceMessageColor: at most 512 with the NUL
    0x08b3: (248, True),      # clif_ShowScript: a 256-byte buffer less the 8-byte head
    0x0afd: (24, False),      # clif_guild_set_position: a NAME_LENGTH name, memcpy'd whole
    0x008d: (248, True),      # clif_GlobalMessage: a 256-byte buffer less the 8-byte head
    0x008e: (256, True),      # clif_displaymessage: 255 characters and the NUL
    0x017f: (252, True),      # clif_guild_message: a 256-byte buffer less the 4-byte head
    0x098e: (228, True),      # clif_clan_message: CHAT_SIZE_MAX less the head and a name
    0x0be3: (256, True),      # clif_whisper_replay: a CHAT_SIZE_MAX message
}
# Not listed, so as long as the length word allows: 0x02c1 (clif_messagecolor_self, a script's coloured
# line), 0x009a and 0x01c3 (broadcasts), 0x00b4 and 0x00b7 (NPC dialog and menu), 0x09de (whispers),
# 0x02c2 and 0x0a6f (formatted messages), 0x02dc (battleground chat), 0x00ae (clif_colorchat_self).
# Hercules asserts a variable packet's length fits an int16 (clif_dispchat, clif_bg_message, ...)
LENGTH_WORD_MAX = 0x7fff


def _fill(seed, n):
    """`n` bytes starting with `seed` (its NUL and what follows dropped), padded with FILL."""
    seed = seed.split(b'\0', 1)[0][:n]
    out = bytearray(seed)
    k = 0
    while len(out) < n:
        out.append(FILL[k % len(FILL)])
        k += 1
    return bytes(out)


def fixed_value(variant, value, n):
    if variant in ('full', 'max', 'over'):
        return _fill(value, n)
    if variant == 'short':
        return _fill(value, n - 1) + b'\0'
    if variant == 'empty':
        return b'\0' * n
    if variant == 'lead':
        return _fill(value, n - 1) + bytes([LEAD])
    if variant == 'lead_nul':
        return _fill(value, n - 2) + bytes([LEAD]) + b'\0'
    if variant == 'caret':
        return _fill(value, n - 2) + b'^\0'
    if variant == 'color':
        return _fill(value, n - 5) + b'^FF0\0'
    raise KeyError(variant)


def tail_value(variant, value, pid, fixed):
    text = value.split(b'\0', 1)[0]
    bound = TAIL_MAX.get(pid)
    if variant == 'full':
        return text
    if variant == 'short':
        return value
    if variant == 'empty':
        return b''
    if variant == 'lead':
        return text + bytes([LEAD])
    if variant == 'lead_nul':
        return text + bytes([LEAD]) + b'\0'
    if variant == 'caret':
        return text + b'^\0'
    if variant == 'color':
        return text + b'^FF0\0'
    if variant == 'max':
        if bound is None:
            return _fill(text, LENGTH_WORD_MAX - fixed - 1) + b'\0'
        n, nul = bound
        return _fill(text, n - 1) + b'\0' if nul else _fill(text, n)
    if variant == 'over':
        if bound is None:
            return _fill(text, LENGTH_WORD_MAX - fixed)
        n, nul = bound
        return _fill(text, n) + b'\0' if nul else _fill(text, n + 1)
    raise KeyError(variant)


def text_fields(layout):
    """[(field, size)] of a layout's (or a sublayout's) field list: size is None for a text tail and
    minus a name's size for a tail of names."""
    out = []
    for field, fmt in layout:
        if fmt == seqgen.TAIL:
            if field in TEXT_TAILS:
                out.append((field, None))
            elif field in NAME_TAILS:
                out.append((field, -NAME_TAILS[field]))
        elif fmt.endswith('s') and field not in BINARY:
            out.append((field, int(fmt[:-1])))
    return out


class Rewrite:
    """seqgen.pack and pack_sub with every text field rewritten to `variant` (only the `only` fields,
    as 'LAYOUT.FIELD', when given). `seen` collects the fields it rewrote."""

    def __init__(self, variant, only=None):
        self.variant = variant
        self.only = set(only) if only else None
        self.seen = set()
        self._pack, self._pack_sub = seqgen.pack, seqgen.pack_sub

    def _want(self, name, field):
        return self.only is None or f'{name}.{field}' in self.only

    def _bytes(self, v):
        return v.encode('cp949') if isinstance(v, str) else (v or b'')

    def pack(self, _name, **fields):
        pid, layout = seqgen.LAYOUTS[_name]
        fixed = struct.calcsize(seqgen.layout_format(_name))
        for field, n in text_fields(layout):
            if not self._want(_name, field):
                continue
            v = self._bytes(fields.get(field))
            if n is None:
                v = tail_value(self.variant, v, pid, fixed)
            elif n < 0:
                v = b''.join(fixed_value(self.variant, v[k:k - n], -n) for k in range(0, len(v) // -n * -n, -n))
            else:
                v = fixed_value(self.variant, v, n)
            fields[field] = v
            self.seen.add(f'{_name}.{field}')
        return self._pack(_name, **fields)

    def pack_sub(self, _name, **fields):
        for field, n in text_fields(seqgen.SUBLAYOUTS[_name]):
            if not self._want(_name, field):
                continue
            fields[field] = fixed_value(self.variant, self._bytes(fields.get(field)), n)
            self.seen.add(f'{_name}.{field}')
        return self._pack_sub(_name, **fields)

    @contextlib.contextmanager
    def active(self):
        mods = (seqgen, seqtarget)
        saved = [(m, m.pack, m.pack_sub) for m in mods]
        for m in mods:
            m.pack, m.pack_sub = self.pack, self.pack_sub
        try:
            yield self
        finally:
            for m, p, ps in saved:
                m.pack, m.pack_sub = p, ps


def build(script, variant, prefix, only=None):
    """(the case's recording, the fields rewritten)."""
    rw = Rewrite(variant, only)
    with rw.active():
        events = seqtarget.events([script], prefix)
        rec = seqfuzz.case_for(list(seqtarget.SCRIPTS).index(script), prefix, events)
    return rec, sorted(rw.seen)


def run(out, scripts, variants, keep):
    prefix = seqfuzz.load_prefix()
    os.makedirs(out, exist_ok=True)
    log = open(os.path.join(out, 'bounds.jsonl'), 'a', encoding='utf-8')
    n_diff = 0
    t0 = time.monotonic()
    for script in scripts:
        made = {}
        for variant in variants:
            rec, seen = build(script, variant, prefix)
            if not seen:
                continue
            # a variant that sends what an earlier one sent (max and over without a text tail are full)
            key = json.dumps(rec['connections'][-1]['events'], sort_keys=True)
            if key in made:
                print(f'{script}_{variant}: as {script}_{made[key]}', flush=True)
                continue
            made[key] = variant
            start = time.monotonic()
            name = f'{script}_{variant}'
            res = seqfuzz.compare_case(rec, prefix, os.path.join(out, name), keep)
            took = time.monotonic() - start
            line = {'script': script, 'variant': variant, 'fields': seen, 'seconds': round(took, 1)}
            line.update(res.to_json())
            log.write(json.dumps(line) + '\n')
            log.flush()
            print(f'{name}: {"DIFFERS" if res.differs else "same"} ({res.n_states} states, {len(seen)} fields, '
                  f'{took:.0f} s)', flush=True)
            if res.differs:
                n_diff += 1
                print('  ' + json.dumps(res.to_json())[:1500], flush=True)
                with open(os.path.join(out, f'{name}.json'), 'w') as f:
                    json.dump({'case': rec, 'result': res.to_json()}, f)
    print(f'done in {time.monotonic() - t0:.0f} s; {n_diff} differ')
    return n_diff


def fields():
    prefix = seqfuzz.load_prefix()
    by_field = {}
    for script in seqtarget.SCRIPTS:
        _, seen = build(script, 'full', prefix)
        for f in seen:
            by_field.setdefault(f, []).append(script)
    every = set()
    for name, (_, layout) in seqgen.LAYOUTS.items():
        every |= {f'{name}.{f}' for f, _ in text_fields(layout)}
    for name, layout in seqgen.SUBLAYOUTS.items():
        every |= {f'{name}.{f}' for f, _ in text_fields(layout)}
    for f in sorted(every):
        print(f'{f:60} {", ".join(by_field.get(f, [])) or "(no script sends it)"}')
    print(f'{len(by_field)} of {len(every)} text fields sent')


def _line_key(line):
    """A differing state line without what moves with its content: its window's id and rectangle."""
    return re.sub(r' (?:id|rect)=\S+', '', line)


SUMMARY_LINES = 12


def summary(out):
    """Each differing case of a run's bounds.jsonl: its failures, its first send difference and the kinds
    of line that differ, with one example of each."""
    with open(os.path.join(out, 'bounds.jsonl'), encoding='utf-8') as f:
        lines = [json.loads(l) for l in f if l.strip()]
    same = 0
    for c in lines:
        if not (c['diffs'] or c['sends'] or c['failures'] or c['streams']):
            same += 1
            continue
        print(f"{c['script']}_{c['variant']}: {len(c['diffs'])} states differ")
        for client, f in c['failures'].items():
            print(f'  {client}: {f}')
        if c['sends']:
            print(f"  sends: {json.dumps(c['sends'])[:400]}")
        kinds = {}
        for state, diff in c['diffs'].items():
            for l in diff:
                kinds.setdefault(_line_key(l), (state, l))
        for k, (state, l) in list(kinds.items())[:SUMMARY_LINES]:
            print(f'  {state} {l[:300]}')
        if len(kinds) > SUMMARY_LINES:
            print(f'  ... {len(kinds) - SUMMARY_LINES} more')
    print(f'{same} of {len(lines)} cases the same')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=['run', 'case', 'fields', 'summary'])
    ap.add_argument('out', nargs='?')
    ap.add_argument('script', nargs='?')
    ap.add_argument('variant', nargs='?')
    ap.add_argument('--scripts')
    ap.add_argument('--variants')
    ap.add_argument('--only')
    ap.add_argument('--keep', action='store_true')
    ap.add_argument('--clients', default='herc,ragexe')
    args = ap.parse_args()
    seqfuzz.PAIR[:] = args.clients.split(',')
    if args.mode == 'summary':
        summary(args.out)
        return
    if args.mode == 'fields':
        fields()
        return
    if args.mode == 'case':
        prefix = seqfuzz.load_prefix()
        rec, seen = build(args.script, args.variant, prefix, args.only.split(',') if args.only else None)
        print('fields: ' + ', '.join(seen))
        d = os.path.join(args.out, f'{args.script}_{args.variant}')
        res = seqfuzz.compare_case(rec, prefix, d, keep=True)
        with open(os.path.join(d, 'case.json'), 'w') as f:
            json.dump(rec, f)
        print(json.dumps(res.to_json(), indent=1))
        sys.exit(1 if res.differs else 0)
    scripts = args.scripts.split(',') if args.scripts else list(seqtarget.SCRIPTS)
    variants = args.variants.split(',') if args.variants else VARIANTS
    sys.exit(1 if run(args.out, scripts, variants, args.keep) else 0)


if __name__ == '__main__':
    main()
