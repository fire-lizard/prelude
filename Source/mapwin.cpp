#include "mapwin.h"
#include "ZSEngine.h"
#include "World.h"
#include "ZSutilities.h"
#include "party.h"
#include "maplocator.h"
#include "zsbutton.h"
#include "Minimap.h"
#include "zsconfirm.h"
#include "ZSHelpWin.h"
#include "events.h"
#include "script.h"

//breathing room between the tag text and the edge of its plaque
#define TAG_PAD 3
#include "Flags.h"

#define MAP_FACTOR (512.0f/3200.0f)
#define MAP_X_OFFSET 37
#define MAP_Y_OFFSET 32
#define MAX_MAP_ZOOM	3
#define MIN_MAP_ZOOM	0

typedef enum
{
	IDC_MAP_NONE,
	IDC_MAP_HERE,
	IDC_MAP_EXIT,
	IDC_MAP_WEST,
	IDC_MAP_NORTH,
	IDC_MAP_SOUTH,
	IDC_MAP_EAST,
	IDC_MAP_ZOOMIN,
	IDC_MAP_ZOOMOUT,
	IDC_MAP_VALLEY = 50,
	IDC_MAP_TOWN_0,
	IDC_MAP_LOCATION = 100,
	

} MAP_WIN_CONTROLS;

LPDIRECTDRAWSURFACE7 MapWin::MapBackground = NULL;
LPDIRECTDRAWSURFACE7 MapWin::MapSurface = NULL;

// Which slice of the world each 400x400 town bitmap covers, so the party can be
// plotted on it. The town maps are hand drawn and carry no scale of their own -
// these were recovered by fitting the world/bitmap coordinate pairs that the
// addlocation script calls in events.txt and people.txt already give for every
// town landmark (median fit error is 5-17 bitmap pixels, so the marker lands on
// the right block, not necessarily the right doorstep). Each one contains that
// town's World::TownAreas rect, and Citadel's spans both the West and East
// Citadel areas, matching the way Show() folds those two into one map.
//
// To redo these: collect the 7-argument addlocation calls, group by map number,
// and take a median-slope fit of world x/y against map x/y per town.
static const struct
{
	const char *Name;
	int left, top, right, bottom;
} TownMapAreas[] =
{
	{ "Kellen",         1043,  970, 1443, 1438 },
	{ "Jerrock",          35,  852,  147, 1055 },
	{ "Crossing East",  1982, 1072, 2436, 1610 },
	{ "the Barrier",    2866, 2509, 3062, 2743 },
	{ "the Academy",     201, 2605,  312, 2761 },
	{ "the Monastery",    44,  -26,  412,  414 },
	{ "Ironwood",       2793,  247, 3204,  762 },
	{ "Citadel",        1729,    3, 2058,  365 },
	{ "Land's End",      301, 1642, 1612, 2987 },
};
#define NUM_TOWN_MAP_AREAS (int)(sizeof(TownMapAreas) / sizeof(TownMapAreas[0]))
#define TOWN_MAP_SIZE 400


void MapWin::SwitchTowns(int NewTown)
{
	if(CurTown == NewTown)
		return;

	CurTown = NewTown;
	int LocID;

	//clear old locators
	LocID = IDC_MAP_LOCATION;
	ZSWindow *pWin;
	
	pWin = GetChild(LocID);
	while(pWin)
	{
		RemoveChild(pWin);
		pWin = NULL;
		LocID++;
		pWin = GetChild(LocID);
	}

	MapLocator *pLocator;

	pLocator = PreludeParty.GetLocations();

	NumLocators = 0;

	ZSButton *pButton;

	//add new locators
	while(pLocator)
	{
		if(pLocator->GetMapNum() == CurTown)
		{
			pButton = new ZSButton(BUTTON_BULLET,IDC_MAP_LOCATION + NumLocators, rMapArea.left + pLocator->GetMapX(), rMapArea.top + pLocator->GetMapY(),8,8);
			//pButton->SetText(pLocator->GetTag());
			AddChild(pButton);
			pLocs[NumLocators] = pLocator;
			NumLocators++;
		}
		pLocator = pLocator->GetNext();
	}

	if(CurTown)
	{
		pWin = GetChild(IDC_MAP_WEST);
		pWin->Hide();
		pWin = GetChild(IDC_MAP_NORTH);
		pWin->Hide();
		pWin = GetChild(IDC_MAP_SOUTH);
		pWin->Hide();
		pWin = GetChild(IDC_MAP_EAST);
		pWin->Hide();
		pWin = GetChild(IDC_MAP_ZOOMIN);
		pWin->Hide();
		pWin = GetChild(IDC_MAP_ZOOMOUT);
		pWin->Hide();
	}
	else
	{
		pWin = GetChild(IDC_MAP_WEST);
		pWin->Show();
		pWin = GetChild(IDC_MAP_NORTH);
		pWin->Show();
		pWin = GetChild(IDC_MAP_SOUTH);
		pWin->Show();
		pWin = GetChild(IDC_MAP_EAST);
		pWin->Show();
		pWin = GetChild(IDC_MAP_ZOOMIN);
		pWin->Show();
		pWin = GetChild(IDC_MAP_ZOOMOUT);
		pWin->Show();
	}

	if(!CurTown)
	{
		if(!MapSurface)
		{
			MapSurface = Engine->Graphics()->CreateSurfaceFromFile("map.bmp", 400, 400, NULL, NULL);
		}
		else
		{
			Engine->Graphics()->LoadFileIntoSurface(MapSurface,"map.bmp",400,400);	
		}
		DrawParty = TRUE;
		Scroll(0,0);
	}
	else
	{
		SetCurrentDirectory(".\\minimaps");

		PTD_ASSERT(CurTown >= 1);
		PTD_ASSERT(CurTown - 1 < 16);
		if(!MapSurface)
		{
			MapSurface = Engine->Graphics()->CreateSurfaceFromFile(&TownBitmaps[CurTown-1][0], 400, 400, NULL, NULL);
		}
		else
		{
			Engine->Graphics()->LoadFileIntoSurface(MapSurface,&TownBitmaps[CurTown-1][0],400,400);
		}

		SetCurrentDirectory(Engine->GetRootDirectory());

		rDrawFrom.left = 0;
		rDrawFrom.right = 400;
		rDrawFrom.top = 0;
		rDrawFrom.bottom = 400;

		DrawParty = FALSE;
		Scroll(0, 0);
	}


}

int MapWin::Draw()
{
	if(Visible)
	{
		//draw our background if any
		if(BackGroundSurface)
		{
			Engine->Graphics()->GetBBuffer()->Blt(&Bounds,BackGroundSurface,NULL,DDBLT_KEYSRC,NULL);
		}

		Engine->Graphics()->GetBBuffer()->Blt(&rMapArea,MapSurface,&rDrawFrom,DDBLT_KEYSRC,NULL);
	
		if(DrawParty)
			Engine->Graphics()->DrawBox(Engine->Graphics()->GetBBuffer(), &rParty, RGB(255,0,0));

		if(GetChild())
		{
			GetChild()->Draw();
		}

		if(pCurLoc)
		{
			//white on a tan map is all but invisible; parchment text is dark
			ZSFontEngine *pFont = Engine->Graphics()->GetFontEngine();
			int TagW = pFont->GetTextWidth(pCurLoc->GetTag());
			int TagH = pFont->GetTextHeight();

			//TagX is the marker's centre, so the tag no longer starts on the
			//marker and runs right across its neighbours
			RECT rTag;
			rTag.left = TagX - TagW / 2 - TAG_PAD;
			rTag.top = TagY;
			rTag.right = rTag.left + TagW + TAG_PAD * 2;
			rTag.bottom = rTag.top + TagH;

			//a tag on a marker near the edge used to be cut off by the map border
			if(rTag.right > rMapArea.right)
			{
				rTag.left -= rTag.right - rMapArea.right;
				rTag.right = rMapArea.right;
			}
			if(rTag.left < rMapArea.left)
			{
				rTag.right += rMapArea.left - rTag.left;
				rTag.left = rMapArea.left;
			}
			if(rTag.top < rMapArea.top)
			{
				rTag.top = rMapArea.top;
				rTag.bottom = rTag.top + TagH;
			}

			//the glyphs are a thin script face that is over half anti-aliased and
			//carries no outline, so on the map's hatching, roads and location
			//squares the text needs a ground of its own to be legible at all.
			//FillSurface writes dwFillColor verbatim and RGB() leaves the top byte
			//clear, so the alpha has to be set here or the fill is transparent -
			//the same trap DrawBox documents.
			Engine->Graphics()->FillSurface(Engine->Graphics()->GetBBuffer(),
				RGB(249,228,175) | 0xFF000000, &rTag);
			Engine->Graphics()->DrawBox(Engine->Graphics()->GetBBuffer(), &rTag, RGB(120,95,70));
			Engine->Graphics()->DrawText(rTag.left + TAG_PAD, rTag.top,
				pCurLoc->GetTag(), TEXT_DARK_GREY_PARCHMENT);
		}


	}

	if(GetSibling())
	{
		GetSibling()->Draw();
	}

	return TRUE;
}


int MapWin::LeftButtonDown(int x, int y, int doubleClick)
{
	
	ZSWindow *pWin;
	
	//check to see if there's child beneath the cursor who should receive the message
	pWin = GetChild(x,y);
	if(pWin)
	{
		return pWin->LeftButtonDown(x,y,doubleClick);
	}

	//if our cursor is the pointing hand, make the finger depress
	if(Cursor == CURSOR_POINT)
	{
		Engine->Graphics()->SetCursorFrame(1);
	}

	SetFocus(this);
	return TRUE;
}

int MapWin::LeftButtonUp(int x, int y)
{
	ReleaseFocus();
	return TRUE;
}

//GoModal
//
int MapWin::GoModal()
{
	//State = WINDOW_STATE_NORMAL;
	//loop while we're not in a done state
	while(State != WINDOW_STATE_DONE && ZSWindow::GetMain()->GetState() != WINDOW_STATE_DONE)
	{
		//ensure that all windows are drawn by call the main window's draw routine
		pMainWindow->Draw();
		Engine->Graphics()->Flip();
		
		//get any user input
		Engine->Input()->Update(pInputFocus);
	}

	return TRUE;
} //GoModal

MapWin::MapWin()
{
	RandomEvent = 0;
	ID = MAP_WIN_ID;
	Visible = FALSE;
	Moveable = TRUE;
	Bounds.left = 0;
	Bounds.top = 0;
	
	FILE *fp;
	fp = SafeFileOpen("gui.ini","rt");
	SeekTo(fp,"[MAP]");
	char *filename;
	int fwidth;
	int fheight;

	SeekTo(fp,"BACKGROUND");

	filename = GetStringNoWhite(fp);
	fwidth = GetInt(fp);
	fheight = GetInt(fp);
	if(!MapSurface)
		MapSurface = Engine->Graphics()->CreateSurfaceFromFile(filename, fwidth, fheight, NULL, NULL);
	else
		Engine->Graphics()->LoadFileIntoSurface(MapSurface,filename,fwidth,fheight);

	delete [] filename;

	D3DVECTOR vScreen;
	vScreen = PreludeWorld->GetCenterScreen();

	RECT rRect;

	rRect.left = (int)(vScreen.x * MAP_FACTOR) - 5 + MAP_X_OFFSET;
	rRect.top = (int)(vScreen.y * MAP_FACTOR) - 5 + MAP_Y_OFFSET;
	rRect.right = (int)(vScreen.x * MAP_FACTOR) + 5 + MAP_X_OFFSET;
	rRect.bottom = (int)(vScreen.y * MAP_FACTOR) + 5 + MAP_Y_OFFSET;

//	Engine->Graphics()->DrawBox(BackGroundSurface, & rRect, D3DRGB(1.0f,1.0f,1.0f));

	Bounds.right = Bounds.left + fwidth;
	Bounds.bottom = Bounds.top + fheight;

	ZSButton *pButton;
	RECT rButton;
	SeekTo(fp,"POSITION");
	LoadRect(&Bounds,fp);

	SeekTo(fp,"MAPAREA");
	LoadRect(&rMapArea,fp);
	
	SeekTo(fp,"MAPEXIT");
	LoadRect(&rButton,fp);
	pButton = new ZSButton("closebutton", IDC_MAP_EXIT, XYWH(rButton),57,34,1);
	pButton->Show();
	AddChild(pButton);

	SeekTo(fp,"ZOOMIN");
	LoadRect(&rButton,fp);
	pButton = new ZSButton(BUTTON_PLUS, IDC_MAP_ZOOMIN, XYWH(rButton));
	pButton->Show();
	AddChild(pButton);

	SeekTo(fp,"ZOOMOUT");
	LoadRect(&rButton,fp);
	pButton = new ZSButton(BUTTON_MINUS, IDC_MAP_ZOOMOUT, XYWH(rButton));
	pButton->Show();
	AddChild(pButton);

	SeekTo(fp,"SCROLLN");
	LoadRect(&rButton,fp);
	pButton = new ZSButton(BUTTON_NORTH, IDC_MAP_NORTH, XYWH(rButton));
	pButton->Show();
	AddChild(pButton);

	SeekTo(fp,"SCROLLE");
	LoadRect(&rButton,fp);
	pButton = new ZSButton(BUTTON_EAST, IDC_MAP_EAST, XYWH(rButton));
	pButton->Show();
	AddChild(pButton);

	SeekTo(fp,"SCROLLS");
	LoadRect(&rButton,fp);
	pButton = new ZSButton(BUTTON_SOUTH, IDC_MAP_SOUTH, XYWH(rButton));
	pButton->Show();
	AddChild(pButton);

	SeekTo(fp,"SCROLLW");
	LoadRect(&rButton,fp);
	pButton = new ZSButton(BUTTON_WEST, IDC_MAP_WEST, XYWH(rButton));
	pButton->Show();
	AddChild(pButton);

	SeekTo(fp,"NUMTOWNS");
	NumTowns = GetInt(fp);

	int n;
	for(n = 0; n < NumTowns; n++)
	{
		GetString(fp,&TownNames[n][0]);
		GetString(fp,&TownBitmaps[n][0]);
	}

	int x;
	int y;
	x = Bounds.left + 460;
	y = Bounds.top + 80;

	pButton = new ZSButton(BUTTON_NONE, IDC_MAP_VALLEY, x,y,104,20);
	pButton->SetTextColor(TEXT_DARK_GREY_PARCHMENT);
	pButton->SetText("the Valley");
	pButton->Show();
	AddChild(pButton);

	y+= 5;

	for(n = 0; n < NumTowns; n++)
	{
		y += 25;
		pButton = new ZSButton(BUTTON_NONE, IDC_MAP_TOWN_0 + n, x, y,104,20);
		pButton->SetTextColor(TEXT_DARK_GREY_PARCHMENT);
		pButton->SetText(&TownNames[n][0]);
		pButton->Show();
		AddChild(pButton);
	}


	//setup all the labels and buttons
	fclose(fp);

	if(!MapBackground)
	{
		BackGroundSurface = Engine->Graphics()->CreateSurfaceFromFile("mapframe.bmp",615, 480,NULL,RGB(255,255,255));
		MapBackground = BackGroundSurface;
	}
	
	BackGroundSurface = MapBackground;
	BackGroundSurface->AddRef();

	ZoomLevel = 0;
	
	rArea.top = 0;
	rArea.bottom = 3200;
	rArea.right = 3200;
	rArea.left = 0;

		//create the child windows;

	

	pButton = new ZSButton("partybutton",IDC_MAP_HERE,0,0,8,8,8,8,1);
	AddChild(pButton);

	CurTown = -1;
	SwitchTowns(0);
	

	Scroll(0,0);
	pCurLoc = 0;
}

void MapWin::Scroll(int XOffset, int YOffset)
{
	//if(CurTown) 
	//	return;
	rArea.left += XOffset;
	rArea.right += XOffset;
	rArea.top += YOffset;
	rArea.bottom += YOffset;

	MapLocator *pLoc;
	pLoc = PreludeParty.GetLocations();
	ZSWindow *pWin;
	
	int BWidth;
	int BWidthOff;
	int BXOffset;
	int BYOffset;
	float MapFactor;

	MapFactor = (float)(rMapArea.right - rMapArea.left) / (float)(rArea.right - rArea.left);

	BXOffset = rMapArea.left;
	BYOffset = rMapArea.top;
	
	BWidth = 12;
	BWidthOff = 6;

	RECT rButton;

	int Width;
	Width = rArea.right - rArea.left;

	if(rArea.left < 0)
	{
		rArea.left = 0;
		rArea.right = Width;
	}
	else
	if(rArea.right > 3200)
	{
		rArea.right = 3200;
		rArea.left = 3200 - Width;
	}

	if(rArea.top < 0)
	{
		rArea.top = 0;
		rArea.bottom = Width;
	}
	else
	if(rArea.bottom > 3200)
	{
		rArea.bottom = 3200;
		rArea.top = 3200 - Width;
	}
	
	for(int n = 0; n < NumLocators; n++)
	{
		pWin = GetChild(IDC_MAP_LOCATION + n);
		pLoc = pLocs[n];

		if(pLoc->GetMapNum())
		{
			rButton.left = pLoc->GetMapX() - BWidthOff + BXOffset;
			rButton.right = (rButton.left + BWidth);
			rButton.top = pLoc->GetMapY() - BWidthOff + BYOffset;
			rButton.bottom = rButton.top + BWidth;
			pWin->SetBounds(&rButton);
			pWin->Show();
		}
		else
		{
			if(pLoc->GetX() > rArea.left && pLoc->GetX() < rArea.right &&
				pLoc->GetY() > rArea.top && pLoc->GetY() < rArea.bottom)
			{
				rButton.left = (((pLoc->GetX() - rArea.left) * MapFactor) - BWidthOff) + BXOffset;
				rButton.right = (rButton.left + BWidth);
				rButton.top = (((pLoc->GetY() - rArea.top) * MapFactor) - BWidthOff) + BYOffset;
				rButton.bottom = (rButton.top + BWidth);
				pWin->SetBounds(&rButton);
				pWin->Show();
			}
			else
			{
				pWin->Hide();
			}
		}

	}

	// Party marker.
	D3DVECTOR *pPosition;
	pPosition = PreludeParty.GetLeader()->GetPosition();

	DrawParty = FALSE;

	if(CurTown < 1)
	{
		//valley map: the bitmap is the world, scaled and scrolled
		if(pPosition->x > rArea.left && pPosition->x < rArea.right &&
			pPosition->y > rArea.top && pPosition->y < rArea.bottom)
		{
			rParty.left = (((pPosition->x - rArea.left) * MapFactor) - 3) + BXOffset;
			rParty.right = (rParty.left + 6);
			rParty.top = (((pPosition->y - rArea.top) * MapFactor) - 3) + BYOffset;
			rParty.bottom = (rParty.top + 6);

			DrawParty = TRUE;
		}
	}
	else
	{
		//town map: drawn unscaled, so bitmap pixels are screen offsets, the
		//same way the town locator buttons above are placed
		for(int t = 0; t < NUM_TOWN_MAP_AREAS; t++)
		{
			if(strcmp(TownMapAreas[t].Name, &TownNames[CurTown - 1][0]))
				continue;

			if(pPosition->x >= TownMapAreas[t].left && pPosition->x <= TownMapAreas[t].right &&
				pPosition->y >= TownMapAreas[t].top && pPosition->y <= TownMapAreas[t].bottom)
			{
				float TownW = (float)(TownMapAreas[t].right - TownMapAreas[t].left);
				float TownH = (float)(TownMapAreas[t].bottom - TownMapAreas[t].top);

				rParty.left = (int)(((pPosition->x - TownMapAreas[t].left) * TOWN_MAP_SIZE) / TownW) - 3 + BXOffset;
				rParty.right = (rParty.left + 6);
				rParty.top = (int)(((pPosition->y - TownMapAreas[t].top) * TOWN_MAP_SIZE) / TownH) - 3 + BYOffset;
				rParty.bottom = (rParty.top + 6);

				DrawParty = TRUE;
			}
			break;
		}
	}


	rDrawFrom.left = rArea.left / 8;
	rDrawFrom.top = rArea.top / 8;
	rDrawFrom.right = rArea.right / 8;
	rDrawFrom.bottom =rArea.bottom / 8;
}

void MapWin::ZoomIn()
{
	if(ZoomLevel >= MAX_MAP_ZOOM)
		return;

	//get the center
	int XCenter;
	int YCenter;
	
	int Width;

	Width = rArea.right - rArea.left;

	XCenter = (rArea.right + rArea.left) / 2;
	YCenter = (rArea.bottom + rArea.top) / 2;

	Width /= 2;

	rArea.left = XCenter - (Width/2);
	rArea.right = XCenter + (Width/2);
	rArea.top = YCenter - (Width/2);
	rArea.bottom = YCenter + (Width/2);

	ZoomLevel++;

	Scroll(0,0);
}

void MapWin::ZoomOut()
{
	if(ZoomLevel <= MIN_MAP_ZOOM)
		return;

	//get the center
	int XCenter;
	int YCenter;
	
	int Width;

	Width = rArea.right - rArea.left;

	XCenter = (rArea.right + rArea.left) / 2;
	YCenter = (rArea.bottom + rArea.top) / 2;

	Width *= 2;

	rArea.left = XCenter - (Width/2);
	rArea.right = XCenter + (Width/2);
	rArea.top = YCenter - (Width/2);
	rArea.bottom = YCenter + (Width/2);

	ZoomLevel--;

	

	Scroll(0,0);
}

int MapWin::Command(int IDFrom, int Command, int Param)
{
	if(Command == COMMAND_BUTTON_CLICKED)
	{
		switch(IDFrom)
		{
		case IDC_MAP_EXIT:
			State = WINDOW_STATE_DONE;
			break;
		case IDC_MAP_ZOOMIN:
			ZoomIn();
			break;
		case IDC_MAP_ZOOMOUT:
			ZoomOut();
			break;
		case IDC_MAP_NORTH:
			Scroll(0, - (32 - (4 * ZoomLevel)));
			break;
		case IDC_MAP_SOUTH:
			Scroll(0, (32 - (4 * ZoomLevel)));
			break;
		case IDC_MAP_EAST:
			Scroll((32 - (4 * ZoomLevel)),0);
			break;
		case IDC_MAP_WEST:
			Scroll(-(32 - (4 * ZoomLevel)),0);
			break;

		case IDC_MAP_VALLEY:
			SwitchTowns(0);
			break;


		default:
			if(IDFrom >= IDC_MAP_LOCATION)
			{
				int LocationNumber;
				LocationNumber = IDFrom - IDC_MAP_LOCATION;
				MapLocator *pLoc;

				pLoc = pLocs[LocationNumber];

				D3DVECTOR vStart;
				D3DVECTOR vEnd;
				D3DVECTOR vRay;
				D3DVECTOR vDrop;

				int DestX;
				int DestY;
				float fDistance;
				int TileDistance;

				//teleport to the location
				//get the x/y positions
				float XPos;
				float YPos;

				vEnd.x = XPos = (float)(pLoc->GetX());
				vEnd.y = YPos = (float)(pLoc->GetY());
				vEnd.z = 0.0f;

				DestX = (int)XPos;
				DestY = (int)YPos;
				
				vStart.x = PreludeParty.GetLeader()->GetPosition()->x;
				vStart.y = PreludeParty.GetLeader()->GetPosition()->y;
				vStart.z = 0.0f;

				vRay = vEnd - vStart;

				fDistance = Magnitude(vRay);

				TileDistance = (int)fDistance;

				if(TileDistance < 0)
				{
					TileDistance *= -1;
				}

				Flag *pCanTravel;
				pCanTravel = PreludeFlags.Get("DISABLETRAVEL");
				if(pLoc->GetTeleport() && !pCanTravel->Value)
				{
					char Blarg[64];

					sprintf(Blarg,"Travel to %s?",pLoc->GetTag());

					if(Confirm(this,Blarg,"yes","no"))
					{
					
						SetLock(NULL,0,0);
				
						int RandomChance;
						int EventNum = 0;
						RandomChance = TileDistance / PreludeEvents.GetRandomFactor();

						int RandomRoll;

						RandomRoll = rand() % 100;

						if(RandomRoll < RandomChance)
						{
							float fRandomDistance;
							float fTiles;
							fRandomDistance = (float)(rand() % 90) / 100.0f;
							// .2 - .8 distance
							fRandomDistance += 0.05f;
							fTiles = fDistance * fRandomDistance;

							vRay = Normalize(vRay);
							vRay *= fTiles;

							TileDistance = (int)fTiles;

							if(TileDistance < 0)
							{
								TileDistance *= -1;
							}

							PreludeWorld->AdvanceTime(TileDistance/2);
							
							unsigned long TTime;
							TTime = PreludeWorld->GetTotalTime();

							//PreludeWorld->AdvanceTime(TTime);

							vDrop = vStart + vRay;

							int DropX;
							int DropY;
							DropX = vDrop.x;
							DropY = vDrop.y;
							PreludeWorld->GetNearestRoad(DropX,DropY, &DestX, &DestY);

							if(!PreludeWorld->GetTown(DestX,DestY)) // don't call if in town
							{
								//EventNum = PreludeEvents.GetRandomEvent(Xpos, YPos);
								PreludeEvents.SetForce();
								pCanTravel = PreludeFlags.Get("TRAVELEVENT");
								pCanTravel->Value = (void *)1;
								RandomEvent = TRUE;
							}
							else
							{
								DestX = (int)XPos;
								DestY = (int)YPos;
								PreludeWorld->AdvanceTime((int)(fDistance)/2);
							}
						}
						else
						{
							PreludeWorld->AdvanceTime(TileDistance/2);
						}

						//Engine->Graphics()->GetD3D()->BeginScene();
					
						int xn;
						int yn;
						int ChunkX;
						int ChunkY;
						ChunkX = DestX / CHUNK_TILE_WIDTH;
						ChunkY = DestY / CHUNK_TILE_HEIGHT;
						for(yn = ChunkY - 1; yn <= ChunkY + 1; yn++)
						for(xn = ChunkX - 1; xn <= ChunkX + 1; xn++)
						{
							Valley->LoadChunk(xn,yn);
							//Valley is the current area, so a valley scale
							//destination can be off the end of a dungeon's
							//grid. World::GotoArea guards the same loop.
							Chunk *pChunk = Valley->GetChunk(xn,yn);
							if(pChunk)
								pChunk->ConvertTerrain();
							//Valley->GetChunk(xn,yn)->CreateTexture(Valley->GetBaseTexture());
						}
						//Engine->Graphics()->GetD3D()->EndScene();
						
						for(int n = 0; n < PreludeParty.GetNumMembers(); n++)
						{
							Valley->RemoveFromUpdate(PreludeParty.GetMember(n));
						}
						PreludeParty.Teleport(DestX, DestY);
						PreludeParty.Occupy();
						if(RandomEvent)
						{
							DEBUG_INFO("Travel Interrupted.\n");
							this->Hide();
							SetFocus(ZSWindow::GetMain());
							PreludeEvents.CheckForRandomEvent();
							ReleaseFocus();
						}
					}
					else
					{
						SetLock(pLoc->GetTag(), XPos, YPos);
					}
				}
				else
				{
					SetLock(pLoc->GetTag(), XPos, YPos);
				}
				State = WINDOW_STATE_DONE;
				pCanTravel = PreludeFlags.Get("TRAVELEVENT");
				pCanTravel->Value = (void *)0;
							
			}
			else
			{
				int TownNum;
				TownNum = (IDFrom - IDC_MAP_TOWN_0) + 1;
				SwitchTowns(TownNum);
			}
			break;
		}
	}

	return TRUE;
}



MapWin::~MapWin()
{
	//mapsurface is releas at game shutdown
	//MapSurface->Release();

	//make sure we're not on the focal stack
	int n = 0, sn;
	for(n = 0; n < OldFocusStackTop; n++)
	{
		if(OldFocusStack[n] == this)
		{
			DEBUG_INFO("Deleting window on focal stack\n");
			for(sn = n; sn < OldFocusStackTop - 1; sn++)
			{
				OldFocusStack[sn] = OldFocusStack[sn+1];
			}
			OldFocusStackTop--;
			n = 0;
		}
	}
}

int MapWin::HandleKeys(BYTE *CurrentKeys, BYTE *LastKeys)
{
	if(CurrentKeys[DIK_F1] & 0x80 && !(LastKeys[DIK_F1] & 0x80))
	{
		ShowHelp("Map Screen");
		return TRUE;
	}
	if(CurrentKeys[DIK_ESCAPE] & 0x80)
	{
		State = WINDOW_STATE_DONE;
	}
	return TRUE;
}

int MapWin::MoveMouse(long *x, long *y, long *z)
{
	//clamp to boundaries
	if(*x < Bounds.left)
	{
		*x = Bounds.left;
	}
	else
	if(*x > Bounds.right)
	{
		*x = Bounds.right;
	}

	if(*y < Bounds.top)
	{
		*y = Bounds.top;
	}
	else
	if(*y > Bounds.bottom)
	{
		*y = Bounds.bottom;
	}

	//check for child
	ZSWindow *pWin;
	pWin = GetChild(*x,*y);

	if(pWin && pWin->GetID() >= IDC_MAP_LOCATION)
	{
		//if child, get number then set up draw
		int LocNumber;
		LocNumber = pWin->GetID() - IDC_MAP_LOCATION;
		pCurLoc = pLocs[LocNumber];
		
		RECT rChild;
		pWin->GetBounds(&rChild);
		//centre of the marker; Draw() centres the tag on it and clamps it
		TagX = (rChild.left + rChild.right) / 2;
		TagY = rChild.top - Engine->Graphics()->GetFontEngine()->GetTextHeight();
	}
	else 
	{
		pCurLoc = NULL;
	}

	return TRUE;
}

void MapWin::Show()
{
	Visible = TRUE;
	if(!PreludeWorld->GetMapHelp())
	{
		PreludeWorld->SetMapHelp(TRUE);
		ShowHelp("Map Screen");
	}
	char *TownName;
	if(!PreludeWorld->GetCurAreaNum())
	{
		TownName = PreludeWorld->GetTown((int)PreludeParty.GetLeader()->GetPosition()->x,(int)PreludeParty.GetLeader()->GetPosition()->y);
	}
	else
	{
		TownName = NULL;
	}

	if(TownName)
	{
		for(int n = 0; n < NumTowns; n++)
		{
			if(!strcmp(&TownNames[n][0],TownName))
			{
				SwitchTowns(n + 1);
			}
			else
			if((!strcmp(TownName,"West Citadel")
				||
				!strcmp(TownName,"East Citadel")) 
				&&
				!strcmp(&TownNames[n][0],"Citadel"))
			{
				SwitchTowns(n + 1);
			}
		}
	}

}