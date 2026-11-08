"""Address citations: for every C function definition in the annotated C tree's src/,
collect the sub_XXXXXX addresses cited in the comment block right above it and inside its body."""
import os, re, sys, json

root = sys.argv[1]
out = sys.argv[2]
ADDR = re.compile(r'\bsub_([0-9a-f]{6})\b')
HEX = re.compile(r'\b0x([0-9a-f]{6})\b')
# a definition: starts at column 0, has a name followed by '(' and ends the line with ')' or '{' (no ';')
DEF = re.compile(r'^(?!static\s+inline|typedef|#|/\*|\*|//)([A-Za-z_][\w\s\*]*?)\b([A-Za-z_]\w*)\s*\(([^;{]*)\)\s*\{?\s*$')

rows = []
for dp, dn, fs in os.walk(root):
    for f in fs:
        if not f.endswith('.c'):
            continue
        p = os.path.join(dp, f)
        rel = os.path.relpath(p, root).replace('\\', '/')
        lines = open(p, encoding='utf-8', errors='replace').read().split('\n')
        i = 0
        n = len(lines)
        while i < n:
            m = DEF.match(lines[i])
            if m and not lines[i].rstrip().endswith(';') and ')' in lines[i]:
                name = m.group(2)
                if name in ('if', 'for', 'while', 'switch', 'return', 'sizeof', 'defined'):
                    i += 1
                    continue
                # comment block above: walk up over blank/comment lines
                j = i - 1
                above = []
                while j >= 0 and (lines[j].strip() == '' or lines[j].lstrip().startswith(('/*', '*', '//')) or lines[j].rstrip().endswith('*/')):
                    above.append(lines[j])
                    if lines[j].lstrip().startswith('/*') and not lines[j].rstrip().endswith('*/'):
                        pass
                    if lines[j].lstrip().startswith('/*'):
                        break
                    j -= 1
                above_text = '\n'.join(reversed(above))
                # body: until a line that is exactly '}' at column 0
                k = i + 1
                depth = lines[i].count('{') - lines[i].count('}')
                body = []
                while k < n:
                    body.append(lines[k])
                    depth += lines[k].count('{') - lines[k].count('}')
                    if lines[k].startswith('}') and depth <= 0:
                        break
                    k += 1
                body_text = '\n'.join(body)
                rows.append({
                    'file': rel, 'line': i + 1, 'name': name,
                    'above': sorted(set(ADDR.findall(above_text))),
                    'above_hex': sorted(set(HEX.findall(above_text))),
                    'body': sorted(set(ADDR.findall(body_text))),
                    'above_text': above_text[-400:],
                })
                i = k + 1
                continue
            i += 1

with open(out, 'w', encoding='utf-8') as fh:
    for r in rows:
        fh.write(json.dumps(r, ensure_ascii=False) + '\n')
cited = sum(1 for r in rows if r['above'] or r['above_hex'])
print(f"functions: {len(rows)}; with an address in the comment above: {cited}; with addresses in the body: {sum(1 for r in rows if r['body'])}")
