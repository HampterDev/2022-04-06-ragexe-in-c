"""Convert the custom client's data files and repair the damaged ones.

Output layout (nothing else is written there):
    <out>\\<path>              loose files of client-files (System\\..., data\\...)
    <out>\\data.grf\\<path>    files whose active copy is inside data.grf; they only
                               take effect once repacked into data.grf (or loose with
                               <readfolder/> in clientinfo.xml)
"""
import collections
import json
import os
import subprocess

from . import cp949, dump, luabc, luafile, repair, rules, sources, tablefile


def out_path(out_root, src):
    if src.container == 'data.grf':
        return os.path.join(out_root, 'data.grf', src.path)
    return os.path.join(out_root, src.path)


def write(path, data):
    if data.startswith(cp949.BOM):
        raise ValueError('refusing to write a BOM: %s' % path)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(data)


def git_show(root, rev, rel):
    proc = subprocess.run(['git', '-C', root, 'show', '%s:%s' % (rev, rel)], capture_output=True)
    return proc.stdout if proc.returncode == 0 else None


def sample(values, n=5):
    return [v.decode('cp949', 'replace') for v in sorted(values)[:n]]


# ------------------------------------------------------------------ conversion

def convert_all(client_root, out_root):
    cf = sources.ClientFiles(client_root)
    entries = []
    for spec in rules.SPECS:
        e = {'name': spec.name, 'loader': spec.loader, 'note': spec.note}
        entries.append(e)
        src = cf.resolve(spec)
        if src is None:
            e['status'] = 'missing'
            continue
        e['source'] = '%s:%s' % (src.container, src.path)
        e['shadowed'] = ['%s:%s' % (s.container, s.path) for s in cf.shadowed(spec, src)]
        if not cp949.has_high(src.data):
            e['status'] = 'ascii'
            continue
        if spec.kind == 'lua':
            rows = dump.run([src.data], call_main=spec.main)
            res = luafile.convert(spec, src.data, rows)
            e['format'] = res.kind
            e['counts'] = dict(res.counts)
            e['converted_slots'] = res.converted_slots
            e['comments_converted'] = res.comments_converted
            e['fully_utf8'] = res.fully_utf8
            e['kept'] = {cls: sorted({res.paths[v].split(' / ')[0] for v, c in res.kept.items() if c == cls})
                         for cls in set(res.kept.values())}
            e['kept_examples'] = {cls: sample([v for v, c in res.kept.items() if c == cls])
                                  for cls in set(res.kept.values())}
            e['conflicts'] = [(v.decode('cp949', 'replace'), cls) for v, cls in res.conflicts]
            e['unreferenced'] = sample(res.unreferenced, 20)
            e['unreferenced_count'] = len(res.unreferenced)
            e['invalid'] = [v.decode('cp949', 'replace') for v in res.invalid]
            changed = res.output != src.data
            if changed:
                e['problems'] = luafile.verify(spec, src.data, res.output, res.convert, rows)
        else:
            res = tablefile.convert(spec, src.data)
            e['format'] = 'table'
            e['counts'] = dict(res.counts)
            e['invalid'] = [v.decode('cp949', 'replace') for v in res.invalid]
            changed = res.output != src.data
            if changed:
                e['problems'] = tablefile.verify(spec, src.data, res.output)
        if changed:
            p = out_path(out_root, src)
            write(p, res.output)
            e['output'] = os.path.relpath(p, out_root)
            e['status'] = 'converted'
        else:
            e['status'] = 'unchanged'
    return cf, entries


# ------------------------------------------------------------------ repairs

MSGSTRING = 'data\\msgstringtable.txt'
JOBNAME = 'data\\luafiles514\\lua files\\Datainfo\\jobName.lua'
STATEICON = 'data\\luafiles514\\lua files\\StateIcon\\stateiconinfo.lua'
HISTORY_REV = '5095725f'   # client-files' initial commit, before the damage in 73cf5a87


def _lua_dump_or_none(data, main=False):
    try:
        return dump.run([data], call_main=main)
    except dump.DumpError:
        return None


def repair_all(cf, out_root):
    reports = []

    def record(rep, rel, check):
        r = {'file': rel, 'restored': len(rep.restored), 'unrecovered': [
            (w, d.decode('utf-8', 'replace'), why) for w, d, why in rep.unrecovered],
            'sources': sorted({s for _, _, s in rep.restored}),
            'examples': [(w, o.decode('cp949', 'replace')) for w, o, _ in rep.restored[:5]],
            'by_key': [(w, o.decode('cp949', 'replace'), [x.decode('cp949', 'replace') for x in others])
                       for w, o, others in rep.by_key]}
        r['problems'] = check(rep)
        p = os.path.join(out_root, rel)
        write(p, rep.output)
        r['output'] = rel
        reports.append(r)

    # msgStringTable.txt: the loose copy (shadowed by data.grf) lost 7 lines.
    damaged = cf.loose(MSGSTRING)
    ref = cf.in_grf('data\\msgStringTable.txt')
    rep = repair.repair_table_lines('msgstringtable.txt', damaged.data, ref.data, 'data.grf msgStringTable.txt')

    def check_table(rep, damaged=damaged.data):
        problems = []
        a, b = tablefile.fields(damaged), tablefile.fields(rep.output)
        if len(a) != len(b):
            problems.append('field count changed')
        if not cp949.is_utf8(rep.output):
            problems.append('output is not valid UTF-8')
        if cp949.is_damaged(rep.output) and not rep.unrecovered:
            problems.append('U+FFFD left in output')
        return problems
    record(rep, damaged.path, check_table)

    # jobName.lua: sprite names, restored as CP949 bytes.
    damaged = cf.loose(JOBNAME)
    grf_lub = cf.in_grf('data\\LuaFiles514\\Lua Files\\DataInfo\\jobName.lub')
    hist = git_show(cf.root, HISTORY_REV, 'data/lua files/Datainfo/jobName.lua')
    pools = [(repair.bytecode_assignments(grf_lub.data), 'data.grf jobName.lub'),
             (repair.dump_candidates(dump.run([grf_lub.data])), 'data.grf jobName.lub')]
    if hist and not cp949.is_damaged(hist):
        pools.append((repair.source_candidates(hist), 'git %s jobName.lua' % HISTORY_REV))
    rep = repair.repair_lua_source('jobName.lua', damaged.data, repair.merge(*pools), display=False)
    record(rep, damaged.path, lambda rep, d=damaged.data: _check_lua_repair(d, rep, display=False))

    # stateiconinfo.lua: status descriptions, restored and written as UTF-8.
    damaged = cf.loose(STATEICON)
    grf_lub = cf.in_grf('data\\LuaFiles514\\Lua Files\\StateIcon\\stateiconinfo.lub')
    hist = git_show(cf.root, HISTORY_REV, 'data/lua files/StateIcon/stateiconinfo.lua')
    pools = [(repair.source_candidates(hist), 'git %s stateiconinfo.lua' % HISTORY_REV),
             (repair.dump_candidates(dump.run([grf_lub.data])), 'data.grf stateiconinfo.lub')]
    rep = repair.repair_lua_source('stateiconinfo.lua', damaged.data, repair.merge(*pools), display=True)
    record(rep, damaged.path, lambda rep, d=damaged.data: _check_lua_repair(d, rep, display=True))
    return reports


def _check_lua_repair(damaged, rep, display):
    problems = []
    before = _lua_dump_or_none(damaged)
    after = _lua_dump_or_none(rep.output)
    if after is None:
        return ['repaired file does not load']
    if before is not None:
        if len(before) != len(after) or [p for p, _ in before] != [p for p, _ in after]:
            problems.append('table structure changed')
        else:
            for (p, a), (_, b) in zip(before, after):
                if a == b:
                    continue
                # every change must be a damaged string replaced by an original that
                # damages into exactly that string
                ok = a[0] == 's' and b[0] == 's' and cp949.is_damaged(a[1])
                if ok:
                    original = b[1].decode('utf-8').encode('cp949') if display else b[1]
                    ok = cp949.damage(original) == a[1] and (not display or cp949.is_utf8(b[1]))
                if not ok:
                    problems.append('unexpected change at %s' % luafile.path_text(p))
                    break
    if rep.output.startswith(cp949.BOM):
        problems.append('BOM')
    return problems


# ------------------------------------------------------------------ report

def text_report(entries, repairs, cf, out_root):
    lines = ['UTF-8 data conversion report', '=' * 30, '',
             'client-files: %s' % cf.root, 'output:       %s' % out_root,
             'readfolder:   %s (data.grf is read %s loose files)' % (
                 cf.read_folder_first, 'after' if cf.read_folder_first else 'before'), '']
    if cf.git_fallbacks:
        lines.append('fell back to git HEAD for: %s' % ', '.join(cf.git_fallbacks))
    else:
        lines.append('no file had to be taken from git HEAD (all present on disk)')
    lines.append('')
    for e in entries:
        lines.append('%s  [%s]' % (e['name'], e.get('status')))
        if 'source' in e:
            lines.append('    source: %s' % e['source'])
        for s in e.get('shadowed', []):
            lines.append('    shadowed (not loaded): %s' % s)
        if e.get('note'):
            lines.append('    rule: %s' % e['note'])
        if 'counts' in e:
            lines.append('    distinct strings: %s' % ', '.join('%s=%d' % kv for kv in sorted(e['counts'].items())))
        if e.get('converted_slots'):
            lines.append('    rewritten constants/literals: %d' % e['converted_slots'])
        for cls, tables in sorted(e.get('kept', {}).items()):
            lines.append('    kept CP949 (%s): %s  e.g. %s' % (cls, ', '.join(tables), e['kept_examples'][cls]))
        if e.get('unreferenced_count'):
            lines.append('    kept CP949 (not reached by any loaded table): %d  e.g. %s' % (
                e['unreferenced_count'], e['unreferenced'][:5]))
        for v, cls in e.get('conflicts', []):
            lines.append('    CONFLICT kept CP949: %r used as %s' % (v, cls))
        for v in e.get('invalid', []):
            lines.append('    INVALID CP949 kept: %r' % v)
        if 'output' in e:
            lines.append('    written: %s  (fully UTF-8: %s)' % (e['output'], e.get('fully_utf8', True)))
        if e.get('problems'):
            lines.append('    PROBLEMS: %s' % e['problems'])
    lines += ['', 'Repairs', '-------']
    for r in repairs:
        lines.append('%s: restored %d, unrecovered %d, from %s -> %s' % (
            r['file'], r['restored'], len(r['unrecovered']), r['sources'], r['output']))
        for w, o in r['examples']:
            lines.append('    %s = %s' % (w, o))
        if r['by_key']:
            lines.append('    %d restored strings damage the same way as another candidate; the table key decided:'
                         % len(r['by_key']))
            for w, o, others in r['by_key'][:8]:
                lines.append('      %s = %s (also fits: %s)' % (w, o, ', '.join(others[:3])))
        for w, d, why in r['unrecovered']:
            lines.append('    UNRECOVERED %s %r: %s' % (w, d, why))
        if r['problems']:
            lines.append('    PROBLEMS: %s' % r['problems'])
    return '\n'.join(lines) + '\n'


def run(client_root, out_root, report_dir):
    cf, entries = convert_all(client_root, out_root)
    repairs = repair_all(cf, out_root)
    os.makedirs(report_dir, exist_ok=True)
    with open(os.path.join(report_dir, 'convert-report.json'), 'w', encoding='utf-8') as f:
        json.dump({'entries': entries, 'repairs': repairs}, f, ensure_ascii=False, indent=1)
    text = text_report(entries, repairs, cf, out_root)
    with open(os.path.join(report_dir, 'convert-report.txt'), 'w', encoding='utf-8') as f:
        f.write(text)
    return text, entries, repairs


# ------------------------------------------------------------------ validation

REPAIRED = {MSGSTRING.lower(): 'table', JOBNAME.lower(): 'lua-cp949', STATEICON.lower(): 'lua-utf8'}


def _source_for_output(cf, out_root, path):
    rel = os.path.relpath(path, out_root)
    if rel.lower().startswith('data.grf' + os.sep):
        return cf.in_grf(rel[len('data.grf') + 1:])
    return cf.loose(rel)


def _spec_for_source(cf, src):
    for spec in rules.SPECS:
        s = cf.resolve(spec)
        if s and s.container == src.container and s.path.lower() == src.path.lower():
            return spec
    return None


def _validate_lua_fields(spec, src_rows, out_rows, counts, problems):
    if [p for p, _ in src_rows] != [p for p, _ in out_rows]:
        problems.append('field paths differ from the source')
        return
    strings = [v[1] for _, v in src_rows if v[0] == 's' and cp949.has_high(v[1])]
    file_utf8 = bool(strings) and all(cp949.is_utf8(v) for v in strings)
    for (path, a), (_, b) in zip(src_rows, out_rows):
        if a[0] != 's' or not cp949.has_high(a[1]):
            if a != b:
                problems.append('non-text value changed at %s' % luafile.path_text(path))
                return
            continue
        cls = rules.classify(spec, path)
        if cls == rules.DISPLAY and not file_utf8 and a != b:
            if not cp949.is_utf8(b[1]) or b[1] != cp949.to_utf8(a[1]):
                problems.append('display field is not the UTF-8 of its source at %s' % luafile.path_text(path))
                return
            counts['display utf-8'] += 1
        elif cls == rules.DISPLAY and not file_utf8:
            counts['display kept cp949 (conflict/unreachable)'] += 1
        elif a != b:
            problems.append('%s field changed at %s' % (cls, luafile.path_text(path)))
            return
        else:
            counts['%s byte-identical' % cls] += 1


def validate_all(client_root, out_root):
    cf = sources.ClientFiles(client_root)
    results = []
    for dirpath, _, files in os.walk(out_root):
        for fn in sorted(files):
            path = os.path.join(dirpath, fn)
            rel = os.path.relpath(path, out_root)
            data = open(path, 'rb').read()
            r = {'file': rel, 'problems': [], 'counts': {}}
            results.append(r)
            counts = collections.Counter()
            if data.startswith(cp949.BOM):
                r['problems'].append('starts with a UTF-8 BOM')
            src = _source_for_output(cf, out_root, path)
            if src is None:
                r['problems'].append('no source file for this output')
                continue
            r['source'] = '%s:%s' % (src.container, src.path)
            kind = REPAIRED.get(rel.lower())
            if kind:
                r['kind'] = 'repair'
                if kind == 'table':
                    a, b = tablefile.fields(src.data), tablefile.fields(data)
                    if len(a) != len(b):
                        r['problems'].append('field count differs from the damaged file')
                    for (_, fa), (_, fb) in zip(a, b):
                        if fa == fb:
                            continue
                        orig = fb.decode('utf-8').encode('cp949')
                        if cp949.damage(orig) != fa:
                            r['problems'].append('changed field is not a restoration')
                            break
                        counts['restored'] += 1
                    if not cp949.is_utf8(data):
                        r['problems'].append('not valid UTF-8')
                else:
                    try:
                        rows_b = dump.run([data])
                        counts['loads with Lua 5.1'] += 1
                    except dump.DumpError as e:
                        r['problems'].append('does not load: %s' % e)
                        continue
                    rows_a = dump.run([src.data])
                    if [p for p, _ in rows_a] != [p for p, _ in rows_b]:
                        r['problems'].append('table structure differs from the damaged file')
                    for (p, a), (_, b) in zip(rows_a, rows_b):
                        if a == b:
                            if a[0] == 's' and cp949.is_damaged(a[1]):
                                counts['still damaged'] += 1
                            continue
                        orig = b[1].decode('utf-8').encode('cp949') if kind == 'lua-utf8' else b[1]
                        if not cp949.is_damaged(a[1]) or cp949.damage(orig) != a[1]:
                            r['problems'].append('change at %s is not a restoration' % luafile.path_text(p))
                            break
                        counts['restored (%s)' % ('utf-8' if kind == 'lua-utf8' else 'cp949')] += 1
                r['counts'] = dict(counts)
                continue
            spec = _spec_for_source(cf, src)
            if spec is None:
                r['problems'].append('output does not belong to any active data file')
                continue
            r['kind'] = spec.name
            if spec.kind == 'lua':
                try:
                    out_rows = dump.run([data], call_main=spec.main)
                    counts['loads with Lua 5.1'] += 1
                except dump.DumpError as e:
                    r['problems'].append('does not load: %s' % e)
                    continue
                src_rows = dump.run([src.data], call_main=spec.main)
                _validate_lua_fields(spec, src_rows, out_rows, counts, r['problems'])
                if src.data[:4] == luabc.SIGNATURE:
                    if luabc.structure_without_strings(luabc.read(src.data)) != \
                            luabc.structure_without_strings(luabc.read(data)):
                        r['problems'].append('bytecode differs outside string constants')
                    else:
                        counts['bytecode identical outside strings'] += 1
            else:
                r['problems'] += tablefile.verify(spec, src.data, data)
                for (_, fa), (_, fb) in zip(tablefile.fields(src.data), tablefile.fields(data)):
                    if cp949.has_high(fa):
                        if not cp949.is_utf8(fb):
                            r['problems'].append('display field is not UTF-8')
                            break
                        counts['display utf-8'] += 1
            r['counts'] = dict(counts)
    return results


def validation_report(results):
    lines = ['UTF-8 data validation report', '=' * 29, '']
    for r in results:
        lines.append('%s  %s' % ('OK  ' if not r['problems'] else 'FAIL', r['file']))
        if r.get('source'):
            lines.append('      source: %s' % r['source'])
        if r['counts']:
            lines.append('      ' + ', '.join('%s=%d' % kv for kv in sorted(r['counts'].items())))
        for p in r['problems']:
            lines.append('      PROBLEM: %s' % p)
    bad = sum(1 for r in results if r['problems'])
    lines += ['', '%d files, %d with problems' % (len(results), bad)]
    return '\n'.join(lines) + '\n'
