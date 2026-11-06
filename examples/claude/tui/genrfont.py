#!/usr/bin/env python3
# Builds tvrfont9.h and tvrfont6.h, the glyphs tvgfx.h's raster modes draw themselves
# because stext skips bytes above 127: from ground/font9x16.h plus glyphs drawn here,
# and for 6x12 from genrfont6.py.
import os,re,sys,unicodedata,collections
here=os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0,here)
src=open(os.path.join(here,'../../../../ground/font9x16.h')).read()
rows=re.findall(r'"([^"]*)"',src[src.index('font9x16Data[]={'):])
g=[rows[i*16:(i+1)*16] for i in range(256)]
low=[0,0x263A,0x263B,0x2665,0x2666,0x2663,0x2660,0x2022,0x25D8,0x25CB,0x25D9,0x2642,0x2640,0x266A,0x266B,0x263C,
     0x25BA,0x25C4,0x2195,0x203C,0x00B6,0x00A7,0x25AC,0x21A8,0x2191,0x2193,0x2192,0x2190,0x221F,0x2194,0x25B2,0x25BC]
# the 256 characters of both interpreter fonts (see genfonts.py): byte -> Unicode
def layout():
    m={0:None,0x7F:0x2302}
    for b in range(1,32): m[b]=low[b]
    for b in range(32,127): m[b]=b
    for b in range(128,256):
        try: m[b]=ord(bytes([b]).decode('cp1251'))
        except UnicodeDecodeError: m[b]=None
    return m
glyphs={}
for b in range(1,32): glyphs[low[b]]=g[b]
glyphs[0x2302]=g[127]
# the font once had one placeholder glyph in many places of 0x80..0xBF; it is left out
seen=collections.Counter(tuple(g[b]) for b in range(128,256))
for b in range(128,256):
    try: u=ord(bytes([b]).decode('cp1251'))
    except UnicodeDecodeError: continue
    if seen[tuple(g[b])]>1: continue
    if u>=0x80 and u not in glyphs: glyphs[u]=g[b]
glyphs[0xA0]=[' '*9]*16
def art(lines): 
    lines=[l.ljust(9,'.') for l in lines]
    return [l.replace('.',' ') for l in lines]
extra={
0x25A0:["",""]+[""]*3+[".........",
 "..*****..","..*****..","..*****..","..*****..","..*****.."]+[""]*6,
0x25CF:["","","",
 "...***...","..*****..",".*******.",".*******.",".*******.","..*****..","...***..."]+[""]*6,
0x2261:["","","","",
 ".*******.","",".*******.","",".*******."]+[""]*7,
}
for u,a in extra.items():
    a=(a+[""]*16)[:16]
    glyphs[u]=art(a)

# ---- Latin-1: accented letters from the font's own letters, the rest drawn here
def grid(rs): return [list(r) for r in rs]
def rows_of(gr): return ["".join(r) for r in gr]
ACC={
 'grave':["...**....","....**..."],
 'acute':["....**...","...**...."],
 'circ': ["...***...","..**.**.."],
 'tilde':["..***.**.",".**.***.."],
 'diaer':["..**.**..","........."],
 'ring': ["...***...","..*...*.."],
}
def put(gr,top,art):
    for i,l in enumerate(art):
        for j,ch in enumerate(l.ljust(9,'.')[:9]):
            if ch=='*': gr[top+i][j]='*'
def compose(base,acc,lower):
    gr=grid(g[ord(base)])
    if base=='i':
        for r in range(0,5): gr[r]=list(' '*9)
    put(gr,2 if lower else 0,ACC[acc])
    return rows_of(gr)
comp={}
for name,ch in [('grave','AEIOUaeiou'),('acute','AEIOUYaeiouy'),('circ','AEIOUaeiou'),('tilde','ANOano'),('diaer','AEIOUaeiouy'),('ring','Aa')]:
    for c in ch:
        u={'grave':0x300,'acute':0x301,'circ':0x302,'tilde':0x303,'diaer':0x308,'ring':0x30A}[name]
        cu=unicodedata.normalize('NFC',c+chr(u))
        if len(cu)==1 and 0xC0<=ord(cu)<=0xFF:
            comp[ord(cu)]=compose(c,name,c.islower())
# cedilla
for base,u in [('C',0xC7),('c',0xE7)]:
    gr=grid(g[ord(base)]); put(gr,12,["....**...","...**...."]); comp[u]=rows_of(gr)
# slashed O
for base,u,top in [('O',0xD8,2),('o',0xF8,5)]:
    gr=grid(g[ord(base)])
    for r in range(top,12):
        c=1+(11-r)*7//(11-top)
        gr[r][c]='*'
    comp[u]=rows_of(gr)
# Ð ð from D and d with a bar
gr=grid(g[ord('D')]); put(gr,6,["*****...."]); comp[0xD0]=rows_of(gr)
def flip(rs,top=2,bot=11,h=False):
    rs=list(rs); seg=rs[top:bot+1][::-1]
    if h: seg=[r[::-1] for r in seg]
    return rs[:top]+seg+rs[bot+1:]
comp[0xA1]=flip(g[ord('!')])
comp[0xBF]=flip(g[ord('?')],h=True)
def art16(top,lines):
    out=['.'*9]*16
    for i,l in enumerate(lines): out[top+i]=l.ljust(9,'.')[:9]
    return [r.replace('.',' ') for r in out]
hand={
0xA2:art16(3,["....*....","..*****..",".**.*.**.",".**.*....",".**.*....",".**.*.**.","..*****..","....*...."]),
0xA3:art16(2,["...***...","..**.**..","..**..*..","..**.....",".****....","..**.....","..**.....","..**.....",".***..**.","**.****.."]),
0xA5:art16(2,[".**...**.",".**...**.","..**.**..","...***...",".*******.","....*....",".*******.","....*....","....*....","...***..."]),
0xA8:art16(2,["..**.**.."]),
0xAA:art16(2,["..****...",".....**..","..*****..",".**..**..","..***.**.","","..******."]),
0xBA:art16(2,["..****...",".**..**..",".**..**..",".**..**..","..****...","",".******.."]),
0xAF:art16(1,[".*******."]),
0xB4:art16(2,["....**...","...**...."]),
0xB8:art16(12,["....**...","...**....","..**....."]),
0xB2:art16(2,["..****...",".**..**..","....**...","...**....","..**.....",".******.."]),
0xB3:art16(2,[".*****...","....**...","..***....","....**...",".**..**..","..****..."]),
0xB9:art16(2,["...**....","..***....","...**....","...**....","...**....","..****..."]),
0xBC:art16(2,[".**......","***......",".**...*..",".**..*...","****.....","...*.....","..*..**..",".*..*.*..","...*****.","......*.."]),
0xBD:art16(2,[".**......","***......",".**...*..",".**..*...","****.....","...*.....","..*.***..",".*....**.","....**...","...*****."]),
0xBE:art16(2,["***......","..**.....",".**...*..","..**.*...","***......","...*.....","..*..**..",".*..*.*..","...*****.","......*.."]),
0xD7:art16(5,[".**...**.","..**.**..","...***...","..**.**..",".**...**."]),
0xF7:art16(4,["...**....","...**....","","*******..","","...**....","...**...."]),
0xDF:art16(2,["..****...",".**..**..",".**..**..",".**.**...",".**..**..",".**...**.",".**...**.",".**...**.",".**..**..",".**.**..."]),
0xC6:art16(2,["..******.",".**.**...","**..**...","**..**...","**..****.","******...","**..**...","**..**...","**..**...","**..*****"]),
0xE6:art16(5,[".***.**..","...**.**.",".*******.","**.**....","**.**....","**.**.**.",".***.**.."]),
0xF0:art16(2,[".**.**...","..***....",".**.**...",".....**..","..******.",".**...**.",".**...**.",".**...**.",".**...**.","..*****.."]),
0xDE:art16(2,[".****....","..**.....","..*****..","..**..**.","..**..**.","..**..**.","..*****..","..**.....","..**.....",".****...."]),
0xFE:art16(2,[".***.....","..**.....","..**.....","..*****..","..**..**.","..**..**.","..**..**.","..**..**.","..**..**.","..*****..","..**.....","..**.....",".****...."]),
0xFF:None,
}
hand[0xFF]=compose('y','diaer',True)
def cyr(c): return grid(g[ord(c.encode('cp1251'))])
def withacc(gr,top,art):
    put(gr,top,art); return rows_of(gr)
uk={}
uk[0x401]=withacc(cyr('Е'),0,ACC['diaer'])
uk[0x451]=withacc(cyr('е'),2,ACC['diaer'])
uk[0x406]=g[ord('I')]
uk[0x456]=g[ord('i')]
gr=grid(g[ord('I')]); gr[0]=list(' '*9); gr[1]=list(' '*9); uk[0x407]=withacc(gr,0,ACC['diaer'])
gr=grid(g[ord('i')])
for r in range(0,5): gr[r]=list(' '*9)
uk[0x457]=withacc(gr,2,ACC['diaer'])
gr=cyr('Г'); put(gr,0,["......**.","......**."]); uk[0x490]=rows_of(gr)
gr=cyr('г'); put(gr,3,["......**.","......**."]); uk[0x491]=rows_of(gr)
breve=["..*...*..","...***..."]
uk[0x40E]=withacc(cyr('У'),0,breve)
uk[0x45E]=withacc(cyr('у'),2,breve)
gr=cyr('С'); put(gr,6,[".*****..."]); uk[0x404]=rows_of(gr)
gr=cyr('с'); put(gr,8,[".*****..."]); uk[0x454]=rows_of(gr)
# Serbian and Macedonian, the rest of Windows-1251
gr=cyr('Г'); uk[0x403]=withacc(gr,0,ACC['acute'])
gr=cyr('г'); uk[0x453]=withacc(gr,2,ACC['acute'])
gr=cyr('К'); uk[0x40C]=withacc(gr,0,ACC['acute'])
gr=cyr('к'); uk[0x45C]=withacc(gr,2,ACC['acute'])
for u,c in ((0x408,'J'),(0x458,'j'),(0x405,'S'),(0x455,'s')): uk[u]=g[ord(c)]
tsh=["...**....","...*****.","...**..**","...**..**","...**..**","...**..**","..****.**"]
uk[0x40B]=art16(2,[".******..",".*.**.*..","...**...."]+tsh)
uk[0x402]=art16(2,[".******..",".*.**.*..","...**...."]+tsh+[".......**",".....***."])
tshl=["..**.....","******...","..**.....","..**.**..","..***.**.","..**..**.","..**..**.","..**..**.","..**..**.",".***..**."]
uk[0x45B]=art16(2,tshl)
uk[0x452]=art16(2,tshl+["......**.","....***.."])
uk[0x40F]=art16(2,[".**...**."]*9+[".*******.","....*....","....*...."])
uk[0x45F]=art16(5,[".**...**."]*6+[".*******.","....*....","....*...."])
uk[0x409]=art16(2,["..****...",".**.**...",".**.**...",".**.**...",".**.****.",".**.**.**",".**.**.**",".**.**.**",".**.**.**","**..****."])
uk[0x40A]=art16(2,[".**.**...",".**.**...",".**.**...",".**.**...",".*******.",".**.**.**",".**.**.**",".**.**.**",".**.**.**",".**.****."])
uk[0x459]=art16(5,["..****...",".**.**...",".**.****.",".**.**.**",".**.**.**",".**.**.**","**..****."])
uk[0x45A]=art16(5,[".**.**...",".**.**...",".*******.",".**.**.**",".**.**.**",".**.**.**",".**.****."])
punct={
0x2013:art16(8,[".*******."]),
0x2014:art16(8,["*********"]),
0x2018:art16(2,["....**...","...**....","...**...."]),
0x2019:art16(2,["....**...","....**...","...**...."]),
0x201A:art16(10,["....**...","....**...","...**...."]),
0x201C:art16(2,["..**.**..",".**.**...",".**.**..."]),
0x201D:art16(2,["..**.**..","..**.**..",".**.**..."]),
0x201E:art16(10,["..**.**..","..**.**..",".**.**..."]),
0x2026:art16(10,["**.**.**.","**.**.**."]),
0x2020:art16(2,["...**....","...**....","*******..","...**....","...**....","...**....","...**....","...**....","...**....","...**...."]),
0x2021:art16(2,["...**....","*******..","...**....","...**....","...**....","...**....","...**....","*******..","...**....","...**...."]),
0x2030:art16(2,[".**...*..","*..*.*...",".**.*....","...*.....","..*......",".*.**.**.","*.*..*..*","...**.**."]),
0x2039:art16(5,["....**...","...**....","..**.....","...**....","....**..."]),
0x203A:art16(5,["..**.....","...**....","....**...","...**....","..**....."]),
0x20AC:art16(2,["...****..","..**..**.",".**......","*****....",".**......","*****....",".**......",".**......","..**..**.","...****.."]),
0x2116:art16(2,["**..*....","**..*....","***.*....","*.***.**.","*..**.**.","*..**.**.","*...*.**.","*...*....","*...*.**.","*...*...."]),
0x2122:art16(2,["***.*...*",".*..**.**",".*..*.*.*",".*..*...*"]),
}
for u,r in list(uk.items())+list(punct.items()): glyphs[u]=r
for u,r in list(comp.items())+list(hand.items()):
    if u not in glyphs: glyphs[u]=r
for u,rs in glyphs.items(): assert len(rs)==16,(hex(u),len(rs))
def write(name,glyphs,w,h,lines):
    codes=sorted(glyphs)
    bits=[]
    for u in codes:
        for r in glyphs[u]:
            v=0
            for i,ch in enumerate(r[:w]):
                if ch=='*': v|=1<<(w-1-i)
            bits.append(v)
    guard=name.upper().replace('.','_')
    out=["// "+l for l in lines]
    out.append("// Each glyph is %d rows of %d bits, the leftmost column in the highest bit;"%(h,w))
    out.append("// rfCode is sorted for a binary search. Made by genrfont.py, do not edit.")
    out.append("#ifndef %s\n#define %s\n"%(guard,guard))
    out.append("#define RF_N %d\n#define RF_W %d\n#define RF_H %d\n"%(len(codes),w,h))
    def wrap(vals,per):
        return ",\n".join("\t"+",".join(vals[i:i+per]) for i in range(0,len(vals),per))
    out.append("int rfCode[%d]={\n%s};\n"%(len(codes),wrap(["0x%04X"%u for u in codes],12)))
    out.append("int rfBits[%d]={\n%s};\n"%(len(bits),wrap([str(v) for v in bits],16)))
    out.append("#endif // %s"%guard)
    open(os.path.join(here,name),'w').write("\n".join(out)+"\n")
    print(name,len(codes),"glyphs, Cyrillic",sum(1 for u in codes if 0x400<=u<0x500),
          ", Latin-1",sum(1 for u in codes if 0xA0<=u<0x100))

def writemap():
    pairs=sorted((u,b) for b,u in layout().items() if u is not None and u>=0x80)
    def wrap(vals,per):
        return ",\n".join("\t"+",".join(vals[i:i+per]) for i in range(0,len(vals),per))
    out=["// Where the characters above ASCII are in the interpreter's two bitmap fonts,",
         "// which genfonts.py lays out alike: IBM PC symbols at 1..31 and 127, Windows-1251",
         "// above 127. rmCode is sorted for a binary search. Made by genrfont.py, do not edit.",
         "#ifndef TVRMAP_H\n#define TVRMAP_H\n",
         "#define RM_N %d\n"%len(pairs),
         "int rmCode[%d]={\n%s};\n"%(len(pairs),wrap(["0x%04X"%u for u,b in pairs],12)),
         "int rmByte[%d]={\n%s};\n"%(len(pairs),wrap([str(b) for u,b in pairs],16)),
         "#endif // TVRMAP_H"]
    open(os.path.join(here,'tvrmap.h'),'w').write("\n".join(out)+"\n")
    print('tvrmap.h',len(pairs),'characters')

if __name__=='__main__':
  # the fonts draw these themselves, so the tables leave them out
  infont=set(u for u in layout().values() if u is not None)
  writemap()
  write('tvrfont9.h',{u:r for u,r in glyphs.items() if u not in infont},9,16,[
    "9x16 glyphs for tvgfx.h's raster mode, for the characters the interpreter's",
    "font9x16 does not have (see tvrmap.h for those it has): Latin-1 letters and",
    "signs, drawn in the font's style."])

  import genrfont6
  g6=genrfont6.build(os.path.join(here,'../../../../ground/font6x12.h'))
  write('tvrfont6.h',{u:r for u,r in g6.items() if u not in infont},6,12,[
    "6x12 glyphs for tvgfx.h's small raster mode, for the characters the interpreter's",
    "font6x12 does not have (see tvrmap.h for those it has): Latin-1 letters and",
    "signs, drawn in genrfont6.py."])
