#ifndef ZSASKWIN_H
#define ZSASKWIN_H

#include "ZSwindow.h"
#include "script.h"

class ZSAskWin : public ZSWindow
{
private:

public:
	static LPDIRECTDRAWSURFACE7 AskSurface;

	int LeftButtonDown(int x, int y, int doubleClick);

	int Command(int IDFrom, int Command, int Param);

	int HandleKeys(BYTE *CurrentKeys, BYTE *LastKeys);

	//nothing to pick means nothing can dismiss it
	BOOL HasOptions();
	int OptionCount();

	ZSAskWin(int NewID, int x, int y, int Width, int Height, ScriptArg *pOptions);
};



#endif