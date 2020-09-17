#include "vt_gui_controls.h"

enum ctrl_mode
{
	cm_none=0,
	cm_integer,
	cm_notename,
	cm_float,
	cm_percent,
	cm_percent_bipolar,
	cm_decibel,
	cm_decibel_squared,
	cm_envelopetime,
	cm_lforate,
	cm_midichannel,
	cm_mutegroup,
	cm_lag,
	cm_frequency20_20k,
	cm_frequency50_50k,
	cm_bitdepth_16,
	cm_frequency0_2k,
	cm_decibelboost12,
	cm_octaves3,
	cm_frequency1hz,
	cm_time1s,	
	cm_frequency_audible,
	cm_frequency_samplerate,
	cm_frequency_khz,
	cm_frequency_khz_bi,
	cm_frequency_hz_bi,
	cm_eq_bandwidth,
	cm_stereowidth,
	cm_mod_decibel,
	cm_mod_pitch,
	cm_mod_freq,	
	cm_mod_percent,
	cm_mod_time,
	cm_polyphony,
	cm_envshape,
	cm_osccount,
	cm_count4,
	cm_noyes,
	cm_temposync,
	cm_int_menu,
};

#define setFloatMin(x) if(fmin) *fmin = x
#define setFloatMax(x) if(fmax) *fmax = x
#define setDefaultValue(x) if(fdefault) *fdefault = x
#define setIntMin(x) if(imin) *imin = x
#define setIntMax(x) if(imax) *imax = x
#define setIntDefaultValue(x) if(idefault) *idefault = x

void scpe_get_limits(int cmode, float* fmin, float* fmax, float* fdefault, int* imin, int* imax, int* idefault)
{
	float f_movespeed;
	switch(cmode)
	{
	case cm_percent_bipolar:
		setFloatMin(-1.f);
		setFloatMax(1.f);		
		setDefaultValue(0.0f);
		f_movespeed = 0.01f;
		break;		
	case cm_frequency1hz:
		setFloatMin(-10);
		setFloatMax(5);
		setDefaultValue(0);
		f_movespeed = 0.04;
		break;
	case cm_lag:
		setFloatMin(-10);
		setFloatMax(5);
		setDefaultValue(0);
		f_movespeed = 0.1;
		break;
	case cm_time1s:
		setFloatMin(-10);
		setFloatMax(10);
		setDefaultValue(0);
		f_movespeed = 0.1;
		break;
	case cm_decibel:
		setFloatMin(-144);
		setFloatMax(24);
		setDefaultValue(0);
		f_movespeed = 0.15;
		break;
	case cm_mod_decibel:
		setFloatMin(-144);
		setFloatMax(144);
		setDefaultValue(0);
		f_movespeed = 0.15;
		break;
	case cm_mod_time:
		setFloatMin(-8);
		setFloatMax(8);
		setDefaultValue(0);
		f_movespeed = 0.01;
		break;
	case cm_mod_pitch:
		setFloatMin(-128);
		setFloatMax(128);
		setDefaultValue(0);
		f_movespeed = 0.1;
		break;
	case cm_mod_freq:
		setFloatMin(-12);
		setFloatMax(12);
		setDefaultValue(0);
		f_movespeed = 0.01;
		break;
	case cm_eq_bandwidth:
		setFloatMin(0.0001);
		setFloatMax(20);
		setDefaultValue(1);
		f_movespeed = 0.01;
		break;
	case cm_mod_percent:
		setFloatMin(-32);
		setFloatMax(32);
		setDefaultValue(0);
		f_movespeed = 0.01;
		break;
	case cm_stereowidth:
		setFloatMin(0);
		setFloatMax(4);
		setDefaultValue(1);
		f_movespeed = 0.01;
		break;
	case cm_frequency_audible:
		setFloatMin(-7);
		setFloatMax(6.0);
		setDefaultValue(0);
		f_movespeed = 0.01;
		break;
	case cm_frequency_samplerate:
		setFloatMin(-5);
		setFloatMax(7.5);
		setDefaultValue(0);
		f_movespeed = 0.01;
		break;
	case cm_frequency_khz:
		setFloatMin(0);
		setFloatMax(20);
		setDefaultValue(0);
		f_movespeed = 0.1;
		break;
	case cm_frequency_hz_bi:
		setFloatMin(-20000);
		setFloatMax(20000);
		setDefaultValue(0);
		f_movespeed = 0.1;		
		break;
	case cm_frequency_khz_bi:
		setFloatMin(-20);
		setFloatMax(20);
		setDefaultValue(0);
		f_movespeed = 0.1;		
		break;
	case cm_polyphony:
		setIntMin(1);
#ifdef SCFREE
		setIntMax(2);
#else
		//setIntMax(max_voices);
		setIntMax(256);
#endif
		break;
	case cm_mutegroup:
		setIntMin(0);
		//setIntMax(n_mute_groups);
		setIntMax(64);
		break;
	case cm_envshape:
		setFloatMin(-10.f);
		setFloatMax(20.f);
		setDefaultValue(0.f);
		f_movespeed = 0.02f;
		break;
	case cm_osccount:
		setFloatMin(0.5f);
		setFloatMax(16.5f);
		f_movespeed = 0.1f;
		break;
	case cm_count4:
		setFloatMin(0.5f);
		setFloatMax(4.5f);
		f_movespeed = 0.1f;
		break;
	case cm_noyes:
		setIntMin(0);
		setIntMax(1);
		setIntDefaultValue(0);
		break;
	case cm_temposync:
		setIntMin(0);
		//setIntMax(n_syncmodes-1);
		//value = -1;
		break;
	default:
		setFloatMin(0.f);
		setFloatMax(1.f);
		setDefaultValue(0.5f);
		f_movespeed = 0.01f;
		break;
	};
}