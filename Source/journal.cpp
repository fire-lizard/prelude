#if defined(BACKPORT_18) && 1
#define BACKPORT_18_JOURNAL
#endif
#ifdef BACKPORT_18_JOURNAL

#include "journal.h"
#include "ZSutilities.h"
#include "zsbutton.h"
#include "ZSText.h"
#include "ZSListBox.h"
#include "ZSEngine.h"
#include "party.h"
#include "World.h"

typedef enum
{
	IDC_JOURNAL_QUIT,
	IDC_JOURNAL_PAGEUP,
	IDC_JOURNAL_PAGEDOWN,
	IDC_JOURNAL_LEFT_TEXT,
	IDC_JOURNAL_RIGHT_TEXT,
	IDC_JOURNAL_QUEST_LIST,
	IDC_JOURNAL_AREA_LIST
} JOURNAL_CONTROLS;

LPDIRECTDRAWSURFACE7 JournalWin::JournalSurface = NULL;

BOOL Journal::IsSetup = FALSE;
int Journal::NumQuests = 0;
char Journal::QuestNames[MAX_QUESTS][128];
int Journal::QuestAreas[MAX_QUESTS];
int Journal::NumAreas = 0;
char Journal::AreaNames[MAX_AREAS][128];

void Journal::GetEntry(int num, char *Dest)
{

}

int Journal::GetEntryQuest(int num)
{
	PTD_ASSERT(num >= 0);
	if ((size_t)num >= MAX_JOURNAL_ENTRY_COUNT)
		SafeExit("Increase MAX_JOURNAL_ENTRY_COUNT");

	int EntryNum = Entry[num*2];
	char IDNum[8];
	sprintf(IDNum,"%i",EntryNum);

	FILE *fp;
	fp = SafeFileOpen("journal.txt","rt");

	SeekToSkip(fp,IDNum);
	char *JournalString;
	SeekTo(fp,"[");
	JournalString = GetString(fp,']');
	int QuestNum = 0;
	QuestNum = GetQuestNum(JournalString);
	if(QuestNum == -1)
	{
		if(GetAreaNum(JournalString) != -1)
		{
			delete[] JournalString;
			SeekTo(fp,"[");
			JournalString = GetString(fp,']');
		}
		else
		{
			delete[] JournalString;
			return -1;
		}
	}
	else
	{
		fclose(fp);
		delete[] JournalString;
		return QuestNum;
	}
	
	QuestNum = GetQuestNum(JournalString);
	fclose(fp);
	
	delete[] JournalString;
	return QuestNum;
}


int Journal::GetEntryArea(int num)
{
	PTD_ASSERT(num >= 0);
	if ((size_t)num >= MAX_JOURNAL_ENTRY_COUNT)
		SafeExit("Increase MAX_JOURNAL_ENTRY_COUNT");

	int EntryNum = Entry[num*2];
	char IDNum[8];
	sprintf(IDNum,"%i",EntryNum);
	
	FILE *fp;
	fp = SafeFileOpen("journal.txt","rt");

	SeekToSkip(fp,IDNum);
	char *JournalString;
	SeekTo(fp,"[");
	JournalString = GetString(fp,']');
	int AreaNum = 0;
	AreaNum = GetAreaNum(JournalString);
	if(AreaNum == -1)
	{
		if(GetQuestNum(JournalString) != -1)
		{
			delete[] JournalString;
			SeekTo(fp,"[");
			JournalString = GetString(fp,']');
		}
		else
		{
			delete[] JournalString;
			return -1;
		}
	}
	else
	{
		fclose(fp);
		delete[] JournalString;
		return AreaNum;
	}
	
	AreaNum = GetAreaNum(JournalString);
	fclose(fp);
	
	delete[] JournalString;
	return AreaNum;
}

char *Journal::GetEntry(int num)
{
	PTD_ASSERT(num >= 0);
	if ((size_t)num >= MAX_JOURNAL_ENTRY_COUNT)
		SafeExit("Increase MAX_JOURNAL_ENTRY_COUNT");

	int EntryNum = Entry[num*2];
	int Base = num;
	BOOL Found = FALSE;
	int Date;
	Date = Entry[num*2+1];
	char DayString[16];
	sprintf(DayString,"Day %i:   ",Date);
	char IDNum[8];
	sprintf(IDNum,"%i",EntryNum);
	FILE *fp;
	fp = SafeFileOpen("journal.txt","rt");

	SeekToSkip(fp,IDNum);
	char JournalString[2048];
	char *RetString;
	char extraData[2048];
	SeekTo(fp,"[");
	GetString(fp,']',JournalString,2048);
	int AreaNum = 0;
	int QuestNum = 0;
	AreaNum = GetAreaNum(JournalString);
	int StringAdd = 16;
	if(AreaNum != -1)
	{
		Area[num] = AreaNum;
		StringAdd += strlen(JournalString);

		SeekTo(fp,"[");
		GetString(fp,']', JournalString, 2048);
	}

	QuestNum = GetQuestNum(JournalString);
	if(QuestNum != -1)
	{
		Quest[num] = QuestNum;
		StringAdd += strlen(JournalString);

		SeekTo(fp,"[");
		GetString(fp,']', JournalString, 2048);
		SeekTo(fp, "[");
		GetString(fp, ']', extraData, 2048);
		StringAdd += (2 + strlen(extraData));
		
	}

	fclose(fp);
		
	RetString = new char[strlen(DayString) + strlen(JournalString) + 6 + StringAdd];

	strcpy(RetString,DayString);
	if(AreaNum != -1)
	{
		strcat(RetString,&AreaNames[AreaNum][0]);
		strcat(RetString,"  ");
		strcat(RetString, JournalString);
	}
	
	if(QuestNum != -1)
	{
		strcat(RetString, JournalString);
		strcat(RetString, "  ");
		strcat(RetString,&QuestNames[QuestNum][0]);
		strcat(RetString,"  ");
		PTD_ASSERT(strlen(RetString) + strlen(extraData) < (strlen(DayString) + strlen(JournalString) + 6 + StringAdd));
		strcat(RetString, extraData);
		
	}
		
	return RetString;
}

BOOL Journal::AddEntry(int Num)
{
	if(PreludeParty.GetBest(INDEX_LITERACY_AND_LORE)->GetData(INDEX_LITERACY_AND_LORE).Value)
	{

		for(int n = 0; n < (NumEntries*2); n+=2)
		{
			if(Entry[n] == Num)
			{
				return FALSE;
			}
		}

		if (NumEntries >= MAX_JOURNAL_ENTRY_COUNT)
			SafeExit("Increase MAX_JOURNAL_ENTRY_COUNT");

		Entry[NumEntries*2] = Num;
		Entry[NumEntries*2+1] = PreludeWorld->GetDay();
		NumEntries++;
		Current = NumEntries - 1;
		if(Current % 2)
		{
			Current -= 1;
		}
	}
	return TRUE;
}

void Journal::RemoveEntry(int Num)
{
	int n, sn;
	for(n = 0; n < NumEntries; n++)
	{
		if(Entry[n*2] == Num)
		{
			NumEntries--;
			for(sn = n; sn < NumEntries; sn ++)
			{
				Entry[sn*2] = Entry[(sn+1)*2];
				Entry[sn*2+1] = Entry[(sn+1)*2+1];
			}
			Current = NumEntries - 1;
			if(Current % 2)
			{
				Current -= 1;
			}
			return;
		}
	}
}

void Journal::Save(FILE *fp)
{
	// Entry count has changed but it's nice to stay bw-compatible with older saves, hence this juggling
	// New saves will have a negative first int, which cannot be a NumEntries
	int magic = -1;
	fwrite(&magic, sizeof(int), 1, fp);

	fwrite(&NumEntries,sizeof(int),1,fp);
	fwrite(&Current,sizeof(int),1,fp);
	fwrite(Entry,sizeof(uint32_t),MAX_JOURNAL_ENTRY_COUNT*2,fp);
}

void Journal::Load(FILE *fp)
{
	// See Save
	int firstInt;
	fread(&firstInt,sizeof(int),1,fp);

	if (firstInt == -1)
	{
		fread(&NumEntries, sizeof(int), 1, fp);
		fread(&Current, sizeof(int), 1, fp);
		fread(Entry, sizeof(uint32_t), MAX_JOURNAL_ENTRY_COUNT * 2, fp);
	}
	else
	{
		// Original game save
		NumEntries = firstInt;
		fread(&Current, sizeof(int), 1, fp);
		fread(Entry, sizeof(uint32_t), 1024 * 2, fp);
		for (size_t i = 1024; i < MAX_JOURNAL_ENTRY_COUNT; ++i)
		{
			Entry[2 * i + 0] = 0;
			Entry[2 * i + 1] = 0;
		}
	}
}

Journal::Journal()
{
	IsSetup = FALSE;
	Current = 0;
	NumEntries = 0;
	ZeroMemory(Entry, MAX_JOURNAL_ENTRY_COUNT*2*sizeof(uint32_t));
	ZeroMemory(Area, MAX_JOURNAL_ENTRY_COUNT*sizeof(int));
	ZeroMemory(Quest, MAX_JOURNAL_ENTRY_COUNT*sizeof(int));
	Init();
}

Journal::~Journal()
{

}

int Journal::GetAreaNum(char *AreaName)
{
	int n;
	for(n = 0; n < NumAreas; n++)
	{
		if(!strcmp(AreaNames[n],AreaName))
		{
			return n;
		}
	}

	return -1;

}

int Journal::GetQuestNum(char *QuestName)
{
	int n;
	for(n = 0; n < NumQuests; n++)
	{
		if(!strcmp(QuestNames[n],QuestName))
		{
			return n;
		}
	}

	return -1;
}

int Journal::GetQuestArea(int QuestNum)
{
	return QuestAreas[QuestNum];

}

void Journal::Init()
{
	if(IsSetup)
		return;

	char AreaName[128];
	int n;

	for (n = 0; n < MAX_QUESTS; n++)
	{
		ZeroMemory(QuestNames[n], 128 * sizeof(char));
	}

	for (n = 0; n < MAX_AREAS; n++)
	{
		ZeroMemory(AreaNames[n], 128 * sizeof(char));
	}

	int IDNum = 0;	
	//open journal
	FILE *fp;
	fp = SafeFileOpen("journalquests.txt","rt");

	BOOL Continue = TRUE;

	NumQuests = 0;
	NumAreas = 0;
	while(Continue)
	{
		if(SeekTo(fp,"["))
		{
			GetString(fp, &QuestNames[NumQuests][0],']'); 

			SeekTo(fp,"[");
			
			GetString(fp, &AreaName[0],']'); 
			int AreaNum;
			AreaNum = GetAreaNum(AreaName);
			if(AreaNum != -1)
			{
				QuestAreas[NumQuests] = AreaNum;
			}
			else
			{
				strcpy(AreaNames[NumAreas],AreaName);
				QuestAreas[NumQuests] = NumAreas;
				NumAreas++;
			}
						
		}
		else
		{
			Continue = FALSE;	
		}

		NumQuests++;
	}

	fclose(fp);

	IsSetup = TRUE;
}

void JournalWin::SetText()
{
	if(pJournal->NumEntries)
	{
		ZSWindow *pWin;
		
		pWin = GetChild(IDC_JOURNAL_LEFT_TEXT);
		
		if (JournalLeft >= 0 &&
			JournalLeft < pJournal->NumEntries)
		{
			pWin->SetText(pJournal->GetEntry(JournalLeft));
		}
		else
		{
			pWin->SetText("No Entry");
		}

		pWin = GetChild(IDC_JOURNAL_RIGHT_TEXT);
		
		if(JournalRight >= 0 && JournalRight < pJournal->NumEntries)
		{
			pWin->SetText(pJournal->GetEntry(JournalRight));
		}
		else
		{
			pWin->SetText("No Entry");
		}
	}
}

int JournalWin::Command(int IDFrom, int Command, int Param)
{
	
	if(Command == COMMAND_BUTTON_CLICKED)
	{
		if(IDFrom == IDC_JOURNAL_QUIT)
		{
			State = WINDOW_STATE_DONE;
		}
		else
		if(IDFrom == IDC_JOURNAL_PAGEUP)
		{
			PageLeft();
			SetText();
		}
		else
		if(IDFrom == IDC_JOURNAL_PAGEDOWN)
		{
			PageRight();
			SetText();
		}
	}
	else
	if(Command == COMMAND_LIST_SELECTED)
	{
		ZSList *pList;
		int NewNum;
		char ListText[128];
		pList =	(ZSList *)this->GetChild(IDFrom);
		if(IDFrom == IDC_JOURNAL_AREA_LIST)
		{
			pList->GetText(pList->GetSelection(),(char *)ListText);
			pList->SetText((char *)ListText);
			NewNum = pJournal->GetAreaNum((char *)ListText);
			if(NewNum != ShowAreaNum)
			{
				ShowAreaNum = NewNum;
				SortQuests();
				Sort();
			}
				
		}
		else
		if(IDFrom == IDC_JOURNAL_QUEST_LIST)
		{
			pList->GetText(pList->GetSelection(),(char *)ListText);
			pList->SetText((char *)ListText);
			NewNum = pJournal->GetQuestNum((char *)ListText);
			if(NewNum != ShowQuestNum)
			{
				ShowQuestNum = NewNum;
				Sort();
			}
		}
	}

	return TRUE;
}

int JournalWin::MatchJournalQuestArea(int current) {
	if (ShowAreaNum >= 0 && (pJournal->GetEntryArea(current) == ShowAreaNum)) {
		return TRUE;		
	}

	if (ShowQuestNum >= 0 && (pJournal->GetEntryQuest(current) == ShowQuestNum)) {
		return TRUE;
	}

	return FALSE;

}

void JournalWin::PageLeft()
{
	pJournal->Current -= 2;
	if(pJournal->Current < 0)
		pJournal->Current = 0;

	if(ShowAreaNum != -1 || ShowQuestNum != -1)
	{
	
		while(pJournal->Current >= 0 && 
			!MatchJournalQuestArea(pJournal->Current))
		{
			pJournal->Current--;
		}
		if(pJournal->Current < 0)
		{
			pJournal->Current = 0;
			while(pJournal->Current < pJournal->NumEntries 
				&& !MatchJournalQuestArea(pJournal->Current))
			{
				pJournal->Current++;
			}
			if(pJournal->Current > pJournal->NumEntries)
			{
				pJournal->Current = 0;
			}
		}
	}

	JournalLeft = pJournal->Current;

	JournalRight = pJournal->Current + 1;

	if(ShowAreaNum != -1 || ShowQuestNum != -1)
	{
			while(JournalRight < pJournal->NumEntries 
				&& ((pJournal->GetEntryArea(JournalRight) != ShowAreaNum && ShowAreaNum == -1)
				|| (pJournal->GetEntryQuest(JournalRight) != ShowQuestNum && ShowQuestNum == -1)))
			{
				JournalRight++;
			}
	}

}


void JournalWin::PageRight()
{
	if(pJournal->Current - 2 < pJournal->NumEntries )
	{
		pJournal->Current += 2;
		if(ShowAreaNum != -1 || ShowQuestNum != -1)
		{
			while(pJournal->Current < pJournal->NumEntries 
					&& ((pJournal->GetEntryArea(pJournal->Current) != ShowAreaNum && ShowAreaNum == -1)
					|| (pJournal->GetEntryQuest(pJournal->Current) != ShowQuestNum && ShowQuestNum == -1)))
				{
					pJournal->Current++;
				}
		}

		JournalLeft = pJournal->Current;

		JournalRight = pJournal->Current + 1;

		if(ShowAreaNum != -1 || ShowQuestNum != -1)
		{
				while(JournalRight < pJournal->NumEntries 
					&& ((pJournal->GetEntryArea(JournalRight) != ShowAreaNum && ShowAreaNum == -1)
					|| (pJournal->GetEntryQuest(JournalRight) != ShowQuestNum && ShowQuestNum == -1)))
				{
					JournalRight++;
				}
		}


	}
}

void JournalWin::SortQuests()
{
	ZSList *pList = (ZSList *)GetChild(IDC_JOURNAL_QUEST_LIST);

	int NumItems;
	NumItems = pList->GetNumItems();
	char ListText[128];
	int n;

	if(ShowAreaNum != -1)
	{
		for(n = 0; n < NumItems - 1; n++)
		{
			pList->GetText(n, ListText);
			if(pJournal->GetQuestArea((char *)ListText) != ShowAreaNum)
			{
				pList->DisableItem(n);
			}
			else
			{
				pList->EnableItem(n);
			}
		}
	}
	else
	{
		for(n = 0; n < NumItems - 1; n++)
		{
			pList->EnableItem(n);
		}
	}
}

void JournalWin::Sort()
{

	PageRight();
	PageLeft();
	SetText();
}

JournalWin::JournalWin(int NewID, int x, int y, int width, int height)
{
	ID = NewID;
	Type = WINDOW_JOURNAL;
	Visible = FALSE;
	Moveable = FALSE;
	Bounds.left = x;
	Bounds.right = x + width;
	Bounds.top = y;
	Bounds.bottom = y + height;
	ShowAreaNum = -1;
	ShowQuestNum = -1;

	FILE *fp;
	RECT rBounds;
	char *FileName;
	int Width;
	int Height;

	fp = SafeFileOpen("gui.ini","rt");

	SeekTo(fp,"[JOURNAL]");

	SeekTo(fp,"BACKGROUND");
	
	FileName = GetStringNoWhite(fp);
	Width = GetInt(fp);
	Height = GetInt(fp);

	if(!JournalSurface)
	{
		BackGroundSurface = Engine->Graphics()->CreateSurfaceFromFile(FileName,Width,Height,NULL,0);
		JournalSurface = BackGroundSurface;
	}
	BackGroundSurface = JournalSurface;
	BackGroundSurface->AddRef();
		
	delete[] FileName;

	ZSButton *pButton;

	SeekTo(fp,"QUIT");
	LoadRect(&rBounds,fp);

	pButton = new ZSButton(BUTTON_NONE, IDC_JOURNAL_QUIT, XYWH(rBounds));
	pButton->Show();
	pButton->SetText("Close");
	AddChild(pButton);

	SeekTo(fp,"PAGEUP");
	LoadRect(&rBounds,fp);

	pButton = new ZSButton("leftpagebutton", IDC_JOURNAL_PAGEUP, XYWH(rBounds),51,68,1);
	pButton->Show();
	AddChild(pButton);

	SeekTo(fp,"PAGEDOWN");
	LoadRect(&rBounds,fp);

	pButton = new ZSButton("rightpagebutton", IDC_JOURNAL_PAGEDOWN, XYWH(rBounds),51,68,1);
	pButton->Show();
	AddChild(pButton);

	ZSText *pText;

	SeekTo(fp,"LEFTPAGE");
	LoadRect(&rBounds,fp);
	pText = new ZSText(IDC_JOURNAL_LEFT_TEXT, XYWH(rBounds)," ",0);	
	pText->Show();
	pText->SetTextColor(TEXT_DARK_GREY_PARCHMENT);
	AddTopChild(pText);

	SeekTo(fp,"RIGHTPAGE");
	LoadRect(&rBounds,fp);
	pText = new ZSText(IDC_JOURNAL_RIGHT_TEXT, XYWH(rBounds)," ",0);	
	pText->Show();
	pText->SetTextColor(TEXT_DARK_GREY_PARCHMENT);
	AddTopChild(pText);


	ZSList *pList;

	pJournal = PreludeParty.GetJournal();

	SeekTo(fp,"QUESTLIST");
	LoadRect(&rBounds,fp);
	pList = new ZSList(IDC_JOURNAL_QUEST_LIST, XYWH(rBounds),1);	
	pList->Show();
	pList->SetTextColor(TEXT_DARK_GREY_PARCHMENT);	
	AddChild(pList);

	int n = 0;

	for(n = pJournal->NumQuests; n > 0; n--)
	{
		pList->AddItem(pJournal->QuestNames[n-1]);
	}
	pList->AddItem("All Quests");
	pList->SetText("All Quests");

	
	SeekTo(fp,"AREALIST");
	LoadRect(&rBounds,fp);
	pList = new ZSList(IDC_JOURNAL_AREA_LIST, XYWH(rBounds),1);	
	pList->Show();
	pList->SetTextColor(TEXT_DARK_GREY_PARCHMENT);	
	AddChild(pList);

	for(n = pJournal->NumAreas; n > 0; n--)
	{
		pList->AddItem(pJournal->AreaNames[n-1]);
	}
	pList->AddItem("All Areas");
	pList->SetText("All Areas");
	
	fclose(fp);

	JournalLeft = pJournal->Current;
	JournalRight = JournalLeft + 1;

	SetText();

}

void Journal::Clear()
{
	NumEntries = 0;
	Current = 0;
	ZeroMemory(Entry, MAX_JOURNAL_ENTRY_COUNT*2*sizeof(uint32_t));
	Init();
}

#else

#include "journal.h"
#include "ZSutilities.h"
#include "zsbutton.h"
#include "ZSText.h"
#include "ZSEngine.h"
#include "party.h"
#include "World.h"

typedef enum
{
	IDC_JOURNAL_QUIT,
	IDC_JOURNAL_PAGEUP,
	IDC_JOURNAL_PAGEDOWN,
	IDC_JOURNAL_LEFT_TEXT,
	IDC_JOURNAL_RIGHT_TEXT,
} JOURNAL_CONTROLS;

LPDIRECTDRAWSURFACE7 JournalWin::JournalSurface = NULL;


void Journal::GetEntry(int num, char *Dest)
{

}

char *Journal::GetEntry(int num)
{
	int EntryNum;
	EntryNum = Entry[num*2];
	int Date;
	Date = Entry[num*2+1];
	char DayString[16];
	sprintf(DayString,"Day %i:   ",Date);
	char IDNum[8];
	sprintf(IDNum,"%i",EntryNum);
	FILE *fp;
	fp = SafeFileOpen("journal.txt","rt");

	SeekToSkip(fp,IDNum);
	SeekTo(fp,"[");
	char *JournalString;
	JournalString = GetString(fp,']');

	fclose(fp);
	char *RetString;
	RetString = new char[strlen(DayString) + strlen(JournalString) + 2];
	strcpy(RetString,DayString);
	strcat(RetString,JournalString);
	delete[] JournalString;
	
	return RetString;
}

BOOL Journal::AddEntry(int Num)
{
	if(PreludeParty.GetBest(INDEX_LITERACY_AND_LORE)->GetData(INDEX_LITERACY_AND_LORE).Value)
	{

		for(int n = 0; n < (NumEntries*2); n+=2)
		{
			if(Entry[n] == Num)
			{
				return FALSE;
			}
		}

		Entry[NumEntries*2] = Num;
		Entry[NumEntries*2+1] = PreludeWorld->GetDay();
		NumEntries++;
		Current = NumEntries - 1;
		if(Current % 2)
		{
			Current -= 1;
		}
	}
	return TRUE;
}

void Journal::RemoveEntry(int Num)
{
	int n, sn;
	for(n = 0; n < NumEntries; n++)
	{
		if(Entry[n*2] == Num)
		{
			NumEntries--;
			for(sn = n; sn < NumEntries; sn ++)
			{
				Entry[sn*2] = Entry[(sn+1)*2];
				Entry[sn*2+1] = Entry[(sn+1)*2+1];
			}
			Current = NumEntries - 1;
			if(Current % 2)
			{
				Current -= 1;
			}
			return;
		}
	}
}

void Journal::Save(FILE *fp)
{
	fwrite(&NumEntries,sizeof(int),1,fp);
	fwrite(&Current,sizeof(int),1,fp);
	fwrite(Entry,sizeof(uint32_t),1024*2,fp);
}

void Journal::Load(FILE *fp)
{
	fread(&NumEntries,sizeof(int),1,fp);
	fread(&Current,sizeof(int),1,fp);
	fread(Entry,sizeof(uint32_t),1024*2,fp);
}

Journal::Journal()
{
	Current = 0;
	NumEntries = 0;
	ZeroMemory(Entry, 1024*2*sizeof(uint32_t));
}

Journal::~Journal()
{

}

void JournalWin::SetText()
{
	if(pJournal->NumEntries)
	{
		ZSWindow *pWin;
		pWin = GetChild(IDC_JOURNAL_LEFT_TEXT);
		pWin->SetText(pJournal->GetEntry(pJournal->Current));

		if(pJournal->Current < pJournal->NumEntries -1)
		{
			pWin = GetChild(IDC_JOURNAL_RIGHT_TEXT);
			pWin->SetText(pJournal->GetEntry(pJournal->Current+1));
		}
		else
		{
			pWin = GetChild(IDC_JOURNAL_RIGHT_TEXT);
			pWin->SetText(" ");
		}
	}
}

int JournalWin::Command(int IDFrom, int Command, int Param)
{
	if(Command == COMMAND_BUTTON_CLICKED)
	{
		if(IDFrom == IDC_JOURNAL_QUIT)
		{
			State = WINDOW_STATE_DONE;
		}
		else
		if(IDFrom == IDC_JOURNAL_PAGEUP)
		{
			if(pJournal->Current - 2 >= 0)
			{
				pJournal->Current -= 2;
				SetText();
			}
		}
		else
		if(IDFrom == IDC_JOURNAL_PAGEDOWN)
		{
			if(pJournal->Current + 2 < pJournal->NumEntries)
			{
				pJournal->Current += 2;
				SetText();
			}
		}
	}
	return TRUE;
}

JournalWin::JournalWin(int NewID, int x, int y, int width, int height)
{
	ID = NewID;
	Type = WINDOW_JOURNAL;
	Visible = FALSE;
	Moveable = FALSE;
	Bounds.left = x;
	Bounds.right = x + width;
	Bounds.top = y;
	Bounds.bottom = y + height;

	FILE *fp;
	RECT rBounds;
	char *FileName;
	int Width;
	int Height;

	fp = SafeFileOpen("gui.ini","rt");

	SeekTo(fp,"[JOURNAL]");

	SeekTo(fp,"BACKGROUND");
	
	FileName = GetStringNoWhite(fp);
	Width = GetInt(fp);
	Height = GetInt(fp);

	if(!JournalSurface)
	{
		BackGroundSurface = Engine->Graphics()->CreateSurfaceFromFile(FileName,Width,Height,NULL,0);
		JournalSurface = BackGroundSurface;
	}
	BackGroundSurface = JournalSurface;
	BackGroundSurface->AddRef();
		
	delete[] FileName;

	ZSButton *pButton;

	SeekTo(fp,"QUIT");
	LoadRect(&rBounds,fp);

	pButton = new ZSButton(BUTTON_NONE, IDC_JOURNAL_QUIT, XYWH(rBounds));
	pButton->Show();
	pButton->SetText("Close");
	AddChild(pButton);

	SeekTo(fp,"PAGEUP");
	LoadRect(&rBounds,fp);

	pButton = new ZSButton("leftpagebutton", IDC_JOURNAL_PAGEUP, XYWH(rBounds),51,68,1);
	pButton->Show();
	AddChild(pButton);

	SeekTo(fp,"PAGEDOWN");
	LoadRect(&rBounds,fp);

	pButton = new ZSButton("rightpagebutton", IDC_JOURNAL_PAGEDOWN, XYWH(rBounds),51,68,1);
	pButton->Show();
	AddChild(pButton);

	ZSText *pText;

	SeekTo(fp,"LEFTPAGE");
	LoadRect(&rBounds,fp);
	pText = new ZSText(IDC_JOURNAL_LEFT_TEXT, XYWH(rBounds)," ",0);	
	pText->Show();
	pText->SetTextColor(TEXT_DARK_GREY_PARCHMENT);
	AddTopChild(pText);

	SeekTo(fp,"RIGHTPAGE");
	LoadRect(&rBounds,fp);
	pText = new ZSText(IDC_JOURNAL_RIGHT_TEXT, XYWH(rBounds)," ",0);	
	pText->Show();
	pText->SetTextColor(TEXT_DARK_GREY_PARCHMENT);
	AddTopChild(pText);

	fclose(fp);

	pJournal = PreludeParty.GetJournal();

	SetText();

}

void Journal::Clear()
{
	NumEntries = 0;
	Current = 0;
	ZeroMemory(Entry, 1024*2*sizeof(uint32_t));
}

#endif