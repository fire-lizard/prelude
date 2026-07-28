#ifndef ZSGETTEXT_H
#define ZSGETTEXT_H

#include "ZSwindow.h"


class ZSGetText : public ZSWindow
{
private:
	
public:

	int Command(int IDFrom, int Command, int Param);

	ZSGetText(const char *Message, const char *StartText);

};


char *GetModalText(const char *Message, const char *StartText = NULL, int Length = 32);


#endif

