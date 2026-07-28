#ifndef ZSMAINDESCRIBE_H
#define ZSMAINDESCRIBE_H

#include "ZSwindow.h"

class ZSMainDescribe : public ZSWindow
{
private:



public:

	int Command(int IDFrom, int Command, int Param);

	ZSMainDescribe(int NewID, int x, int y, int width, int Height);

};


#endif