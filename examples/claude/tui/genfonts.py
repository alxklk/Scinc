#!/usr/bin/env python3
# Rewrites the interpreter's two bitmap fonts, ground/font9x16.h and ground/font6x12.h,
# and the web runtime's copy of the small one, webRT/SC_font6x12.js, so that both
# sizes hold the same 256 characters:
#   0x01..0x1F  the IBM PC symbols (smileys, card suits, arrows, ...), 0x7F the house
#   0x20..0x7E  ASCII, as the fonts had it
#   0x80..0xFF  Windows-1251: Ukrainian, Russian, Belarusian, Serbian, Macedonian
#               letters and the typographic signs (0x98, undefined there, is empty)
# 9x16 keeps every glyph it already had at the right place; the rest comes from
# genrfont.py (9x16) and genrfont6.py (6x12), which draw them in each font's style.
# Running it again gives the same files.
import os,sys,collections
here=os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0,here)
import genrfont as G9
import genrfont6
ground=os.path.join(here,'../../../../ground')
webrt=os.path.join(here,'../../../webRT')

layout=G9.layout

def blank(w,h): return [' '*w]*h

def font9():
    g=G9.g
    seen=collections.Counter(tuple(g[b]) for b in range(128,256))
    out=[]
    for b,u in sorted(layout().items()):
        if b<128 or (u is not None and seen[tuple(g[b])]==1 and u!=0xA0):
            out.append(g[b])            # the font's own glyph
        elif u is None or u==0xA0:
            out.append(blank(9,16))
        else:
            out.append(G9.glyphs[u])
    return out

def font6():
    path=os.path.join(ground,'font6x12.h')
    f=genrfont6.load(path)
    gl=genrfont6.build(path)
    for low in range(32,127): gl[low]=f[low]
    out=[]
    for b,u in sorted(layout().items()):
        if u is None or u==0xA0: out.append(blank(6,12))
        else: out.append(gl[u])
    return out

def check(glyphs,w,h):
    assert len(glyphs)==256
    for rs in glyphs: assert len(rs)==h and all(len(r)==w for r in rs),rs

def name(b,u):
    if u is None: return ''
    if b<32 or b>=127: return ' '+chr(u)
    return ''

def write9(glyphs):
    m=layout()
    out=["#pragma once","",
         "// 9x16, 256 characters: the IBM PC symbols at 0x01..0x1F, ASCII, and Windows-1251",
         "// above 0x7F. Written by examples/claude/tui/genfonts.py.","",
         "extern unsigned char font9x16Data[];","",
         "const SBitFont::rawFont font9x16={","\tSBitFont::FF_CHAR,","\t9,16,0,255,","\tfont9x16Data};","",
         "unsigned char font9x16Data[]={"]
    for b,rs in enumerate(glyphs):
        out.append("// #%d%s"%(b,name(b,m[b])))
        for r in rs: out.append('"%s"'%r)
    out.append("};")
    open(os.path.join(ground,'font9x16.h'),'w').write("\n".join(out)+"\n")

def write6(glyphs):
    m=layout()
    path=os.path.join(ground,'font6x12.h')
    src=open(path).read()
    # everything before the font itself (the digits fonts, the old character art) stays
    head=src[:src.index("extern unsigned char font6x12Data[];")]
    if "// 6x12, 256 characters" in head: head=head[:head.index("// 6x12, 256 characters")]
    out=[head.rstrip("\n"),"",
         "// 6x12, 256 characters: the IBM PC symbols at 0x01..0x1F, ASCII, and Windows-1251",
         "// above 0x7F. Written by examples/claude/tui/genfonts.py.",
         "extern unsigned char font6x12Data[];","",
         "const SBitFont::rawFont font6x12={","SBitFont::FF_BIN,","6,12,0,255,font6x12Data};","",
         "unsigned char font6x12Data[]={"]
    for b,rs in enumerate(glyphs):
        out.append("// #%d%s"%(b,name(b,m[b])))
        for r in rs: out.append('BIN8("%s"),'%r)
    out.append("};")
    open(path,'w').write("\n".join(out)+"\n")

def writejs(glyphs):
    out=["let font6x12="]
    rows=[r for rs in glyphs for r in rs]
    out+=['"%s"+'%r for r in rows[:-1]]
    out.append('"%s";'%rows[-1])
    open(os.path.join(webrt,'SC_font6x12.js'),'w').write("\n".join(out)+"\n")

if __name__=='__main__':
    a=font9(); check(a,9,16)
    b=font6(); check(b,6,12)
    write9(a); write6(b); writejs(b)
    print("font9x16.h, font6x12.h, SC_font6x12.js: 256 glyphs each")
