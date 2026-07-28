#ifndef ZSHELPWIN_H
#define ZSHELPWIN_H

#include "ZSwindow.h"

class HelpNode
{
public:
	char ThisTopic[64];
	HelpNode *pNext;
	HelpNode *pPrev;
	
	HelpNode()
	{
		pNext = NULL;
		pPrev = NULL;
		ThisTopic[0] = '\0';
	}
};

class ZSHelpWin : public ZSWindow
{
private:
	HelpNode *pNodeBase;
	HelpNode *pCurNode;

	void GoTopic(const char *Topic);

public:
	static LPDIRECTDRAWSURFACE7 HelpSurface;

	HelpNode *GetBaseNode() { return pNodeBase; }
	HelpNode *GetCurNode() { return pCurNode; }

	int Command(int IDFrom, int Command, int Param);
	
	ZSHelpWin(int NewID, const char *Topic = NULL);

	virtual int HandleKeys(BYTE* CurrentKeys, BYTE* LastKeys);
};

void ShowHelp(const char *HelpTopic);


#endif