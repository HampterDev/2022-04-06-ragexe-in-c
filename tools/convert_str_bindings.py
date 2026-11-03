"""One-off converter: writes data/effects/effects.json from the C sources of purist 2239d84.

It reads
  - the STR file choice of rag_effect_init (src/effects/rag/rag_str_anim.c),
  - the RagEffect dispatch cases that only play their STR, plus waves, quakes, following and a
    duration (src/effects/rag/rag_effect_process*.c and the helpers they call),
  - the EZ2STR type-to-STR table and its special cases (src/effects/skills/ez2str_effect_files.c),
and writes them as the "RagEffect" and "Ez2StrFile" tables of the effect data file. With
--cases it also prints the dispatch case groups it converted, by file and line, and the ones
it kept with the reason; with --apply it deletes the converted case groups from the C files.

Run it from the repository root against the tree it was written for; it refuses a case body
it does not understand rather than guessing, and the effect trace harness
(tests/test_effect_trace.c) must give the same digests before and after the C code it replaces
is deleted. After that deletion this script has nothing left to read.

    python tools/convert_str_bindings.py [--out data/effects/effects.json] [--cases] [--apply]
"""

import argparse
import json
import re

RAG = "src/effects/rag/"
DISPATCH = [
    (RAG + "rag_effect_process.c", "rag_effect_on_process"),
    (RAG + "rag_effect_process.c", "rag_effect_process300"),
    (RAG + "rag_effect_process_491.c", "rag_process_491"),
    (RAG + "rag_effect_process_800.c", "rag_process_800"),
]
HELPER_FILES = [RAG + f for f in (
    "rag_effect_process.c", "rag_effect_process_491.c", "rag_effect_process_800.c", "rag_unreferenced.c",
    "rag_process_moves.c", "rag_skills_000_019.c", "rag_skills_020_039.c", "rag_skills_040_079.c",
    "rag_skills_080_119.c", "rag_skills_120_219.c", "rag_skills_220_259.c", "rag_skills_920_2373.c")]


class Refused(Exception):
    pass


# ---- tokens ----

TOKEN = re.compile(r"""
    (?P<ws>\s+|/\*.*?\*/|//[^\n]*)
  | (?P<str>"(?:[^"\\\n]|\\.)*")
  | (?P<chr>'(?:[^'\\\n]|\\.)*')
  | (?P<num>0[xX][0-9a-fA-F]+[uUlL]*|(?:\d+\.\d*|\.\d+|\d+)(?:[eE][-+]?\d+)?[fFuUlL]*)
  | (?P<id>[A-Za-z_]\w*)
  | (?P<op>->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^]=|.)
""", re.S | re.X)


class Tok:
    def __init__(self, kind, text, line):
        self.kind, self.text, self.line = kind, text, line

    def __repr__(self):
        return self.text


def tokenize(src):
    out, pos, line = [], 0, 1
    while pos < len(src):
        m = TOKEN.match(src, pos)
        if not m:
            raise ValueError("cannot tokenize at line %d" % line)
        kind = m.lastgroup
        text = m.group()
        if kind != "ws" and not (kind == "op" and text == "#"):
            out.append(Tok(kind, text, line))
        if kind == "op" and text == "#":
            # skip the preprocessor line
            end = src.find("\n", pos)
            line += 1
            pos = end + 1
            continue
        line += text.count("\n")
        pos = m.end()
    return out


def c_string(tokens):
    """The value of adjacent C string literals, escapes decoded."""
    value = ""
    for t in tokens:
        if t.kind != "str":
            raise Refused("not a string literal: %s" % " ".join(x.text for x in tokens))
        body = t.text[1:-1]
        i = 0
        while i < len(body):
            c = body[i]
            if c == "\\":
                n = body[i + 1]
                value += {"\\": "\\", '"': '"', "'": "'", "n": "\n", "t": "\t"}[n]
                i += 2
            else:
                value += c
                i += 1
    return value


def c_float(tokens):
    text = "".join(t.text for t in tokens)
    m = re.fullmatch(r"(-?)(\d+(?:\.\d*)?)[fF]?", text)
    if not m:
        raise Refused("not a float constant: %s" % text)
    return float(m.group(1) + m.group(2))


def c_int(tokens):
    text = "".join(t.text for t in tokens)
    if not re.fullmatch(r"-?(\d+|0[xX][0-9a-fA-F]+)", text):
        raise Refused("not an integer constant: %s" % text)
    return int(text, 0)


# ---- statements ----

def match_paren(toks, i):
    """toks[i] is an opening bracket; returns the index after its closing partner."""
    pairs = {"(": ")", "{": "}", "[": "]"}
    want = [pairs[toks[i].text]]
    i += 1
    while want:
        t = toks[i].text
        if t in pairs:
            want.append(pairs[t])
        elif t == want[-1]:
            want.pop()
        i += 1
    return i


def parse_statements(toks):
    stmts, i = [], 0
    while i < len(toks):
        s, i = parse_statement(toks, i)
        stmts.append(s)
    return stmts


def parse_statement(toks, i):
    t = toks[i].text
    if t == "{":
        end = match_paren(toks, i)
        return ("block", parse_statements(toks[i + 1:end - 1])), end
    if t == "if":
        end = match_paren(toks, i + 1)
        cond = toks[i + 2:end - 1]
        body, j = parse_statement(toks, end)
        if j < len(toks) and toks[j].text == "else":
            raise Refused("else branch")
        return ("if", cond, body), j
    if t in ("for", "while", "do", "switch", "goto"):
        raise Refused("loop or switch")
    j = i
    while toks[j].text != ";":
        if toks[j].text in "({[":
            j = match_paren(toks, j)
        else:
            j += 1
    expr = toks[i:j]
    if t == "return":
        return ("return", expr[1:]), j + 1
    if t == "break":
        return ("break",), j + 1
    return ("expr", expr), j + 1


def split_args(toks):
    args, depth, cur = [], 0, []
    for t in toks:
        if t.text in "({[":
            depth += 1
        elif t.text in ")}]":
            depth -= 1
        if t.text == "," and depth == 0:
            args.append(cur)
            cur = []
        else:
            cur.append(t)
    if cur:
        args.append(cur)
    return args


def as_call(expr):
    if len(expr) >= 3 and expr[0].kind == "id" and expr[1].text == "(" and match_paren(expr, 1) == len(expr):
        return expr[0].text, split_args(expr[2:-1])
    return None


def text(expr):
    return " ".join(t.text for t in expr)


# ---- functions of the sources ----

FUNC = re.compile(r"^(?:static\s+)?(?:unsigned char|void|int)\s+(\w+)\(struct RagEffect \*self\)\s*$")


def read_functions(paths):
    """Functions taking only self, by name: their body tokens."""
    funcs = {}
    for path in paths:
        src = open(path, encoding="utf-8").read()
        toks = tokenize(src)
        lines = src.split("\n")
        for i, t in enumerate(toks):
            if t.kind == "id" and i + 1 < len(toks) and toks[i + 1].text == "(":
                if FUNC.match(lines[t.line - 1].strip()) and FUNC.match(lines[t.line - 1].strip()).group(1) == t.text:
                    j = match_paren(toks, i + 1)
                    if j < len(toks) and toks[j].text == "{":
                        end = match_paren(toks, j)
                        funcs[t.text] = toks[j + 1:end - 1]
    return funcs


# ---- the STR play of one case body ----

STEP_FUNCS = {"rag_str_only", "str_step", "step", "rag_str_step_up80"}


class Play:
    def __init__(self):
        self.timeline = []          # [at or None, after_str, [ops]]
        self.offset = None
        self.end = None             # "Stop", "Continue", "Rewind"
        self.stop_frame = None

    def cue(self, at, ops):
        if ops:
            self.timeline.append([at, self.offset is not None, ops])

    def json(self):
        out = {}
        if self.timeline:
            tl = []
            for at, after, ops in self.timeline:
                e = {}
                if at is not None:
                    e["At"] = at
                if after:
                    e["AfterStr"] = True
                e["Do"] = ops
                tl.append(e)
            out["Timeline"] = tl
        if self.offset != (0.0, -80.0, 0.0):
            out["Offset"] = [number(v) for v in self.offset]
        if self.end != "Stop":
            out["OnStrEnd"] = self.end
        if self.stop_frame is not None:
            out["StopFrame"] = self.stop_frame
        return out


def number(value):
    """A float constant as JSON writes it best: 40 rather than 40.0."""
    return int(value) if value == int(value) else value


def wave_op(name_tokens, height=0.0):
    op = {"Wave": c_string(name_tokens)}
    if height != 0.0:
        op["Height"] = number(height)
    return op


class Interpreter:
    def __init__(self, funcs):
        self.funcs = funcs

    def ops_of(self, stmt):
        """Operations of a statement inside a frame condition (no STR step)."""
        if stmt[0] == "block":
            ops = []
            for s in stmt[1]:
                ops += self.ops_of(s)
            return ops
        if stmt[0] != "expr":
            raise Refused("statement in a cue: %s" % stmt[0])
        expr = stmt[1]
        m = re.fullmatch(r"self -> duration = (\d+)", text(expr))
        if m:
            return [{"Duration": int(m.group(1))}]
        call = as_call(expr)
        if not call:
            raise Refused("expression in a cue: %s" % text(expr))
        name, args = call
        if name == "rag_follow_master":
            return [{"Follow": True}]
        if name == "rag_play_wave_here":
            return [wave_op(args[1])]
        if name == "rag_play_wave":
            return [wave_op(args[1], c_float(args[2]))]
        if name == "follow_wave":
            return [{"Follow": True}, wave_op(args[1], c_float(args[2]))]
        if name in ("sound_play_wave",):
            if text(sum(args[1:], [])) != "0.0f 0.0f 0.0f 250 40 1.0f":
                raise Refused("sound_play_wave arguments")
            return [{"Wave": c_string(args[0]), "AtPlayer": True}]
        if name == "rag_wave_plain":
            return [{"Wave": c_string(args[0]), "AtPlayer": True}]
        if name == "rag_quake":
            return [{"Quake": 0}]
        if name == "quake":
            return [{"Quake": c_int(args[0])}]
        if name in ("mode_message",) and text(args[0]) == "GAME_MSG_QUAKE":
            return [{"Quake": c_int(args[1])}]
        if name == "game_send_message" and text(args[0]) == "GAME_MSG_QUAKE" and \
                text(sum(args[2:], [])) == "0 0 0":
            return [{"Quake": c_int(args[1])}]
        raise Refused("call in a cue: %s" % name)

    def frame_eq(self, cond):
        m = re.fullmatch(r"self -> frame == (\d+)", text(cond))
        return int(m.group(1)) if m else None

    def step_value(self, play, expr):
        """An expression whose value is the STR step result; performs the step."""
        t = text(expr)
        t = re.sub(r"^\( unsigned char \) ", "", t)
        t = re.sub(r" != 0$", "", t)
        toks = tokenize(t)
        call = as_call(toks)
        if not call:
            raise Refused("step expression: %s" % t)
        name, args = call
        if name in STEP_FUNCS and len(args) == 1:
            return self.do_step(play, (0.0, -80.0, 0.0))
        if name == "rag_str_step":
            return self.do_step(play, tuple(c_float(a) for a in args[1:]))
        if name == "wave_then_step":
            play.cue(c_int(args[1]), [wave_op(args[2])])
            return self.do_step(play, (0.0, -80.0, 0.0))
        if name == "follow_wave_str":
            play.cue(c_int(args[1]), [{"Follow": True}, wave_op(args[2], c_float(args[3]))])
            return self.do_step(play, (0.0, -80.0, 0.0))
        if name == "rag_str_step_or_stop":
            return "step_or_stop"
        if len(args) == 1 and name in self.funcs:
            return self.inline_value(play, name)
        raise Refused("step function %s" % name)

    def do_step(self, play, offset):
        if play.offset is not None:
            raise Refused("two STR steps")
        play.offset = offset
        return "step"

    def inline_value(self, play, name):
        """Inlines a helper whose body is cue statements then `return <step>;`."""
        stmts = parse_statements(self.funcs[name])
        for s in stmts[:-1]:
            self.cue_statement(play, s)
        last = stmts[-1]
        if last[0] != "return":
            raise Refused("helper %s does not end with a return" % name)
        return self.step_value(play, last[1])

    def cue_statement(self, play, s):
        if s[0] == "if":
            at = self.frame_eq(s[1])
            if at is None:
                raise Refused("condition %s" % text(s[1]))
            play.cue(at, self.ops_of(s[2]))
        elif s[0] == "expr":
            call = as_call(s[1])
            if call and len(call[1]) == 1 and call[0] in self.funcs and call[0] not in STEP_FUNCS:
                self.inline_void(play, call[0])
            elif call and call[0] == "rag_wave_at":
                play.cue(c_int(call[1][1]), [{"Follow": True}, wave_op(call[1][2])])
            else:
                play.cue(None, self.ops_of(s))
        else:
            raise Refused("statement %s" % s[0])

    def inline_void(self, play, name):
        """Inlines a void helper: cue statements, optionally behind `if (self->frame != N) return;`."""
        stmts = parse_statements(self.funcs[name])
        at = None
        if stmts and stmts[0][0] == "if" and stmts[0][2] == ("return", []):
            m = re.fullmatch(r"self -> frame != (\d+)", text(stmts[0][1]))
            if not m:
                raise Refused("guard of %s" % name)
            at = int(m.group(1))
            stmts = stmts[1:]
        if at is None:
            for s in stmts:
                if s[0] == "if" and s[2][0] == "expr" and as_call(s[2][1]) and \
                        as_call(s[2][1])[0] == "rag_str_rewind":
                    # if (!step) rag_str_rewind(self);
                    self.ending(play, s)
                    return
                self.cue_statement(play, s)
        else:
            ops = []
            for s in stmts:
                ops += self.ops_of(s)
            play.cue(at, ops)

    def ending(self, play, s):
        cond, body = s[1], s[2]
        if cond[0].text != "!":
            raise Refused("ending condition %s" % text(cond))
        value = self.step_value(play, cond[1:])
        if body == ("return", [Tok("num", "0", 0)]) or (body[0] == "return" and text(body[1]) == "0"):
            play.end = "Stop"
            if value == "step_or_stop":
                self.step_or_stop(play)
        elif body[0] == "expr" and as_call(body[1]) and as_call(body[1])[0] == "rag_str_rewind":
            if value != "step":
                raise Refused("rewind of %s" % value)
            play.end = "Rewind"
        else:
            raise Refused("ending body")

    def step_or_stop(self, play):
        body = text(self.funcs["rag_str_step_or_stop"])
        if body != ("if ( ! rag_str_step ( self , 0.0f , -80.0f , 0.0f ) ) { self -> frame = 9999 ; "
                    "return 0 ; } return 1 ;").replace("-80.0f", "- 80.0f"):
            raise Refused("rag_str_step_or_stop changed: %s" % body)
        self.do_step(play, (0.0, -80.0, 0.0))
        play.stop_frame = 9999

    def case_body(self, stmts):
        """The play of a dispatch case body, or Refused."""
        play = Play()
        if not stmts:
            raise Refused("empty case")
        i = 0
        while i < len(stmts):
            s = stmts[i]
            last = i == len(stmts) - 1
            if s[0] == "break":
                if not last:
                    raise Refused("break before the end")
                break
            if s[0] == "return":
                if not last:
                    raise Refused("return before the end")
                value = self.step_value(play, s[1])
                play.end = "Stop"
                if value == "step_or_stop":
                    self.step_or_stop(play)
                break
            if s[0] == "if" and s[1] and s[1][0].text == "!":
                self.ending(play, s)
            elif s[0] == "expr" and self.is_step_statement(s[1]):
                if self.step_value(play, s[1]) != "step":
                    raise Refused("value step")
                play.end = "Continue"
            else:
                self.cue_statement(play, s)
            i += 1
        else:
            raise Refused("no break or return")
        if play.offset is None:
            raise Refused("no STR step")
        if play.end is None:
            # a helper that steps and rewinds, used as a statement
            raise Refused("no ending")
        return play

    def is_step_statement(self, expr):
        call = as_call(expr)
        if not call:
            return False
        name, args = call
        if name in STEP_FUNCS or name == "rag_str_step":
            return True
        if len(args) == 1 and name in self.funcs:
            body = parse_statements(self.funcs[name])
            return bool(body) and body[-1][0] == "return" and "rag_str_step" in text(self.funcs[name]) \
                and name != "rag_str_or_rewind"
        return False


# ---- the dispatch switches ----

def function_body(toks, name):
    for i, t in enumerate(toks):
        if t.text == name and i + 1 < len(toks) and toks[i + 1].text == "(":
            j = match_paren(toks, i + 1)
            if j < len(toks) and toks[j].text == "{":
                return toks[j + 1:match_paren(toks, j) - 1]
    raise ValueError("no function " + name)


def case_groups(body):
    """(labels, body tokens, line of the first label) of every case group of a switch."""
    for i, t in enumerate(body):
        if t.text == "switch":
            end = match_paren(body, i + 1)
            if body[end].text == "{":
                inner = body[end + 1:match_paren(body, end) - 1]
                break
    groups, i = [], 0
    labels, start, first_line = [], None, None
    while i < len(inner):
        t = inner[i]
        if t.text in ("case", "default"):
            if t.text == "case":
                j = i + 1
                while inner[j].text != ":":
                    j += 1
                label = c_int(inner[i + 1:j])
            else:
                j = i + 1
                label = "default"
            if start is not None and i > start:
                groups.append((labels, inner[start:i], first_line))
                labels = []
            if not labels:
                first_line = t.line
            labels.append(label)
            start = j + 1
            i = j + 1
            continue
        if t.text in "({[":
            i = match_paren(inner, i)
        else:
            i += 1
    groups.append((labels, inner[start:], first_line))
    return groups


# ---- the RagEffect STR choice ----

def rag_init_files(path):
    src = open(path, encoding="utf-8").read()
    toks = tokenize(src)
    body = function_body(toks, "rag_effect_init")
    files = {}
    for labels, case, _ in case_groups(body):
        if labels == ["default"]:
            continue
        entry = {}
        for s in parse_statements(case):
            if s[0] == "break":
                continue
            t = text(s[1])
            m = re.fullmatch(r"name = FULL_OR_MIN \( (.*) , (.*) \)", t)
            if m:
                full, mini = split_args(tokenize(m.group(1) + " , " + m.group(2)))
                entry["File"] = c_string(full)
                entry["MinFile"] = c_string(mini)
                continue
            if t.startswith("name = "):
                entry["File"] = c_string(s[1][2:])
                continue
            m = re.fullmatch(r"variant = roll \( (\d+) \)", t)
            if m:
                entry["Variants"] = int(m.group(1))
                continue
            m = re.fullmatch(r"self -> str_max_loops = (\d+)", t)
            if m:
                entry["Loops"] = int(m.group(1))
                continue
            raise Refused("rag_effect_init case %s: %s" % (labels, t))
        order = ("File", "MinFile", "Variants", "Loops")
        for label in labels:
            files[label] = {k: entry[k] for k in order if k in entry}
    return files


# ---- the EZ2STR table ----

def ez2str_files(path):
    src = open(path, encoding="utf-8").read()
    rows = {}
    toks = tokenize(src)
    i = 0
    # the str_files[] rows
    start = next(k for k, t in enumerate(toks) if t.text == "str_files")
    j = start
    while toks[j].text != "{":
        j += 1
    table = toks[j + 1:match_paren(toks, j) - 1]
    for row in split_args(table):
        name, args = as_call(row)
        type_ = c_int(args[0])
        if name == "LOAD":
            rows[type_] = {"File": c_string(args[1])}
        elif name == "OPEN":
            rows[type_] = {"File": c_string(args[1]), "Open": True}
        elif name == "FULL_ONLY":
            rows[type_] = {"File": c_string(args[1]), "FullOnly": True}
        elif name == "MINEFFECT":
            f = c_string(args[1])
            rows[type_] = {"File": f, "MinFile": "mineffect\\" + f}
        elif name == "MINEFFECT2":
            rows[type_] = {"File": c_string(args[2]), "MinFile": c_string(args[1])}
        else:
            raise Refused("EZ2STR row " + name)
    # the special cases of ez2_str_effect_open_for_type, checked word for word
    body = text(function_body(toks, "ez2_str_effect_open_for_type"))
    expected = [
        'case 141 : snprintf ( s , sizeof s , "Pneuma%d.str" , rand_below ( 3 ) + 1 ) ; '
        'ez2_str_particle_load ( p , s ) ; break ;',
        'case 1163 : { snprintf ( s , sizeof s , "MeteorStorm\\\\meteor%d.str" , rand_below ( 3 ) + 1 ) ; '
        'load ( p , s ) ; break ; }',
        'case 1244 : case 1245 : case 1246 : case 1247 : case 1248 : case 1249 : case 1250 : case 1251 : '
        'case 1252 : case 1253 : snprintf ( s , sizeof s , "test_effect\\\\%02d\\\\%02d.str" , type - 1244 , '
        'type - 1244 ) ; ez2_str_particle_load ( p , s ) ; break ;',
    ]
    for e in expected:
        if e not in body:
            raise Refused("ez2_str_effect_open_for_type changed near: " + e[:40])
    for t in (141, 1163, 1412) + tuple(range(1244, 1254)):
        if t in rows:
            raise Refused("table row %d is shadowed by a special case" % t)
    rows[141] = {"File": "Pneuma%d.str", "Variants": 3}
    rows[1163] = {"File": "MeteorStorm\\meteor%d.str", "Variants": 3}
    for t in range(1244, 1254):
        rows[t] = {"File": "test_effect\\%02d\\%02d.str" % (t - 1244, t - 1244)}
    return rows


# ---- main ----

def live_switch(effect_id, p491_ids):
    if 300 <= effect_id <= 373:
        return "rag_effect_process300"
    if effect_id >= 491:
        return "rag_process_491" if effect_id in p491_ids else "rag_process_800"
    return "rag_effect_on_process"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="data/effects/effects.json")
    ap.add_argument("--cases", action="store_true", help="print the converted case groups")
    ap.add_argument("--apply", action="store_true", help="delete the converted case groups from the C files")
    args = ap.parse_args()

    funcs = read_functions(HELPER_FILES)
    interp = Interpreter(funcs)
    rag = {i: {"Str": f} for i, f in rag_init_files(RAG + "rag_str_anim.c").items()}

    switches = {}
    for path, fname in DISPATCH:
        toks = tokenize(open(path, encoding="utf-8").read())
        switches[fname] = (path, case_groups(function_body(toks, fname)))
    p491_ids = {l for labels, _, _ in switches["rag_process_491"][1] for l in labels if l != "default"}

    converted, refused, dead = [], {}, []
    for fname, (path, groups) in switches.items():
        for labels, body, first_line in groups:
            ids = [l for l in labels if l != "default"]
            if not ids or "default" in labels:
                continue
            live = [i for i in ids if live_switch(i, p491_ids) == fname]
            if len(live) != len(ids):
                dead += [(fname, i) for i in ids if i not in live]
            if not live:
                continue
            try:
                play = interp.case_body(parse_statements(body))
            except Refused as e:
                refused[tuple(ids)] = str(e)
                continue
            if len(live) != len(ids):
                raise SystemExit("case group %s of %s mixes live and dead IDs" % (ids, fname))
            for i in ids:
                rag.setdefault(i, {})["Play"] = play.json()
            converted.append((path, fname, ids, first_line, body[-1].line))

    data = {
        "Format": "ragexe effect data 1",
        "RagEffect": {str(k): rag[k] for k in sorted(rag)},
        "Ez2StrFile": {str(k): v for k, v in sorted(ez2str_files("src/effects/skills/ez2str_effect_files.c").items())},
    }
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write(dump(data))
    plays = sum(1 for v in rag.values() if "Play" in v)
    print("RagEffect: %d IDs with an STR file, %d IDs played from data (%d case groups); EZ2STR: %d types."
          % (sum(1 for v in rag.values() if "Str" in v), plays, len(converted), len(data["Ez2StrFile"])))
    if dead:
        print("dead cases (their ID is dispatched elsewhere): %s" % dead)
    if args.cases:
        for path, fname, ids, first, last in converted:
            print("%s %s lines %d-%d: %s" % (path, fname, first, last, ids))
        for ids, why in sorted(refused.items()):
            print("kept %s: %s" % (list(ids), why))
    if args.apply:
        delete_cases(converted)


def delete_cases(converted):
    """Deletes the lines of the converted case groups; a group must own whole lines."""
    by_path = {}
    for path, fname, ids, first, last in converted:
        by_path.setdefault(path, []).append((first, last, ids))
    for path, ranges in by_path.items():
        data = open(path, "rb").read()
        crlf = b"\r\n" in data
        lines = data.split(b"\r\n" if crlf else b"\n")
        drop = set()
        for first, last, ids in ranges:
            head = lines[first - 1].lstrip()
            after = lines[last].strip() if last < len(lines) else b""
            if not head.startswith(b"case "):
                raise SystemExit("%s:%d: group %s does not start its line" % (path, first, ids))
            if after and not (after.startswith(b"case ") or after.startswith(b"default") or after == b"}"):
                raise SystemExit("%s:%d: group %s does not end its line" % (path, last, ids))
            drop.update(range(first - 1, last))
        kept = []
        for n, line in enumerate(lines):
            if n in drop:
                continue
            if not line.strip() and kept and not kept[-1].strip() and (n - 1) in drop:
                continue
            kept.append(line)
        open(path, "wb").write((b"\r\n" if crlf else b"\n").join(kept))
        print("%s: deleted %d case groups, %d lines" % (path, len(ranges), len(lines) - len(kept)))


def dump(data):
    """JSON with one table row per line."""
    out = ["{"]
    out.append('  "Format": %s,' % json.dumps(data["Format"]))
    for table in ("RagEffect", "Ez2StrFile"):
        rows = data[table]
        out.append('  "%s": {' % table)
        keys = list(rows)
        for n, k in enumerate(keys):
            comma = "," if n + 1 < len(keys) else ""
            out.append("    %s: %s%s" % (json.dumps(k), json.dumps(rows[k], ensure_ascii=False,
                                                                separators=(", ", ": ")), comma))
        out.append("  }," if table == "RagEffect" else "  }")
    out.append("}")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    main()
