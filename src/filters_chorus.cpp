//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004-2006 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#define  _USE_MATH_DEFINES
#include "filter_defs.h"
#include "resampling.h"
#include "tools.h"
#include <assert.h>
#include "unitconversion.h"

extern float	SincTableF32[(FIRipol_M+1)*FIRipol_N];
extern float	SincOffsetF32[(FIRipol_M)*FIRipol_N];	

/* chorus			*/

chorus::chorus(float *fp, int *ip) 
: filter(fp,0,true,ip)
{	
	feedback.set_blocksize(block_size);

	strcpy(filtername,"Chorus");
	parameter_count = 7;	
	strcpy(ctrllabel[0], "Delay Time");			
	strcpy(ctrllabel[1], "Mod Rate");			
	strcpy(ctrllabel[2], "Mod Depth");			
	strcpy(ctrllabel[3], "Feedback");			
	strcpy(ctrllabel[4], "Low Cut");
	strcpy(ctrllabel[5], "High Cut");
	strcpy(ctrllabel[6], "Width");	
			
	strcpy(ctrlmode_desc[0], str_timedef);	
	strcpy(ctrlmode_desc[1], str_lfofreqdef);
	strcpy(ctrlmode_desc[2], str_percentdef);	
	strcpy(ctrlmode_desc[3], str_dbdef);
	strcpy(ctrlmode_desc[4], str_freqdef);	
	strcpy(ctrlmode_desc[5], str_freqdef);
	strcpy(ctrlmode_desc[6], str_dbbpdef);				
}

chorus::~chorus()
{
}
void chorus::init_params()
{
	assert(param);
	if(!param) return;
	// preset values

	param[0] = -6.f;
	param[1] = -2.f;
	param[2] = 0.3f;
	param[3] = -96.0f;
	param[4] = -3.f;
	param[5] = 3.f;
	param[6] = 0.f;	
}
void chorus::init()
{
	memset(buffer,0,(max_delay_length+FIRipol_N)*sizeof(float));
	wpos = 0;	
	envf = 0;
	const float gainscale = 1/sqrt(4.f);

	for(int i=0; i<4; i++)
	{
		time[i].setRate(0.001);
		float x = i;
		x /= (4.f-1.f);
		lfophase[i] = x;		
		x = 2.f*x - 1.f;
		voicepan[i][0] = sqrt(0.5 - 0.5*x)*gainscale;
		voicepan[i][1] = sqrt(0.5 + 0.5*x)*gainscale;

		voicepanL4[i] = _mm_set1_ps(voicepan[i][0]);
		voicepanR4[i] = _mm_set1_ps(voicepan[i][1]);
	}

	setvars(true);
}

void chorus::setvars(bool init)
{
	if(init)
	{
		//feedback.set_target(0.5f * amp_to_linear(param[3]));			
		feedback.set_target(db_to_linear(param[3]));			
		hp.coeff_HP(hp.calc_omega(param[4]),0.707);
		lp.coeff_LP2B(lp.calc_omega(param[5]),0.707);		
		width.set_target(db_to_linear(param[6]));
	}
	else
	{
		feedback.set_target_smoothed(db_to_linear(param[3]));			
		float rate = envelope_rate_linear(-param[1]);
		float tm = note_to_pitch(12* param[0]);
		for(int i=0; i<4; i++)
		{
			lfophase[i] += rate;
			if(lfophase[i]>1) lfophase[i] -= 1;	
			//float lfoout = 0.5*storage->lookup_waveshape_warp(3,4.f*lfophase[i]-2.f) * *f[2];	
			float lfoout = (2.f * fabs(2.f*lfophase[i] - 1.f) - 1.f) * param[2];
			time[i].newValue(samplerate * tm * (1+lfoout));	
		}	

		hp.coeff_HP(hp.calc_omega(param[4]),0.707);
		lp.coeff_LP2B(lp.calc_omega(param[5]),0.707);		
		width.set_target_smoothed(db_to_linear(param[6]));
	}
}

void chorus::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch)
{			
	setvars(false);

	Align16 float tbufferL[block_size];
	Align16 float tbufferR[block_size];
	Align16 float fbblock[block_size];
	int k;	

	/*for(k=0; k<block_size; k++)
	{				
		tbufferL[k] = 0;
		tbufferR[k] = 0;
		for(int j=0; j<v; j++)
		{	
			time[j].process();
			float vtime = time[j].v;
			int i_dtime = max(block_size,min((int)vtime,max_delay_length-FIRipol_N-1));				
			int rp = (wpos-i_dtime+k);			
			int sinc = FIRipol_N*limit_range((int)(FIRipol_M*(float(i_dtime + 1)-vtime)),0,FIRipol_M-1);		

			
			float vo = 0.f;
			for(int i=0; i<FIRipol_N; i++)
			{
				vo += buffer[(rp-i)&(max_delay_length-1)]*storage->SincTableF321X[sinc + FIRipol_N - i - 1];			
			}
			tbufferL[k] += vo*voicepan[j][0];
			tbufferR[k] += vo*voicepan[j][1];
		}
		tbufferL[k] *= gainscale;
		tbufferR[k] *= gainscale;
	}	*/

	clear_block_antidenormalnoise(tbufferL,block_size_quad);
	clear_block_antidenormalnoise(tbufferR,block_size_quad);
#if MAC
#else
	
	for(k=0; k<block_size; k++)
	{						
		__m128	L=_mm_setzero_ps(),
				R=_mm_setzero_ps();

		for(int j=0; j<4; j++)
		{	
			time[j].process();
			float vtime = time[j].v;
			int i_dtime = max(block_size,min((int)vtime,max_delay_length-FIRipol_N-1));				
			int rp = ((wpos-i_dtime+k)-FIRipol_N)&(max_delay_length-1);			
			int sinc = FIRipol_N*limit_range((int)(FIRipol_M*(float(i_dtime + 1)-vtime)),0,(int)FIRipol_M-1);		
			
			__m128 vo;			
			vo = _mm_mul_ps(_mm_load_ps(&SincTableF32[sinc]),_mm_loadu_ps(&buffer[rp]));
			vo = _mm_add_ps(vo,_mm_mul_ps(_mm_load_ps(&SincTableF32[sinc+4]),_mm_loadu_ps(&buffer[rp+4])));
			vo = _mm_add_ps(vo,_mm_mul_ps(_mm_load_ps(&SincTableF32[sinc+8]),_mm_loadu_ps(&buffer[rp+8])));

			L = _mm_add_ps(L,_mm_mul_ps(vo,voicepanL4[j]));
			R = _mm_add_ps(R,_mm_mul_ps(vo,voicepanR4[j]));			
		}
		L = sum_ps_to_ss(L);
		R = sum_ps_to_ss(R);	
		_mm_store_ss(&tbufferL[k],L);
		_mm_store_ss(&tbufferR[k],R);
	}
#endif

	lp.process_block(tbufferL,tbufferR);
	hp.process_block(tbufferL,tbufferR);
	add_block(tbufferL,tbufferR,fbblock,block_size_quad);
	feedback.multiply_block(fbblock,block_size_quad);
	hardclip_block(fbblock,block_size_quad);		
	accumulate_block(datainL,fbblock,block_size_quad);
	accumulate_block(datainR,fbblock,block_size_quad);
	
	if(wpos+block_size >= max_delay_length)
	{
		for(k=0; k<block_size; k++)
		{						
			buffer[(wpos+k)&(max_delay_length-1)] = fbblock[k];
		}
	}
	else
	{
		/*for(k=0; k<block_size; k++)
		{									
			buffer[wpos+k] = fbblock[k];
		}*/
		copy_block(fbblock,&buffer[wpos],block_size_quad);
	}

	if(wpos == 0) for(k=0; k<FIRipol_N; k++)	buffer[k+max_delay_length] = buffer[k];		// copy buffer so FIR-core doesn't have to wrap

	// scale width
	Align16 float M[block_size],S[block_size];
	encodeMS(tbufferL,tbufferR,M,S,block_size_quad);
	width.multiply_block(S,block_size_quad);
	decodeMS(M,S,dataoutL,dataoutR,block_size_quad);
	
	
	wpos += block_size;
	wpos = wpos&(max_delay_length-1);
}

void chorus::suspend()
{ 
	init();
}
/*
template<int v>
void chorus<v>::init_ctrltypes()
{
	baseeffect::init_ctrltypes();

	fxdata->p[0].set_name("time");					fxdata->p[0].set_type(ct_delaymodtime);
	fxdata->p[1].set_name("rate");				fxdata->p[1].set_type(ct_lforate);
	fxdata->p[2].set_name("depth");				fxdata->p[2].set_type(ct_percent);	
	fxdata->p[3].set_name("feedback");				fxdata->p[3].set_type(ct_amplitude);		
	fxdata->p[4].set_name("low cut");				fxdata->p[4].set_type(ct_freq_audible);
	fxdata->p[5].set_name("high cut");				fxdata->p[5].set_type(ct_freq_audible);
	fxdata->p[6].set_name("mix");					fxdata->p[6].set_type(ct_percent);
	fxdata->p[7].set_name("width");					fxdata->p[7].set_type(ct_decibel_narrow);	
	
	fxdata->p[0].posy_offset = 1;
	fxdata->p[1].posy_offset = 3;
	fxdata->p[2].posy_offset = 3;
	fxdata->p[3].posy_offset = 5;
	fxdata->p[4].posy_offset = 7;
	fxdata->p[5].posy_offset = 7;	
	fxdata->p[6].posy_offset = 9;
	fxdata->p[7].posy_offset = 9;
}
template<int v>
char* chorus<v>::group_label(int id)
{
	switch (id)
	{
	case 0:
		return "DELAY";		
	case 1:
		return "MODULATION";		
	case 2:
		return "FEEDBACK";			
	case 3:
		return "EQ";
	case 4:
		return "OUTPUT";
	}
	return 0;
}
template<int v>
int chorus<v>::group_label_ypos(int id)
{
	switch (id)
	{
	case 0:
		return 1;		
	case 1:
		return 5;
	case 2:
		return 11;		
	case 3:
		return 15;
	case 4:
		return 21;
	}
	return 0;
}

template<int v>
void chorus<v>::init_default_values()
{
	fxdata->p[0].val.f = -6.f;
	fxdata->p[1].val.f = -2.f;
	fxdata->p[2].val.f = 0.3f;
	fxdata->p[3].val.f = 0.5f;
	fxdata->p[4].val.f = -3.f*12.f;
	fxdata->p[5].val.f = 3.f*12.f;
	fxdata->p[6].val.f = 1.f;
	fxdata->p[7].val.f = 0.f;
}
*/