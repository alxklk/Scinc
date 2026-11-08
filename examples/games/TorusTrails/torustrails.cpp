// Torus Trails - light-trail racing in a three-dimensional wrapped arena.
//
// The arena is a 32x32x32 grid of cells that wraps around on every axis (a
// three-torus): fly out through one face and you come back in through the
// opposite one. Every ship leaves a solid trail one cell wide behind it, and
// any ship that flies into a trail is wrecked. Shots blast a hole into trails
// (and wreck a ship they hit), but ammunition is scarce: two shots to start,
// and gold pickups give one more each (five at most).
//
// Keys: W/S or Up/Down turn up/down, A/D or Left/Right turn left/right,
// Space fires, drag with the mouse to look around (it does not steer), I
// inverts up/down, P pauses, F toggles full screen, Enter starts the next
// round, Esc quits.
//
// Turns happen at cell centres, so the whole world is made of straight runs.
// Drawing: the trails live in one vertex array that only grows (one slot of
// 24 vertices per finished trail cell). Positions are stored as the cell's
// centre (sent as the normal) plus an offset inside the cell; the vertex
// shader wraps the centre to the image nearest to the camera, so the wrapped
// world needs no copies, and fog hides everything past half the arena.
//
// Needs ../../claude/opengl/scgl_dl.so (run ../../claude/opengl/build_so.sh).

#define SCGL_DL "../../claude/opengl/scgl_dl"
#include "../../claude/opengl/scgl.h"
#include "../../claude/tui/tvglfont9.h"

#define GN 32          // cells along each axis
#define NC 32768       // GN^3
#define TH 0.34        // half the thickness of a trail tube
#define MAXS 6000      // trail cell slots
#define MAXSV 144000   // MAXS*24 vertices
#define MAXSH 6        // ships: 0 is the player
#define MAXP 24        // projectiles
#define MAXK 3         // ammo pickups
#define MAXQ 900       // explosion particles
#define NLAT 96        // lattice line vertices (48 lines)
#define NDUST 400

// ---------------------------------------------------------------- tables
int DX[6] = {1, -1, 0, 0, 0, 0};
int DY[6] = {0, 0, 1, -1, 0, 0};
int DZ[6] = {0, 0, 0, 0, 1, -1};
int BITS[6] = {1, 2, 4, 8, 16, 32};
float shipCol[18] = {0.25, 0.9, 1.0,   1.0, 0.55, 0.12,   1.0, 0.25, 0.9,
                     0.55, 1.0, 0.2,   1.0, 0.2, 0.2,     0.6, 0.4, 1.0};
char* shipName[6] = {"YOU", "ORANGE", "MAGENTA", "LIME", "RED", "VIOLET"};

// ---------------------------------------------------------------- grid
int occ[NC];     // 0 free, else owner+1 (a trail cell or a ship's current cell)
int slotOf[NC];  // geometry slot of a finished trail cell, -1 for none
int conn[NC];    // sides joined to the neighbouring trail cells (BITS by direction)
int vis[NC];     // flood-fill stamps
int visStamp = 0;
int bq[200];     // flood-fill queue

// trail geometry: 24 vertices per slot (6 faces, a joined side is collapsed)
float tv[432000];  // offset inside the cell
float tn[432000];  // cell centre
float tt[288000];  // joined-sides mask, owner
int freeS[MAXS];
int nFree = 0;
int highS = 0;

// ---------------------------------------------------------------- ships
int shX[MAXSH];
int shY[MAXSH];
int shZ[MAXSH];
int shF[MAXSH];     // flying direction (0..5)
int shU[MAXSH];     // up direction
int shIn[MAXSH];    // direction it entered the current cell in
int shInC[MAXSH];   // 1 while that cell is joined to the trail behind it
int shCP[MAXSH];    // 1 once past the centre of the current cell
int shAlive[MAXSH];
int shAmmo[MAXSH];
int shKills[MAXSH];
float shT[MAXSH];   // -0.5..0.5 along shF from the cell centre
float shFade[MAXSH];
float shDeadT[MAXSH];
float shCool[MAXSH];
int shGone[MAXSH];  // its trail was removed after the crash
int tq[24];         // queued turns, 4 per ship
int tqn[MAXSH];

// ---------------------------------------------------------------- shots, pickups, particles
int pOn[MAXP];
int pX[MAXP];
int pY[MAXP];
int pZ[MAXP];
int pD[MAXP];
int pR[MAXP];
int pO[MAXP];
float pT[MAXP];

int kOn[MAXK];
int kX[MAXK];
int kY[MAXK];
int kZ[MAXK];
float kTimer[MAXK];

int qOn[MAXQ];
float qX[MAXQ];
float qY[MAXQ];
float qZ[MAXQ];
float qVX[MAXQ];
float qVY[MAXQ];
float qVZ[MAXQ];
float qL[MAXQ];
float qR[MAXQ];
float qG[MAXQ];
float qB[MAXQ];
int qNext = 0;

// ---------------------------------------------------------------- scenery
float latV[288];
float latN[288];
float latC[384];
float dustV[1200];
float dustN[1200];
float dustC[1600];

// ---------------------------------------------------------------- game state
int seed = 12345;
int winW = 1280;
int winH = 800;
int prog = 0;
int uCam = 0;
int uGridN = 0;
int uMode = 0;
int uCol = 0;
int uFade = 0;
int uFogEnd = 0;
int uTh = 0;
int fontTex = 0;
int fontPix[13824];  // 96 glyphs of 9x16 in one row
float fadeArr[6];

int round = 0;
int wins = 0;
int losses = 0;
int roundOver = 0;   // 1 lost, 2 won
float roundT = 0.0;
float countdown = 3.0;
float speed = 5.5;
int paused = 0;
int invertY = 0;

float camQ[4];
float tgtQ[4];
float camR[3];
float camU[3];
float camB[3];
float camP[3];
float viewM[16];
float lookYaw = 0.0;
float lookPitch = 0.0;

int logV[5];
int logK[5];
int logKind[5];   // 0 crashed into a trail, 1 shot
float logT[5];

// scratch
float lo3[3];
float hi3[3];
float pv3[3];
int gF = 0;
int gU = 0;
float sbF[3];
float sbR[3];
float sbU[3];
char* numBuf;

// ======================================================================= helpers
int Rnd(int n)
{
	seed = seed * 1103515245 + 12345;
	int r = (seed >> 16) & 32767;
	return r % n;
}

float Rf()
{
	return Rnd(10000) / 10000.0;
}

int Wrap(int a)
{
	int r = a % GN;
	if (r < 0)
		r = r + GN;
	return r;
}

// the shortest signed distance from a to b around the ring
int WD(int d)
{
	int r = Wrap(d);
	if (r >= GN / 2)
		r = r - GN;
	return r;
}

float WDf(float d)
{
	while (d >= GN / 2)
		d = d - GN;
	while (d < -GN / 2)
		d = d + GN;
	return d;
}

int Idx(int x, int y, int z)
{
	return Wrap(x) + GN * (Wrap(y) + GN * Wrap(z));
}

int Opp(int d)
{
	if (d % 2 == 0)
		return d + 1;
	return d - 1;
}

int DirOf(int x, int y, int z)
{
	if (x > 0) return 0;
	if (x < 0) return 1;
	if (y > 0) return 2;
	if (y < 0) return 3;
	if (z > 0) return 4;
	return 5;
}

int Cross(int a, int b)
{
	int cx = DY[a] * DZ[b] - DZ[a] * DY[b];
	int cy = DZ[a] * DX[b] - DX[a] * DZ[b];
	int cz = DX[a] * DY[b] - DY[a] * DX[b];
	return DirOf(cx, cy, cz);
}

int Step(int idx, int d)
{
	int x = idx % GN;
	int y = (idx / GN) % GN;
	int z = idx / (GN * GN);
	return Idx(x + DX[d], y + DY[d], z + DZ[d]);
}

// the new (gF, gU) after turn 1 up, 2 down, 3 left, 4 right
void TurnDirs(int F, int U, int code)
{
	gF = F;
	gU = U;
	int R = Cross(F, U);
	if (code == 1)
	{
		gF = U;
		gU = Opp(F);
	}
	if (code == 2)
	{
		gF = Opp(U);
		gU = F;
	}
	if (code == 3)
		gF = Opp(R);
	if (code == 4)
		gF = R;
}

// ======================================================================= trail geometry
void SetV(int vi, float x, float y, float z, float cx, float cy, float cz, float m, float o)
{
	tv[vi * 3] = x;
	tv[vi * 3 + 1] = y;
	tv[vi * 3 + 2] = z;
	tn[vi * 3] = cx;
	tn[vi * 3 + 1] = cy;
	tn[vi * 3 + 2] = cz;
	tt[vi * 2] = m;
	tt[vi * 2 + 1] = o;
}

void ClearSlot(int s)
{
	for (int v = s * 24; v < s * 24 + 24; v++)
		SetV(v, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
}

// a tube piece filling the cell: joined sides reach the cell's edge, the
// others close at TH; faces on joined sides collapse to a point
void BuildSlot(int s, int idx, int mask, int owner)
{
	float cx = idx % GN;
	float cy = (idx / GN) % GN;
	float cz = idx / (GN * GN);
	for (int a = 0; a < 3; a++)
	{
		lo3[a] = -TH;
		hi3[a] = TH;
		if (mask & BITS[a * 2 + 1])
			lo3[a] = -0.5;
		if (mask & BITS[a * 2])
			hi3[a] = 0.5;
	}
	for (int f = 0; f < 6; f++)
	{
		int v0 = s * 24 + f * 4;
		if (mask & BITS[f])
		{
			for (int k = 0; k < 4; k++)
				SetV(v0 + k, 0.0, 0.0, 0.0, cx, cy, cz, mask, owner);
		}
		else
		{
			int a = f / 2;
			int b = (a + 1) % 3;
			int c = (a + 2) % 3;
			pv3[a] = lo3[a];
			if (f % 2 == 0)
				pv3[a] = hi3[a];
			for (int k = 0; k < 4; k++)
			{
				pv3[b] = lo3[b];
				pv3[c] = lo3[c];
				if (k == 1 || k == 2)
					pv3[b] = hi3[b];
				if (k >= 2)
					pv3[c] = hi3[c];
				SetV(v0 + k, pv3[0], pv3[1], pv3[2], cx, cy, cz, mask, owner);
			}
		}
	}
}

int AllocSlot()
{
	if (nFree > 0)
	{
		nFree = nFree - 1;
		return freeS[nFree];
	}
	if (highS < MAXS)
	{
		highS = highS + 1;
		return highS - 1;
	}
	return -1;
}

void FreeSlotOf(int idx)
{
	int s = slotOf[idx];
	if (s >= 0)
	{
		ClearSlot(s);
		freeS[nFree] = s;
		nFree = nFree + 1;
		slotOf[idx] = -1;
	}
}

// the cell a ship is leaving (or died in) becomes a finished trail piece
void FinishCell(int idx, int owner)
{
	int s = AllocSlot();
	slotOf[idx] = s;
	if (s >= 0)
		BuildSlot(s, idx, conn[idx], owner);
}

// the live ship whose current cell is idx, or -1
int HeadShip(int idx)
{
	for (int i = 0; i < MAXSH; i++)
	{
		if (shAlive[i])
		{
			if (Idx(shX[i], shY[i], shZ[i]) == idx)
				return i;
		}
	}
	return -1;
}

void DestroyCell(int idx)
{
	FreeSlotOf(idx);
	int m = conn[idx];
	occ[idx] = 0;
	conn[idx] = 0;
	for (int d = 0; d < 6; d++)
	{
		if (m & BITS[d])
		{
			int nb = Step(idx, d);
			if (occ[nb] > 0)
			{
				int ob = BITS[Opp(d)];
				if (conn[nb] & ob)
					conn[nb] = conn[nb] - ob;
				if (slotOf[nb] >= 0)
					BuildSlot(slotOf[nb], nb, conn[nb], occ[nb] - 1);
				else
				{
					int h = HeadShip(nb);
					if (h >= 0)
						shInC[h] = 0;
				}
			}
		}
	}
}

void RemoveTrail(int owner)
{
	for (int i = 0; i < NC; i++)
	{
		if (occ[i] == owner + 1)
		{
			FreeSlotOf(i);
			occ[i] = 0;
			conn[i] = 0;
		}
	}
}

// ======================================================================= effects, log
void Burst(float x, float y, float z, float r, float g, float b, int n, float spd)
{
	for (int k = 0; k < n; k++)
	{
		int i = qNext;
		qNext = (qNext + 1) % MAXQ;
		float vx = Rf() * 2.0 - 1.0;
		float vy = Rf() * 2.0 - 1.0;
		float vz = Rf() * 2.0 - 1.0;
		while (vx * vx + vy * vy + vz * vz > 1.0)
		{
			vx = Rf() * 2.0 - 1.0;
			vy = Rf() * 2.0 - 1.0;
			vz = Rf() * 2.0 - 1.0;
		}
		float s = spd * (0.3 + 0.7 * Rf());
		qOn[i] = 1;
		qX[i] = x;
		qY[i] = y;
		qZ[i] = z;
		qVX[i] = vx * s;
		qVY[i] = vy * s;
		qVZ[i] = vz * s;
		qL[i] = 0.8 + 0.8 * Rf();
		float w = Rf() * 0.5;
		qR[i] = r + (1.0 - r) * w;
		qG[i] = g + (1.0 - g) * w;
		qB[i] = b + (1.0 - b) * w;
	}
}

void AddLog(int victim, int killer, int kind)
{
	for (int k = 4; k > 0; k--)
	{
		logV[k] = logV[k - 1];
		logK[k] = logK[k - 1];
		logKind[k] = logKind[k - 1];
		logT[k] = logT[k - 1];
	}
	logV[0] = victim;
	logK[0] = killer;
	logKind[0] = kind;
	logT[0] = 6.0;
}

void ShipPos(int i)
{
	float t = shT[i];
	pv3[0] = shX[i] + DX[shF[i]] * t;
	pv3[1] = shY[i] + DY[shF[i]] * t;
	pv3[2] = shZ[i] + DZ[shF[i]] * t;
}

void KillShip(int i, int killer, int kind)
{
	if (shAlive[i] == 0)
		return;
	shAlive[i] = 0;
	shDeadT[i] = 0.0;
	int idx = Idx(shX[i], shY[i], shZ[i]);
	if (occ[idx] == i + 1 && slotOf[idx] < 0)
		FinishCell(idx, i);
	ShipPos(i);
	Burst(pv3[0], pv3[1], pv3[2], shipCol[i * 3], shipCol[i * 3 + 1], shipCol[i * 3 + 2], 160, 6.0);
	Burst(pv3[0], pv3[1], pv3[2], 1.0, 1.0, 1.0, 40, 3.0);
	AddLog(i, killer, kind);
	if (kind == 1)
	{
		if (killer >= 0)
			shKills[killer] = shKills[killer] + 1;
	}
}

// ======================================================================= shots
void Fire(int i)
{
	if (shAmmo[i] <= 0)
		return;
	if (shCool[i] > 0.0)
		return;
	for (int p = 0; p < MAXP; p++)
	{
		if (pOn[p] == 0)
		{
			pOn[p] = 1;
			pX[p] = shX[i];
			pY[p] = shY[i];
			pZ[p] = shZ[i];
			pD[p] = shF[i];
			pT[p] = shT[i];
			pR[p] = 28;
			pO[p] = i;
			shAmmo[i] = shAmmo[i] - 1;
			shCool[i] = 0.35;
			return;
		}
	}
}

// a shot reached a filled cell: blast the trails around it, wreck a ship in it
void Hit(int idx, int owner)
{
	int x = idx % GN;
	int y = (idx / GN) % GN;
	int z = idx / (GN * GN);
	for (int dz = -1; dz <= 1; dz++)
		for (int dy = -1; dy <= 1; dy++)
			for (int dx = -1; dx <= 1; dx++)
			{
				int n = Idx(x + dx, y + dy, z + dz);
				if (occ[n] > 0)
				{
					int h = HeadShip(n);
					if (h >= 0)
					{
						if (n == idx)
							KillShip(h, owner, 1);
					}
					else
						DestroyCell(n);
				}
			}
	float r = shipCol[owner * 3];
	float g = shipCol[owner * 3 + 1];
	float b = shipCol[owner * 3 + 2];
	Burst(x, y, z, 1.0, 1.0, 0.85, 90, 4.0);
	Burst(x, y, z, r, g, b, 60, 2.5);
}

void UpdateShots(float dt)
{
	for (int p = 0; p < MAXP; p++)
	{
		if (pOn[p])
		{
			pT[p] = pT[p] + 22.0 * dt;
			while (pOn[p] && pT[p] >= 0.5)
			{
				pT[p] = pT[p] - 1.0;
				pX[p] = Wrap(pX[p] + DX[pD[p]]);
				pY[p] = Wrap(pY[p] + DY[pD[p]]);
				pZ[p] = Wrap(pZ[p] + DZ[pD[p]]);
				pR[p] = pR[p] - 1;
				int idx = Idx(pX[p], pY[p], pZ[p]);
				if (occ[idx] > 0)
				{
					Hit(idx, pO[p]);
					pOn[p] = 0;
				}
				if (pR[p] <= 0)
					pOn[p] = 0;
			}
		}
	}
}

// ======================================================================= pickups
void PlacePickup(int k)
{
	for (int tries = 0; tries < 50; tries++)
	{
		int x = Rnd(GN);
		int y = Rnd(GN);
		int z = Rnd(GN);
		if (occ[Idx(x, y, z)] == 0)
		{
			kX[k] = x;
			kY[k] = y;
			kZ[k] = z;
			kOn[k] = 1;
			return;
		}
	}
}

// ======================================================================= AI
// how many cells a flood fill reaches from idx, up to lim
int Reach(int idx, int lim)
{
	if (occ[idx] > 0)
		return 0;
	visStamp = visStamp + 1;
	int head = 0;
	int tail = 1;
	bq[0] = idx;
	vis[idx] = visStamp;
	while (head < tail && tail < lim)
	{
		int c = bq[head];
		head = head + 1;
		for (int d = 0; d < 6; d++)
		{
			int n = Step(c, d);
			if (occ[n] == 0)
			{
				if (vis[n] != visStamp)
				{
					vis[n] = visStamp;
					if (tail < lim)
					{
						bq[tail] = n;
						tail = tail + 1;
					}
				}
			}
		}
	}
	return tail;
}

int Dist3(int x, int y, int z, int x2, int y2, int z2)
{
	int a = WD(x2 - x);
	int b = WD(y2 - y);
	int c = WD(z2 - z);
	if (a < 0) a = -a;
	if (b < 0) b = -b;
	if (c < 0) c = -c;
	return a + b + c;
}

// a filled cell, or the next cell of another ship (head-on crashes)
int Blocked(int idx, int self)
{
	if (occ[idx] > 0)
		return 1;
	for (int i = 0; i < MAXSH; i++)
	{
		if (shAlive[i] && i != self)
		{
			if (Idx(shX[i] + DX[shF[i]], shY[i] + DY[shF[i]], shZ[i] + DZ[shF[i]]) == idx)
				return 1;
		}
	}
	return 0;
}

// called when a computer ship enters a cell: picks the turn for its centre
void AIDecide(int i)
{
	int x = shX[i];
	int y = shY[i];
	int z = shZ[i];
	int idx = Idx(x, y, z);
	// the nearest pickup is worth a detour
	int pk = -1;
	int pkd = 12;
	for (int k = 0; k < MAXK; k++)
	{
		if (kOn[k])
		{
			int d = Dist3(x, y, z, kX[k], kY[k], kZ[k]);
			if (d < pkd)
			{
				pkd = d;
				pk = k;
			}
		}
	}
	// some rivals go for the player: aim three cells ahead of it
	int hunt = 0;
	int hx = 0;
	int hy = 0;
	int hz = 0;
	int hd = 0;
	if (shAlive[0] && i % 2 == 1)
	{
		hx = shX[0] + DX[shF[0]] * 3;
		hy = shY[0] + DY[shF[0]] * 3;
		hz = shZ[0] + DZ[shF[0]] * 3;
		hd = Dist3(x, y, z, hx, hy, hz);
		if (hd < 14)
			hunt = 1;
	}
	float best = -1000.0;
	int bestCode = 0;
	int bestRun = 0;
	int straightRun = 0;
	int bestReach = 0;
	for (int code = 0; code < 5; code++)
	{
		TurnDirs(shF[i], shU[i], code);
		int d = gF;
		int run = 0;
		int blocked = 0;
		for (int k = 1; k <= 10; k++)
		{
			if (blocked == 0)
			{
				int c = Idx(x + DX[d] * k, y + DY[d] * k, z + DZ[d] * k);
				if (k <= 2)
				{
					if (Blocked(c, i))
						blocked = 1;
				}
				else if (occ[c] > 0)
					blocked = 1;
				else
					run = run + 1;
			}
		}
		if (code == 0)
			straightRun = run;
		float score = run * 1.0;
		int reach = 0;
		if (run > 0)
		{
			reach = Reach(Step(idx, d), 120);
			score = score + reach * 0.06;
		}
		else
			score = score - 50.0;
		if (code == 0)
			score = score + 1.2;
		score = score + Rf() * 1.4;
		if (pk >= 0)
		{
			int d2 = Dist3(x + DX[d], y + DY[d], z + DZ[d], kX[pk], kY[pk], kZ[pk]);
			if (d2 < pkd)
				score = score + 2.0;
		}
		if (hunt)
		{
			if (Dist3(x + DX[d], y + DY[d], z + DZ[d], hx, hy, hz) < hd)
				score = score + 1.5;
		}
		if (score > best)
		{
			best = score;
			bestCode = code;
			bestRun = run;
			bestReach = reach;
		}
	}
	tqn[i] = 0;
	if (bestCode != 0)
	{
		tq[i * 4] = bestCode;
		tqn[i] = 1;
	}
	// cornered: blast a way out straight ahead
	if (shAmmo[i] > 0)
	{
		if (bestRun == 0 || (bestReach < 20 && straightRun <= 2))
		{
			Fire(i);
			tqn[i] = 0;
		}
	}
	// hunting: the player sits right ahead in the line of fire
	if (shAlive[0] && shAmmo[i] >= 2)
	{
		int ax = WD(shX[0] - x);
		int ay = WD(shY[0] - y);
		int az = WD(shZ[0] - z);
		int along = ax * DX[shF[i]] + ay * DY[shF[i]] + az * DZ[shF[i]];
		int off = Dist3(x, y, z, shX[0], shY[0], shZ[0]) - along;
		if (along > 1 && along < 14 && off == 0 && Rf() < 0.6)
			Fire(i);
	}
}

// ======================================================================= ships
void ApplyTurn(int i)
{
	if (tqn[i] <= 0)
		return;
	TurnDirs(shF[i], shU[i], tq[i * 4]);
	shF[i] = gF;
	shU[i] = gU;
	for (int k = 0; k < 3; k++)
		tq[i * 4 + k] = tq[i * 4 + k + 1];
	tqn[i] = tqn[i] - 1;
}

void EnterCell(int i)
{
	int idx = Idx(shX[i], shY[i], shZ[i]);
	conn[idx] = conn[idx] | BITS[shF[i]];
	FinishCell(idx, i);
	int nx = Wrap(shX[i] + DX[shF[i]]);
	int ny = Wrap(shY[i] + DY[shF[i]]);
	int nz = Wrap(shZ[i] + DZ[shF[i]]);
	int n = Idx(nx, ny, nz);
	if (occ[n] > 0)
	{
		// the joint drawn to the wall, then the wreck
		shT[i] = 0.5;
		int o = occ[n] - 1;
		int h = HeadShip(n);
		KillShip(i, o, 0);
		if (h >= 0)
			KillShip(h, i, 0);
		return;
	}
	shX[i] = nx;
	shY[i] = ny;
	shZ[i] = nz;
	shT[i] = shT[i] - 1.0;
	shIn[i] = shF[i];
	shInC[i] = 1;
	shCP[i] = 0;
	occ[n] = i + 1;
	conn[n] = BITS[Opp(shF[i])];
	for (int k = 0; k < MAXK; k++)
	{
		if (kOn[k])
		{
			if (kX[k] == nx && kY[k] == ny && kZ[k] == nz)
			{
				kOn[k] = 0;
				kTimer[k] = 10.0;
				if (shAmmo[i] < 5)
					shAmmo[i] = shAmmo[i] + 1;
				Burst(nx, ny, nz, 1.0, 0.85, 0.3, 50, 2.0);
			}
		}
	}
	if (i > 0)
		AIDecide(i);
}

void MoveShip(int i, float dt)
{
	shT[i] = shT[i] + speed * dt;
	if (shCP[i] == 0 && shT[i] >= 0.0)
	{
		shCP[i] = 1;
		ApplyTurn(i);
	}
	// a turn pressed just after the centre still counts
	if (shCP[i] == 1 && shT[i] < 0.12 && shF[i] == shIn[i])
		ApplyTurn(i);
	if (shT[i] >= 0.5)
		EnterCell(i);
}

// ======================================================================= rounds
void StartRound()
{
	round = round + 1;
	for (int i = 0; i < NC; i++)
	{
		occ[i] = 0;
		slotOf[i] = -1;
		conn[i] = 0;
	}
	highS = 0;
	nFree = 0;
	for (int p = 0; p < MAXP; p++)
		pOn[p] = 0;
	for (int q = 0; q < MAXQ; q++)
		qOn[q] = 0;
	for (int i = 0; i < MAXSH; i++)
	{
		int ok = 0;
		int tries = 0;
		while (ok == 0)
		{
			shX[i] = Rnd(GN);
			shY[i] = Rnd(GN);
			shZ[i] = Rnd(GN);
			ok = 1;
			for (int j = 0; j < i; j++)
			{
				if (Dist3(shX[i], shY[i], shZ[i], shX[j], shY[j], shZ[j]) < 12 - tries / 20)
					ok = 0;
			}
			tries = tries + 1;
		}
		shF[i] = Rnd(6);
		shU[i] = (shF[i] / 2 * 2 + 2 + Rnd(4)) % 6;
		shIn[i] = shF[i];
		shInC[i] = 0;
		shCP[i] = 0;
		shT[i] = -0.5;
		shAlive[i] = 1;
		shAmmo[i] = 2;
		shFade[i] = 1.0;
		shGone[i] = 0;
		shCool[i] = 0.0;
		tqn[i] = 0;
		occ[Idx(shX[i], shY[i], shZ[i])] = i + 1;
	}
	for (int k = 0; k < MAXK; k++)
	{
		kOn[k] = 0;
		PlacePickup(k);
	}
	for (int i = 1; i < MAXSH; i++)
		AIDecide(i);
	roundOver = 0;
	roundT = 0.0;
	countdown = 3.0;
	speed = 5.5;
	lookYaw = 0.0;
	lookPitch = 0.0;
}

// ======================================================================= camera
void QuatFromDirs(int F, int U, float* q)
{
	int R = Cross(F, U);
	// rotation columns: right, up, back
	float m00 = DX[R];
	float m10 = DY[R];
	float m20 = DZ[R];
	float m01 = DX[U];
	float m11 = DY[U];
	float m21 = DZ[U];
	float m02 = -DX[F];
	float m12 = -DY[F];
	float m22 = -DZ[F];
	float tr = m00 + m11 + m22;
	if (tr > 0.0)
	{
		float s = sqrt(tr + 1.0) * 2.0;
		q[0] = 0.25 * s;
		q[1] = (m21 - m12) / s;
		q[2] = (m02 - m20) / s;
		q[3] = (m10 - m01) / s;
	}
	else if (m00 > m11 && m00 > m22)
	{
		float s = sqrt(1.0 + m00 - m11 - m22) * 2.0;
		q[0] = (m21 - m12) / s;
		q[1] = 0.25 * s;
		q[2] = (m01 + m10) / s;
		q[3] = (m02 + m20) / s;
	}
	else if (m11 > m22)
	{
		float s = sqrt(1.0 + m11 - m00 - m22) * 2.0;
		q[0] = (m02 - m20) / s;
		q[1] = (m01 + m10) / s;
		q[2] = 0.25 * s;
		q[3] = (m12 + m21) / s;
	}
	else
	{
		float s = sqrt(1.0 + m22 - m00 - m11) * 2.0;
		q[0] = (m10 - m01) / s;
		q[1] = (m02 + m20) / s;
		q[2] = (m12 + m21) / s;
		q[3] = 0.25 * s;
	}
}

void UpdateCamera(float dt, int snap)
{
	QuatFromDirs(shF[0], shU[0], tgtQ);
	float dot = camQ[0] * tgtQ[0] + camQ[1] * tgtQ[1] + camQ[2] * tgtQ[2] + camQ[3] * tgtQ[3];
	float sg = 1.0;
	if (dot < 0.0)
		sg = -1.0;
	float k = 1.0 - exp(-dt * 13.0);
	if (snap)
		k = 1.0;
	float len = 0.0;
	for (int c = 0; c < 4; c++)
	{
		camQ[c] = camQ[c] + (sg * tgtQ[c] - camQ[c]) * k;
		len = len + camQ[c] * camQ[c];
	}
	len = sqrt(len);
	for (int c = 0; c < 4; c++)
		camQ[c] = camQ[c] / len;
	float w = camQ[0];
	float x = camQ[1];
	float y = camQ[2];
	float z = camQ[3];
	camR[0] = 1.0 - 2.0 * (y * y + z * z);
	camR[1] = 2.0 * (x * y + w * z);
	camR[2] = 2.0 * (x * z - w * y);
	camU[0] = 2.0 * (x * y - w * z);
	camU[1] = 1.0 - 2.0 * (x * x + z * z);
	camU[2] = 2.0 * (y * z + w * x);
	camB[0] = 2.0 * (x * z + w * y);
	camB[1] = 2.0 * (y * z - w * x);
	camB[2] = 1.0 - 2.0 * (x * x + y * y);
	for (int c = 0; c < 16; c++)
		viewM[c] = 0.0;
	for (int c = 0; c < 3; c++)
	{
		viewM[c * 4] = camR[c];
		viewM[c * 4 + 1] = camU[c];
		viewM[c * 4 + 2] = camB[c];
	}
	viewM[15] = 1.0;
	if (shAlive[0])
	{
		ShipPos(0);
		camP[0] = pv3[0];
		camP[1] = pv3[1];
		camP[2] = pv3[2];
	}
}

// ======================================================================= shaders
char* vsSrc =
	"#version 120\n"
	"uniform vec3 cam;\n"
	"uniform float gridN;\n"
	"uniform int mode;\n"
	"uniform vec3 col[6];\n"
	"uniform float fade[6];\n"
	"varying vec3 vP;\n"
	"varying vec3 vR;\n"
	"varying vec4 vC;\n"
	"varying float vM;\n"
	"void main(){\n"
	// the cell centre comes as the normal: take its image nearest the camera
	"  vec3 rel = gl_Normal - cam;\n"
	"  rel -= gridN * floor(rel / gridN + 0.5);\n"
	"  vec3 p = rel + gl_Vertex.xyz;\n"
	"  vP = gl_Vertex.xyz;\n"
	"  vR = p;\n"
	"  if (mode == 0) {\n"
	"    int o = int(gl_MultiTexCoord0.y + 0.5);\n"
	"    vC = vec4(col[o], fade[o]);\n"
	"    vM = gl_MultiTexCoord0.x;\n"
	"  } else {\n"
	"    vC = gl_Color;\n"
	"    vM = 0.0;\n"
	"  }\n"
	"  gl_Position = gl_ModelViewProjectionMatrix * vec4(p, 1.0);\n"
	"}\n";

char* fsSrc =
	"#version 120\n"
	"uniform int mode;\n"
	"uniform float fogEnd;\n"
	"uniform float th;\n"
	"varying vec3 vP;\n"
	"varying vec3 vR;\n"
	"varying vec4 vC;\n"
	"varying float vM;\n"
	"float B(float m, float k){ return mod(floor(m / k + 0.001), 2.0); }\n"
	"void main(){\n"
	"  float fog = 1.0 - smoothstep(fogEnd * 0.3, fogEnd, length(vR));\n"
	"  if (mode == 1) { gl_FragColor = vec4(vC.rgb * vC.a * fog, 1.0); return; }\n"
	"  vec3 n = normalize(cross(dFdx(vR), dFdy(vR)));\n"
	"  float f = abs(dot(n, normalize(vR)));\n"
	"  if (mode == 2) {\n"
	"    vec3 c = vC.rgb * (0.25 + 0.75 * f) + vC.rgb * pow(1.0 - f, 3.0);\n"
	"    gl_FragColor = vec4(c * fog, 1.0);\n"
	"    return;\n"
	"  }\n"
	// trail tube: glassy faces, bright edges except where cells join
	"  float m = floor(vM + 0.5);\n"
	"  vec3 a = abs(vP);\n"
	"  vec3 jp = vec3(B(m, 1.0), B(m, 4.0), B(m, 16.0));\n"
	"  vec3 jn = vec3(B(m, 2.0), B(m, 8.0), B(m, 32.0));\n"
	"  vec3 j = mix(jn, jp, step(0.0, vP));\n"
	"  vec3 e = (1.0 - j) * smoothstep(th - 0.07, th - 0.01, a);\n"
	"  float edge = clamp(e.x + e.y + e.z - 1.0, 0.0, 1.0);\n"
	"  float body = 0.06 + 0.28 * pow(1.0 - f, 2.0);\n"
	"  vec3 c = vC.rgb * (body + 1.2 * edge) + vec3(0.3 * edge);\n"
	"  gl_FragColor = vec4(c * fog * vC.a, 1.0);\n"
	"}\n";

// ======================================================================= scenery
void BuildScenery()
{
	int v = 0;
	for (int ax = 0; ax < 3; ax++)
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
			{
				int b = (ax + 1) % 3;
				int c = (ax + 2) % 3;
				for (int e = 0; e < 2; e++)
				{
					for (int k = 0; k < 3; k++)
					{
						latV[v * 3 + k] = 0.0;
						latN[v * 3 + k] = 0.0;
					}
					latV[v * 3 + ax] = -GN + e * 2.0 * GN;
					latN[v * 3 + b] = i * 8 + 3.5;
					latN[v * 3 + c] = j * 8 + 3.5;
					latC[v * 4] = 0.10;
					latC[v * 4 + 1] = 0.22;
					latC[v * 4 + 2] = 0.45;
					latC[v * 4 + 3] = 1.0;
					v = v + 1;
				}
			}
	for (int d = 0; d < NDUST; d++)
	{
		for (int k = 0; k < 3; k++)
		{
			dustV[d * 3 + k] = 0.0;
			dustN[d * 3 + k] = Rf() * GN;
		}
		float b = 0.25 + 0.4 * Rf();
		dustC[d * 4] = b * 0.7;
		dustC[d * 4 + 1] = b * 0.8;
		dustC[d * 4 + 2] = b;
		dustC[d * 4 + 3] = 1.0;
	}
}

void BuildFont()
{
	for (int ch = 32; ch < 128; ch++)
	{
		int g = -1;
		for (int j = 0; j < GL_N; j++)
		{
			if (glCode[j] == ch)
				g = j;
		}
		for (int r = 0; r < 16; r++)
		{
			int bits = 0;
			if (g >= 0)
				bits = glBits[g * 16 + r];
			for (int c = 0; c < 9; c++)
			{
				int px = 16777215;
				if ((bits >> (8 - c)) & 1)
					px = -1;
				fontPix[r * 864 + (ch - 32) * 9 + c] = px;
			}
		}
	}
	fontTex = glwGenTexture();
	glBindTexture(GL_TEXTURE_2D, fontTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glwTexImage2D(864, 16, fontPix);
}

// ======================================================================= drawing
// one side-open tube piece along direction d from a to b (a < b), for the
// cells ships are flying through
void Piece(int d, float a, float b, int owner)
{
	int ax = d / 2;
	float sg = 1.0;
	if (d % 2 == 1)
		sg = -1.0;
	for (int k = 0; k < 3; k++)
	{
		lo3[k] = -TH;
		hi3[k] = TH;
	}
	lo3[ax] = a * sg;
	hi3[ax] = b * sg;
	if (sg < 0.0)
	{
		lo3[ax] = -b;
		hi3[ax] = -a;
	}
	glTexCoord2f(BITS[ax * 2] + BITS[ax * 2 + 1], owner);
	for (int f = 0; f < 6; f++)
	{
		int fa = f / 2;
		if (fa != ax)
		{
			int fb = (fa + 1) % 3;
			int fc = (fa + 2) % 3;
			pv3[fa] = lo3[fa];
			if (f % 2 == 0)
				pv3[fa] = hi3[fa];
			for (int k = 0; k < 4; k++)
			{
				pv3[fb] = lo3[fb];
				pv3[fc] = lo3[fc];
				if (k == 1 || k == 2)
					pv3[fb] = hi3[fb];
				if (k >= 2)
					pv3[fc] = hi3[fc];
				glVertex3f(pv3[0], pv3[1], pv3[2]);
			}
		}
	}
}

void DrawHeads()
{
	glBegin(GL_QUADS);
	for (int i = 0; i < MAXSH; i++)
	{
		if (shAlive[i])
		{
			glNormal3f(shX[i], shY[i], shZ[i]);
			float lo = -TH;
			if (shInC[i])
				lo = -0.5;
			float end = shT[i] - 0.3;
			if (shF[i] == shIn[i])
			{
				if (end > lo)
					Piece(shIn[i], lo, end, i);
			}
			else
			{
				Piece(shIn[i], lo, TH, i);
				if (end > TH)
					Piece(shF[i], TH, end, i);
			}
		}
	}
	glEnd();
}

void ShipV(float a, float b, float c)
{
	glVertex3f(a * sbF[0] + b * sbR[0] + c * sbU[0], a * sbF[1] + b * sbR[1] + c * sbU[1], a * sbF[2] + b * sbR[2] + c * sbU[2]);
}

void ShipBasis(int i)
{
	int R = Cross(shF[i], shU[i]);
	sbF[0] = DX[shF[i]];
	sbF[1] = DY[shF[i]];
	sbF[2] = DZ[shF[i]];
	sbU[0] = DX[shU[i]];
	sbU[1] = DY[shU[i]];
	sbU[2] = DZ[shU[i]];
	sbR[0] = DX[R];
	sbR[1] = DY[R];
	sbR[2] = DZ[R];
}

void DrawShips(float now)
{
	glBegin(GL_TRIANGLES);
	for (int i = 1; i < MAXSH; i++)
	{
		if (shAlive[i])
		{
			ShipPos(i);
			ShipBasis(i);
			glNormal3f(pv3[0], pv3[1], pv3[2]);
			glColor4f(shipCol[i * 3], shipCol[i * 3 + 1], shipCol[i * 3 + 2], 1.0);
			// nose, wings, fin, keel
			ShipV(0.5, 0.0, 0.0); ShipV(-0.35, -0.34, 0.0); ShipV(-0.3, 0.0, 0.16);
			ShipV(0.5, 0.0, 0.0); ShipV(-0.3, 0.0, 0.16); ShipV(-0.35, 0.34, 0.0);
			ShipV(0.5, 0.0, 0.0); ShipV(-0.35, 0.34, 0.0); ShipV(-0.3, 0.0, -0.08);
			ShipV(0.5, 0.0, 0.0); ShipV(-0.3, 0.0, -0.08); ShipV(-0.35, -0.34, 0.0);
			ShipV(-0.35, -0.34, 0.0); ShipV(-0.35, 0.34, 0.0); ShipV(-0.3, 0.0, 0.16);
			ShipV(-0.35, -0.34, 0.0); ShipV(-0.3, 0.0, -0.08); ShipV(-0.35, 0.34, 0.0);
		}
	}
	// ammo pickups: spinning gold octahedra
	float an = now * 2.0;
	float s = 0.28;
	float ca = cos(an) * s;
	float sa = sin(an) * s;
	glColor4f(1.0, 0.78, 0.25, 1.0);
	for (int k = 0; k < MAXK; k++)
	{
		if (kOn[k])
		{
			glNormal3f(kX[k], kY[k], kZ[k] + 0.08 * sin(now * 3.0 + k));
			for (int q = 0; q < 4; q++)
			{
				float x1 = ca;
				float z1 = sa;
				float x2 = -sa;
				float z2 = ca;
				if (q == 1) { x1 = -sa; z1 = ca; x2 = -ca; z2 = -sa; }
				if (q == 2) { x1 = -ca; z1 = -sa; x2 = sa; z2 = -ca; }
				if (q == 3) { x1 = sa; z1 = -ca; x2 = ca; z2 = sa; }
				glVertex3f(0.0, s * 1.4, 0.0); glVertex3f(x1, 0.0, z1); glVertex3f(x2, 0.0, z2);
				glVertex3f(0.0, -s * 1.4, 0.0); glVertex3f(x2, 0.0, z2); glVertex3f(x1, 0.0, z1);
			}
		}
	}
	glEnd();
}

void DrawGlows(float now)
{
	// engines and pickup halos
	glPointSize(7.0);
	glBegin(GL_POINTS);
	for (int i = 1; i < MAXSH; i++)
	{
		if (shAlive[i])
		{
			ShipPos(i);
			ShipBasis(i);
			glNormal3f(pv3[0], pv3[1], pv3[2]);
			glColor4f(shipCol[i * 3] * 0.5 + 0.5, shipCol[i * 3 + 1] * 0.5 + 0.5, shipCol[i * 3 + 2] * 0.5 + 0.5, 1.0);
			ShipV(-0.4, 0.0, 0.02);
		}
	}
	for (int p = 0; p < MAXP; p++)
	{
		if (pOn[p])
		{
			int o = pO[p];
			glNormal3f(pX[p] + DX[pD[p]] * pT[p], pY[p] + DY[pD[p]] * pT[p], pZ[p] + DZ[pD[p]] * pT[p]);
			glColor4f(0.8 + 0.2 * shipCol[o * 3], 0.8 + 0.2 * shipCol[o * 3 + 1], 0.8 + 0.2 * shipCol[o * 3 + 2], 1.0);
			glVertex3f(0.0, 0.0, 0.0);
		}
	}
	glEnd();
	// shot streaks
	glBegin(GL_LINES);
	for (int p = 0; p < MAXP; p++)
	{
		if (pOn[p])
		{
			int o = pO[p];
			int d = pD[p];
			glNormal3f(pX[p] + DX[d] * pT[p], pY[p] + DY[d] * pT[p], pZ[p] + DZ[d] * pT[p]);
			glColor4f(1.0, 1.0, 1.0, 1.0);
			glVertex3f(0.0, 0.0, 0.0);
			glColor4f(shipCol[o * 3], shipCol[o * 3 + 1], shipCol[o * 3 + 2], 1.0);
			glVertex3f(-DX[d] * 1.6, -DY[d] * 1.6, -DZ[d] * 1.6);
		}
	}
	glEnd();
	// explosion sparks
	glPointSize(3.0);
	glBegin(GL_POINTS);
	for (int q = 0; q < MAXQ; q++)
	{
		if (qOn[q])
		{
			float a = qL[q];
			if (a > 1.0)
				a = 1.0;
			glColor4f(qR[q], qG[q], qB[q], a);
			glNormal3f(qX[q], qY[q], qZ[q]);
			glVertex3f(0.0, 0.0, 0.0);
		}
	}
	glEnd();
}

void UpdateParticles(float dt)
{
	float drag = exp(-dt * 1.5);
	for (int q = 0; q < MAXQ; q++)
	{
		if (qOn[q])
		{
			qX[q] = qX[q] + qVX[q] * dt;
			qY[q] = qY[q] + qVY[q] * dt;
			qZ[q] = qZ[q] + qVZ[q] * dt;
			qVX[q] = qVX[q] * drag;
			qVY[q] = qVY[q] * drag;
			qVZ[q] = qVZ[q] * drag;
			qL[q] = qL[q] - dt;
			if (qL[q] <= 0.0)
				qOn[q] = 0;
		}
	}
}

void DrawWorld(float now)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(72.0, winW * 1.0 / winH, 0.03, 40.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(lookPitch, 1.0, 0.0, 0.0);
	glRotatef(lookYaw, 0.0, 1.0, 0.0);
	glMultMatrixf(viewM);

	glUseProgram(prog);
	glUniform3f(uCam, camP[0], camP[1], camP[2]);
	glUniform1f(uGridN, GN * 1.0);
	glUniform1f(uFogEnd, GN * 0.5 - 0.5);
	glUniform1f(uTh, TH);
	glUniform3fv(uCol, 6, shipCol);
	for (int i = 0; i < MAXSH; i++)
		fadeArr[i] = shFade[i];
	glUniform1fv(uFade, 6, fadeArr);

	// solid things
	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
	glDisable(GL_BLEND);
	glUniform1i(uMode, 2);
	DrawShips(now);

	// glowing things add up
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDepthMask(0);
	glUniform1i(uMode, 1);
	glwVertexArray(3, latV);
	glwNormalArray(latN);
	glwColorArray(4, latC);
	glDrawArrays(GL_LINES, 0, NLAT);
	glwVertexArray(3, dustV);
	glwNormalArray(dustN);
	glwColorArray(4, dustC);
	glPointSize(2.0);
	glDrawArrays(GL_POINTS, 0, NDUST);
	glwDisableArrays();
	DrawGlows(now);

	glUniform1i(uMode, 0);
	if (highS > 0)
	{
		glwVertexArray(3, tv);
		glwNormalArray(tn);
		glwTexCoordArray(2, tt);
		glDrawArrays(GL_QUADS, 0, highS * 24);
		glwDisableArrays();
	}
	DrawHeads();

	glDepthMask(1);
	glUseProgram(0);
}

// ---------------------------------------------------------------- HUD
float DrawText(float x, float y, char* s, float sc, float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
	glBegin(GL_QUADS);
	int k = 0;
	int ch = s[0];
	while (ch != 0)
	{
		if (ch >= 32 && ch < 128)
		{
			float u0 = (ch - 32) * 9.0 / 864.0;
			float u1 = (ch - 31) * 9.0 / 864.0;
			glTexCoord2f(u0, 0.0); glVertex2f(x, y);
			glTexCoord2f(u1, 0.0); glVertex2f(x + 9.0 * sc, y);
			glTexCoord2f(u1, 1.0); glVertex2f(x + 9.0 * sc, y + 16.0 * sc);
			glTexCoord2f(u0, 1.0); glVertex2f(x, y + 16.0 * sc);
		}
		x = x + 9.0 * sc;
		k = k + 1;
		ch = s[k];
	}
	glEnd();
	return x;
}

int TextLen(char* s)
{
	int k = 0;
	int ch = s[0];
	while (ch != 0)
	{
		k = k + 1;
		ch = s[k];
	}
	return k;
}

char* NumStr(int v)
{
	int neg = 0;
	if (v < 0)
	{
		neg = 1;
		v = -v;
	}
	int n = 0;
	char tmp[12];
	while (n == 0 || v > 0)
	{
		tmp[n] = '0' + v % 10;
		v = v / 10;
		n = n + 1;
	}
	int k = 0;
	if (neg)
	{
		numBuf[0] = '-';
		k = 1;
	}
	while (n > 0)
	{
		n = n - 1;
		numBuf[k] = tmp[n];
		k = k + 1;
	}
	numBuf[k] = 0;
	return numBuf;
}

// text with a dark outline, centred on x when centre is set
float Say(float x, float y, char* s, float sc, int centre, float r, float g, float b, float a)
{
	if (centre)
		x = x - TextLen(s) * 4.5 * sc;
	DrawText(x + sc, y + sc, s, sc, 0.0, 0.0, 0.0, a * 0.8);
	return DrawText(x, y, s, sc, r, g, b, a);
}

void Rect(float x, float y, float w, float h)
{
	glVertex2f(x, y);
	glVertex2f(x + w, y);
	glVertex2f(x + w, y + h);
	glVertex2f(x, y + h);
}

void DrawRadar()
{
	float cx = winW * 0.5;
	float cy = winH - 95.0;
	float rx = 130.0;
	float ry = 48.0;
	float range = GN * 0.5;
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.0, 0.05, 0.1, 0.55);
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(cx, cy);
	for (int k = 0; k <= 48; k++)
		glVertex2f(cx + rx * cos(k * 6.2832 / 48.0), cy + ry * sin(k * 6.2832 / 48.0));
	glEnd();
	glColor4f(0.2, 0.5, 0.8, 0.8);
	glBegin(GL_LINE_LOOP);
	for (int k = 0; k < 48; k++)
		glVertex2f(cx + rx * cos(k * 6.2832 / 48.0), cy + ry * sin(k * 6.2832 / 48.0));
	glEnd();
	glBegin(GL_LINES);
	glVertex2f(cx - rx, cy); glVertex2f(cx + rx, cy);
	glVertex2f(cx, cy - ry); glVertex2f(cx, cy + ry);
	for (int i = 1; i < MAXSH + MAXK; i++)
	{
		float wx = 0.0;
		float wy = 0.0;
		float wz = 0.0;
		int show = 0;
		if (i < MAXSH)
		{
			if (shAlive[i])
			{
				ShipPos(i);
				wx = pv3[0];
				wy = pv3[1];
				wz = pv3[2];
				show = 1;
				glColor4f(shipCol[i * 3], shipCol[i * 3 + 1], shipCol[i * 3 + 2], 1.0);
			}
		}
		else if (kOn[i - MAXSH])
		{
			wx = kX[i - MAXSH];
			wy = kY[i - MAXSH];
			wz = kZ[i - MAXSH];
			show = 1;
			glColor4f(1.0, 0.8, 0.3, 0.9);
		}
		if (show)
		{
			float dx = WDf(wx - camP[0]);
			float dy = WDf(wy - camP[1]);
			float dz = WDf(wz - camP[2]);
			float x = (dx * camR[0] + dy * camR[1] + dz * camR[2]) / range;
			float y = (dx * camU[0] + dy * camU[1] + dz * camU[2]) / range;
			float z = (dx * camB[0] + dy * camB[1] + dz * camB[2]) / range;
			if (x * x + y * y + z * z < 1.0)
			{
				float bx = cx + x * rx;
				float by = cy + z * ry;
				float ty = by - y * ry * 1.6;
				glVertex2f(bx, by); glVertex2f(bx, ty);
				glVertex2f(bx - 4.0, ty); glVertex2f(bx + 4.0, ty);
				glVertex2f(bx - 4.0, ty - 1.0); glVertex2f(bx + 4.0, ty - 1.0);
				glVertex2f(bx - 4.0, ty + 1.0); glVertex2f(bx + 4.0, ty + 1.0);
			}
		}
	}
	glEnd();
}

void DrawHUD(float now)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, winW, winH, 0.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// crosshair, red when a wall is two cells ahead or closer
	float cx = winW * 0.5;
	float cy = winH * 0.5;
	glDisable(GL_TEXTURE_2D);
	int danger = 0;
	if (shAlive[0])
	{
		for (int k = 1; k <= 2; k++)
		{
			if (occ[Idx(shX[0] + DX[shF[0]] * k, shY[0] + DY[shF[0]] * k, shZ[0] + DZ[shF[0]] * k)] > 0)
				danger = 1;
		}
	}
	if (danger)
		glColor4f(1.0, 0.25, 0.2, 0.95);
	else
		glColor4f(0.6, 0.95, 1.0, 0.7);
	glBegin(GL_LINES);
	glVertex2f(cx - 14.0, cy); glVertex2f(cx - 5.0, cy);
	glVertex2f(cx + 5.0, cy); glVertex2f(cx + 14.0, cy);
	glVertex2f(cx, cy - 14.0); glVertex2f(cx, cy - 5.0);
	glVertex2f(cx, cy + 5.0); glVertex2f(cx, cy + 14.0);
	glEnd();
	// ammo cells
	glBegin(GL_QUADS);
	for (int a = 0; a < 5; a++)
	{
		if (a < shAmmo[0])
			glColor4f(1.0, 0.8, 0.3, 0.95);
		else
			glColor4f(0.3, 0.3, 0.35, 0.5);
		Rect(110.0 + a * 22.0, 22.0, 16.0, 24.0);
	}
	glEnd();
	DrawRadar();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, fontTex);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	Say(20.0, 18.0, "AMMO", 2.0, 0, 1.0, 0.85, 0.4, 1.0);
	int rivals = 0;
	for (int i = 1; i < MAXSH; i++)
		rivals = rivals + shAlive[i];
	float x = Say(20.0, 56.0, "RIVALS ", 1.0, 0, 0.7, 0.85, 1.0, 1.0);
	x = Say(x, 56.0, NumStr(rivals), 1.0, 0, 1.0, 1.0, 1.0, 1.0);
	x = Say(x, 56.0, "   SHOT DOWN ", 1.0, 0, 0.7, 0.85, 1.0, 1.0);
	Say(x, 56.0, NumStr(shKills[0]), 1.0, 0, 1.0, 1.0, 1.0, 1.0);
	x = Say(20.0, 74.0, "ROUND ", 1.0, 0, 0.7, 0.85, 1.0, 1.0);
	x = Say(x, 74.0, NumStr(round), 1.0, 0, 1.0, 1.0, 1.0, 1.0);
	x = Say(x, 74.0, "   WON ", 1.0, 0, 0.7, 0.85, 1.0, 1.0);
	x = Say(x, 74.0, NumStr(wins), 1.0, 0, 1.0, 1.0, 1.0, 1.0);
	x = Say(x, 74.0, "   LOST ", 1.0, 0, 0.7, 0.85, 1.0, 1.0);
	Say(x, 74.0, NumStr(losses), 1.0, 0, 1.0, 1.0, 1.0, 1.0);
	x = Say(20.0, 92.0, "SPEED ", 1.0, 0, 0.7, 0.85, 1.0, 1.0);
	int sp = int(speed * 10.0 + 0.5);
	x = Say(x, 92.0, NumStr(sp / 10), 1.0, 0, 1.0, 1.0, 1.0, 1.0);
	x = Say(x, 92.0, ".", 1.0, 0, 1.0, 1.0, 1.0, 1.0);
	x = Say(x, 92.0, NumStr(sp % 10), 1.0, 0, 1.0, 1.0, 1.0, 1.0);
	Say(x, 92.0, " cells/s", 1.0, 0, 0.7, 0.85, 1.0, 1.0);

	// wreck log, top right
	for (int k = 0; k < 5; k++)
	{
		if (logT[k] > 0.0)
		{
			float a = logT[k];
			if (a > 1.0)
				a = 1.0;
			float y = 18.0 + k * 20.0;
			int v = logV[k];
			int o = logK[k];
			float w = TextLen(shipName[v]) + 20;
			if (o >= 0)
				w = w + TextLen(shipName[o]);
			x = winW - 20.0 - w * 9.0;
			x = Say(x, y, shipName[v], 1.0, 0, shipCol[v * 3], shipCol[v * 3 + 1], shipCol[v * 3 + 2], a);
			if (logKind[k] == 1)
				x = Say(x, y, " shot down by ", 1.0, 0, 0.8, 0.8, 0.8, a);
			else if (o == v)
				x = Say(x, y, " hit own trail", 1.0, 0, 0.8, 0.8, 0.8, a);
			else
				x = Say(x, y, " crashed into ", 1.0, 0, 0.8, 0.8, 0.8, a);
			if (o != v || logKind[k] == 1)
				Say(x, y, shipName[o], 1.0, 0, shipCol[o * 3], shipCol[o * 3 + 1], shipCol[o * 3 + 2], a);
		}
	}

	if (countdown > 0.0)
		Say(cx, cy - 120.0, NumStr(int(countdown) + 1), 5.0, 1, 1.0, 1.0, 1.0, 1.0);
	if (roundOver == 1)
	{
		Say(cx, cy - 110.0, "WRECKED", 4.0, 1, 1.0, 0.3, 0.25, 1.0);
		Say(cx, cy + 40.0, "Enter: next round", 1.5, 1, 0.9, 0.9, 0.9, 1.0);
	}
	if (roundOver == 2)
	{
		Say(cx, cy - 110.0, "LAST ONE FLYING", 3.0, 1, 0.4, 1.0, 0.6, 1.0);
		Say(cx, cy + 40.0, "Enter: next round", 1.5, 1, 0.9, 0.9, 0.9, 1.0);
	}
	if (paused)
		Say(cx, cy - 60.0, "PAUSED", 3.0, 1, 1.0, 1.0, 1.0, 1.0);
	if (round == 1 && roundT < 12.0)
		Say(cx, winH - 172.0, "WASD/arrows turn   Space fire   drag mouse to look   I invert   P pause", 1.0, 1, 0.75, 0.85, 1.0, 0.9);
	glDisable(GL_TEXTURE_2D);
}

// ======================================================================= main
void Update(float dt)
{
	for (int k = 0; k < 5; k++)
		logT[k] = logT[k] - dt;
	UpdateParticles(dt);
	if (countdown > 0.0)
	{
		countdown = countdown - dt;
		return;
	}
	roundT = roundT + dt;
	speed = 5.5 + roundT * 0.05;
	if (speed > 10.0)
		speed = 10.0;
	for (int i = 0; i < MAXSH; i++)
	{
		if (shAlive[i])
		{
			shCool[i] = shCool[i] - dt;
			MoveShip(i, dt);
		}
	}
	UpdateShots(dt);
	for (int i = 0; i < MAXSH; i++)
	{
		if (shAlive[i] == 0 && shGone[i] == 0)
		{
			shDeadT[i] = shDeadT[i] + dt;
			shFade[i] = 1.0 - (shDeadT[i] - 1.0) / 1.5;
			if (shFade[i] > 1.0)
				shFade[i] = 1.0;
			if (shFade[i] <= 0.0)
			{
				shFade[i] = 0.0;
				RemoveTrail(i);
				shGone[i] = 1;
			}
		}
	}
	for (int k = 0; k < MAXK; k++)
	{
		if (kOn[k] == 0)
		{
			kTimer[k] = kTimer[k] - dt;
			if (kTimer[k] <= 0.0)
				PlacePickup(k);
		}
	}
	if (roundOver == 0)
	{
		int rivals = 0;
		for (int i = 1; i < MAXSH; i++)
			rivals = rivals + shAlive[i];
		if (shAlive[0] == 0)
		{
			roundOver = 1;
			losses = losses + 1;
		}
		else if (rivals == 0)
		{
			roundOver = 2;
			wins = wins + 1;
		}
	}
}

void QueueTurn(int code)
{
	if (shAlive[0] == 0 || roundOver != 0)
		return;
	if (tqn[0] < 3)
	{
		tq[tqn[0]] = code;
		tqn[0] = tqn[0] + 1;
	}
}

int main()
{
	for (int i = 0; i < NC; i++)
		vis[i] = 0;
	for (int i = 0; i < MAXSH; i++)
	{
		shKills[i] = 0;
		shAlive[i] = 0;
		shFade[i] = 1.0;
	}
	for (int k = 0; k < 5; k++)
		logT[k] = 0.0;
	for (int q = 0; q < MAXQ; q++)
		qOn[q] = 0;
	numBuf = malloc(16);

	glwMultisample(4);
	if (!glwInit(winW, winH, "Torus Trails"))
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}
	seed = int(glwTime() * 10000000.0) + 4711;
	prog = glwCreateProgram(vsSrc, fsSrc);
	if (prog == 0)
	{
		glwDone();
		return 1;
	}
	uCam = glGetUniformLocation(prog, "cam");
	uGridN = glGetUniformLocation(prog, "gridN");
	uMode = glGetUniformLocation(prog, "mode");
	uCol = glGetUniformLocation(prog, "col");
	uFade = glGetUniformLocation(prog, "fade");
	uFogEnd = glGetUniformLocation(prog, "fogEnd");
	uTh = glGetUniformLocation(prog, "th");
	BuildScenery();
	BuildFont();
	glClearColor(0.0, 0.0, 0.012, 1.0);
	glwSwapInterval(1);

	StartRound();
	UpdateCamera(0.0, 1);

	int ev[5];
	int running = 1;
	int dragging = 0;
	int lastX = 0;
	int lastY = 0;
	int fullscreen = 0;
	double tPrev = glwTime();
	while (running && glwPoll())
	{
		double now = glwTime();
		float dt = now - tPrev;
		seed = seed + int(dt * 1000000.0);
		tPrev = now;
		if (dt > 0.05)
			dt = 0.05;
		if (glwResized())
		{
			winW = glwWidth();
			winH = glwHeight();
			glViewport(0, 0, winW, winH);
		}
		while (glwNextEvent(ev))
		{
			if (ev[0] == GLW_EV_KEY && ev[4] == 0)
			{
				int k = ev[1];
				int l = ev[3];
				if (k == GLW_KEY_ESC)
					running = 0;
				else if (k == GLW_KEY_UP || l == 'w')
				{
					if (invertY)
						QueueTurn(2);
					else
						QueueTurn(1);
				}
				else if (k == GLW_KEY_DOWN || l == 's')
				{
					if (invertY)
						QueueTurn(1);
					else
						QueueTurn(2);
				}
				else if (k == GLW_KEY_LEFT || l == 'a')
					QueueTurn(3);
				else if (k == GLW_KEY_RIGHT || l == 'd')
					QueueTurn(4);
				else if (k == ' ')
				{
					if (roundOver != 0)
					{
						StartRound();
						UpdateCamera(0.0, 1);
					}
					else if (countdown <= 0.0 && paused == 0 && shAlive[0])
						Fire(0);
				}
				else if (k == GLW_KEY_ENTER)
				{
					if (roundOver != 0)
					{
						StartRound();
						UpdateCamera(0.0, 1);
					}
				}
				else if (l == 'p')
					paused = 1 - paused;
				else if (l == 'i')
					invertY = 1 - invertY;
				else if (l == 'f')
				{
					fullscreen = 1 - fullscreen;
					glwSetFullscreen(fullscreen);
				}
			}
		}
		// mouse look: drag to look around, let go and the view swings back
		int mx = glwMouseX();
		int my = glwMouseY();
		if (glwMouseButtons() != 0)
		{
			if (dragging)
			{
				lookYaw = lookYaw + 0.3 * (mx - lastX);
				lookPitch = lookPitch + 0.3 * (my - lastY);
				if (lookYaw > 180.0) lookYaw = 180.0;
				if (lookYaw < -180.0) lookYaw = -180.0;
				if (lookPitch > 85.0) lookPitch = 85.0;
				if (lookPitch < -85.0) lookPitch = -85.0;
			}
			dragging = 1;
		}
		else
		{
			dragging = 0;
			float k = exp(-dt * 5.0);
			lookYaw = lookYaw * k;
			lookPitch = lookPitch * k;
		}
		lastX = mx;
		lastY = my;

		if (paused == 0)
		{
			Update(dt * 0.5);
			Update(dt * 0.5);
		}
		UpdateCamera(dt, 0);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawWorld(now);
		DrawHUD(now);
		glwSwap();
	}
	glwDone();
	return 0;
}
