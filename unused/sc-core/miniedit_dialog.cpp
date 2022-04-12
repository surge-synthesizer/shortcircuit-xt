/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#error THIS FILE IS UNUSED AND SHOULD NOT COMPILE

#if 0
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

#endif
