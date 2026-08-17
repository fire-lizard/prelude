#include "ZSaskwin.h"
#include "ZSListBox.h"
#include "ZSEngine.h"

#define IDC_ASK_LIST	567
LPDIRECTDRAWSURFACE7 ZSAskWin::AskSurface = NULL;


//This window is modal and its only way out was clicking one of its options, so
//an ask that ends up with nothing to click - or one you can't see - froze the
//game with the world still drawing behind it.  Escape now answers with the
//first option, the same as clicking it.
int ZSAskWin::HandleKeys(BYTE *CurrentKeys, BYTE *LastKeys)
{
	if(CurrentKeys[DIK_ESCAPE] & 0x80 && !(LastKeys[DIK_ESCAPE] & 0x80))
	{
		ReturnCode = 0;
		State = WINDOW_STATE_DONE;
		return TRUE;
	}

	return ZSWindow::HandleKeys(CurrentKeys, LastKeys);
}

int ZSAskWin::OptionCount()
{
	ZSList *pList;
	pList = (ZSList *)GetChild(IDC_ASK_LIST);

	if(!pList)
		return 0;

	return pList->GetNumItems();
}

BOOL ZSAskWin::HasOptions()
{
	ZSList *pList;
	pList = (ZSList *)GetChild(IDC_ASK_LIST);

	if(!pList)
		return FALSE;

	return pList->GetNumItems() > 0;
}

int ZSAskWin::Command(int IDFrom, int Command, int Param)
{

	if(IDFrom == IDC_ASK_LIST && Command == COMMAND_LIST_SELECTED)
	{
		State = WINDOW_STATE_DONE;

		ReturnCode = Param;
	}

	return TRUE;
}

ZSAskWin::ZSAskWin(int NewID, int x, int y, int Width, int Height, ScriptArg *pOptions)
{
	ID = NewID;
	Visible = FALSE;
	State = WINDOW_STATE_NORMAL;
	Moveable = FALSE;
	Bounds.left = x;
	Bounds.right = x + Width;
	Bounds.top = y;
	Bounds.bottom = y + Height;

	Cursor = CURSOR_POINT;

	if(!AskSurface)
	{
		CreateWoodBorderedBackground(0,1);
		AskSurface = BackGroundSurface;
	}
	BackGroundSurface = AskSurface;
	BackGroundSurface->AddRef();
	
	Border = 0;

	ZSList *pWin;

	pWin = new ZSList(IDC_ASK_LIST,x,y,Width,Height);
	pWin->SetTextColor(TEXT_LIGHT_PAINTED_WOOD);
	
	int n = 0;

	while(pOptions[n].GetType() != ARG_TERMINATOR)
	{
		pWin->AddItem((char *)pOptions[n].Evaluate()->GetValue());
		n++;
	}
	
	pWin->Show();

	AddChild(pWin);

#ifdef AUTOTEST
	State = WINDOW_STATE_DONE;

	ReturnCode = rand() % pWin->GetNumItems();
#endif

	return; 
}

int ZSAskWin::LeftButtonDown(int x, int y, int doubleClick)
{
	ZSWindow *pWin;
	
	//check to see if there's child beneath the cursor who should receive the message
	pWin = GetChild(x,y);
	if(pWin)
	{
		return pWin->LeftButtonDown(x,y, doubleClick);
	}
	else
	{
		pWin = this->GetMain()->GetChild(x,y);
		if(pWin)
		{
			if(GetParent() == pWin)
				return pWin->LeftButtonDown(x,y, doubleClick);
		}
	}

	//if our cursor is the pointing hand, make the finger depress
	if(Cursor == CURSOR_POINT)
	{
		Engine->Graphics()->SetCursorFrame(1);
	}

	SetFocus(this);
	return TRUE; 
}