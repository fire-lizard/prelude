#ifndef ZSCONFIRM_H
#define ZSCONFIRM_H

#include "ZSwindow.h"

class ZSConfirmWin : public ZSWindow
{
private:

public:

	int Command(int IDFrom, int Command, int Param);
	int MoveMouse(long *x, long *y, long *z);

	int HandleKeys(BYTE *CurrentKeys, BYTE *LastKeys);

	ZSConfirmWin(const char *Message, const char *ResponseA, const char *ResponseB);

};

BOOL Confirm(ZSWindow *pParent, const char *Message, const char *ResponseA, const char *ResponseB);

#endif