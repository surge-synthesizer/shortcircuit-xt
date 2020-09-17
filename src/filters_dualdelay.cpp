//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2005 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "filter_defs.h"

extern float	SincTableF32[(FIRipol_M+1)*FIRipol_N];
extern float	SincOffsetF32[(FIRipol_M)*FIRipol_N];	


/* dualdelay			*/

dualdelay::dualdelay(float* fp, int* ip) 
: filter(fp,0,true,ip), timeL(0.0001), timeR(0.0001),
lp(),hp()
{	
	strcpy(filtername,"delay");
	parameter_count = 9;
	strcpy(ctrllabel[0], "time (L)");
	strcpy(ctrllabel[1], "time (R)");
	strcpy(ctrllabel[2], "feedback");
	strcpy(ctrllabel[3], "crossfeed");
	strcpy(ctrllabel[4], "low cut");
	strcpy(ctrllabel[5], "high cut");
	strcpy(ctrllabel[6], "mod rate");
	strcpy(ctrllabel[7], "mod depth");
	strcpy(ctrllabel[8], "pan");	

	strcpy(ctrlmode_desc[0], str_timedef);
	strcpy(ctrlmode_desc[1], str_timedef);
	strcpy(ctrlmode_desc[2], str_dbdef);
	strcpy(ctrlmode_desc[3], str_dbdef);
	strcpy(ctrlmode_desc[4], str_freqdef);
	strcpy(ctrlmode_desc[5], str_freqdef);
	strcpy(ctrlmode_desc[6], str_freqdef);
	strcpy(ctrlmode_desc[7], str_percentdef);
	strcpy(ctrlmode_desc[8], str_percentbpdef);

	pan.set_blocksize(block_size);
	feedback.set_blocksize(block_size);
	crossfeed.set_blocksize(block_size);
	init();
}

void dualdelay::init_params()
{
	if(!param) return;
	// preset values
	param[0] = -1.0f;
	param[1] = -1.0f;
	param[2] = -16.0f;
	param[3] = -48.0f;
	param[4] = -2.0f;
	param[5] = 2.5f;
	param[6] = -2.0f;
	param[7] = 0.1f;
	param[8] = 0.0f;	
}

dualdelay::~dualdelay()
{
}

void dualdelay::init()
{
	memset(buffer[0],0,(max_delay_length+FIRipol_N)*sizeof(float));
	memset(buffer[1],0,(max_delay_length+FIRipol_N)*sizeof(float));
	wpos = 0;
	lfophase = 0;
	ringout_time = 100000;
	envf = 0;
	LFOval = 0;
	LFOdirection = true;
	lp.suspend();
	hp.suspend();	
	setvars(true);
}

void dualdelay::setvars(bool init)
{
	float fb = db_to_linear(param[2]);
	float cf = db_to_linear(param[3]);

	feedback.set_target_smoothed(fb);		
	crossfeed.set_target_smoothed(cf);			 	

	float lforate = envelope_rate_linear(-param[6]);
	lfophase += lforate;
	if(lfophase>0.5) 
	{
		lfophase -= 1;
		LFOdirection = !LFOdirection;
	}

	float lfo_increment = (0.00000000001f + powf(2,param[7] * (1.f/12.f))-1.f) * block_size;
	// small bias to avoid denormals

	const float ca = 0.99f;
	if(param[7] == 0.f) LFOval = 0.f;	// will make noises during the switch, but at least its better than frozen modulation
	else if (LFOdirection) LFOval = ca*LFOval + lfo_increment;
	else LFOval = ca*LFOval - lfo_increment;
	
	timeL.newValue(samplerate * (note_to_pitch(12* param[0])) + LFOval - FIRoffset);			
	timeR.newValue(samplerate * (note_to_pitch(12* param[1])) - LFOval - FIRoffset);	
	
	const float db96 = powf(10.f,0.05f*-96.f);
	float maxfb = max(db96,fb+cf);
	if(maxfb<1.f)
	{				
		float f = inv_block_size * max(timeL.v,timeR.v) * (1.f + log(db96)/log(maxfb)); 
		ringout_time = (int)f;
	}
	else
	{		
		ringout_time = -1;
//		ringout = 0;
	}
	
	pan.set_target_smoothed(limit_range(param[8],-1.f,1.f));
	
	hp.coeff_HP(hp.calc_omega(param[4]),0.707);
	lp.coeff_LP2B(lp.calc_omega(param[5]),0.707);
	if(init)
	{
		timeL.instantize();
		timeR.instantize();
		feedback.instantize();
		crossfeed.instantize();
		pan.instantize();
		hp.coeff_instantize();
		lp.coeff_instantize();
	}
}

void dualdelay::process(float *datain, float *dataout, float pitch)
{
	
}
void dualdelay::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch)
{			
	setvars(false);
	
	Align16 float wbL[block_size];	// wb = write-buffer
	Align16 float wbR[block_size];
	int k;
	
	for(k=0; k<block_size; k++)
	{
		timeL.process();
		timeR.process();		
		
		int i_dtimeL = max(block_size,min((unsigned int)timeL.v,max_delay_length-FIRipol_N-1));
		int i_dtimeR = max(block_size,min((unsigned int)timeR.v,max_delay_length-FIRipol_N-1));
		
		int rpL = ((wpos-i_dtimeL+k)-FIRipol_N)&(max_delay_length-1);
		int rpR = ((wpos-i_dtimeR+k)-FIRipol_N)&(max_delay_length-1);
				
		int sincL = FIRipol_N*limit_range((int)(FIRipol_M*(float(i_dtimeL + 1)-timeL.v)),0,(int)FIRipol_M-1);
		int sincR = FIRipol_N*limit_range((int)(FIRipol_M*(float(i_dtimeR + 1)-timeR.v)),0,(int)FIRipol_M-1);		
		
#ifdef MAC
#else
		__m128 L,R;			
		L = _mm_mul_ps(_mm_load_ps(&SincTableF32[sincL]),_mm_loadu_ps(&buffer[0][rpL]));
		L = _mm_add_ps(L,_mm_mul_ps(_mm_load_ps(&SincTableF32[sincL+4]),_mm_loadu_ps(&buffer[0][rpL+4])));
		L = _mm_add_ps(L,_mm_mul_ps(_mm_load_ps(&SincTableF32[sincL+8]),_mm_loadu_ps(&buffer[0][rpL+8])));
		L = sum_ps_to_ss(L);
		R = _mm_mul_ps(_mm_load_ps(&SincTableF32[sincR]),_mm_loadu_ps(&buffer[1][rpR]));
		R = _mm_add_ps(R,_mm_mul_ps(_mm_load_ps(&SincTableF32[sincR+4]),_mm_loadu_ps(&buffer[1][rpR+4])));
		R = _mm_add_ps(R,_mm_mul_ps(_mm_load_ps(&SincTableF32[sincR+8]),_mm_loadu_ps(&buffer[1][rpR+8])));		
		R = sum_ps_to_ss(R);
		_mm_store_ss(&dataoutL[k],L);
		_mm_store_ss(&dataoutR[k],R);
#endif
	}
	
	softclip_block(dataoutL,block_size_quad);
	softclip_block(dataoutR,block_size_quad);
		
	lp.process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);
	hp.process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);

	pan.trixpan_blocks(datainL,datainR,wbL,wbR,block_size_quad);

	feedback.MAC_2_blocks_to(dataoutL,dataoutR,wbL,wbR,block_size_quad);
	crossfeed.MAC_2_blocks_to(dataoutL,dataoutR,wbR,wbL,block_size_quad);

	if(wpos+block_size >= max_delay_length)
	{
		for(k=0; k<block_size; k++)
		{						
			buffer[0][(wpos+k)&(max_delay_length-1)] = wbL[k];
			buffer[1][(wpos+k)&(max_delay_length-1)] = wbR[k];
		}
	}
	else
	{	
		copy_block(wbL,&buffer[0][wpos],block_size_quad);
		copy_block(wbR,&buffer[1][wpos],block_size_quad);
	}

	if(wpos == 0) 
	{
		for(k=0; k<FIRipol_N; k++)	
		{
			buffer[0][k+max_delay_length] = buffer[0][k];		// copy buffer so FIR-core doesn't have to wrap
			buffer[1][k+max_delay_length] = buffer[1][k];
		}
	}	

	wpos += block_size;
	wpos = wpos&(max_delay_length-1);
}

void dualdelay::suspend()
{ 
	init();	
}
/*
void dualdelay::init_ctrltypes()
{
	baseeffect::init_ctrltypes();

	fxdata->p[0].set_name("left");				fxdata->p[0].set_type(ct_envtime);
	fxdata->p[1].set_name("right");				fxdata->p[1].set_type(ct_envtime);
	fxdata->p[2].set_name("feedback");			fxdata->p[2].set_type(ct_amplitude);	
	fxdata->p[3].set_name("crossfeed");			fxdata->p[3].set_type(ct_amplitude);
	fxdata->p[4].set_name("low cut");			fxdata->p[4].set_type(ct_freq_audible);
	fxdata->p[5].set_name("high cut");			fxdata->p[5].set_type(ct_freq_audible);
	fxdata->p[6].set_name("rate");				fxdata->p[6].set_type(ct_lforate);
	fxdata->p[7].set_name("depth");				fxdata->p[7].set_type(ct_detuning);	
	fxdata->p[8].set_name("pan");			fxdata->p[8].set_type(ct_percent_bidirectional);	

	fxdata->p[10].set_name("mix");				fxdata->p[10].set_type(ct_percent);	

	fxdata->p[0].posy_offset = 5;
	fxdata->p[1].posy_offset = 5;
	
	fxdata->p[2].posy_offset = 7;
	fxdata->p[3].posy_offset = 7;
	fxdata->p[4].posy_offset = 7;
	fxdata->p[5].posy_offset = 7;
	
	fxdata->p[6].posy_offset = 9;
	fxdata->p[7].posy_offset = 9;

	fxdata->p[8].posy_offset = -15;

	fxdata->p[10].posy_offset = 7;
}*/
/*
void dualdelay::init_default_values()
{
	fxdata->p[0].val.f = -2.f;
	fxdata->p[1].val.f = -2.f;
	fxdata->p[2].val.f = 0.0f;
	fxdata->p[3].val.f = 0.0f;
	fxdata->p[4].val.f = -24.f;
	fxdata->p[5].val.f = 30.f;
	fxdata->p[6].val.f = -2.f;
	fxdata->p[7].val.f = 0.f;
	fxdata->p[8].val.f = 0.f;
	//fxdata->p[9].val.f = 0.f;
	fxdata->p[10].val.f = 1.f;
}*/