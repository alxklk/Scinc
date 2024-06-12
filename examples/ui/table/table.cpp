#include <stdio.h>

#define G_SCREEN_WIDTH 1024
#define G_SCREEN_HEIGHT 640

#ifdef __JS__
#define G_SCREEN_SCALE 1
#else
#define G_SCREEN_SCALE 2
#endif

#include "graphics.h"

#define DIGI_ASSERT(x)

#define bool int
#define true 1
#define false 0

#define NO_GREEDY_EVENT

#include "../../ws.h"

int numEntries = 16;
int numInputs = 40;

int widths[128];
int curws[128];
int totalWidth = 0;

struct SMovingItem
{
	int src;
	int dst;
	int row;
};

class TStemDialog
{
public:
	bool hid[128];
	void Init()
	{
		for (int i = 0; i < 128; i++)
		{
			hid[i] = false;
		}
	}
	bool ShouldHideColumn(int col, int st)
	{
		if (col > 0 && col < 128)
			return hid[col-1];
		return false;
	}
};

TStemDialog stemDialog;

class ItemList
{
public:
	int N;
	int s[128];
	int operator[](int i) const
	{
		return s[i];
	}
	void push_back(int x) { s[N++] = x; }
	int size() const { return N; }
};

class TStemDialogEntry
{
public:
	ItemList list;
	int sel; // bitmasl
	void ClearSel() { sel = 0; }
	void SetSel(int i) { sel = sel | (1 << i); }
	void SelAll(int i) { sel = 0xffffffff; }
	bool IsNthItemSelected(int i)
	{
		return sel & (1 << (i-1));
	};

	void Select(int idx)
	{
		for (int i = 0; i < list.size(); i++)
		{
			if (list[i] == idx)
			{
				SetSel(i);
				return;
			}
		}
	}
	bool IsSelected(int idx)
	{
		for (int i = 0; i < list.size(); i++)
		{
			if (list[i] == idx)
			{
				return IsNthItemSelected(i+1);
			}
		}
		return false;
	}
	ItemList &GetItemList()
	{
		return list;
	}
	void Init()
	{
		list.N = 0;
	}
};

typedef int EStemDialogPane;
typedef int Cmn_Int16;

struct GridCell
{
	int x;
	int y;
	int GetH() { return x; }
	int GetV() { return x; }
	void SetV(int iy) { y = iy; }
	void SetH(int ix) { x = ix; }
};

class TStemContainerView
{
public:
	TStemDialog *GetStemDialog() { return &stemDialog; }
	EStemDialogPane GetStemType() { return 1; }
};

TStemContainerView contView;

struct VRect
{
	int x;
	int y;
	int w;
	int h;
};

struct CRect
{
	int x;
	int y;
	int w;
	int h;
};

class GridView
{
public:
	TStemContainerView mContView;
	TStemContainerView *GetContainerView() { return &mContView; }
	TStemDialogEntry entries[32];
	TStemDialogEntry *GetEntry(int rowNum)
	{
		if (rowNum <= 0)
			return 0;
		if (rowNum > numEntries)
			return 0;
		return &entries[rowNum-1];
	}
	void Init()
	{
		fNumOfCols = 64;
		for (int i = 0; i < 32; i++)
		{
			entries[i].Init();
		}
	}
	int GetNumberEntries() { return 16; };
	int fNumOfCols;
	void CellToVRect(GridCell &cell, VRect &vRect)
	{
		if (cell.x <= 0)
		{
			vRect.x = 0;
			vRect.y = 0;
			vRect.w = 10;
			vRect.w = 10;
			return;
		}
		vRect.x = 140 + curws[cell.x];
		vRect.y = 70 + (cell.y) * 20;
		vRect.w = widths[cell.x];
		vRect.h = 20;
	}
	void ViewToQDRect(VRect &vRect, CRect &cRect)
	{
		cRect.x = vRect.x;
		cRect.y = vRect.y;
		cRect.w = vRect.w;
		cRect.h = vRect.h;
	}
};

GridView *mMainGridView;

bool rectSelect = false;
int rectX;
int rectY;
int rectW;
int rectH;
bool moving = false;
int moveStartX = 0;
int moveStartY = 0;
int moveDeltaX = 0;
int moveDeltaY = 0;

Graph g;

int gRGBColorDkGray = 0;

struct Cmn_Point32s
{
	int x;
	int y;
};

Cmn_Point32s Cmn_Point32(int x, int y)
{
	return {x, y};
}

struct Cmn_Rect32
{
	int x;
	int y;
	int w;
	int h;
	void Inset(Cmn_Point32s r)
	{
		return;
		x += r.x;
		y += r.y;
		w -= r.x * 2;
		h -= r.y * 2;
	}
};

struct DFW_Painter
{
	bool Get() { return true; }
	void SetForegroundColor(int col)
	{
	}
	void DrawRect(Cmn_Rect32 &r)
	{
		g.rgba32(0x8000ff00);
		g.lineH(r.x, r.y, r.w);
		g.lineH(r.x, r.y + r.h - 1, r.w);
		g.lineV(r.x, r.y + 1, r.h - 2);
		g.lineV(r.x + r.w - 1, r.y + 1, r.h - 2);
	}
};

typedef DFW_Painter *DFW_Painter_SmartPtr;

DFW_Painter overlay;

DFW_Painter_SmartPtr GetOverlayPainter(bool f) { return &overlay; }

int Abs(int x)
{
	return x < 0 ? -x : x;
}

Cmn_Rect32 RectToCmn_Rect(CRect &r)
{
	return {r.x, r.y, r.w, r.h};
}

bool IsMouseEvent(int eventType)
{
	if (eventType == 'MMOV')
		return true;
	if (eventType == 'MLUP')
		return true;
	if (eventType == 'MLDN')
		return true;
	if (eventType == 'MMUP')
		return true;
	if (eventType == 'MMDN')
		return true;
	if (eventType == 'MRUP')
		return true;
	if (eventType == 'MRDN')
		return true;
	return false;
}

void DrawRect(int rectX, int rectY, int rectW, int rectH, int col)
{
	if (rectW < 0)
	{
		rectW = -rectW;
		rectX -= rectW;
	}

	g.lineH(rectX, rectY, rectW, col);
	g.lineV(rectX, rectY, rectH, col);
	g.lineH(rectX + rectW + 1, rectY + rectH, -rectW - 1, col);
	g.lineV(rectX + rectW, rectY + rectH, -rectH, col);
}

void RectSelect()
{
	int rX = rectX;
	int rY = rectY;
	int rW = rectW;
	int rH = rectH;
	if (rW < 0)
	{
		rX += rW;
		rW = -rW;
	}
	if (rH < 0)
	{
		rY += rH;
		rH = -rH;
	}
	if (rY + rH < 70)
		return;

	int selStartX = (rX - 140);
	int selStartY = (rY - 70) / 20;
	int selEndX = (rX - 140 + rW);
	int selEndY = (rY - 70 + rH) / 20;

	for (int i = 0; i < numEntries; i++)
	{
		mMainGridView->GetEntry(i)->ClearSel();
	}

	for (int j = 0; j <= numInputs; j++)
	{
		if ((curws[j] + widths[j] >= selStartX) && (curws[j] < selEndX))
		{
			for (int i = selStartY; i <= selEndY; i++)
			{
				TStemDialogEntry &ent = *mMainGridView->GetEntry(i);
				ent.Select(j);
			}
		}
	}
}

void AdjustColumnWidths(TStemDialog *stemDialog)
{
	totalWidth = 0;
	for (int i = 0; i < numInputs; i++)
	{
		curws[i] = totalWidth;
		if (stemDialog->ShouldHideColumn(i+1, 1))
		{
			widths[i] = 4;
		}
		else
		{
			widths[i] = 20;
		}
		totalWidth += widths[i];
	}
}

bool useNew = false;

int main()
{
	SScincEvent event;

	GridView mGridView;
	mGridView.Init();
	mMainGridView = &mGridView;
	TStemDialog nStemDialog;
	nStemDialog.Init();
	nStemDialog.hid[12] = 1;
	nStemDialog.hid[13] = 1;
	nStemDialog.hid[14] = 1;
	nStemDialog.hid[15] = 1;

	nStemDialog.hid[20] = 1;
	nStemDialog.hid[21] = 1;
	nStemDialog.hid[22] = 1;
	nStemDialog.hid[23] = 1;

	nStemDialog.hid[0] = 1;
	nStemDialog.hid[1] = 1;
	nStemDialog.hid[36] = 1;
	nStemDialog.hid[37] = 1;
	nStemDialog.hid[38] = 1;
	nStemDialog.hid[39] = 1;

	TStemDialog *mStemDialog = &nStemDialog;
	{
		int curItem = 0;
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				while (true)
				{
					if (nStemDialog.ShouldHideColumn(curItem+1, 1))
					{
						curItem++;
					}
					else
					{
						break;
					}
				}
				mMainGridView->GetEntry(i+1)->GetItemList().push_back(curItem);
				curItem++;
			}
		}
	}
	int hoverx = -1;
	int hovery = -1;
	int clickx = -1;
	int clicky = -1;
	int markEntry = -1;

	int moveStartCell = -1;
	int moveHoverCell = -1;

	AdjustColumnWidths(mStemDialog);

	while (true)
	{
		WaitForScincEvent(.5);
		while (GetScincEvent(event))
		{
			if (event.type == 'KBDN')
			{
				if (event.x == KEYCODE_N)
				{
					useNew = !useNew;
					printf("%s\n", useNew?"use new":"use old");
				}
			}
			if (IsMouseEvent(event.type))
			{
				int mx = event.x;
				int my = event.y;
				hoverx = (mx - 140);
				for (int i = 0; i < numInputs; i++)
				{
					int loc = hoverx - curws[i];
					if ((loc >= 0) && (loc < widths[i]))
					{
						hoverx = i;
						break;
					}
				}
				hovery = (my - 70);
				if (hovery < 0)
					hovery = -1;
				else
					hovery /= 20;

				if (event.type == 'MLDN')
				{
					moveStartCell = hoverx;
					clickx = hoverx;
					clicky = hovery;
					rectX = mx;
					rectY = my;
					rectW = 0;
					rectH = 0;
					moveStartX = mx;
					moveStartY = my;
					moveDeltaX = 0;
					moveDeltaY = 0;
					rectSelect = false;
					TStemDialogEntry *ent = mMainGridView->GetEntry(hovery+1);
					if ((hovery >= 0) && (hovery < numEntries) && ent->IsSelected(hoverx+1))
					{
						moving = true;
					}
					else
					{
						rectSelect = true;
						RectSelect();
					}
				}
				else if (event.type == 'MLUP')
				{
					if (hovery == -1)
					{
						if ((hoverx >= 0) && (hoverx < numInputs))
						{
							printf("Toggle input %i: %i\n", hoverx, nStemDialog.hid[hoverx]);
							nStemDialog.hid[hoverx] = !nStemDialog.hid[hoverx];
							AdjustColumnWidths(mStemDialog);
						}
					}
					if ((clickx == hoverx) && (clicky == hovery))
					{
						if ((hovery >= 0) && (hovery < numEntries))
						{
							for (int i = 0; i < numEntries; i++)
								mMainGridView->GetEntry(i+1)->ClearSel();
							TStemDialogEntry *entry = mMainGridView->GetEntry(hovery+1);
							entry->Select(hoverx);
						}
					}
					rectSelect = false;
					moving = false;
				}
				else if (event.type == 'MMOV')
				{
					if (moving)
					{
						moveDeltaX = mx - moveStartX;
						moveDeltaY = my - moveStartY;
						moveHoverCell = hoverx;
					}
					if (rectSelect)
					{
						rectW = mx - rectX;
						rectH = my - rectY;
						RectSelect();
					}
				}
			}
		}

		g.rgba32(0xff505050);
		g.FillRT();
		g.rgba32(0xff000000);
		for (int i = 0; i < numEntries; i++)
		{
			int col = (i & 1) ? 0xff676767 : 0xff575757;
			g.Rect(0, 70 + i * 20, 140 + totalWidth, 21, col);
		}

		{

			char *ls[8] = {"L", "R", "C", "Lss", "Rss", "Lsr", "Rsr", "LFE"};

			for (int i = 0; i < numEntries; i++)
			{
				TStemDialogEntry *entry = mMainGridView->GetEntry(i+1);
				ItemList &list = entry->GetItemList();

				for (int l = 0; l < list.N; l++)
				{
					int col = 0xff404040;
					int j = list[l];
					int x = 141 + curws[j];
					int y = 70 + i * 20;
					int w = widths[j];
					int h = 20;
					g.Rect(x, y, w - 1, h, col);
					g.Clip(x, y, w, h);
					if (l < 8)
						stext(ls[l], 141 + widths[j] / 2 + curws[j] - 3, 75 + i * 20, 0xffffffff);
					g.UnClip();
					if (!moving && (entry->IsNthItemSelected(l+1)))
						DrawRect(x, y + 1, w - 2, h - 1, 0xffffffff);
				}
			}
		}

		int lineCol = 0x40000000;
		g.lineH(140, 10, totalWidth, lineCol);
		g.lineH(140, 30, totalWidth, lineCol);
		g.lineH(140, 50, totalWidth, lineCol);
		g.lineH(0, 70, 140 + totalWidth, lineCol);
		g.lineH(0, 70 + 20 * numEntries, 140 + totalWidth, lineCol);
		{
			stext("Nucleus", 450, 15, 0xffffffff);
			int cur = 140;
			for (int i = 0; i <= numInputs; i++)
			{
				int dl = 20;
				if (i == 0)
					dl -= 20;
				if (i == numInputs)
					dl -= 20;
				if (i % 8)
					dl += 20;
				g.lineV(cur, 10 + dl, 60 + 20 * numEntries - dl, lineCol);
				if (i && (widths[i - 1] > 5))
				{
					char s[10];
					snprintf(s, 10, "%i", (i - 1) % 8 + 1);
					stext(s, cur - 12, 55, 0xffffffff);
					//snprintf(s, 10, "%i", i - 1);
					//stext(s, cur - 12, 45, 0x90ffffff);
				}
				cur += widths[i];
			}
		}

		for (int i = 0; i < numEntries; i++)
		{
			char s[10];
			snprintf(s, 10, "%i", i + 1);
			stext(s, 10, 75 + i * 20, 0xffffffff);
			snprintf(s, 10, "Out %i-%i", i * 2 + 1, i * 2 + 2);
			stext(s, 30, 75 + i * 20, 0xffffffff);
		}

		if (rectSelect)
		{
			g.Rect(rectX, rectY, rectW, rectH, 0x2000ffff);
			DrawRect(rectX, rectY, rectW, rectH, 0xff00ffff);
		}
		else if ((hovery >= 0) && (hovery < numEntries))
		{
			// g.Rect(0,70+hovery*20,140+20*numInputs,20,0x2000ff00);
			if ((hoverx >= 0) && (hoverx < numInputs))
			{
				g.Rect(140 + curws[hoverx], 70 + hovery * 20, widths[hoverx], 20, 0x200080ff);
				DrawRect(140 + curws[hoverx], 70 + hovery * 20, widths[hoverx], 20, 0x500080ff);
			}
		}

		if (hovery == -1)
		{
			if ((hoverx >= 0) && (hoverx < numInputs))
			{
				g.Rect(140 + curws[hoverx], 70 + hovery * 20, widths[hoverx], 20, 0x200080ff);
				DrawRect(140 + curws[hoverx], 70 + hovery * 20, widths[hoverx], 20, 0x500080ff);
			}
		}

		if (moving)
		{

			if (!useNew)
			{
				// **********************************

				int dx = hoverx - moveStartCell;
				int minx = 10000000;
				int maxx = -1;
				for (int i = 0; i < numEntries; i++)
				{
					TStemDialogEntry *entry = mMainGridView->GetEntry(i);
					const ItemList &list = entry->GetItemList();
					for (int l = 0; l < list.size(); l++)
					{
						if (entry->IsNthItemSelected(l+1))
						{
							int d = list[l] + dx;
							if (d < minx)
							{
								minx = d;
							}
							if (d > maxx)
							{
								maxx = d;
							}
						}
					}
				}
				if (minx < 0)
				{
					dx -= minx;
				}
				if (maxx >= numInputs)
				{
					dx -= maxx - numInputs + 1;
				}

				SMovingItem movItems[128];
				int nMovItems = 0;
				int dragged = -1;
				for (int i = 0; i < numEntries; i++)
				{
					TStemDialogEntry *entry = mMainGridView->GetEntry(i+1);
					ItemList &list = entry->GetItemList();
					for (int l = 0; l < list.N; l++)
					{
						if (entry->IsNthItemSelected(l+1))
						{
							movItems[nMovItems].row = i;
							movItems[nMovItems].src = list[l];
							movItems[nMovItems].dst = list[l];
							if (list[l] == clickx)
							{
								dragged = nMovItems;
							}
							nMovItems++;
						}
					}
				}
				{
					char s[64];
					snprintf(s, 64, "cli=%i dra[%i]=%i", clickx, dragged, (dragged == -1) ? -1 : movItems[dragged].src);
					//stext(s, 10, 400, 0xff00ff00);
				}

				//printf("\nMoving old\n");
				//for(int i=0;i<nMovItems;i++)
				//printf("[%i->%i,%i] ", movItems[i].src, movItems[i].dst, movItems[i].row);

				bool limit = false;
				printf("dragged=%i hover=%i dx=%i\n", dragged, hoverx, dx);
				if (dx != 0)
				{
					int dir = 0;
					int count = 0;
					if (dx > 0)
					{
						dir = 1;
						count = dx;
					}
					else if (dx < 0)
					{
						dir = -1;
						count = -dx;
					}
					for (int j = 0; j < count; j++)
					{
						if (dragged != -1)
						{
							if ((dir == 1) && (movItems[dragged].dst >= hoverx))
							{
								printf("Got dragged up\n");
								break;
							}
							else if ((dir == -1) && (movItems[dragged].dst <= hoverx))
							{
								printf("Got dragged donw\n");
								break;
							}
						}
						int movDstBak[128];
						for (int i = 0; i < nMovItems; i++)
						{
							movDstBak[i] = movItems[i].dst;
						}
						for (int i = 0; i < nMovItems; i++)
						{
							SMovingItem &mi = movItems[i];
							mi.dst += dir;
							if (mi.dst < 0)
							{
								limit = true;
								break;
							}
							if (mi.dst >= numInputs)
							{
								limit = true;
								break;
							}
							while (nStemDialog.ShouldHideColumn(mi.dst + 1, 1))
							{
								mi.dst += dir;
								if (mi.dst < 0)
								{
									limit = true;
									printf("limit\n");
									break;
								}
								if (mi.dst >= numInputs)
								{
									limit = true;
									printf("limit\n");
									break;
								}
							}
							if (limit)
							{
								break;
							}
						}
						if (limit)
						{
							for (int i = 0; i < nMovItems; i++)
							{
								movItems[i].dst = movDstBak[i];
							}
							break;
						}
					}
				}

				for (int i = 0; i < nMovItems; i++)
				{
					SMovingItem &mi = movItems[i];
					int col = 0xff404040;
					int x = 141 + curws[mi.dst];
					int y = 70 + ( mi.row + 1 ) * 20;
					int w = widths[mi.dst];
					int h = 20;
					if (limit)
						DrawRect(x, y + 1, w - 2, h - 1, 0xffff8000);
					else
						DrawRect(x, y + 1, w - 2, h - 1, 0xffffff00);
				}

				// **********************************
			}
			else
			{

				GridCell mCurrentCell = {hoverx, hovery};
				GridCell mStartCell = {clickx, clicky};

				{
					int numEntries = mMainGridView->GetNumberEntries();
					int numInputs = mMainGridView->fNumOfCols;
					TStemDialog *stemDialog = mMainGridView->GetContainerView()->GetStemDialog();
					EStemDialogPane stemType = mMainGridView->GetContainerView()->GetStemType();

					Cmn_Int16 cellOffset = mCurrentCell.GetH() - mStartCell.GetH();

					// int dx=hoverx-moveStartCell;
					int dx = cellOffset;

					int clickx = mStartCell.GetH();
					int hoverx = mCurrentCell.GetH();

					// **********************************

					int minx = 10000000;
					int maxx = -1;
					for (int i = 0; i < numEntries; i++)
					{
						TStemDialogEntry *entry = mMainGridView->GetEntry(i + 1);
						const ItemList &list = entry->GetItemList();
						for (int l = 0; l < list.size(); l++)
						{
							if (entry->IsNthItemSelected(l))
							{
								int d = list[l] + dx;
								if (d < minx)
								{
									minx = d;
								}
								if (d > maxx)
								{
									maxx = d;
								}
							}
						}
					}
					if (minx < 0)
					{
						dx -= minx;
					}
					if (maxx >= numInputs)
					{
						dx -= maxx - numInputs + 1;
					}

					SMovingItem movItems[512];
					int nMovItems = 0;
					int dragged = -1;
					for (int i = 0; i < numEntries; i++)
					{
						TStemDialogEntry *entry = mMainGridView->GetEntry(i + 1);
						const ItemList &list = entry->GetItemList();
						for (int l = 0; l < list.size(); l++)
						{
							if (entry->IsNthItemSelected(l + 1))
							{
								movItems[nMovItems].row = i;
								movItems[nMovItems].src = list[l];
								movItems[nMovItems].dst = list[l];
								if (list[l] == clickx)
								{
									dragged = nMovItems;
								}
								nMovItems++;
							}
						}
					}

					//printf("\nMoving\n");
					//for(int i=0;i<nMovItems;i++)
					//printf("[%i->%i,%i] ", movItems[i].src, movItems[i].dst, movItems[i].row);

					bool limit = false;
					printf("dragged=%i hover=%i dx=%i\n", dragged, hoverx, dx);
					if (dx != 0)
					{
						int dir = 0;
						int count = 0;
						if (dx > 0)
						{
							dir = 1;
							count = dx;
						}
						else if (dx < 0)
						{
							dir = -1;
							count = -dx;
						}
						int mpy=420;
						int mpx=10;
						for (int j = 0; j < count; j++)
						{
							if (dragged != -1)
							{
								if((dir==1)&&movItems[dragged].dst>=hoverx)
								{
									printf("Got dragged up\n");
									break;
								}
								else if((dir==-1)&&(movItems[dragged].dst<=hoverx))
								{
									printf("Got dragged donw\n");
									break;
								}
							}
							int movDstBak[128];
							for (int i = 0; i < nMovItems; i++)
							{
								movDstBak[i] = movItems[i].dst;
							}
							for (int i = 0; i < nMovItems; i++)
							{
								SMovingItem &mi = movItems[i];
								mi.dst += dir;
								if (mi.dst < 0)
								{
									limit = true;
									break;
								}
								if (mi.dst >= numInputs)
								{
									limit = true;
									break;
								}
								while (stemDialog->ShouldHideColumn(mi.dst + 1, stemType))
								{
									mi.dst += dir;
									if (mi.dst < 0)
									{
										limit = true;
										break;
									}
									if (mi.dst >= numInputs)
									{
										limit = true;
										break;
									}
								}
								if (limit)
								{
									break;
								}
							}
							if (limit)
							{
								for (int i = 0; i < nMovItems; i++)
								{
									movItems[i].dst = movDstBak[i];
								}
								break;
							}
						}
					}

					DFW_Painter_SmartPtr aPainter = GetOverlayPainter(true);
					for (int i = 0; i < nMovItems; i++)
					{
						SMovingItem &mi = movItems[i];
						// int col=0xff404040;
						// int x=141+curws[mi.dst];
						// int y=70+mi.row*20;
						// int w=widths[mi.dst];
						// int h=20;
						// if(limit)
						//	DrawRect(x,y+1,w-2,h-1,0xffff8000);
						// else
						// DrawRect(x,y+1,w-2,h-1,0xffffff00);
						GridCell nthItemCell;
						VRect aVRect;
						CRect aCRect;
						nthItemCell.SetV(mi.row + 1);
						nthItemCell.SetH(mi.dst + 1);
						mMainGridView->CellToVRect(nthItemCell, aVRect);
						mMainGridView->ViewToQDRect(aVRect, aCRect);
						// if (aPainter.Get() != 0)
						{
							aPainter->SetForegroundColor(gRGBColorDkGray);
							Cmn_Rect32 drawR = RectToCmn_Rect(aCRect);
							aPainter->DrawRect(drawR);
							drawR.Inset(Cmn_Point32(1, 1));
							aPainter->DrawRect(drawR);
						}
					}
				}
			}
		}

		Present();
	}
	return 0;
}