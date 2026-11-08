#!/usr/bin/env python3
# Builds tvglfont9.h and tvglfont6.h for tvgl.h, the OpenGL backend, which puts every
# glyph into one texture atlas and so needs the bitmaps themselves: the 256
# characters of the interpreter's fonts (ground/font9x16.h, ground/font6x12.h, as
# genfonts.py writes them) and the Latin-1 letters and signs of tvrfont9.h and
# tvrfont6.h, in one table sorted by Unicode code. Run genfonts.py first after glyph
# changes, then this.
import os,re,sys
here=os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0,here)
import genrfont as G9
import genrfont6
ground=os.path.join(here,'../../../../ground')

def font_rows(path,w,h):
    s=open(path).read()
    if w==9:
        rows=re.findall(r'"([^"]*)"',s[s.index('font9x16Data[]={'):])
    else:
        rows=re.findall(r'BIN8\("([^"]*)"\)',s[s.index('font6x12Data[]={'):])
    assert len(rows)==256*h,(path,len(rows))
    return [[r.ljust(w)[:w] for r in rows[b*h:(b+1)*h]] for b in range(256)]

def table(w,h,extra):
    glyphs={}
    rows=font_rows(os.path.join(ground,'font%dx%d.h'%(w,h)),w,h)
    for b,u in G9.layout().items():
        if u is not None: glyphs[u]=rows[b]
    for u,r in extra.items():
        if u not in glyphs: glyphs[u]=r
    glyphs[0xA0]=[' '*w]*h
    return glyphs

def write(name,glyphs,w,h):
    codes=sorted(glyphs)
    bits=[]
    for u in codes:
        for r in glyphs[u]:
            v=0
            for i,ch in enumerate(r[:w]):
                if ch=='*': v|=1<<(w-1-i)
            bits.append(v)
    guard=name.upper().replace('.','_')
    def wrap(vals,per):
        return ",\n".join("\t"+",".join(vals[i:i+per]) for i in range(0,len(vals),per))
    out=["// %dx%d glyphs for tvgl.h's texture atlas: the interpreter's font%dx%d (ASCII,"%(w,h,w,h),
         "// IBM PC symbols, Windows-1251) and the Latin-1 letters and signs it lacks.",
         "// Each glyph is %d rows of %d bits, the leftmost column in the highest bit;"%(h,w),
         "// glCode is sorted for a binary search. Made by genglfont.py, do not edit.",
         "#ifndef %s\n#define %s\n"%(guard,guard),
         "#define GL_N %d\n#define GL_W %d\n#define GL_H %d\n"%(len(codes),w,h),
         "int glCode[%d]={\n%s};\n"%(len(codes),wrap(["0x%04X"%u for u in codes],12)),
         "int glBits[%d]={\n%s};\n"%(len(bits),wrap([str(v) for v in bits],16)),
         "#endif // %s"%guard]
    open(os.path.join(here,name),'w').write("\n".join(out)+"\n")
    print(name,len(codes),"glyphs")

if __name__=='__main__':
    write('tvglfont9.h',table(9,16,G9.glyphs),9,16)
    write('tvglfont6.h',table(6,12,genrfont6.build(os.path.join(ground,'font6x12.h'))),6,12)
