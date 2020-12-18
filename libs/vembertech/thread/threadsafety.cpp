#include "threadsafety.h"
#include "assert.h"
#if MAC
#include <CoreServices/CoreServices.h>
#endif

c_sec::c_sec()
{
	refcount = 0;
#if MAC
	MPCreateCriticalRegion (&cs);
#elif LINUX
#warning IMPLEMENT THIS
#else
	InitializeCriticalSection(&cs);
#endif
}

c_sec::~c_sec()
{
#if MAC
	MPDeleteCriticalRegion(cs);
#elif LINUX
#else
	DeleteCriticalSection(&cs);
#endif
}

void c_sec::enter()
{
#if MAC
	MPEnterCriticalRegion(cs,kDurationForever);
#elif LINUX
#else
	EnterCriticalSection(&cs);
#endif
	refcount++;
	assert(refcount>0);
	assert(!(refcount>5));	// if its >5 there's some crazy *§%* going on ^^	
}

void c_sec::leave()
{
	refcount--;
	if(refcount < 0)
	{
		int breakpointme=0;	
	}
	assert(refcount >= 0);	
#if MAC
	MPExitCriticalRegion(cs);
#elif LINUX
#else
	LeaveCriticalSection(&cs);
#endif
}
