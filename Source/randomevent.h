#ifndef RANDOMEVENT_H
#define RANDOMEVENT_H

#ifdef __linux__
#include "linux_aux_wrapper.h"
#endif
#include "defs.h"

class RandomEventManager
{
private:
	unsigned long LastTime;
	int RandomOverride;
	int ValleyEventTypes[20][20];

public:

	void SetOverride(int NewOV) { RandomOverride = NewOV; }

	BOOL CheckForRandomEvent();

	RandomEventManager();

};



#endif
