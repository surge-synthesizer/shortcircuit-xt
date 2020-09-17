//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include "globals.h"

using namespace std;

const int n_custom_controllers = 16;

enum midi_controller_type
{
	mct_none=0,
	mct_cc,
	mct_rpn,
	mct_nrpn,
	n_ctypes,
};

const char ct_titles[n_ctypes][8] = { ("none"), ("CC"), ("RPN"), ("NRPN") };

struct midi_controller
{
	midi_controller_type type;
	int number;
	char name[16];
};

class configuration
{
public:
	configuration();
	bool load(wstring filename);
	bool save(wstring filename);
	
	void decode_pathW(wstring in, wchar_t *out, wchar_t *extension, int *program_id, int *sample_id);	
			
	wstring relative;
	int stereo_outputs, mono_outputs;
	wstring conf_filename;
	string pathlist[4];
	string skindir;
	int headroom;
	int keyboardmode;
	bool store_in_projdir;	
	midi_controller MIDIcontrol[n_custom_controllers];
	float mPreviewLevel;
	bool mAutoPreview;
	bool mUseMiniDumper;
};

