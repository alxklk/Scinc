#!/usr/bin/env python3
# Writes the molecules of the Objects screen saver into tvtoys.h, between the
# "molecules (genmols.py)" markers: 3D shapes from RDKit (embedding, then the MMFF
# force field), centred and turned so that the longest way lies along x. Small
# molecules only (twelve atoms at most: bigger ones look noisy at this size),
# then two pieces of crystal, built straight from their lattices.
# Needs RDKit (pip install rdkit). Running it again gives the same table.
import os
import itertools
import numpy as np
from rdkit import Chem
from rdkit.Chem import AllChem, rdMolDescriptors

MOLS=[("Water","O"),("Ethanol","CCO"),("Methane","C"),("Ammonia","N"),
      ("Carbon dioxide","O=C=O"),("Acetylene","C#C"),("Formaldehyde","C=O"),
      ("Benzene","c1ccccc1")]

def prune(p,c,r,bond):
    """Lattice points p within r of c, less those hanging by fewer than two bonds."""
    q=p[np.linalg.norm(p-c,axis=1)<r]
    while True:
        d=np.linalg.norm(q[:,None]-q[None],axis=2)
        n=((d<bond)&(d>0.1)).sum(1)
        if (n>=2).all():
            return q
        q=q[n>=2]

def diamond():
    """Fused cages of the diamond lattice about an empty middle: 26 carbons."""
    a=3.567
    p=[]
    for i,j,k in itertools.product(range(-2,3),repeat=3):
        for f in ([0,0,0],[0,.5,.5],[.5,0,.5],[.5,.5,0]):
            for b in ([0,0,0],[.25,.25,.25]):
                p.append((np.array([i,j,k])+f+b)*a)
    return prune(np.array(p),np.array([.5,.5,.5])*a,3.5,1.7)

def graphene():
    """Seven hexagons of a graphene sheet (coronene's carbons): 24 atoms."""
    cc=1.42
    p=[]
    for i,j in itertools.product(range(-6,7),repeat=2):
        b=np.array([i*1.5*cc,(i*0.5+j)*np.sqrt(3)*cc,0.])
        p+=[b,b+[cc,0.,0.]]
    return prune(np.array(p),np.array([cc/2,np.sqrt(3)/2*cc,0.]),4.,1.6)

# name, formula, carbon positions, bond order
CRYSTALS=[("Diamond","C",diamond(),1),("Graphene","C",graphene(),4)]

EL={'H':0,'C':1,'N':2,'O':3,'Cl':4}
BEGIN="// ---- molecules (genmols.py)"
END="// ---- end of molecules"

def table():
    atoms=[];bonds=[];a0=[0];b0=[0];forms=[]
    for name,smi in MOLS:
        m=Chem.AddHs(Chem.MolFromSmiles(smi))
        assert AllChem.EmbedMolecule(m,randomSeed=11)==0
        AllChem.MMFFOptimizeMolecule(m,maxIters=2000)
        p=m.GetConformer().GetPositions()
        p=p-p.mean(0)
        w,v=np.linalg.eigh(np.cov(p.T))
        p=p@v[:,::-1]
        for a,q in zip(m.GetAtoms(),p):
            atoms+=[EL[a.GetSymbol()]]+[int(round(c*1000)) for c in q]
        for b in m.GetBonds():
            o=4 if b.GetIsAromatic() else int(b.GetBondTypeAsDouble()+0.5)
            bonds+=[b.GetBeginAtomIdx(),b.GetEndAtomIdx(),o]
        a0.append(a0[-1]+m.GetNumAtoms())
        b0.append(b0[-1]+m.GetNumBonds())
        f=rdMolDescriptors.CalcMolFormula(m)
        forms.append({"H3N":"NH3"}.get(f,f))
    for name,form,p,order in CRYSTALS:
        p=p-p.mean(0)
        w,v=np.linalg.eigh(np.cov(p.T))
        p=p@v[:,::-1]
        for q in p:
            atoms+=[1]+[int(round(c*1000)) for c in q]
        d=np.linalg.norm(p[:,None]-p[None],axis=2)
        nb=0
        for i in range(len(p)):
            for j in range(i+1,len(p)):
                if d[i,j]<1.7:
                    bonds+=[i,j,order]
                    nb+=1
        a0.append(a0[-1]+len(p))
        b0.append(b0[-1]+nb)
        forms.append(form)
    def arr(name,v,n):
        rows=['\t'+','.join(map(str,v[i:i+n]))+',' for i in range(0,len(v),n)]
        rows[-1]=rows[-1].rstrip(',')
        return 'int %s[%d]={\n%s};'%(name,len(v),'\n'.join(rows))
    return '\n'.join([BEGIN,
        "// Atoms: element (0 H, 1 C, 2 N, 3 O, 4 Cl), x, y, z in thousandths of an",
        "// angstrom about the middle. Bonds: two atoms, numbered within the molecule,",
        "// and the order (4 aromatic). The molecule's atoms and bonds start at",
        "// obMolA0[n] and obMolB0[n]. The pieces of crystal come last.",
        "#define OB_MOLS %d"%(len(MOLS)+len(CRYSTALS)),
        "#define OB_CRYST %d    // the first piece of crystal"%len(MOLS),
        arr('obMolAtom',atoms,16),arr('obMolBond',bonds,18),
        'int obMolA0[%d]={%s};'%(len(a0),','.join(map(str,a0))),
        'int obMolB0[%d]={%s};'%(len(b0),','.join(map(str,b0))),
        'char* obMolNames="%s";'%'|'.join([n for n,s in MOLS]+[c[0] for c in CRYSTALS]),
        'char* obMolForms="%s";'%'|'.join(forms),
        END])

if __name__=='__main__':
    path=os.path.join(os.path.dirname(os.path.abspath(__file__)),'tvtoys.h')
    src=open(path).read()
    i=src.index(BEGIN)
    j=src.index(END,i)+len(END)
    open(path,'w').write(src[:i]+table()+src[j:])
    print("tvtoys.h: %d molecules, %d crystals"%(len(MOLS),len(CRYSTALS)))
