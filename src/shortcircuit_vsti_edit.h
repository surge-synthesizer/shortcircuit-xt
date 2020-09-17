//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "shortcircuit_vsti.h"

#ifndef __AudioEffect__
#include <AudioEffect.h>
#endif

class shortcircuit_vsti_edit : public shortcircuit_vsti
{
public:
	shortcircuit_vsti_edit(audioMasterCallback audioMaster);
	~shortcircuit_vsti_edit();
	virtual void setParameter (long index, float value);

#ifndef _DEMO_
	virtual VstInt32 getChunk (void** data, bool isPreset = false);
	virtual VstInt32 setChunk (void* data, VstInt32 byteSize, bool isPreset = false);	
#endif

};
