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
	IDC_JOURNAL_AREA_LIST,
	IDC_JOURNAL_HIDEDONE
} JOURNAL_CONTROLS;

LPDIRECTDRAWSURFACE7 JournalWin::JournalSurface = NULL;

BOOL Journal::IsSetup = FALSE;
int Journal::NumQuests = 0;
char Journal::QuestNames[MAX_QUESTS][128];
int Journal::QuestAreas[MAX_QUESTS];
int Journal::NumAreas = 0;
char Journal::AreaNames[MAX_AREAS][128];
int Journal::QuestEndings[MAX_QUESTS][MAX_QUEST_ENDINGS];
int Journal::NumQuestEndings[MAX_QUESTS];

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

	//an entry reads "<id> [Quest] [Area]".  Both tags have to be read before
	//the quest can be named, because the area is what tells the Generals apart.
	char *First;
	char *Second;

	SeekTo(fp,"[");
	First = GetString(fp,']');
	SeekTo(fp,"[");
	Second = GetString(fp,']');

	fclose(fp);

	int QuestNum = -1;
	int AreaNum;
	char *QuestTag = NULL;

	AreaNum = GetAreaNum(Second);
	if(AreaNum != -1)
	{
		QuestTag = First;
	}
	else
	{
		//tolerate the tags the other way round
		AreaNum = GetAreaNum(First);
		if(AreaNum != -1)
		{
			QuestTag = Second;
		}
	}

	if(QuestTag)
	{
		QuestNum = GetQuestNum(QuestTag, AreaNum);
		if(QuestNum == -1)
		{
			//journal.txt 609 files River Pilgrim under Ironwood while
			//journalquests.txt files it under the Monastery.  Fall back to the
			//name so entries like that stay reachable - a General always
			//resolves on the pair, so this cannot reintroduce the collapse.
			QuestNum = GetQuestNum(QuestTag);
		}
	}

	delete[] First;
	delete[] Second;

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

//journalquests.txt gives every area its own [General], so a quest is only
//unique as (name, area).  Matching on the name alone lands all ten Generals
//on the first one, the Watcher's Quest.
int Journal::GetQuestNum(char *QuestName, int AreaNum)
{
	int n;
	for(n = 0; n < NumQuests; n++)
	{
		if(QuestAreas[n] == AreaNum && !strcmp(QuestNames[n],QuestName))
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

//pulls the Which'th [bracketed] field out of one line, NULL if it has none
static char *GetBracketField(const char *Line, int Which, char *Dest, int Size)
{
	const char *pAt = Line;
	int n;

	for(n = 0; n <= Which; n++)
	{
		pAt = strchr(pAt, '[');
		if(!pAt)
			return NULL;
		pAt++;
	}

	const char *pEnd = strchr(pAt, ']');
	if(!pEnd)
		return NULL;

	int Length;
	Length = pEnd - pAt;
	if(Length > Size - 1)
		Length = Size - 1;

	strncpy(Dest, pAt, Length);
	Dest[Length] = '\0';

	return Dest;
}

//journalquests.txt is "[Quest] [Area]" per line, plus an optional third
//field listing the entries that finish the quest: "[933 935]".  Read a line
//at a time - SeekTo would happily wander onto the next line looking for the
//third field and swallow the next quest's name.
void Journal::Init()
{
	if(IsSetup)
		return;

	int n;
	for (n = 0; n < MAX_QUESTS; n++)
	{
		ZeroMemory(QuestNames[n], 128 * sizeof(char));
		NumQuestEndings[n] = 0;
	}

	for (n = 0; n < MAX_AREAS; n++)
	{
		ZeroMemory(AreaNames[n], 128 * sizeof(char));
	}

	FILE *fp;
	fp = SafeFileOpen("journalquests.txt","rt");

	NumQuests = 0;
	NumAreas = 0;

	char Line[512];
	char QuestName[128];
	char AreaName[128];
	char Endings[128];

	while(fgets(Line, sizeof(Line), fp) && NumQuests < MAX_QUESTS)
	{
		if(!GetBracketField(Line, 0, QuestName, sizeof(QuestName))
			|| !GetBracketField(Line, 1, AreaName, sizeof(AreaName)))
		{
			continue;	//the blank line between two areas
		}

		strcpy(QuestNames[NumQuests], QuestName);

		int AreaNum;
		AreaNum = GetAreaNum(AreaName);
		if(AreaNum == -1)
		{
			if(NumAreas >= MAX_AREAS)
				break;

			AreaNum = NumAreas;
			strcpy(AreaNames[NumAreas], AreaName);
			NumAreas++;
		}
		QuestAreas[NumQuests] = AreaNum;

		if(GetBracketField(Line, 2, Endings, sizeof(Endings)))
		{
			char *pAt = Endings;
			while(NumQuestEndings[NumQuests] < MAX_QUEST_ENDINGS)
			{
				while(*pAt == ' ' || *pAt == '	' || *pAt == ',')
					pAt++;

				if(*pAt < '0' || *pAt > '9')
					break;

				QuestEndings[NumQuests][NumQuestEndings[NumQuests]] = atoi(pAt);
				NumQuestEndings[NumQuests]++;

				while(*pAt >= '0' && *pAt <= '9')
					pAt++;
			}
		}

		NumQuests++;
	}

	fclose(fp);

	IsSetup = TRUE;
}

//a quest counts as finished once the party holds one of the entries that
//journalquests.txt names as an ending.  Branching quests name all of them.
BOOL Journal::IsQuestEnding(int QuestNum, int EntryNum)
{
	if(QuestNum < 0 || QuestNum >= MAX_QUESTS)
		return FALSE;

	int n;
	for(n = 0; n < NumQuestEndings[QuestNum]; n++)
	{
		if(QuestEndings[QuestNum][n] == EntryNum)
			return TRUE;
	}

	return FALSE;
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

int JournalWin::HandleKeys(BYTE *CurrentKeys, BYTE *LastKeys)
{
	//escape shuts the book, same as the quit button
	if(CurrentKeys[DIK_ESCAPE] & 0x80 && !(LastKeys[DIK_ESCAPE] & 0x80))
	{
		State = WINDOW_STATE_DONE;
		return TRUE;
	}

	//everything else (F1 help, the hard exit) stays with the base window
	return ZSWindow::HandleKeys(CurrentKeys, LastKeys);
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
		if(IDFrom == IDC_JOURNAL_HIDEDONE)
		{
			HideDone = !HideDone;
			((ZSButton *)GetChild(IDC_JOURNAL_HIDEDONE))->SetText(
				HideDone ? "Show finished" : "Hide finished");

			//the rows move, so whichever quest was picked is not that row now
			ShowQuestNum = -1;
			BuildQuestList();
			SortQuests();
			Sort();
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
			NewNum = QuestFromList(pList->GetSelection());
			if(NewNum != ShowQuestNum)
			{
				ShowQuestNum = NewNum;
				Sort();
			}
		}
	}

	return TRUE;
}

//an entry has to clear both filters; "All" is -1 and matches everything.
int JournalWin::MatchJournalQuestArea(int current) {
	if (ShowAreaNum >= 0 && (pJournal->GetEntryArea(current) != ShowAreaNum)) {
		return FALSE;
	}

	if (ShowQuestNum >= 0 && (pJournal->GetEntryQuest(current) != ShowQuestNum)) {
		return FALSE;
	}

	return TRUE;
}

//Each row keeps its insertion index as its ID, and the list only holds the
//quests the party has written about, so the row -> quest mapping is recorded
//while the list is built.  Going back through the row's text would collapse
//every General onto one quest.  -1 is the "All Quests" row.
int JournalWin::QuestFromList(int ItemID)
{
	if(ItemID < 0 || ItemID >= MAX_QUESTS)
		return -1;

	return ListQuest[ItemID];
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
				&& !MatchJournalQuestArea(JournalRight))
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
					&& !MatchJournalQuestArea(pJournal->Current))
				{
					pJournal->Current++;
				}
		}

		JournalLeft = pJournal->Current;

		JournalRight = pJournal->Current + 1;

		if(ShowAreaNum != -1 || ShowQuestNum != -1)
		{
				while(JournalRight < pJournal->NumEntries 
					&& !MatchJournalQuestArea(JournalRight))
				{
					JournalRight++;
				}
		}


	}
}

//Which quests the party has actually written anything about.  GetEntryQuest
//re-reads journal.txt for every entry, so this is done once when the book is
//opened rather than on every click.  A fresh JournalWin is built per open, so
//the answer cannot go stale while it is up.
void JournalWin::FindQuestsWithEntries()
{
	ZeroMemory(QuestHasEntries, sizeof(QuestHasEntries));
	ZeroMemory(QuestDone, sizeof(QuestDone));

	int n;
	for(n = 0; n < pJournal->NumEntries; n++)
	{
		int QuestNum;
		QuestNum = pJournal->GetEntryQuest(n);
		if(QuestNum >= 0 && QuestNum < MAX_QUESTS)
		{
			QuestHasEntries[QuestNum] = TRUE;

			if(pJournal->IsQuestEnding(QuestNum, pJournal->Entry[n*2]))
			{
				QuestDone[QuestNum] = TRUE;
			}
		}
	}

	ZeroMemory(AreaHasEntries, sizeof(AreaHasEntries));
	for(n = 0; n < pJournal->NumQuests; n++)
	{
		if(QuestHasEntries[n])
		{
			AreaHasEntries[pJournal->GetQuestArea(n)] = TRUE;
		}
	}
}

//a diary has no page for a quest the party never heard of, so only the ones
//they have written about are listed - and finished ones can be put away too.
//AddItem stamps each row with its insertion index, which is what ListQuest is
//keyed on, so the mapping has to be recorded as the list is filled.
void JournalWin::BuildQuestList()
{
	ZSList *pList = (ZSList *)GetChild(IDC_JOURNAL_QUEST_LIST);

	pList->Clear();

	int n;
	int NextRow = 0;
	for(n = pJournal->NumQuests; n > 0; n--)
	{
		if(!QuestHasEntries[n-1])
			continue;

		if(HideDone && QuestDone[n-1])
			continue;

		ListQuest[NextRow] = n-1;
		NextRow++;
		pList->AddItem(pJournal->QuestNames[n-1]);
	}

	ListQuest[NextRow] = -1;
	pList->AddItem("All Quests");
	pList->SetText("All Quests");
}

void JournalWin::SortQuests()
{
	ZSList *pList = (ZSList *)GetChild(IDC_JOURNAL_QUEST_LIST);

	int NumItems;
	NumItems = pList->GetNumItems();
	int n;

	for(n = 0; n < NumItems - 1; n++)
	{
		int QuestNum;
		QuestNum = QuestFromList(n);

		if(ShowAreaNum != -1 && pJournal->GetQuestArea(QuestNum) != ShowAreaNum)
		{
			pList->DisableItem(n);
		}
		else
		{
			pList->EnableItem(n);

			//finished reads red on the parchment, still running stays default
			pList->SetItemColor(n, QuestDone[QuestNum] ? TEXT_RED_PARCHMENT : -1);
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
	HideDone = FALSE;

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

	//the free strip between the page-back arrow (226..277) and Close (450..),
	//and above the left page (starts at 125).  GetChild(x,y) hands the click to
	//the first child whose bounds contain it, so overlapping any of them would
	//quietly turn the page instead.
	pButton = new ZSButton(BUTTON_NONE, IDC_JOURNAL_HIDEDONE, 290, 100, 155, 24);
	pButton->Show();
	pButton->SetText("Hide finished");
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

	FindQuestsWithEntries();

	SeekTo(fp,"QUESTLIST");
	LoadRect(&rBounds,fp);
	pList = new ZSList(IDC_JOURNAL_QUEST_LIST, XYWH(rBounds),1);	
	pList->Show();
	pList->SetTextColor(TEXT_DARK_GREY_PARCHMENT);	
	AddChild(pList);

	int n = 0;

	BuildQuestList();

	
	SeekTo(fp,"AREALIST");
	LoadRect(&rBounds,fp);
	pList = new ZSList(IDC_JOURNAL_AREA_LIST, XYWH(rBounds),1);	
	pList->Show();
	pList->SetTextColor(TEXT_DARK_GREY_PARCHMENT);	
	AddChild(pList);

	for(n = pJournal->NumAreas; n > 0; n--)
	{
		if(AreaHasEntries[n-1])
			pList->AddItem(pJournal->AreaNames[n-1]);
	}
	pList->AddItem("All Areas");
	pList->SetText("All Areas");
	
	fclose(fp);

	JournalLeft = pJournal->Current;
	JournalRight = JournalLeft + 1;

	SortQuests();

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

int JournalWin::HandleKeys(BYTE *CurrentKeys, BYTE *LastKeys)
{
	//escape shuts the book, same as the quit button
	if(CurrentKeys[DIK_ESCAPE] & 0x80 && !(LastKeys[DIK_ESCAPE] & 0x80))
	{
		State = WINDOW_STATE_DONE;
		return TRUE;
	}

	//everything else (F1 help, the hard exit) stays with the base window
	return ZSWindow::HandleKeys(CurrentKeys, LastKeys);
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