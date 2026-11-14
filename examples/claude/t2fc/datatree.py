#!/usr/bin/env python3
# The tree of a JSON or XML file for t2fc's F3, from Python's own parsers (json,
# xml.parsers.expat). Errors and warnings become rows where they occur: a parse
# error ends the tree there, under the element or after the value it stopped in,
# with the source lines around it.
#
#   python3 datatree.py json|xml FILE [MAXROWS]
#
# Prints one row a line: "DEPTH KIND SEG<tab>TEXT", KIND b a branch, l a leaf,
# e an error, w a warning, s a source line; SEG the row's part of the path
# (.key, [3], /tag[2]). The first line, "H <tab>TEXT", says what was read.
import sys, json, re
import xml.parsers.expat

MAXTEXT = 300
out = []
counts = {"e": 0, "w": 0}
maxrows = 50000


def clean(s):
    s = s.replace("\t", "\\t").replace("\r", "\\r").replace("\n", "\\n")
    s = "".join(c if (c >= " " and c != "\x7f") else "\\x%02x" % ord(c) for c in s)
    if len(s) > MAXTEXT:
        s = s[:MAXTEXT] + "\u2026"
    return s


def row(depth, kind, seg, text):
    if kind in counts:
        counts[kind] += 1
    out.append((depth, kind, clean(seg), clean(text)))


def source(depth, lines, line, col):
    # the line of the problem with the two before it, and a caret under the column
    for n in range(max(1, line - 2), line + 1):
        if n - 1 < len(lines):
            row(depth, "s", "", "%5d  %s" % (n, lines[n - 1].expandtabs(4)))
    if col > 0 and line - 1 < len(lines):
        pre = lines[line - 1][:col - 1].expandtabs(4)
        row(depth, "s", "", "       " + " " * len(pre) + "^")


def line_col(text, pos):
    line = text.count("\n", 0, pos) + 1
    return line, pos - (text.rfind("\n", 0, pos) + 1) + 1


# ---------------------------------------------------------------- JSON

class Obj(list):      # an object's members in their order, duplicates kept
    pass


class Num(str):       # a number as written
    pass


class Const(str):     # NaN, Infinity, -Infinity: Python takes them, JSON has none
    pass


IDENT = re.compile(r"^[A-Za-z_$][A-Za-z0-9_$]*$")


def jkey(k):
    return k if IDENT.match(k) else json.dumps(k, ensure_ascii=False)


def jscalar(v):
    if isinstance(v, (Num, Const)):
        return str(v)
    if v is True:
        return "true"
    if v is False:
        return "false"
    if v is None:
        return "null"
    return json.dumps(v, ensure_ascii=False)


def jvalue(depth, seg, label, v):
    if isinstance(v, Const):
        row(depth, "l", seg, label + str(v))
        row(depth, "w", "", "%s is not JSON: Python reads it, most readers do not" % v)
        return
    if isinstance(v, Obj):
        if not v:
            row(depth, "l", seg, label + "{}")
            return
        row(depth, "b", seg, label + "{%d}" % len(v))
        seen = set()
        for k, x in v:
            ks = "." + k if IDENT.match(k) else "[" + json.dumps(k, ensure_ascii=False) + "]"
            jvalue(depth + 1, ks, jkey(k) + ": ", x)
            if k in seen:
                row(depth + 1, "w", "", "duplicate key %s: most readers keep only the last"
                    % json.dumps(k, ensure_ascii=False))
            seen.add(k)
        return
    if isinstance(v, list):
        if not v:
            row(depth, "l", seg, label + "[]")
            return
        row(depth, "b", seg, label + "[%d]" % len(v))
        for i, x in enumerate(v):
            jvalue(depth + 1, "[%d]" % i, "[%d]: " % i, x)
        return
    row(depth, "l", seg, label + jscalar(v))


def do_json(data, name):
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError as e:
        line, col = line_col(data[:e.start].decode("utf-8", "replace"), e.start)
        row(0, "e", "", "line %d, column %d: not UTF-8 text (%s)" % (line, col, e.reason))
        return "JSON"
    if text.startswith("\ufeff"):
        row(0, "w", "", "the file starts with a byte order mark, which JSON does not allow")
        text = text[1:]
    lines = text.split("\n")
    dec = json.JSONDecoder(object_pairs_hook=Obj, parse_float=Num, parse_int=Num,
                           parse_constant=Const)
    lines_ok = name.lower().endswith((".jsonl", ".ndjson"))
    pos = 0
    n = 0
    ws = re.compile(r"[ \t\n\r]*")
    while True:
        pos = ws.match(text, pos).end()
        if pos >= len(text):
            break
        if n == 1 and not lines_ok:
            line, col = line_col(text, pos)
            row(0, "w", "", "line %d, column %d: a second value, after the end of the first: "
                "JSON has one (JSON Lines, .jsonl, has one a line)" % (line, col))
        try:
            v, end = dec.raw_decode(text, pos)
        except json.JSONDecodeError as e:
            row(0, "e", "", "line %d, column %d: %s" % (e.lineno, e.colno, e.msg))
            source(1, lines, e.lineno, e.colno)
            break
        except RecursionError:
            line, col = line_col(text, pos)
            row(0, "e", "", "line %d, column %d: nested too deep for Python's json" % (line, col))
            break
        n += 1
        jvalue(0, "", "", v)
        pos = end
    if n == 0 and counts["e"] == 0:
        row(0, "e", "", "the file holds no JSON value")
    return "JSON"


# ---------------------------------------------------------------- XML

class Node:
    def __init__(self, kind, text, line):
        self.kind = kind      # e element, t text, c comment, p instruction, d doctype, x error, w warning
        self.text = text
        self.line = line
        self.attrs = []
        self.kids = []
        self.extra = None     # an error's source position


def do_xml(data, name):
    root = Node("r", "", 0)
    stack = [root]
    ids = {}
    p = xml.parsers.expat.ParserCreate()
    p.buffer_text = True
    p.ordered_attributes = True

    def start(tag, attrs):
        n = Node("e", tag, p.CurrentLineNumber)
        n.attrs = list(zip(attrs[0::2], attrs[1::2]))
        stack[-1].kids.append(n)
        stack.append(n)
        for a, v in n.attrs:
            if a in ("id", "xml:id"):
                if v in ids:
                    w = Node("w", "id %s is used already, on line %d" % (json.dumps(v), ids[v]), 0)
                    n.kids.append(w)
                else:
                    ids[v] = p.CurrentLineNumber

    def end(tag):
        stack.pop()

    def chars(s):
        if s.strip():
            stack[-1].kids.append(Node("t", s.strip(), p.CurrentLineNumber))

    def comment(s):
        stack[-1].kids.append(Node("c", "<!--" + s + "-->", p.CurrentLineNumber))

    def pi(target, s):
        stack[-1].kids.append(Node("p", "<?%s %s?>" % (target, s), p.CurrentLineNumber))

    def doctype(name_, sys_id, pub_id, internal):
        t = "<!DOCTYPE " + name_
        if pub_id:
            t += ' PUBLIC "%s"' % pub_id
        if sys_id:
            t += ' "%s"' % sys_id
        stack[-1].kids.append(Node("d", t + ">", p.CurrentLineNumber))

    p.StartElementHandler = start
    p.EndElementHandler = end
    p.CharacterDataHandler = chars
    p.CommentHandler = comment
    p.ProcessingInstructionHandler = pi
    p.StartDoctypeDeclHandler = doctype
    try:
        p.Parse(data, True)
    except xml.parsers.expat.ExpatError as e:
        err = Node("x", "line %d, column %d: %s" % (e.lineno, e.offset + 1,
                   xml.parsers.expat.ErrorString(e.code)), e.lineno)
        err.extra = (e.lineno, e.offset + 1)
        stack[-1].kids.append(err)
    except RecursionError:
        stack[-1].kids.append(Node("x", "nested too deep for Python", 0))
    lines = data.decode("utf-8", "replace").split("\n")
    elements = [0]

    def emit(n, depth, idx):
        if n.kind == "x":
            row(depth, "e", "", n.text)
            if n.extra:
                source(depth + 1, lines, n.extra[0], n.extra[1])
            return
        if n.kind == "w":
            row(depth, "w", "", n.text)
            return
        if n.kind == "t":
            row(depth, "l", "/text()", json.dumps(n.text, ensure_ascii=False))
            return
        if n.kind in "cpd":
            row(depth, "l", "", n.text)
            return
        elements[0] += 1
        tag = "<" + n.text + "".join(' %s="%s"' % (a, v) for a, v in n.attrs)
        here = "/" + n.text + idx
        if not n.kids:
            row(depth, "l", here, tag + "/>")
            return
        if len(n.kids) == 1 and n.kids[0].kind == "t":
            row(depth, "l", here, tag + ">" + n.kids[0].text + "</" + n.text + ">")
            return
        row(depth, "b", here, tag + ">")
        same = {}
        for k in n.kids:
            if k.kind == "e":
                same[k.text] = same.get(k.text, 0) + 1
        nth = {}
        for k in n.kids:
            if k.kind == "e" and same[k.text] > 1:
                nth[k.text] = nth.get(k.text, 0) + 1
                emit(k, depth + 1, "[%d]" % nth[k.text])
            else:
                emit(k, depth + 1, "")

    for k in root.kids:
        emit(k, 0, "")
    if elements[0] == 0 and counts["e"] == 0:
        row(0, "e", "", "the file holds no XML element")
    return "XML"


def main():
    global maxrows
    if len(sys.argv) < 3 or sys.argv[1] not in ("json", "xml"):
        sys.stderr.write("usage: datatree.py json|xml FILE [MAXROWS]\n")
        return 2
    if len(sys.argv) > 3:
        maxrows = int(sys.argv[3])
    try:
        with open(sys.argv[2], "rb") as f:
            data = f.read()
    except OSError as e:
        row(0, "e", "", str(e))
        data = None
    what = sys.argv[1].upper()
    if data is not None:
        sys.setrecursionlimit(20000)
        what = (do_json if sys.argv[1] == "json" else do_xml)(data, sys.argv[2])
    shown = out
    cut = len(out) > maxrows
    if cut:
        shown = out[:maxrows - 1]
    info = "%s, read by Python %d.%d: %d rows" % (what, sys.version_info[0], sys.version_info[1], len(out))
    if counts["e"]:
        info += ", %d error%s" % (counts["e"], "s" if counts["e"] > 1 else "")
    if counts["w"]:
        info += ", %d warning%s" % (counts["w"], "s" if counts["w"] > 1 else "")
    w = sys.stdout.buffer
    w.write(("H \t" + info + "\n").encode("utf-8", "replace"))
    for d, k, s, t in shown:
        w.write(("%d %s %s\t%s\n" % (d, k, s, t)).encode("utf-8", "replace"))
    if cut:
        w.write(("0 w \tonly the first %d of %d rows are shown\n" % (maxrows - 1, len(out))).encode())
    return 0


sys.exit(main())
