#define G_SCREEN_MODE 1
#define G_SCREEN_SCALE 2
#define G_SCREEN_WIDTH 1024
#define G_SCREEN_HEIGHT 1024

#include "graphics.h"
#include "flt2.h"
#include "../ws.h"

Graph g;

#define HAT_PI 3.141592654
#define TRACE_ENTER(name) {printf("enter %s\n",name);}

#define GEOM_HAT 0
#define GEOM_META 1

#define LABEL_H1 0
#define LABEL_H  1
#define LABEL_T  2
#define LABEL_P  3
#define LABEL_F  4

#define TYPE_H 0
#define TYPE_T 1
#define TYPE_P 2
#define TYPE_F 3

#define HAT_POINTS 13
#define MAX_META_POINTS 6
#define MAX_CHILDREN 32
#define BUILD_LEVEL 5

float sw=G_SCREEN_WIDTH;
float sh=G_SCREEN_HEIGHT;
float hr3=0.8660254037844386;

struct SAffine
{
	float a;
	float b;
	float c;
	float d;
	float e;
	float f;
};

struct SGeom;

struct SChild
{
	SAffine T;
	SGeom* geom;
};

struct SMetaTile
{
	int num_pts;
	flt2 shape[MAX_META_POINTS];
	float width;
	int num_children;
	SChild children[MAX_CHILDREN];
};

struct SGeom
{
	int kind;
	int label;
	SMetaTile* meta;
};

struct SRule
{
	int mode;
	int a;
	int b;
	int c;
	int d;
	int e;
	int f;
};

struct SMetaTileLevel
{
	SMetaTile item[4];
};

struct SGeomLevel
{
	SGeom item[4];
};

flt2 hat_outline[HAT_POINTS];
SGeom hat_geoms[5];
SMetaTileLevel meta_levels[BUILD_LEVEL+1];
SGeomLevel geom_levels[BUILD_LEVEL+1];

int hat_colors[5]=
{
	0xff0089d4,
	0xff94cdeb,
	0xfffbfbfb,
	0xfffafafa,
	0xffbfbfbf
};

flt2 Pt(float x, float y)
{
	TRACE_ENTER("Pt");
	return flt2::New(x,y);
}

flt2 HexPt(float x, float y)
{
	TRACE_ENTER("HexPt");
	return flt2::New(x+0.5*y,hr3*y);
}

SAffine Affine(float a, float b, float c, float d, float e, float f)
{
	TRACE_ENTER("Affine");
	SAffine t;
	t.a=a;
	t.b=b;
	t.c=c;
	t.d=d;
	t.e=e;
	t.f=f;
	return t;
}

SAffine Ident()
{
	TRACE_ENTER("Ident");
	return Affine(1,0,0,0,1,0);
}

SAffine Translate(float tx, float ty)
{
	TRACE_ENTER("Translate");
	return Affine(1,0,tx,0,1,ty);
}

SAffine Rotate(float ang)
{
	TRACE_ENTER("Rotate");
	float c=cos(ang);
	float s=sin(ang);
	return Affine(c,-s,0,s,c,0);
}

SAffine Mul(SAffine A, SAffine B)
{
	TRACE_ENTER("Mul");
	return Affine(
		A.a*B.a + A.b*B.d,
		A.a*B.b + A.b*B.e,
		A.a*B.c + A.b*B.f + A.c,
		A.d*B.a + A.e*B.d,
		A.d*B.b + A.e*B.e,
		A.d*B.c + A.e*B.f + A.f
	);
}

SAffine Inv(SAffine T)
{
	TRACE_ENTER("Inv");
	float det=T.a*T.e-T.b*T.d;
	return Affine(
		T.e/det,
		-T.b/det,
		(T.b*T.f-T.c*T.e)/det,
		-T.d/det,
		T.a/det,
		(T.c*T.d-T.a*T.f)/det
	);
}

flt2 TransPt(SAffine M, flt2 P)
{
	TRACE_ENTER("TransPt");
	return flt2::New(
		M.a*P.x + M.b*P.y + M.c,
		M.d*P.x + M.e*P.y + M.f
	);
}

flt2 RotateVec(flt2 v, float ang)
{
	TRACE_ENTER("RotateVec");
	float c=cos(ang);
	float s=sin(ang);
	return flt2::New(c*v.x-s*v.y,s*v.x+c*v.y);
}

SAffine MatchSeg(flt2 p, flt2 q)
{
	TRACE_ENTER("MatchSeg");
	return Affine(
		q.x-p.x,
		p.y-q.y,
		p.x,
		q.y-p.y,
		q.x-p.x,
		p.y
	);
}

SAffine MatchTwo(flt2 p1, flt2 q1, flt2 p2, flt2 q2)
{
	TRACE_ENTER("MatchTwo");
	return Mul(MatchSeg(p2,q2),Inv(MatchSeg(p1,q1)));
}

SAffine RotAbout(flt2 p, float ang)
{
	TRACE_ENTER("RotAbout");
	return Mul(Translate(p.x,p.y),Mul(Rotate(ang),Translate(-p.x,-p.y)));
}

flt2 Intersect(flt2 p1, flt2 q1, flt2 p2, flt2 q2)
{
	TRACE_ENTER("Intersect");
	float d=(q2.y-p2.y)*(q1.x-p1.x)-(q2.x-p2.x)*(q1.y-p1.y);
	float uA=((q2.x-p2.x)*(p1.y-p2.y)-(q2.y-p2.y)*(p1.x-p2.x))/d;
	return flt2::New(
		p1.x + uA*(q1.x-p1.x),
		p1.y + uA*(q1.y-p1.y)
	);
}

float FMin(float a, float b)
{
	TRACE_ENTER("FMin");
	return a<b?a:b;
}

int GeomPointCount(SGeom* g0)
{
	TRACE_ENTER("GeomPointCount");
	if(g0->kind==GEOM_HAT)return HAT_POINTS;
	return g0->meta->num_pts;
}

flt2 GeomPoint(SGeom* g0, int idx)
{
	TRACE_ENTER("GeomPoint");
	if(g0->kind==GEOM_HAT)return hat_outline[idx];
	return g0->meta->shape[idx];
}

void MetaInit(SMetaTile& meta, int num_pts, float width)
{
	TRACE_ENTER("MetaInit");
	meta.num_pts=num_pts;
	meta.width=width;
	meta.num_children=0;
}

void AddChild(SMetaTile& meta, SAffine T, SGeom* geom)
{
	TRACE_ENTER("AddChild");
	meta.children[meta.num_children].T=T;
	meta.children[meta.num_children].geom=geom;
	meta.num_children++;
}

flt2 EvalChild(SMetaTile& meta, int child_idx, int pt_idx)
{
	TRACE_ENTER("EvalChild");
	SChild& ch=meta.children[child_idx];
	return TransPt(ch.T,GeomPoint(ch.geom,pt_idx));
}

void Recentre(SMetaTile& meta)
{
	TRACE_ENTER("Recentre");
	float cx=0;
	float cy=0;
	for(int i=0;i<meta.num_pts;i++)
	{
		cx+=meta.shape[i].x;
		cy+=meta.shape[i].y;
	}
	cx/=(float)meta.num_pts;
	cy/=(float)meta.num_pts;

	for(int i=0;i<meta.num_pts;i++)
	{
		meta.shape[i].x-=cx;
		meta.shape[i].y-=cy;
	}

	SAffine move=Translate(-cx,-cy);
	for(int i=0;i<meta.num_children;i++)
	{
		meta.children[i].T=Mul(move,meta.children[i].T);
	}
}

void InitHatOutline()
{
	TRACE_ENTER("InitHatOutline");
	hat_outline[0]=HexPt(0,0);
	hat_outline[1]=HexPt(-1,-1);
	hat_outline[2]=HexPt(0,-2);
	hat_outline[3]=HexPt(2,-2);
	hat_outline[4]=HexPt(2,-1);
	hat_outline[5]=HexPt(4,-2);
	hat_outline[6]=HexPt(5,-1);
	hat_outline[7]=HexPt(4,0);
	hat_outline[8]=HexPt(3,0);
	hat_outline[9]=HexPt(2,2);
	hat_outline[10]=HexPt(0,3);
	hat_outline[11]=HexPt(0,2);
	hat_outline[12]=HexPt(-1,2);
}

void InitHatGeoms()
{
	TRACE_ENTER("InitHatGeoms");
	for(int i=0;i<5;i++)
	{
		hat_geoms[i].kind=GEOM_HAT;
		hat_geoms[i].label=i;
		hat_geoms[i].meta=0;
	}
}

void WrapLevel(int level)
{
	TRACE_ENTER("WrapLevel");
	for(int i=0;i<4;i++)
	{
		geom_levels[level].item[i].kind=GEOM_META;
		geom_levels[level].item[i].label=-1;
		geom_levels[level].item[i].meta=&meta_levels[level].item[i];
	}
}

void InitLevel0()
{
	TRACE_ENTER("InitLevel0");
	WrapLevel(0);

	SMetaTile& H=meta_levels[0].item[TYPE_H];
	MetaInit(H,6,2);
	H.shape[0]=Pt(0,0);
	H.shape[1]=Pt(4,0);
	H.shape[2]=Pt(4.5,hr3);
	H.shape[3]=Pt(2.5,5*hr3);
	H.shape[4]=Pt(1.5,5*hr3);
	H.shape[5]=Pt(-0.5,hr3);
	AddChild(H,MatchTwo(hat_outline[5],hat_outline[7],H.shape[5],H.shape[0]),&hat_geoms[LABEL_H]);
	AddChild(H,MatchTwo(hat_outline[9],hat_outline[11],H.shape[1],H.shape[2]),&hat_geoms[LABEL_H]);
	AddChild(H,MatchTwo(hat_outline[5],hat_outline[7],H.shape[3],H.shape[4]),&hat_geoms[LABEL_H]);
	AddChild(H,
		Mul(
			Translate(2.5,hr3),
			Mul(
				Affine(-0.5,-hr3,0,hr3,-0.5,0),
				Affine(0.5,0,0,0,-0.5,0)
			)
		),
		&hat_geoms[LABEL_H1]);

	SMetaTile& T=meta_levels[0].item[TYPE_T];
	MetaInit(T,3,2);
	T.shape[0]=Pt(0,0);
	T.shape[1]=Pt(3,0);
	T.shape[2]=Pt(1.5,3*hr3);
	AddChild(T,Affine(0.5,0,0.5,0,0.5,hr3),&hat_geoms[LABEL_T]);

	SMetaTile& P=meta_levels[0].item[TYPE_P];
	MetaInit(P,4,2);
	P.shape[0]=Pt(0,0);
	P.shape[1]=Pt(4,0);
	P.shape[2]=Pt(3,2*hr3);
	P.shape[3]=Pt(-1,2*hr3);
	AddChild(P,Affine(0.5,0,1.5,0,0.5,hr3),&hat_geoms[LABEL_P]);
	AddChild(P,
		Mul(
			Translate(0,2*hr3),
			Mul(
				Affine(0.5,hr3,0,-hr3,0.5,0),
				Affine(0.5,0,0,0,0.5,0)
			)
		),
		&hat_geoms[LABEL_P]);

	SMetaTile& F=meta_levels[0].item[TYPE_F];
	MetaInit(F,5,2);
	F.shape[0]=Pt(0,0);
	F.shape[1]=Pt(3,0);
	F.shape[2]=Pt(3.5,hr3);
	F.shape[3]=Pt(3,2*hr3);
	F.shape[4]=Pt(-1,2*hr3);
	AddChild(F,Affine(0.5,0,1.5,0,0.5,hr3),&hat_geoms[LABEL_F]);
	AddChild(F,
		Mul(
			Translate(0,2*hr3),
			Mul(
				Affine(0.5,hr3,0,-hr3,0.5,0),
				Affine(0.5,0,0,0,0.5,0)
			)
		),
		&hat_geoms[LABEL_F]);
}

void ConstructPatch(SMetaTile& patch, SGeom* H, SGeom* T, SGeom* P, SGeom* F)
{
	TRACE_ENTER("ConstructPatch");
	SGeom* shapes[4];
	shapes[TYPE_H]=H;
	shapes[TYPE_T]=T;
	shapes[TYPE_P]=P;
	shapes[TYPE_F]=F;

	MetaInit(patch,0,H->meta->width);

	// Published H/T/P/F substitution translated from the official hatviz tool.
	SRule rules[29]=
	{
		{0,TYPE_H,0,0,0,0,0},
		{1,0,0,TYPE_P,2,0,0},
		{1,1,0,TYPE_H,2,0,0},
		{1,2,0,TYPE_P,2,0,0},
		{1,3,0,TYPE_H,2,0,0},
		{1,4,4,TYPE_P,2,0,0},
		{1,0,4,TYPE_F,3,0,0},
		{1,2,4,TYPE_F,3,0,0},
		{2,4,1,3,2,TYPE_F,0},
		{1,8,3,TYPE_H,0,0,0},
		{1,9,2,TYPE_P,0,0,0},
		{1,10,2,TYPE_H,0,0,0},
		{1,11,4,TYPE_P,2,0,0},
		{1,12,0,TYPE_H,2,0,0},
		{1,13,0,TYPE_F,3,0,0},
		{1,14,2,TYPE_F,1,0,0},
		{1,15,3,TYPE_H,4,0,0},
		{1,8,2,TYPE_F,1,0,0},
		{1,17,3,TYPE_H,0,0,0},
		{1,18,2,TYPE_P,0,0,0},
		{1,19,2,TYPE_H,2,0,0},
		{1,20,4,TYPE_F,3,0,0},
		{1,20,0,TYPE_P,2,0,0},
		{1,22,0,TYPE_H,2,0,0},
		{1,23,4,TYPE_F,3,0,0},
		{1,23,0,TYPE_F,3,0,0},
		{1,16,0,TYPE_P,2,0,0},
		{2,9,4,0,2,TYPE_T,2},
		{1,4,0,TYPE_F,3,0,0}
	};

	int nrules=sizeof(rules)/sizeof(SRule);
	for(int i=0;i<nrules;i++)
	{
		SRule& r=rules[i];
		if(r.mode==0)
		{
			AddChild(patch,Ident(),shapes[r.a]);
		}
		else if(r.mode==1)
		{
			SChild& ref=patch.children[r.a];
			int n=GeomPointCount(ref.geom);
			flt2 p0=TransPt(ref.T,GeomPoint(ref.geom,(r.b+1)%n));
			flt2 q0=TransPt(ref.T,GeomPoint(ref.geom,r.b));
			SGeom* nshp=shapes[r.c];
			int nn=GeomPointCount(nshp);
			AddChild(patch,MatchTwo(GeomPoint(nshp,r.d),GeomPoint(nshp,(r.d+1)%nn),p0,q0),nshp);
		}
		else
		{
			SChild& chP=patch.children[r.a];
			SChild& chQ=patch.children[r.c];
			flt2 p0=TransPt(chQ.T,GeomPoint(chQ.geom,r.d));
			flt2 q0=TransPt(chP.T,GeomPoint(chP.geom,r.b));
			SGeom* nshp=shapes[r.e];
			int nn=GeomPointCount(nshp);
			AddChild(patch,MatchTwo(GeomPoint(nshp,r.f),GeomPoint(nshp,(r.f+1)%nn),p0,q0),nshp);
		}
	}
}

void CopyPatchChild(SMetaTile& dst, SMetaTile& patch, int idx)
{
	TRACE_ENTER("CopyPatchChild");
	AddChild(dst,patch.children[idx].T,patch.children[idx].geom);
}

void ConstructMetatiles(SMetaTile& patch, SMetaTile& newH, SMetaTile& newT, SMetaTile& newP, SMetaTile& newF)
{
	TRACE_ENTER("ConstructMetatiles");
	flt2 bps1=EvalChild(patch,8,2);
	flt2 bps2=EvalChild(patch,21,2);
	flt2 rbps=TransPt(RotAbout(bps1,-2.0*HAT_PI/3.0),bps2);
	flt2 p72=EvalChild(patch,7,2);
	flt2 p252=EvalChild(patch,25,2);
	flt2 llc=Intersect(bps1,rbps,EvalChild(patch,6,2),p72);

	MetaInit(newH,6,patch.width*2);
	flt2 w=EvalChild(patch,6,2)-llc;
	newH.shape[0]=llc;
	newH.shape[1]=bps1;
	w=RotateVec(w,-HAT_PI/3.0);
	newH.shape[2]=newH.shape[1]+w;
	newH.shape[3]=EvalChild(patch,14,2);
	w=RotateVec(w,-HAT_PI/3.0);
	newH.shape[4]=newH.shape[3]-w;
	newH.shape[5]=EvalChild(patch,6,2);
	int hidx[10]={0,9,16,27,26,6,1,8,10,15};
	for(int i=0;i<10;i++)CopyPatchChild(newH,patch,hidx[i]);

	MetaInit(newP,4,patch.width*2);
	newP.shape[0]=p72;
	newP.shape[1]=p72+(bps1-llc);
	newP.shape[2]=bps1;
	newP.shape[3]=llc;
	int pidx[5]={7,2,3,4,28};
	for(int i=0;i<5;i++)CopyPatchChild(newP,patch,pidx[i]);

	MetaInit(newF,5,patch.width*2);
	newF.shape[0]=bps2;
	newF.shape[1]=EvalChild(patch,24,2);
	newF.shape[2]=EvalChild(patch,25,0);
	newF.shape[3]=p252;
	newF.shape[4]=p252+(llc-bps1);
	int fidx[6]={21,20,22,23,24,25};
	for(int i=0;i<6;i++)CopyPatchChild(newF,patch,fidx[i]);

	flt2 AAA=newH.shape[2];
	flt2 BBB=newH.shape[1]+(newH.shape[4]-newH.shape[5]);
	flt2 CCC=TransPt(RotAbout(BBB,-HAT_PI/3.0),AAA);
	MetaInit(newT,3,patch.width*2);
	newT.shape[0]=BBB;
	newT.shape[1]=CCC;
	newT.shape[2]=AAA;
	CopyPatchChild(newT,patch,11);

	Recentre(newH);
	Recentre(newP);
	Recentre(newF);
	Recentre(newT);
}

void BuildLevels()
{
	TRACE_ENTER("BuildLevels");
	InitHatOutline();
	InitHatGeoms();
	InitLevel0();

	for(int level=1;level<=BUILD_LEVEL;level++)
	{
		SMetaTile patch;
			ConstructPatch(
			patch,
			&geom_levels[level-1].item[TYPE_H],
			&geom_levels[level-1].item[TYPE_T],
			&geom_levels[level-1].item[TYPE_P],
			&geom_levels[level-1].item[TYPE_F]
		);
		ConstructMetatiles(
			patch,
			meta_levels[level].item[TYPE_H],
			meta_levels[level].item[TYPE_T],
			meta_levels[level].item[TYPE_P],
			meta_levels[level].item[TYPE_F]
		);
		WrapLevel(level);
	}
}

void PathPolygon(flt2* pts, int n, SAffine T)
{
	TRACE_ENTER("PathPolygon");
	flt2 p=TransPt(T,pts[0]);
	g.clear();
	g.M(p.x,p.y);
	for(int i=1;i<n;i++)
	{
		p=TransPt(T,pts[i]);
		g.L(p.x,p.y);
	}
	g.close();
	g.fin();
}

void DrawHat(int label, SAffine T)
{
	TRACE_ENTER("DrawHat");
	PathPolygon(hat_outline,HAT_POINTS,T);
	g.rgba32(hat_colors[label]);
	g.fill1();

	PathPolygon(hat_outline,HAT_POINTS,T);
	g.width(1.0,1.0);
	g.rgba32(0xff202020);
	g.stroke();
}

void DrawGeom(SGeom* geom, SAffine T)
{
	TRACE_ENTER("DrawGeom");
	if(geom->kind==GEOM_HAT)
	{
		DrawHat(geom->label,T);
	}
	else
	{
		SMetaTile& meta=*geom->meta;
		for(int i=0;i<meta.num_children;i++)
		{
			DrawGeom(meta.children[i].geom,Mul(T,meta.children[i].T));
		}
	}
}

void DrawOutline(SMetaTile& meta, SAffine T, unsigned int color, float w)
{
	TRACE_ENTER("DrawOutline");
	PathPolygon(&meta.shape[0],meta.num_pts,T);
	g.width(w,1.0);
	g.rgba32(color);
	g.stroke();
}

int CountHats(SGeom* geom)
{
	TRACE_ENTER("CountHats");
	if(geom->kind==GEOM_HAT)return 1;
	int n=0;
	SMetaTile& meta=*geom->meta;
	for(int i=0;i<meta.num_children;i++)
	{
		n+=CountHats(meta.children[i].geom);
	}
	return n;
}

SAffine MakeScreenTransform(SMetaTile* root)
{
	TRACE_ENTER("MakeScreenTransform");
	printf("MakeScreenTransform root ptr=%p\n",root);
	fflush(stdout);
	if(root==0)
	{
		printf("MakeScreenTransform root is null\n");
		fflush(stdout);
		return Ident();
	}
	printf("MakeScreenTransform root: num_pts=%i width=%f num_children=%i\n",root->num_pts,root->width,root->num_children);
	for(int i=0;i<root->num_pts;i++)
	{
		printf("MakeScreenTransform root.shape[%i]=(%f,%f)\n",i,root->shape[i].x,root->shape[i].y);
	}
	fflush(stdout);
	float minx=root->shape[0].x;
	float maxx=root->shape[0].x;
	float miny=root->shape[0].y;
	float maxy=root->shape[0].y;
	for(int i=1;i<root->num_pts;i++)
	{
		if(root->shape[i].x<minx)minx=root->shape[i].x;
		if(root->shape[i].x>maxx)maxx=root->shape[i].x;
		if(root->shape[i].y<miny)miny=root->shape[i].y;
		if(root->shape[i].y>maxy)maxy=root->shape[i].y;
	}

	float bw=maxx-minx;
	float bh=maxy-miny;
	float sc=0.88*FMin(sw/bw,sh/bh);
	float cx=(minx+maxx)*0.5;
	float cy=(miny+maxy)*0.5;
	return Affine(sc,0,sw*0.5-sc*cx,0,-sc,sh*0.5+sc*cy);
}

void RenderScene()
{
	TRACE_ENTER("RenderScene");
	SGeom* root=&geom_levels[BUILD_LEVEL].item[TYPE_H];
	printf("RenderScene root ptr=%p kind=%i label=%i meta=%p\n",root,root->kind,root->label,root->meta);
	fflush(stdout);
	SMetaTile* top=root->meta;
	SAffine screen=MakeScreenTransform(top);

	printf("hat tiling level %i, hats: %i\n", BUILD_LEVEL, CountHats(root));

	g.rgba32(0xfff6f3ea);
	g.FillRT();
	DrawGeom(root,screen);
	DrawOutline(*top,screen,0x80202020,2.5);
}

int main()
{
	TRACE_ENTER("main");
	BuildLevels();
	SetPresentWait(true);
	RenderScene();

	while(true)
	{
		Present();
		Poll();
		Wait(.01);
	}
	return 0;
}
