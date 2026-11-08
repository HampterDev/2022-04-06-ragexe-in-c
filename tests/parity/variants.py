"""Client data varied from DATA_ROOT's: the data set a run folder gets under PARITY_VARIANT.

PARITY_VARIANT is a list of edits separated by ';', each `kind:args`, or the name of a preset in
PRESETS. Each edit names a file as the client asks for it (`data\\msgstringtable.txt`,
`System\\tipbox_EN.lub`, `data\\luafiles514\\lua files\\datainfo\\jobname.lub`):

    msg_empty:A-B          msgStringTable.txt's entries A to B (inclusive) made empty
    msg_set:ID=TEXT        entry ID's text made TEXT (TEXT may hold \\r, \\n, \\t and \\xNN escapes)
    msg_cut:N              the table cut after its first N entries
    msg_noeol              the table's last line left without its line ending
    empty:PATH             the file made empty
    drop:PATH              a loose file removed (a file in an archive cannot be: see below)
    bom:PATH               a UTF-8 byte-order mark put before the file's text
    lf:PATH                every CRLF line ending made LF
    noeol:PATH             the trailing line endings removed
    lua:PATH@SNIPPET(A,..) variants\\SNIPPET.lua's code run after the file's own, DV_ARGS = {"A", ...}
                           (a Lua file's tables changed after it builds them: keys removed, values
                           of other types)
    set:PATH@FILE          the file replaced by variants\\FILE

A file the archives hold is changed through an archive of the changed files only
(parity_variant.grf), which the run folder's data.ini puts first in the search order (key 0): both
clients search key 0 first, and nothing is copied out of the large archives. A loose file (one no
archive holds, such as System\\*.lub) is changed in the run folder. A file in an archive cannot be
made missing this way: an archive entry the client finds is read from that archive.

The edits apply in order; each reads the file as the edits before it left it.
"""
import hashlib
import os
import re
import struct
import threading
import time
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
SNIPPETS = os.path.join(HERE, 'variants')
OVERLAY = 'parity_variant.grf'
_BUILD = threading.Lock()

# Named variants kept because they found something, each with why it is worth running.
PRESETS = {
}


def _open(path):
    """The file opened for reading. A client's folder being emptied can hold a hard link to the archive in
    deletion for a moment, which refuses every open of the file meanwhile: those are tried again."""
    for _ in range(60):
        try:
            return open(path, 'rb')
        except PermissionError:
            time.sleep(0.5)
    return open(path, 'rb')


def _grf_table(path):
    """{lower-case name: (stored size, compressed size, real size, flags, offset)} of a GRF 0x200 archive."""
    with _open(path) as f:
        hdr = f.read(46)
        if hdr[:15] != b'Master of Magic':
            return {}
        off, seed, cnt, ver = struct.unpack('<IIII', hdr[30:46])
        if ver & 0xff00 != 0x200:
            return {}
        f.seek(off + 46)
        csz, usz = struct.unpack('<II', f.read(8))
        t = zlib.decompress(f.read(csz))
    out = {}
    i = 0
    while i < len(t):
        j = t.index(b'\0', i)
        name = t[i:j].lower()
        c, ca, r, fl, ofs = struct.unpack('<IIIBI', t[j + 1:j + 18])
        i = j + 18
        out.setdefault(name, (c, ca, r, fl, ofs))
    return out


class Archives:
    """The run folder's archives in the client's search order (data.ini key 0 first)."""

    def __init__(self, run_dir, data_root=None):
        self.run_dir = run_dir
        self.paths = []
        ini = os.path.join(run_dir, 'data.ini')
        keys = {}
        if os.path.exists(ini):
            for line in open(ini, encoding='mbcs', errors='replace'):
                m = re.match(r'\s*(\d)\s*=\s*(.+?)\s*$', line)
                if m:
                    keys[int(m.group(1))] = m.group(2)
        for k in sorted(keys):
            # an archive the run folder links from data_root is read there: a hard link another client holds
            # open can refuse a second open
            linked = data_root and os.path.join(data_root, keys[k])
            self.paths.append(linked if linked and os.path.exists(linked) else os.path.join(run_dir, keys[k]))
        self.tables = {}

    def find(self, name):
        key = name.replace('/', '\\').lower().encode('cp949')
        for p in self.paths:
            if not os.path.exists(p):
                continue
            if p not in self.tables:
                self.tables[p] = _grf_table(p)
            e = self.tables[p].get(key)
            if e:
                c, ca, r, fl, ofs = e
                with _open(p) as f:
                    f.seek(ofs + 46)
                    raw = f.read(ca)
                return zlib.decompress(raw)[:r] if fl & 1 else raw[:r]
        return None


def write_grf(path, files):
    """A GRF 0x200 archive of `files` ({name: bytes}), every entry deflated."""
    body = bytearray()
    table = bytearray()
    for name, data in files.items():
        packed = zlib.compress(data)
        table += name.replace('/', '\\').encode('cp949') + b'\0'
        table += struct.pack('<IIIBI', len(packed), len(packed), len(data), 1, len(body))
        body += packed
    packed_table = zlib.compress(bytes(table))
    header = b'Master of Magic\0' + bytes(14) + struct.pack('<IIII', len(body), 0, len(files) + 7, 0x200)
    with open(path, 'wb') as f:
        f.write(header + bytes(body) + struct.pack('<II', len(packed_table), len(table)) + packed_table)


def _unescape(text):
    return re.sub(r'\\(r|n|t|x[0-9a-fA-F]{2}|\\)',
                  lambda m: {'r': '\r', 'n': '\n', 't': '\t', '\\': '\\'}.get(m.group(1)) or chr(int(m.group(1)[1:], 16)),
                  text)


def _msg_entries(data):
    """msgStringTable.txt as a list of entries (each the text before its '#') and what follows the last one."""
    parts = data.split(b'#')
    return parts[:-1], parts[-1]


def _msg_join(entries, tail):
    return b'#'.join(entries) + b'#' + tail if entries else tail


def _entry_text(e):
    """An entry's text and the line break before it (the table's entries are 'text#' lines)."""
    m = re.match(rb'^(\r?\n)?(.*)$', e, re.S)
    return m.group(1) or b'', m.group(2)


MSG = 'data\\msgstringtable.txt'


def _edit(kind, args, read):
    """(path, new bytes or None to remove) for one edit; read(path) is the file as it stands."""
    if kind == 'msg_empty':
        a, b = (int(x, 0) for x in args.split('-'))
        entries, tail = _msg_entries(read(MSG))
        for i in range(a, min(b, len(entries) - 1) + 1):
            entries[i] = _entry_text(entries[i])[0]
        return MSG, _msg_join(entries, tail)
    if kind == 'msg_set':
        n, text = args.split('=', 1)
        entries, tail = _msg_entries(read(MSG))
        entries[int(n, 0)] = _entry_text(entries[int(n, 0)])[0] + _unescape(text).encode('cp949', 'replace')
        return MSG, _msg_join(entries, tail)
    if kind == 'msg_cut':
        entries, tail = _msg_entries(read(MSG))
        return MSG, _msg_join(entries[:int(args, 0)], b'\r\n')
    if kind == 'msg_noeol':
        return MSG, read(MSG).rstrip(b'\r\n')
    if kind == 'empty':
        return args, b''
    if kind == 'drop':
        return args, None
    if kind == 'bom':
        return args, b'\xef\xbb\xbf' + read(args)
    if kind == 'lf':
        return args, read(args).replace(b'\r\n', b'\n')
    if kind == 'noeol':
        return args, read(args).rstrip(b'\r\n')
    if kind == 'lua':
        path, snippet = args.rsplit('@', 1)
        m = re.match(r'^(\w+)(?:\((.*)\))?$', snippet)
        with open(os.path.join(SNIPPETS, m.group(1) + '.lua'), 'rb') as f:
            extra = f.read()
        values = m.group(2).split(',') if m.group(2) is not None else []
        head = 'local DV_ARGS = {' + ', '.join('"%s"' % v for v in values) + '}\r\n'
        base = read(path)
        if base[:4] == b'\x1bLua':
            # a compiled chunk: run as a string of its bytes, which loadstring takes as it takes text
            base = b'assert(loadstring("' + b''.join(b'\\%d' % c for c in base) + b'"))()'
        return path, base + b'\r\ndo\r\n' + head.encode() + extra + b'\r\nend\r\n'
    if kind == 'set':
        path, name = args.rsplit('@', 1)
        with open(os.path.join(SNIPPETS, name), 'rb') as f:
            return path, f.read()
    raise ValueError(f'unknown variant edit {kind!r}')


# ---- server text: what the map server sends, varied in a replay (PARITY_SERVER_VARIANT) ----------

# The texts the map server sends at the end of a packet that carries its length: id -> the text's offset.
SERVER_TEXTS = {
    0x008d: 8,      # ZC_NOTIFY_CHAT: id.W len.W gid.L text
    0x008e: 4,      # ZC_NOTIFY_PLAYERCHAT
    0x009a: 4,      # ZC_BROADCAST
    0x01c3: 16,     # ZC_BROADCAST2: id.W len.W colour.L font.W size.W align.W y.W text
    0x02c1: 12,     # ZC_NPC_CHAT: id.W len.W gid.L colour.L text
    0x00b4: 8,      # ZC_SAY_DIALOG: id.W len.W npc.L text
    0x09de: 33,     # ZC_WHISPER: id.W len.W gid.L sender.24B admin.B text
}
SERVER_MENUS = {0x00b7: 8}          # ZC_MENU_LIST: id.W len.W npc.L "a:b:c"
# Fixed 24-byte names: id -> offsets (a negative offset counts from the packet's end).
SERVER_NAMES = {
    0x09fd: (-24,), 0x09fe: (-24,), 0x09ff: (-24,),       # the unit packets' name, last
    0x0a30: (6, 30, 54, 78),                              # ZC_ACK_REQNAMEALL2: name, party, guild, title
    0x0adf: (10, 34),                                     # ZC_ACK_REQNAME_TITLE: name, title
}
TEXTS = {
    'long': b'W' * 1000,
    'w200': b'W' * 200,
    'empty': b'',
    'caret': b'^FF00^^ab^0000FFtext^',
    'tag': b'<ITEM>Apple<INFO>512</INFO> <NAVI>[x]<INFO>prontera,1,1,0,000,0</INFO> <URL>link<INFO>http://x',
    'noterm': b'unterminated',
}
NAMES = {'full': b'N' * 24, 'empty': b'\0' * 24, 'caret': b'^FF0000Name^\0'.ljust(24, b'\0'),
         'tag': b'<ITEM>N<INFO>1</INFO\0'.ljust(24, b'\0')}


def _server_packet(pid, pkt, what):
    if 'texts' in what and pid in SERVER_TEXTS or 'menus' in what and pid in SERVER_MENUS:
        at = SERVER_TEXTS.get(pid) or SERVER_MENUS[pid]
        how = what.get('texts') if pid in SERVER_TEXTS else what['menus']
        if len(pkt) < at:
            return pkt
        text = TEXTS[how] + (b'' if how == 'noterm' else b'\0')
        out = bytearray(pkt[:at] + text)
        struct.pack_into('<H', out, 2, len(out))
        return bytes(out)
    if 'names' in what and pid in SERVER_NAMES:
        out = bytearray(pkt)
        for at in SERVER_NAMES[pid]:
            at = len(out) + at if at < 0 else at
            if 0 <= at and at + 24 <= len(out):
                out[at:at + 24] = NAMES[what['names']]
        return bytes(out)
    return pkt


def server(connections, spec, sizes):
    """The recording's connections with the map server's texts varied. spec: `kind:how` items separated
    by ';', kind texts (SERVER_TEXTS), menus (SERVER_MENUS) or names (SERVER_NAMES), how a key of TEXTS
    or NAMES. A packet goes out with the chunk that held its last byte, as the replay sends one split
    across chunks."""
    what = dict(item.split(':', 1) for item in filter(None, (s.strip() for s in spec.split(';'))))
    out = []
    for c in connections:
        events = c.get('events', [])
        sends = b''.join(bytes.fromhex(e['data']) for e in events if 'c' in e)
        if sends[:2] != struct.pack('<H', 0x0436):
            out.append(c)
            continue
        chunks = [i for i, e in enumerate(events) if 's' in e]
        ends, buf = [], b''
        for i in chunks:
            buf += bytes.fromhex(events[i]['data'])
            ends.append(len(buf))
        new = {i: bytearray() for i in chunks}
        pos, k = 0, 0
        while pos + 2 <= len(buf):
            pid = buf[pos] | buf[pos + 1] << 8
            size = sizes.get(pid, -1)
            if size == -1:
                size = buf[pos + 2] | buf[pos + 3] << 8 if pos + 4 <= len(buf) else 0
            if size < 2 or pos + size > len(buf):
                break
            while ends[k] < pos + size:
                k += 1
            new[chunks[k]] += _server_packet(pid, buf[pos:pos + size], what)
            pos += size
        if pos < len(buf):
            while ends[k] < len(buf):
                k += 1
            new[chunks[k]] += buf[pos:]
        c = dict(c, events=[dict(e, data=new[i].hex()) if i in new else e for i, e in enumerate(events)])
        out.append(c)
    return out


def edits(spec):
    """The spec's edits as (kind, args), presets expanded."""
    out = []
    for item in filter(None, (s.strip() for s in spec.split(';'))):
        if item in PRESETS:
            out += edits(PRESETS[item]['spec'])
        else:
            kind, _, args = item.partition(':')
            out.append((kind, args))
    return out


def apply(run_dir, spec, cache_dir, data_root=None):
    """Lays the variant over a prepared run folder (its archives read from data_root where they are there).
    Returns a line saying what it changed."""
    archives = Archives(run_dir, data_root)
    changed = {}

    def loose_path(path):
        return os.path.join(run_dir, path.replace('/', '\\'))

    def read(path):
        key = path.replace('/', '\\').lower()
        if key in changed:
            return changed[key][1] or b''
        data = archives.find(path)
        if data is None and os.path.exists(loose_path(path)):
            with open(loose_path(path), 'rb') as f:
                data = f.read()
        if data is None:
            raise FileNotFoundError(f'variant: no file {path} in the archives or the folder')
        return data

    for kind, args in edits(spec):
        path, data = _edit(kind, args, read)
        key = path.replace('/', '\\').lower()
        in_archive = changed[key][0] if key in changed else archives.find(path) is not None
        if data is None and in_archive:
            raise ValueError(f'variant: {path} is in an archive and cannot be removed')
        changed[key] = (in_archive, data, path)

    grf_files = {k: d for k, (a, d, p) in changed.items() if a}
    for k, (a, d, p) in changed.items():
        if a:
            continue
        dst = loose_path(p)
        if os.path.exists(dst):
            os.remove(dst)
        if d is not None:
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            with open(dst, 'wb') as f:
                f.write(d)
    if grf_files:
        digest = hashlib.sha1(repr(sorted((k, hashlib.sha1(d).hexdigest()) for k, d in grf_files.items()))
                              .encode()).hexdigest()[:16]
        os.makedirs(cache_dir, exist_ok=True)
        cached = os.path.join(cache_dir, digest + '.grf')
        with _BUILD:                    # a pair makes its two clients' folders at once
            if not os.path.exists(cached):
                tmp = f'{cached}.{os.getpid()}.tmp'
                write_grf(tmp, grf_files)
                os.replace(tmp, cached)
        os.link(cached, os.path.join(run_dir, OVERLAY))
        ini = os.path.join(run_dir, 'data.ini')
        text = open(ini, encoding='mbcs').read() if os.path.exists(ini) else '[Data]\r\n'
        if re.search(r'^\s*0\s*=', text, re.M):
            raise ValueError('variant: data.ini already has a key 0')
        text = re.sub(r'(\[Data\][^\n]*\n)', lambda m: m.group(1) + f'0={OVERLAY}\r\n', text, count=1)
        with open(ini, 'w', encoding='mbcs', newline='') as f:
            f.write(text)
    return (f'variant: {len(grf_files)} archived file(s) in {OVERLAY}, '
            f'{len(changed) - len(grf_files)} loose file(s) changed')
