#ifndef ZSMESSAGE_H
#define ZSMESSAGE_H

#include "ZSwindow.h"
#include "zsbutton.h"
#include "ZSText.h"

#define ID_MESSAGE_OK		768
#define ID_MESSAGE_TEXT		769

class ZSMessage : public ZSWindow
{
protected:
	
public:
	int Command(int IDFrom, int Command, int Param);
	int GoModal();
	int HandleKeys(BYTE *CurrentKeys, BYTE* LastKeys);

	ZSMessage(const char *Text, const char *ConfirmText);

};

void Message(const char *Text, const char *ConfirmText);

#endif
