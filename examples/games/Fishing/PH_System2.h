#define NSeg 240

typedef float PH_Num;

struct PH_Num3
{
	float x;
	float y;
	float z;
};

struct PH_Node
{
	PH_Num  m;// mass
	PH_Num3 v;// speed
	PH_Num3 f;// force
	PH_Num3 xf;// force
	PH_Num3 p;// position
	PH_Num3 p0;// previous position
	PH_Num3 a;// acceleration
	bool uw;
	bool fix;
	int nl;
};

struct PH_Link
{
	int n0;
	int n1;
	void Set(int in0, int in1)
	{
		n0=in0;
		n1=in1;
	}
};

class PH_System2
{
	float AWDempf;
	float UWDempf;
	float seg_len;

	void Reset()
	{
		AWDempf=0.9975f;
		UWDempf=0.5f;
		seg_len=1.5;
	}
}

#ifdef UNDEFINED

typedef float PH_Num;
typedef float2 PH_Num2;
typedef float3 PH_Num3;

class PH_System2
{
public:
	float AWDempf=0.9975f;
	float UWDempf=0.5f;
	float seg_len=1.5;
	struct PH_Node
	{
		PH_Num  m;// mass
		PH_Num3 v;// speed
		PH_Num3 f;// force
		PH_Num3 xf;// force
		PH_Num3 p;// position
		PH_Num3 p0;// previous position
		PH_Num3 a;// acceleration
		bool uw;
		bool fix;
		int nl;
	};

	struct PH_Link
	{
		int n0;
		int n1;
		void Set(int in0, int in1)
		{
			n0=in0;
			n1=in1;
		}
	};

	PH_Node* nodes;
	int NNodes;
	int nt;
	int tp;
	PH_Link* links;
	int NLinks;

	
	PH_Num area;
	PH_Num2 center;
	PH_Num pressure;
	PH_Num speed;
	bool force;
	PH_Num2 forcepos;
	PH_Num2 forcedir;

	PH_Num minx;
	PH_Num miny;
	PH_Num maxx;
	PH_Num maxy;

	PH_Num3 tiePos;
	PH_Num3 tieForce;
	

	int undoN;


	void Extends()
	{
		if(NNodes)
		{
			PH_System2::PH_Node& n0=nodes[0];
			minx=n0.p.x;
			miny=n0.p.y;
			maxx=n0.p.x;
			maxy=n0.p.y;
			for(int i=1; i<NNodes; i++)
			{
				PH_System2::PH_Node& n=nodes[i];
				minx=Min(minx, n.p.x);
				miny=Min(miny, n.p.y);
				maxx=Max(maxx, n.p.x);
				maxy=Max(maxy, n.p.y);
			}
		}
	}

	float Length()
	{
		float len=0;
		for(int i=0;i<NLinks;i++)
		{
			PH_Node& n0=nodes[links[i].n0];
			PH_Node& n1=nodes[links[i].n1];

			PH_Num3 d=(n1.p-n0.p);
			len+=d.length();
		}
		return len;
	}

	void CalcPositions(PH_Num dt)
	{
		speed=0;
		for(int i=0;i<NNodes;i++)
		{
			float line=0;
			PH_Node& n0=nodes[i];
			
			{
				PH_Num dempf;
				PH_Num3 vd=(n0.p-n0.p0);
				PH_Num v=vd.length();
				
				if(i<NNodes-2)
				{
					PH_Node& n1=nodes[i];
					float3 dp=n1.p-n0.p;
					if((dp.length()>=0.001f)&&(vd.length()>=0.001))
					{
						dp.normalize();
						vd.normalize();
						line=1-Fabs(vdot(dp,vd));
					}
				}


				if(n0.p.y<0)
				{
					dempf=0.8/(1+line);
				}
				else
				{
//					dempf=0.9975;
					dempf=0.9975;
				}
				if(i==0)
					dempf=0.6;
				PH_Num3 np=n0.p+(n0.p-n0.p0)*dempf+n0.a*(dt*dt);
				n0.p0=n0.p;
				n0.p=np;
			}
		}

		//length constraints
		for(int j=0;j<10;j++)
/*		for(int i=1;i<NLinks;i++)
		{
			PH_Node& n0=nodes[links[i].n0];
			PH_Node& n1=nodes[links[i].n1];*/

		for(int i=1;i<NNodes-1;i++)
		{
			PH_Node& n0=nodes[i];
			PH_Node& n1=nodes[i+1];
			PH_Num3 d=(n1.p-n0.p);
			PH_Num3 dn=d.normalized();
			//			d.normalize();
			PH_Num f=d.length()-seg_len;
			d=dn*f*0.5;
			n0.p+=d;
			n1.p-=d;
		}

	
		// straighting
		{
			for(int i=1;i<NNodes-1;i++)
			{
				PH_Num3 dp=((nodes[i-1].p+nodes[i+1].p)*0.5f-nodes[i].p)*0.25f;
				nodes[i].p+=dp;
				nodes[i].p0+=dp;
			}
		}


	};

	void CollectForces()
	{

		// elasticity
		if(1)
		for(int i=0;i<NLinks;i++)
		{
			PH_Node& n0=nodes[links[i].n0];
			PH_Node& n1=nodes[links[i].n1];

			PH_Num3 d=(n1.p-n0.p)*1.0f;
			PH_Num3 dn=d.normalized();
			//			d.normalize();
			PH_Num f=d.length()-seg_len;
			if(i==0)
				d=dn*f*4.0f;
			else
				d=dn*f*160.0f;
			n0.f+=d;
			n1.f-=d;
		}
/*
		{
			PH_Node& n1=nodes[0];

			PH_Num3 d=(n1.p-tiePos)*1.0f;
			PH_Num3 dn=d.normalized();
			//			d.normalize();
			PH_Num f=d.length()-seg_len;
			d=dn*f*160.0f;
			tieForce=d;
			n1.f-=d;
		}
*/

		// gravity
		if(1)
		{
			for(int i=0;i<NNodes;i++)
			{
				PH_Node& n0=nodes[i];
				n0.f.y-=n0.m*9.8*7;
			}
		}

		//float
		if(1)
		{
			for(int i=0;i<NNodes;i++)
			{
				PH_Node& n0=nodes[i];
				if(n0.p.y<0)
				{
					n0.f.y+=2;
					n0.uw=true;
				}
				else
					n0.uw=false;
			}
		}

		// straighting force
		if(1)
		{
			for(int i=1;i<NNodes-1;i++)
			{
				nodes[i].f+=((nodes[i-1].p+nodes[i+1].p)*0.5f-nodes[i].p)*0.3f;
			}
		}

		// acceleration
		for(int i=0;i<NNodes;i++)
		{
			PH_Node& n0=nodes[i];
			n0.a=n0.f/n0.m;
		}
		nodes[NNodes-1].a=nodes[NNodes-1].f/2;
		
	}

	void Clean()
	{
		for(int i=0;i<NNodes;i++)
		{
			PH_Node& n0=nodes[i];
			n0.f=n0.xf;
		}
	}


	void Step(PH_Num dt)
	{
		Clean();
		CollectForces();
		CalcPositions(dt);
	}

	PH_System2()
	{
		nodes=0;
		links=0;
		Reset();
	}

	void Reset()
	{
		{
			NNodes=NSeg;
			nodes=new PH_Node[NNodes];
			for(int i=0;i<NNodes;i++)
			{
				PH_Node& n0=nodes[i];
				n0.v=0;
				n0.f=0;
				n0.xf=0;
				n0.fix=false;
				n0.a=0;
				n0.m=.002;
				n0.nl=0;
				n0.p=n0.p0=PH_Num3(i,i/3.0f,10.0-i/(float)NNodes);
				n0.fix=false;
				n0.nl=0;
				n0.f=0;
				n0.a=0;
				n0.v=0;
			}
			nodes[NNodes-1].m=50;
			nodes[0].m=20;
			tiePos=nodes[0].p;
			NLinks=NNodes-1;
			links=new PH_Link[NLinks];
			for(int i=0;i<NLinks;i++)
			{
				links[i].Set(i,i+1);
			}
		}
	}

	~PH_System2()
	{
		delete [] nodes;
		delete [] links;
	}

};

#endif