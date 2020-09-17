//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "filter_defs.h"
#include "resampling.h"
#include "unitconversion.h"
#include "tools.h"

extern float	SincTableF32[(FIRipol_M+1)*FIRipol_N];
extern float	SincOffsetF32[(FIRipol_M)*FIRipol_N];	

/* RING ringmodulation				*/

RING::RING(float *fp) : filter(fp), pre(2,false), post(4,false)
{	
	strcpy(filtername,"ringmod (sin)");
	parameter_count = 2;
	strcpy(ctrllabel[0], "frequency");				ctrlmode[0] = cm_frequency_audible;				
	strcpy(ctrllabel[1], "amount");					ctrlmode[1] = cm_percent;	
	amount.setBlockSize(block_size*2);

	strcpy(ctrlmode_desc[0], str_freqdef);	
	strcpy(ctrlmode_desc[1], str_percentdef);	
}

RING::~RING()
{	
}

void RING::init_params()
{
	if(!param) return;
	// preset values
	param[0] = 0.0f;
	param[1] = 1.0f;	
}


void RING::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch)
{
	amount.newValue(limit_range(param[1],0.f,1.f));	
	double omega = 0.5*440*powf(2,param[0])*pi2*samplerate_inv;
	qosc.set_rate(omega);
	
	const int bs2 = block_size<<1;	
	Align16 float OS[2][bs2];

	pre.process_block_U2(datainL,datainR,OS[0],OS[1],bs2);
		
	for(int k=0; k<bs2; k++)
	{			
		amount.process();
		qosc.process();
		float m = 2.f*(amount.v*qosc.r + (1-amount.v));
		OS[0][k] *= m;
		OS[1][k] *= m;
	}
	post.process_block_D2(OS[0],OS[1],bs2,dataoutL,dataoutR);	
}	

void RING::process(float *datain, float *dataout, float pitch)
{		
	amount.newValue(limit_range(param[1],0.f,1.f));	
	double omega = 0.5*440*powf(2,param[0])*pi2*samplerate_inv;
	qosc.set_rate(omega);
	
	const int bs2 = block_size<<1;	
	Align16 float OS[2][bs2];

	pre.process_block_U2(datain,datain,OS[0],OS[1],bs2);
		
	for(int k=0; k<bs2; k++)
	{			
		amount.process();
		qosc.process();
		OS[0][k] *= 2.f*(amount.v*qosc.r + (1-amount.v));
	}

	post.process_block_D2(OS[0],OS[1],bs2,dataout,0);	
}

/* RING ringmodulation				*/

FREQSHIFT::FREQSHIFT(float *fp, int *ip) : filter(fp,0,true,ip), fcL(6,true), fcR(6,true)
{	
	strcpy(filtername,"frequency shift");
	parameter_count = 1;
	strcpy(ctrllabel[0], "frequency");				ctrlmode[0] = cm_frequency_khz_bi;		

	strcpy(ctrlmode_desc[0],  "f,-20,0.01,20,0,kHz");		
			
	int ord = 6;
	bool steep = true;	
}

FREQSHIFT::~FREQSHIFT()
{	
}

void FREQSHIFT::init_params()
{
	if(!param) return;
	// preset values
	param[0] = 0.0f;	
	iparam[0] = 0;
}

void FREQSHIFT::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch)
{
	double omega;
	if(iparam[0])
		omega = (1000*param[0] + 440*pow((double)1.05946309435,(double)pitch))*pi2*samplerate_inv;
	else
		omega = 1000*param[0]*pi2*samplerate_inv;

	o1.set_rate(M_PI*0.5 - min(0.0, omega));
	o2.set_rate(M_PI*0.5 + max(0.0, omega));	

	Align16 float L_R[block_size],L_I[block_size],R_R[block_size],R_I[block_size];
		
	for(int k=0; k<block_size; k++)
	{			
		// quadrature oscillator 1
		o1.process();		
		L_R[k] = datainL[k]*o1.r;
		L_I[k] = datainL[k]*o1.i;
		R_R[k] = datainR[k]*o1.r;
		R_I[k] = datainR[k]*o1.i;
	}
				
	fcL.process_block(L_R,L_I,block_size);	
	fcR.process_block(R_R,R_I,block_size);	

	for(int k=0; k<block_size; k++)
	{		
		// quadrature oscillator 2		
		o2.process();
		L_R[k] *= o2.r;
		L_I[k] *= o2.i;
		R_R[k] *= o2.r;
		R_I[k] *= o2.i;
		// sum output
		dataoutL[k] = 2.f*(L_R[k] + L_I[k]);
		dataoutR[k] = 2.f*(R_R[k] + R_I[k]);
	}
}

void FREQSHIFT::process(float *datain, float *dataout, float pitch)
{	
	double omega;
	if(iparam[0])
		omega = (1000.0*param[0] + 440.0*pow((double)1.05946309435,(double)pitch))*pi2*samplerate_inv;
	else
		omega = 1000.0*param[0]*pi2*samplerate_inv;		

	o1.set_rate(M_PI*0.5 - min(0.0, omega));
	o2.set_rate(M_PI*0.5 + max(0.0, omega));	

	Align16 float L_R[block_size],L_I[block_size];
		
	for(int k=0; k<block_size; k++)
	{			
		// quadrature oscillator 1
		o1.process();		
		L_R[k] = datain[k]*o1.r;
		L_I[k] = datain[k]*o1.i;
	}
				
	fcL.process_block(L_R,L_I,block_size);	

	for(int k=0; k<block_size; k++)
	{		
		// quadrature oscillator 2		
		o2.process();
		L_R[k] *= o2.r;
		L_I[k] *= o2.i;
		// sum output
		dataout[k] = 2.f*(L_R[k] + L_I[k]);
	}
}	


int	FREQSHIFT::get_ip_count()
{
	return 1;
}
const char* FREQSHIFT::get_ip_label(int ip_id)
{
	return "mode";
}
int	FREQSHIFT::get_ip_entry_count(int ip_id)
{
	if(ip_id == 0) return 2;
	return 0;
}

const char* FREQSHIFT::get_ip_entry_label(int ip_id, int c_id)
{
	if (c_id == 0) 
		return "abs";
	return "rel";
}


/* PMOD phase modulation				*/

PMOD::PMOD(float *fp) : filter(fp), pre(6,true), post(6,true)
{	
	strcpy(filtername,"phasemod (sin)");
	parameter_count = 2;
	strcpy(ctrllabel[0], "transpose");				ctrlmode[0] = cm_mod_pitch;
	strcpy(ctrllabel[1], "amount");					ctrlmode[1] = cm_decibel;

	strcpy(ctrlmode_desc[0], "f,-96,0.01,96,0,");	
	strcpy(ctrlmode_desc[1], str_dbmoddef);	
	
	phase[0] = 0;
	phase[1] = 0;

	if(fp)
	{		
	}	
}

PMOD::~PMOD()
{
	
}

void PMOD::init_params()
{
	if(!param) return;
	// preset values
	param[0] = 0.0f;
	param[1] = 0.0f;
}

void PMOD::process(float *datain, float *dataout, float pitch)
{
	omega.newValue(0.5*440*note_to_pitch(pitch + param[0])*pi2*samplerate_inv);
	//amp.newValue(3.1415 * dB_to_linear(param[1]));

	pregain.set_target(3.1415 * dB_to_linear(param[1]));
	//postgain.set_target(dB_to_linear(max(0,-param[1])));

	const int bs2 = block_size<<1;
	Align16 float OS[2][bs2];

	pregain.multiply_block_to(datain,OS[0],block_size_quad);
	pre.process_block_U2(OS[0],OS[0],OS[0],OS[1],bs2);

	for(int k=0; k<bs2; k++)
	{	
		omega.process();		
		phase[0] += omega.v;
		phase[1] += omega.v;
		OS[0][k] = 0.5*(sin((float)phase[0] + OS[0][k]) -  sin(phase[0]));		
	}
	post.process_block_D2(OS[0],OS[0],bs2,dataout,0);
	//postgain.multiply_block(dataout,block_size_quad);
}
void PMOD::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch)
{
	omega.newValue(0.5*440*note_to_pitch(pitch + param[0])*pi2*samplerate_inv);
	pregain.set_target(3.1415 * dB_to_linear(param[1]));
	//postgain.set_target(dB_to_linear(max(0,-param[1])));

	const int bs2 = block_size<<1;
	Align16 float OS[2][bs2];

	pregain.multiply_2_blocks_to(datainL,datainR,OS[0],OS[1],block_size_quad);
	pre.process_block_U2(OS[0],OS[1],OS[0],OS[1],bs2);

	for(int k=0; k<bs2; k++)
	{	
		omega.process();
		phase[0] += omega.v;
		phase[1] += omega.v;
		OS[0][k] = 0.5*(sin((float)phase[0] + OS[0][k]) -  sin(phase[0]));
		OS[1][k] = 0.5*(sin((float)phase[1] + OS[1][k]) -  sin(phase[1]));
	}

	post.process_block_D2(OS[0],OS[1],bs2,dataoutL,dataoutR);
}

/* rotary_speaker	*/

enum rsparams
{
	rsp_rate = 0,	
	rsp_doppler,
	rsp_amp,
	rsp_numparams
};

rotary_speaker::rotary_speaker(float *fp, int *ip) : filter(fp)
{	
	strcpy(filtername,"rotary speaker");
	parameter_count = 3;
	strcpy(ctrllabel[0], "horn rate");				ctrlmode[0] = cm_lforate;
	strcpy(ctrllabel[1], "doppler");				ctrlmode[1] = cm_percent;
	strcpy(ctrllabel[2], "ampmod");					ctrlmode[2] = cm_percent;

	strcpy(ctrlmode_desc[0], str_lfofreqdef);	
	strcpy(ctrlmode_desc[1], str_percentdef);	
	strcpy(ctrlmode_desc[2], str_percentdef);
	
	if(fp)
	{		
	}	
}

rotary_speaker::~rotary_speaker()
{
	
}

void rotary_speaker::init_params()
{
	if(!param) return;
	// preset values
	param[0] = 1.0f;
	param[1] = 0.25f;
	param[2] = 0.5f;
}

void rotary_speaker::init()
{
	memset(buffer,0,max_delay_length*sizeof(float));	

	xover.suspend();
	lowbass.suspend();
	xover.coeff_LP2B(xover.calc_omega(0.862496f),0.707);
	lowbass.coeff_LP2B(xover.calc_omega(-1.14f),0.707);
	wpos = 0;
}

void rotary_speaker::suspend()
{
	memset(buffer,0,max_delay_length*sizeof(float));
	xover.suspend();
	lowbass.suspend();
	wpos = 0;	
}

void rotary_speaker::process(float *datain, float *dataout, float pitch)
{
}

void rotary_speaker::process_stereo(float *datainL, float* datainR, float *dataL, float* dataR, float pitch)
{	
	const int mindelay = 64;	//33?
	
	lfo.set_rate(2*M_PI*powf(2,param[rsp_rate])*samplerate_inv*block_size);
	lf_lfo.set_rate(0.7*2*M_PI*powf(2,param[rsp_rate])*samplerate_inv);
			
	float precalc0 = (-2 - (float)lfo.i);	
	float precalc1 = (-1 - (float)lfo.r);	
	float precalc2 = (+1 - (float)lfo.r);
	float lenL = sqrt(precalc0*precalc0 + precalc1*precalc1);
	float lenR = sqrt(precalc0*precalc0 + precalc2*precalc2);

	float delay = samplerate * 0.0018f * param[rsp_doppler];
	dL.newValue(delay*lenL);
	dR.newValue(delay*lenR);
	float dotp_L = (precalc1*(float)lfo.r + precalc0*(float)lfo.i)/lenL;
	float dotp_R = (precalc2*(float)lfo.r + precalc0*(float)lfo.i)/lenR;

	float a = param[rsp_amp]*0.6f;
	hornamp[0].newValue((1.f-a) + a*dotp_L);
	hornamp[1].newValue((1.f-a) + a*dotp_R);

	lfo.process();
	
	Align16 float upper[block_size];
	Align16 float lower[block_size];
	Align16 float lower_sub[block_size];
	Align16 float tbufferL[block_size];
	Align16 float tbufferR[block_size];		

	int k;
	
	for(k=0; k<block_size; k++)
	{
		//float input = (float)tanh_fast(0.5f*dataL[k]+dataR[k]*drive.v);
		float input = 0.5f*(datainL[k]+datainR[k]);
		upper[k] = input;
		lower[k] = input;		
		//drive.process();
	}
		
	xover.process_block(lower);
	//xover->process(lower,0);
	
	for(k=0; k<block_size; k++)
	{	
		// feed delay input
		int wp = (wpos+k)&(max_delay_length-1);
		lower_sub[k] = lower[k];
		upper[k] -= lower[k];
		buffer[wp] = upper[k];				

		int i_dtimeL = max(block_size,min((unsigned int)dL.v,max_delay_length-FIRipol_N-1));
		int i_dtimeR = max(block_size,min((unsigned int)dR.v,max_delay_length-FIRipol_N-1));
		
		int rpL = (wpos-i_dtimeL+k);
		int rpR = (wpos-i_dtimeR+k);
				
		int sincL = FIRipol_N*limit_range((int)(FIRipol_M*(float(i_dtimeL + 1)-dL.v)),0,(int)FIRipol_M-1);
		int sincR = FIRipol_N*limit_range((int)(FIRipol_M*(float(i_dtimeR + 1)-dR.v)),0,(int)FIRipol_M-1);		
				
		// get delay output
		tbufferL[k] = 0;
		tbufferR[k] = 0;
		for(int i=0; i<FIRipol_N; i++)
		{
			tbufferL[k] += buffer[(rpL-i)&(max_delay_length-1)]*SincTableF32[sincL + FIRipol_N - i];
			tbufferR[k] += buffer[(rpR-i)&(max_delay_length-1)]*SincTableF32[sincR + FIRipol_N - i];			
		}	
		dL.process();
		dR.process();
	}

	lowbass.process_block(lower_sub);

	for(k=0; k<block_size; k++)
	{
		lower[k] -= lower_sub[k];		

		float bass = lower_sub[k] + lower[k]*(lf_lfo.r*0.6f+0.3f);
		
		dataL[k] = hornamp[0].v*tbufferL[k]+bass;
		dataR[k] = hornamp[1].v*tbufferR[k]+bass;
		lf_lfo.process();
		hornamp[0].process();
		hornamp[1].process();
	}
	
	wpos += block_size;
	wpos = wpos&(max_delay_length-1);
}

/* phaser	*/

enum phaseparam
{
	pp_base=0,	
	pp_feedback,
	pp_q,
	pp_lforate,
	pp_lfodepth,
	pp_stereo,
	pp_mix,	
	pp_nparams,
};

phaser::phaser(float *fp, int *ip) : filter(fp)
{	
	strcpy(filtername,"phaser");
	parameter_count = 6;
	strcpy(ctrllabel[0], "base freq");				ctrlmode[0] = cm_percent;
	strcpy(ctrllabel[1], "feedback");				ctrlmode[1] = cm_percent;
	strcpy(ctrllabel[2], "Q");						ctrlmode[2] = cm_percent;
	strcpy(ctrllabel[3], "LFO rate");				ctrlmode[3] = cm_percent;
	strcpy(ctrllabel[4], "LFO depth");				ctrlmode[4] = cm_percent;
	strcpy(ctrllabel[5], "width");					ctrlmode[5] = cm_percent;

	strcpy(ctrlmode_desc[0], str_percentbpdef);	
	strcpy(ctrlmode_desc[1], str_percentbpdef);	
	strcpy(ctrlmode_desc[2], str_percentbpdef);
	strcpy(ctrlmode_desc[3], str_lfofreqdef);
	strcpy(ctrlmode_desc[4], str_percentdef);
	strcpy(ctrlmode_desc[5], str_percentdef);	
	
	if(fp)
	{		
	}	

	for(int i=0; i<n_bq_units; i++)
	{
		biquad[i] = (biquadunit*) _mm_malloc(sizeof(biquadunit),16);
		memset(biquad[i],0,sizeof(biquadunit));
		new(biquad[i]) biquadunit();		
	}	
	feedback.setBlockSize(block_size*slowrate);
	bi = 0;	
}

phaser::~phaser()
{
	for(int i=0; i<n_bq_units; i++)	_mm_free(biquad[i]);
}

void phaser::init_params()
{
	if(!param) return;
	// preset values
	param[0] = 0.0f;
	param[1] = -0.625f;
	param[2] = 0.284f;
	param[3] = -1.3f;
	param[4] = 0.81f;
	param[5] = 0.75f;
}

void phaser::init()
{
	lfophase = 0.25f;
	//setvars(true);	
	for(int i=0; i<n_bq_units; i++) 
	{
		//notch[i]->coeff_LP(1.0,1.0);
		biquad[i]->suspend();						
	}
	clear_block(L,block_size_quad);
	clear_block(R,block_size_quad);	
	bi = 0;
	dL = 0;
	dR = 0;
	bi = 0;
#if USE_SSE2
	ddL = _mm_setzero_pd();
	ddR = _mm_setzero_pd();
#endif
}

float basefreq[4] = {1.5/12, 19.5/12, 35/12, 50/12};
float basespan[4] = {2.0, 1.5, 1.0, 0.5};

void phaser::suspend()
{
	init();		
}

void phaser::setvars()
{
	const double octdiv = 1.0/12.0;
	
	double rate = envelope_rate_linear(-param[pp_lforate]);
	lfophase += (float)slowrate*rate;
	if(lfophase>1) lfophase -= 1;	
	float lfophaseR = lfophase + 0.5 * param[pp_stereo];
	if(lfophaseR>1) lfophaseR -= 1;	
	double lfoout = 1.f - fabs(2.0-4.0*lfophase);
	double lfooutR = 1.f - fabs(2.0-4.0*lfophaseR);

	for(int i=0; i<n_bq; i++)
	{
		double omega = biquad[0]->calc_omega(2.0 * param[pp_base] + basefreq[i] + basespan[i] * lfoout * param[pp_lfodepth]);		
		biquad[i]->coeff_APF(omega,1.0 + 0.8* param[pp_q]);
		omega = biquad[0]->calc_omega(2.0 * param[pp_base] + basefreq[i] + basespan[i] * lfooutR * param[pp_lfodepth]);		
		biquad[i+n_bq]->coeff_APF(omega,1.0 + 0.8* param[pp_q]);
	}

	feedback.newValue(0.95f * param[pp_feedback]);	
}

void phaser::process_stereo(float *datainL, float* datainR, float *dataL, float* dataR, float pitch)
{	
	if(bi==0) setvars();	
	bi = (bi+1)&slowrate_m1;
	for(int i=0; i<block_size; i++)
	{
#if USE_SSE2
		feedback.process();
		__m128d fb,Lin,Rin;
		fb = _mm_cvtss_sd(fb,_mm_load_ss(&feedback.v));		
		ddL = _mm_add_sd(_mm_mul_sd(ddL,fb),_mm_cvtss_sd(Lin,_mm_load_ss(datainL + i)));
		ddR = _mm_add_sd(_mm_mul_sd(ddR,fb),_mm_cvtss_sd(Rin,_mm_load_ss(datainR + i)));		
		ddL = hardclip8_sd(ddL);
		ddR = hardclip8_sd(ddR);		
		ddL = biquad[0]->process_sample_sd(ddL);
		ddL = biquad[1]->process_sample_sd(ddL);
		ddL = biquad[2]->process_sample_sd(ddL);
		ddL = biquad[3]->process_sample_sd(ddL);
		ddR = biquad[4]->process_sample_sd(ddR);
		ddR = biquad[5]->process_sample_sd(ddR);
		ddR = biquad[6]->process_sample_sd(ddR);
		ddR = biquad[7]->process_sample_sd(ddR);
		__m128 tL,tR;
		tL = _mm_cvtsd_ss(tL,ddL);
		tR = _mm_cvtsd_ss(tR,ddR);
		_mm_store_ss(dataL+i,tL);
		_mm_store_ss(dataR+i,tR);		
#else			
		feedback.process();
		dL = datainL[i] + dL*feedback.v;
		dR = datainR[i] + dR*feedback.v;
		dL = limit_range(dL,-8.f,8.f);
		dR = limit_range(dR,-8.f,8.f);
		dL = biquad[0]->process_sample(dL);
		dL = biquad[1]->process_sample(dL);
		dL = biquad[2]->process_sample(dL);
		dL = biquad[3]->process_sample(dL);
		dR = biquad[4]->process_sample(dR);
		dR = biquad[5]->process_sample(dR);
		dR = biquad[6]->process_sample(dR);
		dR = biquad[7]->process_sample(dR);
		dataL[i] = dL;
		dataR[i] = dR;				
#endif
	}
}