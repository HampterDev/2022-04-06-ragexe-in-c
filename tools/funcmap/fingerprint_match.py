"""Extend the function map by three rules, in rounds (each round's matches become anchors):
  ustr   a string literal that occurs in exactly one asm function and exactly one C function
  callee a mapped pair (F, g) whose unmapped callees are one on each side
  jacc   mutual-best Jaccard over strings, imports and calls into mapped functions
A held-out check on the cited pairs estimates each rule's precision first."""
import os, re, sys, json, csv, collections, random

fmap, tsv, main_src, out_tsv = sys.argv[1:5]
THRESH = 0.5

DEF = re.compile(r'^(?!typedef|#|/\*|\*|//|\s)([A-Za-z_][\w\s\*]*?)\b([A-Za-z_]\w*)\s*\(([^;{]*)\)\s*\{?\s*$')
KW = {'if', 'for', 'while', 'switch', 'return', 'sizeof', 'defined'}
STR = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
CALL = re.compile(r'\b([A-Za-z_]\w*)\s*\(')
CKW = KW | {'else', 'do', 'case', 'struct', 'union', 'enum', 'goto', 'break', 'continue', 'typedef', 'static', 'const', 'void', 'int', 'char', 'float', 'double', 'unsigned', 'short', 'long', 'STATIC_ASSERT', 'offsetof'}
GAME = ('text/actor', 'text/contents', 'text/core', 'text/effects', 'text/gamemode', 'text/loginmode', 'text/session', 'text/world', 'text/render', 'text/skills', 'text/fileio', 'text/navi', 'text/iteminfo', 'text/platform', 'text/asyncwork', 'text/replay')
CRT_C = {'mem_alloc', 'mem_free', 'memcpy', 'memset', 'memcmp', 'strlen', 'strcpy', 'strncpy', 'strcmp', 'sprintf', 'snprintf', '_snprintf_s', 'malloc', 'free', 'calloc', 'realloc', 'abs', 'rand', 'atoi', 'atof', 'strtol', 'strchr', 'strstr', 'strcat', 'fopen', 'fclose', 'fread', 'fwrite', 'fprintf', 'printf', 'qsort', 'vec3', 'string_init', 'string_free', 'string_assign', 'string_assign_cstr', 'string_cstr', 'string_copy', 'string_move', 'string_append', 'string_append_cstr', 'string_format', 'list_erase', 'list_push_back', 'vec_push', 'vec_free', 'vec_init', 'tree_find', 'timeGetTime', 'GetTickCount', 'MulDiv', 'sqrtf', 'cosf', 'sinf', 'atan2f', 'floorf', 'fabsf', 'ceilf', 'powf'}

def norm_import(s):
    s = s.lstrip('_'); s = re.sub(r'@\d+$', '', s); s = re.sub(r'^imp_+', '', s)
    return s.lower()

def unescape(s):
    try: return bytes(s, 'utf-8').decode('unicode_escape')
    except Exception: return s

asm = {}
for l in open(fmap, encoding='utf-8'):
    d = json.loads(l)
    if d['addr'] is None or 'thunk' in d['name'] or d['name'].startswith(('label_', 'xt_')) or not d['chunk'].startswith(GAME):
        continue
    asm['%06x' % d['addr']] = d
label2addr = {d['name']: a for a, d in asm.items()}
def callee_addrs(d):
    out = []
    for c in d['calls']:
        a = c[4:] if (c.startswith('sub_') and len(c) == 10 and c[4:] in asm) else label2addr.get(c)
        if a: out.append(a)
    return out

rows = list(csv.DictReader(open(tsv, encoding='utf-8'), delimiter='\t'))
addr2final = {}
for r in rows:
    if r['final_name'] and r['method'] != 'cited_multi':
        addr2final.setdefault(r['addr'], r['final_name'])

cfun = {}
for dp, dn, fs in os.walk(main_src):
    for f in fs:
        if not f.endswith('.c'): continue
        p = os.path.join(dp, f); rel = os.path.relpath(p, main_src).replace('\\', '/')
        lines = open(p, encoding='utf-8', errors='replace').read().split('\n')
        i, n = 0, len(lines)
        while i < n:
            m = DEF.match(lines[i])
            if m and not lines[i].rstrip().endswith(';') and m.group(2) not in KW:
                name = m.group(2); k = i + 1
                depth = lines[i].count('{') - lines[i].count('}')
                while k < n:
                    depth += lines[k].count('{') - lines[k].count('}')
                    if lines[k].startswith('}') and depth <= 0: break
                    k += 1
                body = '\n'.join(lines[i + 1:k + 1])
                strs = {unescape(s) for s in STR.findall(body) if len(s) > 2}
                calls = [c for c in CALL.findall(body) if c not in CKW and c != name]
                cfun[name] = {'file': f'{rel}:{i+1}', 'strings': strs, 'calls': calls, 'size': k - i}
                i = k + 1; continue
            i += 1

# string occurrence counts
asm_str_count = collections.Counter(s for d in asm.values() for s in set(d['strings']))
c_str_count = collections.Counter(s for g in cfun.values() for s in g['strings'])
c_by_str = collections.defaultdict(set)
for g, f in cfun.items():
    for s in f['strings']: c_by_str[s].add(g)

def rule_ustr(anchors, mapped_c):
    acc = {}
    for a, d in asm.items():
        if a in anchors: continue
        hits = set()
        for s in set(d['strings']):
            if asm_str_count[s] == 1 and c_str_count.get(s, 0) == 1:
                g = next(iter(c_by_str[s]))
                if g not in mapped_c: hits.add(g)
        if len(hits) == 1:
            acc[a] = (hits.pop(), 'ustr')
    return acc

def rule_callee(anchors, mapped_c):
    acc = collections.defaultdict(set)
    for a, g in anchors.items():
        if a not in asm or g not in cfun: continue
        ua = [c for c in dict.fromkeys(callee_addrs(asm[a])) if c not in anchors and asm[c]['ninsn'] >= 4]
        ug = [c for c in dict.fromkeys(cfun[g]['calls']) if c in cfun and c not in mapped_c and c not in CRT_C]
        if len(ua) == 1 and len(ug) == 1:
            acc[ua[0]].add(ug[0])
    return {a: (next(iter(gs)), 'callee') for a, gs in acc.items() if len(gs) == 1}

def feats_asm(d, anchors):
    s = {('s', x) for x in d['strings'] if x.strip()}
    i = {('i', norm_import(x)) for x in d['imports']}
    c = {('c', anchors[a]) for a in callee_addrs(d) if a in anchors}
    return s, i, c
def feats_c(g):
    s = {('s', x) for x in g['strings']}
    i = {('i', c.lower()) for c in g['calls'] if c not in cfun}
    c = {('c', c) for c in g['calls'] if c in cfun}
    return s, i, c
def jacc(fa, fc):
    sh = 3 * len(fa[0] & fc[0]) + len(fa[1] & fc[1]) + 2 * len(fa[2] & fc[2])
    un = 3 * len(fa[0] | fc[0]) + len(fa[1] | fc[1]) + 2 * len(fa[2] | fc[2])
    return (sh / un if un else 0.0), len(fa[0] & fc[0]) + len(fa[1] & fc[1]) + len(fa[2] & fc[2])

def rule_jacc(anchors, mapped_c):
    cf = {g: feats_c(cfun[g]) for g in cfun if g not in mapped_c}
    index = collections.defaultdict(set)
    for g, (s, i, c) in cf.items():
        for x in s | i | c: index[x].add(g)
    best_a, best_c = {}, {}
    for a, d in asm.items():
        if a in anchors or d['ninsn'] < 6: continue
        fa = feats_asm(d, anchors); feats = fa[0] | fa[1] | fa[2]
        if not feats: continue
        cands = set().union(*(index.get(x, set()) for x in feats))
        scored = sorted(((jacc(fa, cf[g]), g) for g in cands), reverse=True)
        scored = [(sc, g) for (sc, nsh), g in scored if nsh >= 2]
        if scored and scored[0][0] >= THRESH and (len(scored) == 1 or scored[0][0] > scored[1][0] + 0.1):
            best_a[a] = scored[0]
    for a, (sc, g) in best_a.items():
        if g not in best_c or best_c[g][0] < sc: best_c[g] = (sc, a)
    return {a: (g, 'jacc') for a, (sc, g) in best_a.items() if best_c[g][1] == a}

def rounds(anchors, max_rounds=8, log=True):
    anchors = dict(anchors); mapped_c = set(anchors.values()); new = {}
    for r in range(max_rounds):
        got = {}
        for rule in (rule_ustr, rule_callee, rule_jacc):
            acc = rule(anchors, mapped_c)
            for a, (g, how) in acc.items():
                if a not in anchors and g not in mapped_c:
                    anchors[a] = g; mapped_c.add(g); got[a] = (g, how, r + 1)
        if log: print(f'  round {r+1}: +{len(got)} ' + str(collections.Counter(h for _, h, _ in got.values())))
        new.update(got)
        if not got: break
    return new

# held-out precision per rule
known = {a: f for a, f in addr2final.items() if a in asm and f in cfun}
random.seed(1)
sample = set(random.sample(sorted(known), min(600, len(known))))
pred = rounds({a: f for a, f in addr2final.items() if a not in sample}, log=False)
byrule = collections.defaultdict(lambda: [0, 0])
for a, (g, how, r) in pred.items():
    if a in sample:
        byrule[how][1] += 1
        if known[a] == g: byrule[how][0] += 1
print('held-out (600 cited pairs hidden):', {h: f'{c}/{n} correct' for h, (c, n) in byrule.items()}, 'recall', f'{sum(c for c,_ in byrule.values())}/{len(sample)}')

new = rounds(dict(addr2final))
fields = ['addr', 'asm_label', 'asm_chunk', 'class', 'ref_name', 'ref_file', 'final_name', 'main_where', 'method', 'n_addrs', 'round']
with open(out_tsv, 'w', encoding='utf-8', newline='') as fh:
    fh.write('\t'.join(fields) + '\n')
    for r in rows:
        fh.write('\t'.join(str(r.get(k, '')) for k in fields) + '\n')
    for a in sorted(new):
        g, how, r = new[a]; d = asm[a]
        fh.write('\t'.join([a, d['name'], f"{d['chunk']}:{d['line0']}-{d['line1']}", '', '', '', g, cfun[g]['file'], how, '1', str(r)]) + '\n')
print('new pairs', len(new), 'total rows', len(rows) + len(new))
