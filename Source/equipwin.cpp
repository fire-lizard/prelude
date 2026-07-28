#include "Equipwin.h"
#include "ZSutilities.h"
#include "ZSEngine.h"
#include "zsitemslot.h"
#include "things.h"
#include "items.h"
#include "creatures.h"
#include "gameitem.h"
#include <assert.h>

typedef enum
{
	EQUIP_SLOT_HEAD = 10,
	EQUIP_SLOT_NECK,
	EQUIP_SLOT_CHEST,
	EQUIP_SLOT_AMMO,
	EQUIP_SLOT_GLOVES,
	EQUIP_SLOT_RHAND,
	EQUIP_SLOT_LHAND,
	EQUIP_SLOT_RRING,
	EQUIP_SLOT_LRING,
	EQUIP_SLOT_LEGS,
	EQUIP_SLOT_FEET,
} EQUIP_SLOT_IDS;

void EquipWin::ClearSlots()
{
	ZSItemSlot *pISlot;
	for(int n = EQUIP_SLOT_HEAD; n <= EQUIP_SLOT_FEET; n++)
	{
		pISlot = (ZSItemSlot *)GetChild(n);
		if(pISlot->Look())
		{
			pISlot->Give(NULL);
		}
	}
}

void EquipWin::SetSlots()
{
	Creature *pCreature;
	pCreature = (Creature *)pTarget;

	ClearSlots();

	ZSItemSlot *pISlot;

	GameItem *pGI;
	pISlot = (ZSItemSlot *)GetChild( EQUIP_SLOT_HEAD);
	pGI = pCreature->GetEquipment("HEAD");
	pISlot->Give(pGI);
	
	pISlot = (ZSItemSlot *)GetChild( EQUIP_SLOT_NECK);
	pGI = pCreature->GetEquipment("NECK");
	pISlot->Give(pGI);
	
	pISlot = (ZSItemSlot *)GetChild( EQUIP_SLOT_CHEST);
	pGI = pCreature->GetEquipment("CHEST");
	pISlot->Give(pGI);
	
	pISlot = (ZSItemSlot *)GetChild( EQUIP_SLOT_AMMO);
	pGI = pCreature->GetEquipment("AMMO");
	pISlot->Give(pGI);
	
	pISlot = (ZSItemSlot *)GetChild( EQUIP_SLOT_GLOVES);
	pGI = pCreature->GetEquipment("GLOVES");
	pISlot->Give(pGI);
	
	pISlot = (ZSItemSlot *)GetChild( EQUIP_SLOT_RHAND);
	pGI = pCreature->GetEquipment("RIGHTHAND");
	pISlot->Give(pGI);
	
	pISlot = (ZSItemSlot *)GetChild( EQUIP_SLOT_LHAND);
	pGI = pCreature->GetEquipment("LEFTHAND");
	pISlot->Give(pGI);
	
	pISlot = (ZSItemSlot *)GetChild( EQUIP_SLOT_RRING);
	pGI = pCreature->GetEquipment("RIGHTRING");
	pISlot->Give(pGI);
	
	pISlot = (ZSItemSlot *)GetChild( EQUIP_SLOT_LRING);
	pGI = pCreature->GetEquipment("LEFTRING");
	pISlot->Give(pGI);
	
	pISlot = (ZSItemSlot *)GetChild( EQUIP_SLOT_LEGS);
	pGI = pCreature->GetEquipment("LEGS");
	pISlot->Give(pGI);
	
	pISlot = (ZSItemSlot *)GetChild( EQUIP_SLOT_FEET);
	pGI = pCreature->GetEquipment("FEET");
	pISlot->Give(pGI);
	

}

int EquipWin::Command(int IDFrom, int Command, int Param)
{
	if(Command == COMMAND_ITEMSLOT_CHANGED)
	{
		SetSlots();
	}
	return TRUE;
	
}

EquipWin::EquipWin(int NewID, int x, int y, int width, int height, Thing *pNewTarget)
{
	//I can not exist without a target
	PTD_ASSERT(pNewTarget);
	ID = NewID;
	State = WINDOW_STATE_NORMAL;
	Visible = FALSE;
	Moveable = FALSE;
	Type = WINDOW_EQUIP;

	//set up the bounding rect
	Bounds.left = x;
	Bounds.right = x + width;
	Bounds.top = y;
	Bounds.bottom = y + height;

	pTarget = pNewTarget;

	FILE *fp;
	fp = SafeFileOpen("gui.ini","rt");

	if(pTarget->GetData(INDEX_SEX).Value)
	{
		SeekTo(fp,"[FEMALEEQUIP]");
	}
	else
	{
		SeekTo(fp,"[MALEEQUIP]");
	}

	ZSItemSlot *pISlot;

	int ChildX;
	int ChildY;
	char *FileName;

	SeekTo(fp,"BACKGROUND");
	FileName = GetStringNoWhite(fp);
	ChildX = GetInt(fp);
	ChildY = GetInt(fp);

	BackGroundSurface = Engine->Graphics()->CreateSurfaceFromFile(FileName,ChildX,ChildY,NULL,0);
	
	delete[] FileName;

	SeekTo(fp,"HEAD");
	ChildX = GetInt(fp);
	ChildY = GetInt(fp);
	pISlot = new ZSItemSlot(EQUIP_SLOT_HEAD,x + ChildX,y + ChildY,32,32);
	pISlot->Show();
	pISlot->SetSourceType(SOURCE_EQUIP);
	pISlot->SetSource((void *)this);
	AddChild(pISlot);

	SeekTo(fp,"NECK");
	ChildX = GetInt(fp);
	ChildY = GetInt(fp);
	pISlot = new ZSItemSlot(EQUIP_SLOT_NECK,x + ChildX,y + ChildY,32,32);
	pISlot->Show();
	pISlot->SetSourceType(SOURCE_EQUIP);
	pISlot->SetSource((void *)this);
	AddChild(pISlot);
	
	SeekTo(fp,"CHEST");
	ChildX = GetInt(fp);
	ChildY = GetInt(fp);
	pISlot = new ZSItemSlot(EQUIP_SLOT_CHEST,x + ChildX,y + ChildY,32,32);
	pISlot->Show();
	pISlot->SetSourceType(SOURCE_EQUIP);
	pISlot->SetSource((void *)this);
	AddChild(pISlot);
	
	SeekTo(fp,"AMMO");
	ChildX = GetInt(fp);
	ChildY = GetInt(fp);
	pISlot = new ZSItemSlot(EQUIP_SLOT_AMMO,x + ChildX,y + ChildY,32,32);
	pISlot->Show();
	pISlot->SetSourceType(SOURCE_EQUIP);
	pISlot->SetSource((void *)this);
	AddChild(pISlot);
	
	SeekTo(fp,"GLOVES");
	ChildX = GetInt(fp);
	ChildY = GetInt(fp);
	pISlot = new ZSItemSlot(EQUIP_SLOT_GLOVES,x + ChildX,y + ChildY,32,32);
	pISlot->Show();
	pISlot->SetSourceType(SOURCE_EQUIP);
	pISlot->SetSource((void *)this);
	AddChild(pISlot);
	
	SeekTo(fp,"RHAND");
	ChildX = GetInt(fp);
	ChildY = GetInt(fp);
	pISlot = new ZSItemSlot(EQUIP_SLOT_RHAND,x + ChildX,y + ChildY,32,32);
	pISlot->Show();
	pISlot->SetSourceType(SOURCE_EQUIP);
	pISlot->SetSource((void *)this);
	AddChild(pISlot);
	
	SeekTo(fp,"LHAND");
	ChildX = GetInt(fp);
	ChildY = GetInt(fp);
	pISlot = new ZSItemSlot(EQUIP_SLOT_LHAND,x + ChildX,y + ChildY,32,32);
	pISlot->Show();
	pISlot->SetSourceType(SOURCE_EQUIP);
	pISlot->SetSource((void *)this);
	AddChild(pISlot);
	
	SeekTo(fp,"RRING");
	ChildX = GetInt(fp);
	ChildY = GetInt(fp);
	pISlot = new ZSItemSlot(EQUIP_SLOT_RRING,x + ChildX,y + ChildY,32,32);
	pISlot->Show();
	pISlot->SetSourceType(SOURCE_EQUIP);
	pISlot->SetSource((void *)this);
	AddChild(pISlot);
	
	SeekTo(fp,"LRING");
	ChildX = GetInt(fp);
	ChildY = GetInt(fp);
	pISlot = new ZSItemSlot(EQUIP_SLOT_LRING,x + ChildX,y + ChildY,32,32);
	pISlot->Show();
	pISlot->SetSourceType(SOURCE_EQUIP);
	pISlot->SetSource((void *)this);
	AddChild(pISlot);
	
	SeekTo(fp,"LEGS");
	ChildX = GetInt(fp);
	ChildY = GetInt(fp);
	pISlot = new ZSItemSlot(EQUIP_SLOT_LEGS,x + ChildX,y + ChildY,32,32);
	pISlot->Show();
	pISlot->SetSourceType(SOURCE_EQUIP);
	pISlot->SetSource((void *)this);
	AddChild(pISlot);
	
	SeekTo(fp,"FEET");
	ChildX = GetInt(fp);
	ChildY = GetInt(fp);
	pISlot = new ZSItemSlot(EQUIP_SLOT_FEET,x + ChildX,y + ChildY,32,32);
	pISlot->Show();
	pISlot->SetSourceType(SOURCE_EQUIP);
	pISlot->SetSource((void *)this);
	AddChild(pISlot);

	fclose(fp);

	SetSlots();

}

BOOL EquipWin::ReceiveItem(Object *pToReceive, ZSWindow *pWinFrom, int x, int y)
{
	Creature *pCreature;
	pCreature = (Creature *)pTarget;
	GameItem *pGI;
	pGI = (GameItem *)pToReceive;

	if (pWinFrom->GetParent() && pWinFrom->GetParent()->GetType() == WINDOW_EQUIP) {
		// game is crashing when trying to move objects into itself
		return FALSE;
	}

	if(pCreature->Equip(pGI, pGI->GetQuantity()))
	{
		 SetSlots();
		 return TRUE;
	}
	
	return FALSE;
}

