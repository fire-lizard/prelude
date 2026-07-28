#include "zsitemslot.h"
#include "ZSEngine.h"
#include "items.h"
#include "party.h"
#include "zsactionwindow.h"
#include "World.h"
#include "actionmenuclass.h"
#include "mainwindow.h"
#include "ZSportrait.h"
#include "party.h"
#include "gameitem.h"
#include "inventorywin.h"
#include "Equipwin.h"
#include "Barter.h"
#include "assert.h"
#include "zsgetnumber.h"

#define MOUSEOVER_TIME	100

ZSItemSlot *ZSItemSlot::Grabbed = NULL;

int ZSItemSlot::DrawMouse(int x, int y) 
{ 
	char Num[8];

	if(Grabbed && Grabbed->Look())
	{

		RECT rDrawAt = *Engine->Input()->GetMouseRect();
		rDrawAt.top -= 16;
		rDrawAt.left -= 16;
		rDrawAt.right = rDrawAt.left + 32;
		rDrawAt.bottom = rDrawAt.top + 32;

		Grabbed->Look()->GetItem()->Draw2D(Engine->Graphics()->GetBBuffer(), &rDrawAt);
		if(Grabbed->Look()->GetQuantity() > 1)
		{
			sprintf(Num,"%i",Grabbed->Look()->GetQuantity());
			Engine->Graphics()->DrawText(rDrawAt.left, rDrawAt.top, Num);
		}
	}

	if(Cursor != CURSOR_NONE)
	{
		Engine->Graphics()->DrawCursor(Engine->Input()->GetMouseRect());
	}

	return TRUE; 
}// DrawMouse

int ZSItemSlot::Draw()
{
	char Num[8];
	if(Visible)
	{
		if(pThing && Grabbed != this)
		{
			pThing->GetItem()->Draw2D(Engine->Graphics()->GetBBuffer(), &Bounds);
			if(pThing->GetQuantity() > 1)
			{
				sprintf(Num,"%i",pThing->GetQuantity());
				Engine->Graphics()->DrawText(Bounds.left, Bounds.top, Num);
			}
		}
		if(Engine->Input()->GetMouseRect()->left >= Bounds.left &&
			Engine->Input()->GetMouseRect()->left <= Bounds.right &&
			Engine->Input()->GetMouseRect()->top >= Bounds.top &&
			Engine->Input()->GetMouseRect()->top <= Bounds.bottom)
		{
			if(ShowDescTime)
			{
				if(timeGetTime() > ShowDescTime)
				{
					if(pThing)
					{
						Engine->Graphics()->DrawText(Bounds.left, Bounds.top - 22, pThing->GetItem()->GetData(INDEX_NAME).String);
					}
				}
			}
			else
			{
				ShowDescTime = timeGetTime() + MOUSEOVER_TIME;
			}
		}
		else
		{
			ShowDescTime = 0;
		}
	}

	if(GetSibling())
	{
		GetSibling()->Draw();
	}
	return TRUE;
}


ZSItemSlot::ZSItemSlot(int NewID, int x, int y, int width, int height, GameItem *NewThing)
{
	LBDown = FALSE;
	RBDown = FALSE;
	pSource = NULL;
	SourceType = SOURCE_NONE;
	ID = NewID;
	State = WINDOW_STATE_NORMAL;
	Visible = FALSE;
	Moveable = FALSE;
	Type = WINDOW_ITEMSLOT;
	Cursor = CURSOR_POINT;

	//set up the bounding rect
	Bounds.left = x;
	Bounds.right = x + width;
	Bounds.top = y;
	Bounds.bottom = y + height;

	pThing = NewThing;

	ShowDescTime = 0;
}

int ZSItemSlot::TransferItem(ZSWindow * target, int NumberItems, int x, int y) {
	int NumberToTransfer;
	if (NumberItems > 1) {
		NumberToTransfer = GetNumber(0, NumberItems, NumberItems);

		if (!NumberToTransfer) {
			return TRUE;
		}
	}
	else {
		NumberItems = NumberToTransfer = 1;
	}
	GameItem *pGI;
	pGI = new GameItem(this->Look());
	pGI->SetQuantity(NumberToTransfer);

	if (target->ReceiveItem(pGI, this, x, y)) {
		switch (GetSourceType()) {
		case SOURCE_INVENTORY:
			InventoryWin *pIWin;
			pIWin = (InventoryWin *)GetParent();
			pIWin->GetOwner()->RemoveItem(pGI->GetItem(), NumberToTransfer);
			pIWin->SetSlots();
			return TRUE;
			break;
		case SOURCE_BARTER_MERCHANT:
		case SOURCE_BARTER_PLAYER:
			BarterWin *pBarter;
			pBarter = (BarterWin *)GetParent();
			if (NumberToTransfer >= NumberItems) {
				pGI = this->Take();
				delete pGI;
				this->Give(NULL);
			}
			else {
				pGI = this->Look();
				pGI->SetQuantity(pGI->GetQuantity() - NumberToTransfer);
			}
			pBarter->CalculateValues();
			break;
		case SOURCE_EQUIP:
			EquipWin *pEquip;
			pEquip = (EquipWin *)GetParent();
			if (NumberToTransfer >= NumberItems) {
				((Creature *)pEquip->GetTarget())->Unequip(this->Look());
			}
			else {
				pGI = this->Look();
				pGI->SetQuantity(pGI->GetQuantity() - NumberToTransfer);
			}

			pEquip->SetSlots();
			break;
		default:
			break;
		}
	}

	return TRUE;
}

int ZSItemSlot::LeftButtonDown(int x, int y, int doubleClick)
{
	SetFocus(this);
	LBDown = TRUE;
	if(pThing)
	{
		Grabbed = this;
	}
	else
	{
		Grabbed = NULL;
	}
	
	if (this->Look() && doubleClick) {
		ActionMenu pMenu = ActionMenu();
		Creature *pActive = NULL;
		pActive = (Creature *)pThing->GetOwner();

		if (!pActive || !PreludeParty.IsMember(pActive)) {
			pActive = PreludeParty.GetLeader();
		}

		if (pActive->GetValidActions(pThing, &pMenu)) {
			int i;
			for (i = 0; i < pMenu.GetNumActions(); i++) {
				Action * action = pMenu.GetAction(i);
				debug_info("act %i",action->GetType());
				if (action->GetType() == ACTION_USEITEM) {
					debug_info("item has action");
					Action * pAction = Action::GetNewAction(ACTION_USEITEM, action->GetTarget(), action->GetData(), FALSE);
					pAction->SetNext(pActive->GetAction());
					pActive->SetAction(pAction);
					pActive->UseItem();
					if (pActive->GetAction() == pAction && pAction->GetNext()) {
						pActive->SetAction(pAction->GetNext());
						pAction->Release();
					}
					Grabbed = NULL;
					return TRUE;
				}
			}
		}

		int num = this->Look()->GetQuantity();

		if (Grabbed && num == 1) {

			int sourceType = GetParent()->GetType();
			int targetType = -1;
			if (sourceType == WINDOW_EQUIP) {
				targetType = WINDOW_INVENTORY;
			}
			else if (sourceType == WINDOW_INVENTORY) {
				targetType = WINDOW_EQUIP;
			}
			else {
				return TRUE;
			}

			ZSWindow * parent = GetParent()->GetParent();

			ZSWindow * win = parent->GetChild();

			while (win && win->GetType() != targetType) {
				win = win->GetSibling();
			}

			if (!win) {
				debug_info("WARNING could not find target window on double click");
				Grabbed = NULL;

				return TRUE;
			}

			TransferItem(win, 1, x, y);

			Grabbed = NULL;
		}
	}

	return TRUE;
}

int ZSItemSlot::LeftButtonUp(int x, int y)
{
	ReleaseFocus();
	LBDown = FALSE;

	if(!Grabbed)
	{
		return TRUE;
	}

	Grabbed = NULL;
	
	//first make sure we have an item in hand
	if(!this->Look())
	{
		return TRUE;
	}
	
	//first confirm that we can put the item into the destination
	//different destinations require different checks
	int NumberItems;
	int NumberToTransfer;

	NumberItems = this->Look()->GetQuantity();

	return TransferItem(GetMain(), NumberItems, x, y);
	

}

int ZSItemSlot::RightButtonDown(int x, int y)
{
	SetFocus(this);
	RBDown = TRUE;
	return TRUE;
}
int ZSItemSlot::RightButtonUp(int x, int y)
{
	ReleaseFocus();
	RBDown = FALSE;
	if(LBDown)
		return TRUE;


	if(pThing)
	{
		//create an action menu of the appropriate type
		ActionMenu *pMenu = new ActionMenu;
		Creature *pActive = NULL;
		pActive = (Creature *)pThing->GetOwner();

		if(!pActive || !PreludeParty.IsMember(pActive))
		{
			pActive = PreludeParty.GetLeader();
		}

		if(pActive->GetValidActions(pThing, pMenu))
		{
			ZSActionWin *pActionWin;
			pActionWin = new ZSActionWin(pMenu,x,y,GetMain());
			pActionWin->SetActor(pActive);
			pActionWin->Show();
		
			pActionWin->GoModal();

			GetMain()->RemoveChild(pActionWin);
		
			if(GetParent()->GetType() == WINDOW_INVENTORY)
			{
				((InventoryWin *)GetParent())->Reset();
			}		
		}
		delete pMenu;
	}
	return TRUE;
}

int ZSItemSlot::MoveMouse(long *x, long *y, long *z)
{
	RECT rBounds;
	GetMain()->GetBounds(&rBounds);

	if(*x < rBounds.left)
	{
		*x = rBounds.left;
	}
	else
	if(*x > rBounds.right)
	{
		*x = rBounds.right;
	}

	if(*y < rBounds.top)
	{
		*y = rBounds.top;
	}
	else
	if(*y > rBounds.bottom)
	{
		*y = rBounds.bottom;
	}

	return TRUE;

}


