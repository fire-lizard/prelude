#ifndef JOURNAL_H
#define JOURNAL_H

#include "ZSwindow.h"

// Doesn't work but still useful to have around
#if defined(BACKPORT_18) && 1
#define BACKPORT_18_JOURNAL
#endif

#ifdef BACKPORT_18_JOURNAL

#define MAX_AREAS 32
#define MAX_QUESTS 128

static const size_t MAX_JOURNAL_ENTRY_COUNT = 1048;
class Journal
{
public:
	int NumEntries;
	static BOOL IsSetup;
	//time and Entry Number;
	
	unsigned long Entry[MAX_JOURNAL_ENTRY_COUNT *2];
	unsigned int Area[MAX_JOURNAL_ENTRY_COUNT];
	unsigned int Quest[MAX_JOURNAL_ENTRY_COUNT];

	static int NumQuests;
	static char QuestNames[MAX_QUESTS][128];
	static int QuestAreas[MAX_QUESTS];
	static int NumAreas;
	static char AreaNames[MAX_AREAS][128];

	int GetAreaNum(char *AreaName);
	int GetQuestNum(char *QuestName);
	int GetQuestArea(int QuestNum);
	int GetQuestArea(char *QuestName) { return GetQuestArea(GetQuestNum(QuestName)); }

	int Current;

	void GetEntry(int num, char *Dest);
	char *GetEntry(int num);
	int GetEntryQuest(int num);
	int GetEntryArea(int num);

	char *GetSortedEntry(int num, int QuestNum, int AreaNum);
	
	BOOL AddEntry(int Num);
	void RemoveEntry(int Num);

	void Save(FILE *fp);
	void Load(FILE *fp);

	void Clear();

	//load quests and areas
	void Init();

	Journal();
	~Journal();

};

class JournalWin : public ZSWindow
{
private:
	Journal *pJournal;
	int NumLines;
	int ShowAreaNum;
	int ShowQuestNum;

	int JournalLeft;
	int JournalRight;

	void Sort();
	void SortQuests();
	void SetText();
	void PageLeft();
	void PageRight();

public:
	static LPDIRECTDRAWSURFACE7 JournalSurface;

	int Command(int IDFrom, int Command, int Param);
	int HandleKeys(BYTE *CurrentKeys, BYTE *LastKeys);

	int MatchJournalQuestArea(int current);

	JournalWin(int NewID, int x, int y, int width, int height);

};

#else

class Journal
{
public:
	int NumEntries;
	//time and Entry Number;
	uint32_t Entry[1024*2];
	int Current;

	void GetEntry(int num, char *Dest);
	char *GetEntry(int num);
	BOOL AddEntry(int Num);
	void RemoveEntry(int Num);

	void Save(FILE *fp);
	void Load(FILE *fp);

	void Clear();
	Journal();
	~Journal();

};

class JournalWin : public ZSWindow
{
private:
	Journal *pJournal;
	int NumLines;

	void SetText();

public:
	static LPDIRECTDRAWSURFACE7 JournalSurface;

	int Command(int IDFrom, int Command, int Param);
	int HandleKeys(BYTE *CurrentKeys, BYTE *LastKeys);

	int MatchJournalQuestArea(int current);

	JournalWin(int NewID, int x, int y, int width, int height);

};
#endif

#endif