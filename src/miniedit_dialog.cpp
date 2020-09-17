//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

// #include "stdafx.h"
#ifndef _WINDEF_
#include "windef.h"
#endif
#include "resource2.h"

#include "miniedit_dialog.h"

float spawn_miniedit_float(float f, int ctype)
{
	CMiniEditDialog me;	
	me.SetFValue(f);
	me.DoModal(::GetActiveWindow(), NULL);	
	if (me.updated)
		return me.fvalue;
	return f;
}

int spawn_miniedit_int(int i, int ctype)
{
	CMiniEditDialog me;
	me.SetValue(i);
	me.irange = 16777216;	
	me.DoModal(::GetActiveWindow(), NULL);
	if (me.updated)
		return me.ivalue;
	return i;
}