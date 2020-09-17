//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
#pragma once

#if WINDOWS
#include <Windows.h>
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
#else
	CRITICAL_SECTION cs;
#endif
	int refcount;
};
