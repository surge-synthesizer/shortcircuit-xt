//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "threadsafety.h"
#include "assert.h"

c_sec::c_sec()
{
	refcount = 0;
	InitializeCriticalSectionAndSpinCount(&cs, 32);
}

c_sec::~c_sec()
{
	DeleteCriticalSection(&cs);
}

void c_sec::enter()
{
	//assert(cs.RecursionCount == 0);
	// temporary investigation assertion
	// try find any locks
	
	EnterCriticalSection(&cs);
	refcount++;
	assert(refcount>0);
	assert(!(refcount>5));	// if its >5 there's some crazy shit going on ^^	
}

void c_sec::leave()
{
	refcount--;
	assert(refcount >= 0);	
	LeaveCriticalSection(&cs);
}

// critical section
/*CRITICAL_SECTION cs_patch, cs_gui, cs_engine;
int patchcount = 0, guicount = 0;

void init_critsec()
{
	InitializeCriticalSection(&cs_patch);
	InitializeCriticalSection(&cs_gui);
	InitializeCriticalSection(&cs_engine);
}
void delete_critsec()
{
	DeleteCriticalSection(&cs_patch);
	DeleteCriticalSection(&cs_gui);
	DeleteCriticalSection(&cs_engine);
}

void enterPatch()
{
	patchcount++;
	if(patchcount > 1)
	{
		int i = 0;		// block occured
	}
	EnterCriticalSection(&cs_patch);
}

void leavePatch()
{
	patchcount--;
	LeaveCriticalSection(&cs_patch);
}

void enterGUI()
{
	guicount++;
	if(guicount > 1)
	{
		int i = 0;		// block occured
	}
	EnterCriticalSection(&cs_gui);
}

void leaveGUI()
{
	guicount--;
	LeaveCriticalSection(&cs_gui);
}

void enterEngine()
{
	EnterCriticalSection(&cs_engine);
}

void leaveEngine()
{
	LeaveCriticalSection(&cs_engine);
}

*/