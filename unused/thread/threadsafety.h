#pragma once

#error THIS IS NO LONGER NEEDED

#if MAC
#include <CoreServices/CoreServices.h>
#elif LINUX
#else
#include "windows.h"
#endif

class c_sec
{
public:
	c_sec();
	~c_sec();
	void enter();
	void leave();
protected:
	#if MAC
	MPCriticalRegionID cs;
        #elif LINUX
	#else
	CRITICAL_SECTION cs;
	#endif
	int refcount;
};
