"""Edit fields typed to their limits and past them: a scenario played with some of its typed lines made
longer, on two clients against the scenario's own recording, the clients judged by each other as
`parity.py pair` judges them (their send streams, states, windows and saved files).

    python -I editbounds.py list SCENARIO...
        each type() / text() / say() call of the scenarios, numbered, with the text it types
    python -I editbounds.py pair VARIANT SCENARIO[:N,N...]... [--clients herc,ragexe]
        the scenarios with their calls N (FIELDS' when not given) typed as VARIANT

A call made longer types its own text first, so the edit holds what the scenario meant it to hold
and the rest runs past it:

    long    ASCII to LONG characters (digits for a call that types digits)
    kr      one ASCII character, then two-byte CP949 characters to LONG bytes: the edit's limit falls
            inside a character when the limit is even
    kr0     two-byte characters only: the limit falls inside a character when it is odd

The server's answers are the recording's (each due by the client's sent byte count and its frames, as
in a replay), so a line sent longer than recorded is not answered as the server would answer it; what
both clients send and show is still the same input on both. The recording's byte comparisons are left
out (pair), every other difference counts.

Run from tests/parity with PARITY_RUNS set to your own run root.
"""
import argparse
import os
import sys
import threading

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import inputs  # noqa: E402
import parity  # noqa: E402

LONG = 300
FILL = 'Abcdefghij0123456789klmnopqrstuvwxyz'
KR = '가'                     # '가', CP949 b0 a1

# The calls that type into an edit field, by scenario: {call number: what the field is}. Calls that
# type a command, a single type-ahead letter or a key are left alone.
FIELDS = {
    'chat_limits': {2: 'chat line', 3: 'chat line (said)', 7: 'chat room title'},
    'chat_room_make': {2: 'chat room title', 3: 'chat room password', 4: 'chat room line (said)'},
    'charselect': {0: 'login id', 1: 'login password', 2: 'new character name'},
    'agency': {2: 'agency level from', 3: 'agency level to', 7: 'agency memo'},
    'buying_store': {7: 'buying store title', 8: 'buying store price', 13: 'buying store limit'},
    'emotion_windows': {2: 'macro line'},
    'guild': {3: 'guild notice subject', 4: 'guild notice body'},
    'mail_bank': {4: 'bank amount', 5: 'mail recipient', 6: 'mail title', 7: 'mail body', 8: 'mail zeny',
                  9: 'mail item count'},
    'market': {6: 'market search'},
    'party': {2: 'party name'},
    'tipbox_quests': {2: 'tip search'},
    'economy': {8: 'buy count', 13: 'storage search'},
    'social': {2: 'whisper line (said)'},
    'vending': {4: 'shop title', 6: 'vending price'},
    'lookups': {2: 'monster info command', 3: 'monster search', 5: 'item search', 9: 'recipe search'},
    'keyboard': {5: 'chat line'},
}


class Calls:
    """Seq.type / Seq.text with call numbers, rewriting the chosen ones."""

    def __init__(self, variant=None, chosen=()):
        self.variant = variant
        self.chosen = set(chosen)
        self.n = 0
        self.log = []
        self._type, self._text = inputs.Seq.type, inputs.Seq.text
        self._load = parity.load_phases
        self._lock = threading.Lock()

    def _longer(self, s):
        if self.variant == 'long':
            fill = '9' if s and s.isdigit() else None
            out = s
            k = 0
            while len(out) < LONG:
                out += fill or FILL[k % len(FILL)]
                k += 1
            return out
        if self.variant == 'kr':
            head = s or 'a'
            return head + KR * ((LONG - len(head.encode('cp949'))) // 2)
        if self.variant == 'kr0':
            return KR * (LONG // 2)
        raise KeyError(self.variant)

    def _call(self, kind, seq, s, then):
        k = self.n
        self.n += 1
        self.log.append((k, kind, seq.frame, s))
        if k in self.chosen:
            s = self._longer(s)
            # characters type() has no key for go in as WM_CHARs
            if kind == 'type' and any(c not in inputs._TYPE_KEYS for c in s):
                kind = 'text'
        return (self._type if kind == 'type' else self._text)(seq, s, then)

    def load_phases(self, name):
        # each client's run loads the scenario again, so the numbering starts over for each load
        with self._lock:
            self.n = 0
            return self._load(name)

    def uninstall(self):
        inputs.Seq.type, inputs.Seq.text = self._type, self._text
        parity.load_phases = self._load

    def install(self):
        calls = self

        def type_(seq, s, then=4):
            return calls._call('type', seq, s, then)

        def text_(seq, s, then=4):
            return calls._call('text', seq, s, then)

        inputs.Seq.type, inputs.Seq.text = type_, text_
        parity.load_phases = self.load_phases
        return self


def scenario_calls(name):
    """[(number, kind, frame, text)] of a scenario's type() and text() calls (say() is a text())."""
    c = Calls().install()
    try:
        parity.load_phases(name)
    finally:
        c.uninstall()
    return c.log


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=['list', 'pair'])
    ap.add_argument('args', nargs='+')
    ap.add_argument('--clients', default='herc,ragexe')
    ap.add_argument('--out')
    args = ap.parse_args()
    if args.mode == 'list':
        for name in args.args:
            print(f'== {name}')
            for k, kind, frame, s in scenario_calls(name):
                mark = FIELDS.get(name, {}).get(k, '')
                print(f'  {k:3} {kind:4} @{frame:<6} {s!r:50} {mark}')
        return
    variant, names = args.args[0], args.args[1:]
    clients = args.clients.split(',')
    base = args.out or os.path.join(parity.RUN_ROOT, f'edit-{variant}')
    ok = True
    for spec in names:
        name, _, nums = spec.partition(':')
        chosen = [int(n) for n in nums.split(',')] if nums else sorted(FIELDS[name])
        print(f'{name}: calls {chosen} as {variant}', flush=True)
        c = Calls(variant, chosen).install()
        try:
            ok &= parity.pair(name, clients, os.path.join(base, name))
        finally:
            c.uninstall()
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
