"""rag_record_map.py <root> <records dump> <out.inc>

Writes the tables test_rag_effect_process.c compares the prim record unions with. The original
keeps a prim's records as four slots of 180 bytes with one field layout for every prim kind;
this tree keeps a union of per-kind structs. The map says, per union member and field unit (four
bytes or less), where in a slot the original keeps it, with a per-kind entry where kinds differ.

It is derived from a record dump of both sides, written by the test with
ORACLE_RECORD_DUMP=<file> (one line per prim: effect id, option, kind, the lift's union bytes,
the original's slot bytes), and from the union layout, which a small C program printed here
reports; cl must be on the path (run it from the environment build.bat sets up)."""
import glob
import os
import re
import subprocess
import sys
from collections import defaultdict

root, dump_path, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
SLOT = 0xb4
work = os.path.join(root, "build", "tests", "oracle")
os.makedirs(work, exist_ok=True)


def union_layout():
    hdr = open(os.path.join(root, "src", "effects", "rag", "prim_records.h"), encoding="utf-8").read()
    structs = {}
    for m in re.finditer(r"struct (Prim\w+) \{", hdr):
        depth, i = 0, m.end() - 1
        while True:
            if hdr[i] == "{":
                depth += 1
            elif hdr[i] == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        body = re.sub(r"struct \{[^}]*\}", "struct", hdr[m.end():i], flags=re.S)
        structs[m.group(1)] = [(fm.group(1).strip(), fm.group(2), fm.group(3))
                               for fm in (re.match(r"^(.+?)\s+([A-Za-z_]\w*)((?:\[[^\]]+\])*);$", l.strip()) for l in body.split("\n")) if fm]
    um = re.search(r"union PrimRecords \{(.*?)\};", hdr, re.S)
    members = [(mm.group(1), mm.group(2), mm.group(3)) for mm in
               (re.match(r"^struct (Prim\w+) (\w+)((?:\[[^\]]+\])*);$", l.strip()) for l in um.group(1).split("\n")) if mm]
    src = ['#include <stdio.h>', '#include <stddef.h>', '#include "effects/rag/rag_effect.h"', 'int main(void) {']
    for stype, mname, arr in members:
        elem = mname + ("[0]" if arr else "")
        src.append('    printf("member %s %%u %%u %%u\\n", (unsigned)offsetof(union PrimRecords, %s), (unsigned)sizeof(((union PrimRecords *)0)->%s), (unsigned)sizeof(struct %s));'
                   % (mname, mname, mname, stype))
        for ftype, fname, farr in structs[stype]:
            src.append('    printf("field %s %s %%u %%u\\n", (unsigned)offsetof(union PrimRecords, %s.%s), (unsigned)sizeof(((union PrimRecords *)0)->%s.%s));'
                       % (mname, fname, elem, fname, elem, fname))
    src += ['    return 0;', '}']
    c_path = os.path.join(work, "record_layout.c")
    open(c_path, "w", encoding="utf-8").write("\n".join(src) + "\n")
    exe = os.path.join(work, "record_layout.exe")
    subprocess.check_call(["cl", "/nologo", "/W3", "/TC", "/std:c11", "/MD", "/D_CRT_SECURE_NO_WARNINGS",
                           "/I" + os.path.join(root, "src"), "/I" + os.path.join(root, "vendor", "lua-5.1.4", "src"),
                           c_path, "/Fe:" + exe, "/Fo:" + os.path.join(work, "record_layout.obj")], stdout=subprocess.DEVNULL)
    layout = subprocess.check_output([exe]).decode()
    members_out, fields = {}, defaultdict(list)
    for line in layout.split("\n"):
        p = line.split()
        if not p:
            continue
        if p[0] == "member":
            members_out[p[1]] = (int(p[2]), int(p[3]), int(p[4]))
        else:
            fields[p[1]].append((p[2], int(p[3]), int(p[4])))
    return members_out, fields


def kind_members_table():
    rag = os.path.join(root, "src", "effects", "rag")
    src = open(os.path.join(rag, "effect_prim_process.c"), encoding="utf-8").read()
    body = re.search(r"switch \(\(unsigned\)self->kind\) \{(.*?)\n    \}", src, re.S).group(1)
    kind_fn = {}
    for block in re.split(r"\n    case ", body)[1:]:
        head = block.split("\n")[0]
        fns = re.findall(r"(prim_\w+)\(self", block)
        for k in re.findall(r"(\d+):", head):
            kind_fn[int(k)] = fns[0] if fns else None
    fn_info = {}
    for path in glob.glob(os.path.join(rag, "prim_*.c")) + glob.glob(os.path.join(rag, "effect_prim*.c")):
        s = open(path, encoding="utf-8").read()
        for fm in re.finditer(r"\n(?:static )?(?:unsigned char|void|int) (prim_\w+)\(struct EffectPrim \*self[^)]*\)\s*\{", s):
            depth, i = 0, fm.end() - 1
            while True:
                if s[i] == "{":
                    depth += 1
                elif s[i] == "}":
                    depth -= 1
                    if depth == 0:
                        break
                i += 1
            fbody = s[fm.end():i]
            fn_info[fm.group(1)] = (set(re.findall(r"records\.(\w+)", fbody)), set(re.findall(r"\b(prim_\w+)\(self", fbody)))

    def members_of(fn, seen):
        if fn in seen or fn not in fn_info:
            return set()
        seen.add(fn)
        mems, calls = fn_info[fn]
        out = set(mems)
        for c in calls:
            out |= members_of(c, seen)
        return out
    return {k: sorted(members_of(fn, set())) if fn else [] for k, fn in kind_fn.items()}


members, fields = union_layout()
kind_members = kind_members_table()

by_member, kinds_of = defaultdict(list), defaultdict(set)
for line in open(dump_path):
    eff, opt, kind, lift_hex, orig_hex = line.split()
    kind = int(kind)
    l, o = bytes.fromhex(lift_hex), bytes.fromhex(orig_hex)
    if not any(l) and not any(o):
        continue
    for m in kind_members.get(kind, []):
        by_member[m].append((kind, l, o))
        kinds_of[m].add(kind)


def units(fname, foff, fsize):
    if fsize <= 4:
        return [(fname, foff, fsize)]
    return [("%s+%d" % (fname, i), foff + i, min(4, fsize - i)) for i in range(0, fsize, 4)]


def resolve(mname, sams, report):
    base, total, esize = members[mname]
    count = total // esize
    resolved, prev, taken = {}, None, []
    for fname, foff, fsize in fields[mname]:
        for uname, uoff, usize in units(fname, foff, fsize):
            per_elem = {}
            for e in range(count):
                loff = base + uoff + e * esize
                informative = [(l[loff:loff + usize], o) for _, l, o in sams if any(l[loff:loff + usize])]
                if informative:
                    lo, hi = e * SLOT, (e + 1) * SLOT - usize + 1
                    per_elem[e] = [o for o in range(lo, hi) if all(ob[o:o + usize] == lb for lb, ob in informative)]
            if not per_elem:
                continue
            within = None
            for e, cands in sorted(per_elem.items()):
                offs = [c - e * SLOT for c in cands]
                if within is None and len(offs) == 1:
                    within = offs[0]
            if within is None and prev is not None:
                want = prev[1] + (uoff - prev[0])
                if all(want + e * SLOT in cands for e, cands in per_elem.items()):
                    within = want
            if within is None:
                common = None
                for e, cands in per_elem.items():
                    offs = set(c - e * SLOT for c in cands)
                    common = offs if common is None else common & offs
                free = sorted(o for o in (common or set()) if not any(t <= o < t + ts for t, ts in taken))
                if len(free) == 1:
                    within = free[0]
            if within is None:
                for e, cands in sorted(per_elem.items()):
                    offs = [c - e * SLOT for c in cands]
                    report.append("   %-22s +%03x %2d  elem %d %s" % (uname, uoff, usize, e, "not found" if not offs else "ambiguous " + str([hex(x) for x in offs])))
                continue
            if any(within + e * SLOT not in cands for e, cands in per_elem.items()):
                report.append("   %-22s +%03x %2d  -> slot+%03x but the elements disagree" % (uname, uoff, usize, within))
                continue
            report.append("   %-22s +%03x %2d  -> slot+%03x" % (uname, uoff, usize, within))
            resolved[(uoff, usize)] = within
            prev = (uoff, within)
            taken.append((within, usize))
    return resolved


report, entries, differing = [], [], []
for mname in sorted(by_member, key=lambda m: members[m][0]):
    report.append("== %s: %d samples from kinds %s" % (mname, len(by_member[mname]), sorted(kinds_of[mname])))
    per_kind = {}
    for kind in sorted(kinds_of[mname]):
        report.append(" -- kind %d" % kind)
        per_kind[kind] = resolve(mname, [s for s in by_member[mname] if s[0] == kind], report)
    for uoff, usize in sorted(set(u for r in per_kind.values() for u in r)):
        withins = {k: r[(uoff, usize)] for k, r in per_kind.items() if (uoff, usize) in r}
        if len(set(withins.values())) == 1:
            entries.append((mname, 0, uoff, next(iter(withins.values())), usize))
        else:
            for k, w in sorted(withins.items()):
                entries.append((mname, k, uoff, w, usize))
            differing.append((mname, uoff, usize, withins))

kept = []
if os.path.exists(out_path):
    for line in open(out_path):
        m = re.match(r'\s*\{ "(\w+)", (\d+), 0x([0-9a-f]+), 0x([0-9a-f]+), 0 \},', line)
        if m:
            kept.append((m.group(1), int(m.group(2)), int(m.group(3), 16), int(m.group(4), 16), 0))
entries += kept

open(os.path.join(work, "record_map.txt"), "w").write("\n".join(report) + "\n")
with open(out_path, "w", newline="\n") as f:
    f.write("/* generated by tests/oracle/rag_record_map.py from a record dump of both sides */\n\n")
    f.write("static const struct RecordField record_fields[] = {\n")
    for mname, kind, uoff, within, usize in entries:
        f.write('    { "%s", %d, 0x%x, 0x%x, %u },\n' % (mname, kind, uoff, within, usize))
    f.write("};\n\nstatic const struct RecordMember record_members[] = {\n")
    for mname in sorted(by_member, key=lambda m: members[m][0]):
        base, total, esize = members[mname]
        f.write('    { "%s", 0x%x, %u, %u },\n' % (mname, base, esize, total // esize))
    f.write("};\n\nstatic const struct KindRecord kind_records[] = {\n")
    for kind in sorted(kind_members):
        for m in kind_members[kind]:
            if m in by_member:
                f.write('    { %d, "%s" },\n' % (kind, m))
    f.write("};\n")
print("%s: %d field entries; %d units differ between kinds%s" % (out_path, len(entries), len(differing), "".join("\n  %s +0x%x: %s" % (m, u, w) for m, u, s, w in differing)))
