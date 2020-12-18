//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#define  _USE_MATH_DEFINES
#include "filter_defs.h"
#include "unitconversion.h"
#include "resampling.h"
#include "tools.h"
#include <assert.h>

#include <cmath>
#include <cstring>
#include <math.h>
#include <algorithm>
using std::min;
using std::max;

// morph eq

morphEQ::morphEQ(float *fp,void *loader, int *ip) : filter(fp,loader,true,ip)
{	
	strcpy(filtername,"morphEQ");
	parameter_count = 4;	
	strcpy(ctrllabel[0], "morph");					ctrlmode[0] = cm_percent;	
	strcpy(ctrllabel[1], "freq offset");			ctrlmode[1] = cm_mod_freq;	
	strcpy(ctrllabel[2], "gain offset");			ctrlmode[2] = cm_mod_decibel;
	strcpy(ctrllabel[3], "BW offset");				ctrlmode[3] = cm_mod_percent;	
			
	strcpy(ctrlmode_desc[0], str_percentdef);	
	strcpy(ctrlmode_desc[1], str_freqmoddef);
	strcpy(ctrlmode_desc[2], str_dbbpdef);	
	strcpy(ctrlmode_desc[3], str_percentmoddef);	

	gain.set_blocksize(block_size);

	if (loader && fp)
	{
		this->loader = loader;
		morphEQ_loader *mload = (morphEQ_loader*)loader;
		int src = max(ip[0],0);
		int dst = max(ip[1],0);
		if (src >= mload->n_loaded) src = 0;
		if (dst >= mload->n_loaded) dst = 0;

		memcpy(&snap[0],&mload->snapshot[src],sizeof(meq_snapshot));
		memcpy(&snap[1],&mload->snapshot[dst],sizeof(meq_snapshot));		
		for(int i=0; i<8; i++)
			b_active[i] = snap[0].bands[i].active || snap[1].bands[i].active;
	}
	gaintarget = 1.f;
	lastparam[0] = -165464686;	// se till att cal_coeffs altlid utförs första gången
}	

void morphEQ::init_params()
{
	assert(param);
	if(!param) return;
	// preset values
	param[0] = 0.0f;
	param[1] = 0.0f;
	param[2] = 0.0f;
	param[3] = 0.0f;
	iparam[0] = 0;
	iparam[1] = 0;
}


int	morphEQ::get_ip_count()
{
	return 2;
}

const char* morphEQ::get_ip_label(int ip_id)
{
	return (ip_id==1)?"snapshot 2":"snapshot 1";
}

int	morphEQ::get_ip_entry_count(int ip_id)
{
	if((ip_id&0xfe) != 0) return 0;

	morphEQ_loader *mload = (morphEQ_loader*)loader;
	return mload->n_loaded;
}

const char* morphEQ::get_ip_entry_label(int ip_id, int c_id)
{
	if((ip_id&0xfe) != 0) return 0;
	if(c_id > this->get_ip_entry_count(ip_id)) return 0;
	
	morphEQ_loader *mload = (morphEQ_loader*)loader;		
	return mload->snapshot[c_id].name;	
}

void morphEQ::calc_coeffs()
{
	assert(param);
	if((lastparam[0] != param[0])||(lastparam[1] != param[1])||(lastparam[2] != param[2])||(lastparam[3] != param[3]))
	{

		float morph = clamp01(param[0]);
		float morph_m1 = 1 - morph;	
		const float Q = 1/sqrt(2.0);

		gaintarget = powf(10,0.05*(snap[0].gain*morph_m1 + snap[1].gain*morph));		

		if (b_active[0]) b[0].coeff_HP(biquadunit::calc_omega(snap[0].bands[0].freq * morph_m1 + snap[1].bands[0].freq*morph + param[1]),Q);		
		if (b_active[7]) b[7].coeff_LP2B(biquadunit::calc_omega(snap[0].bands[7].freq * morph_m1 + snap[1].bands[7].freq*morph + param[1]),Q);	

		for(int i=1; i<7; i++)
		{		
			if (b_active[i]) b[i].coeff_peakEQ(biquadunit::calc_omega(snap[0].bands[i].freq * morph_m1 + snap[1].bands[i].freq*morph + param[1]),
				snap[0].bands[i].BW * morph_m1 + snap[1].bands[i].BW*morph + param[3],
				snap[0].bands[i].gain * morph_m1 + snap[1].bands[i].gain*morph + param[2]);		
		}
		memcpy(lastparam,param,sizeof(lastparam));
	}
	gain.set_target(gaintarget);
}

float morphEQ::get_freq_graph(float f)
{
	float y = 1;
	for(int k=0; k<8; k++)
		if (b_active[k]) y *= b[k].plot_magnitude(f);
		
	return y*gain.get_target();
}

void morphEQ::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch)
{
	calc_coeffs();	
	gain.multiply_2_blocks_to(datainL,datainR,dataoutL,dataoutR,block_size_quad);
	if (b_active[0]) b[0].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);
	if (b_active[1]) b[1].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);
	if (b_active[2]) b[2].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);
	if (b_active[3]) b[3].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);
	if (b_active[4]) b[4].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);
	if (b_active[5]) b[5].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);
	if (b_active[6]) b[6].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);
	if (b_active[7]) b[7].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);	
}

void morphEQ::process(float *datain, float *dataout, float pitch)
{		
	calc_coeffs();	
	
	gain.multiply_block_to(datain,dataout,block_size_quad);
	if (b_active[0]) b[0].process_block(dataout);
	if (b_active[1]) b[1].process_block(dataout);
	if (b_active[2]) b[2].process_block(dataout);
	if (b_active[3]) b[3].process_block(dataout);
	if (b_active[4]) b[4].process_block(dataout);
	if (b_active[5]) b[5].process_block(dataout);
	if (b_active[6]) b[6].process_block(dataout);
	if (b_active[7]) b[7].process_block(dataout);	
}

// EQ 2-band parametric
EQ2BP_A::EQ2BP_A(float *fp, int *ip) : filter(fp,0,true,ip)
{	
	strcpy(filtername,"parametric type A");
	parameter_count = 6;
	strcpy(ctrllabel[0], "gain");				ctrlmode[0] = cm_mod_decibel;
	strcpy(ctrllabel[1], "freq");				ctrlmode[1] = cm_frequency_audible;	
	strcpy(ctrllabel[2], "BW");					ctrlmode[2] = cm_eq_bandwidth;	
	strcpy(ctrllabel[3], "gain");				ctrlmode[3] = cm_mod_decibel;
	strcpy(ctrllabel[4], "freq");				ctrlmode[4] = cm_frequency_audible;	
	strcpy(ctrllabel[5], "BW");					ctrlmode[5] = cm_eq_bandwidth;	

	strcpy(ctrlmode_desc[0], str_dbbpdef);	
	strcpy(ctrlmode_desc[1], str_freqdef);
	strcpy(ctrlmode_desc[2], str_bwdef);	
	strcpy(ctrlmode_desc[3], str_dbbpdef);
	strcpy(ctrlmode_desc[4], str_freqdef);	
	strcpy(ctrlmode_desc[5], str_bwdef);

	lastparam[0] = -1654646816;	// se till att cal_coeffs altlid utförs första gången
}

void EQ2BP_A::init_params()
{
	assert(param);
	if(!param) return;
	// preset values
	param[0] = 0.0f;
	param[1] = -1.0f;
	param[2] = 1.0f;
	param[3] = 0.0f;
	param[4] = 2.0f;
	param[5] = 1.0f;
	if(!iparam) return;
	iparam[0] = 0;
	iparam[1] = 0;
}

float calc_GB_type_B(bool b, float x)
{
	if (b && (fabs(x) > 6))
	{
		if (x>0)
			x = 3;
		else
			x = -3;
	}	
	else
		x *= 0.5;
	
	return dB_to_linear(x);
}

void EQ2BP_A::calc_coeffs()
{
	assert(param);
	if((lastparam[0] != param[0])||(lastparam[1] != param[1])||(lastparam[2] != param[2])||(lastparam[3] != param[3])||(lastparam[4] != param[4])||(lastparam[5] != param[5]))
	{		
		parametric[0].coeff_orfanidisEQ(biquadunit::calc_omega(param[1]),param[2],dB_to_linear(param[0]), calc_GB_type_B(iparam[0],param[0]),1);
		parametric[1].coeff_orfanidisEQ(biquadunit::calc_omega(param[4]),param[5],dB_to_linear(param[3]), calc_GB_type_B(iparam[1],param[3]),1);	
		memcpy(lastparam,param,sizeof(lastparam));
	}
}

float EQ2BP_A::get_freq_graph(float f)
{
	return parametric[0].plot_magnitude(f)*parametric[1].plot_magnitude(f);
}

void EQ2BP_A::process(float *datain, float *dataout, float pitch)
{
	calc_coeffs();
	parametric[0].process_block_to(datain,dataout);	
	parametric[1].process_block(dataout);
}
void EQ2BP_A::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch)
{
	calc_coeffs();
	parametric[0].process_block_to(datainL,datainR,dataoutL,dataoutR);	
	parametric[1].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);
}

int		EQ2BP_A::get_ip_count(){ return 2; }
const char*	EQ2BP_A::get_ip_label(int ip_id){ return "type"; }
int		EQ2BP_A::get_ip_entry_count(int ip_id){ return 2; }
const char*	EQ2BP_A::get_ip_entry_label(int ip_id, int c_id)
{
	if (c_id == 1) return "B";
	return "A";
}


/*// EQ 2-band parametric
EQ2BP_B::EQ2BP_B(float *fp) : filter(fp)
{	
	strcpy(filtername,"parametric type B");
	parameter_count = 6;
	strcpy(ctrllabel[0], "gain");				ctrlmode[0] = cm_mod_decibel;
	strcpy(ctrllabel[1], "freq");				ctrlmode[1] = cm_frequency_audible;	
	strcpy(ctrllabel[2], "BW");					ctrlmode[2] = cm_eq_bandwidth;	
	strcpy(ctrllabel[3], "gain");				ctrlmode[3] = cm_mod_decibel;
	strcpy(ctrllabel[4], "freq");				ctrlmode[4] = cm_frequency_audible;	
	strcpy(ctrllabel[5], "BW");					ctrlmode[5] = cm_eq_bandwidth;	

	strcpy(ctrlmode_desc[0], str_dbmoddef);	
	strcpy(ctrlmode_desc[1], str_freqdef);
	strcpy(ctrlmode_desc[2], str_bwdef);	
	strcpy(ctrlmode_desc[3], str_dbmoddef);
	strcpy(ctrlmode_desc[4], str_freqdef);	
	strcpy(ctrlmode_desc[5], str_bwdef);
	
	lastparam[0] = -16546468416816816;
}

void EQ2BP_B::init_params()
{
	assert(param);
	if(!param) return;
	// preset values
	param[0] = 0.0f;
	param[1] = -1.0f;
	param[2] = 2.0f;
	param[3] = 0.0f;
	param[4] = 2.0f;
	param[5] = 2.0f;
}



void EQ2BP_B::calc_coeffs()
{
	assert(param);
	if((lastparam[0] != param[0])||(lastparam[1] != param[1])||(lastparam[2] != param[2])||(lastparam[3] != param[3])||(lastparam[4] != param[4])||(lastparam[5] != param[5]))
	{
		parametric[0].coeff_orfanidisEQ(biquadunit::calc_omega(param[1]),param[2],dB_to_linear(param[0]), calc_GB_type_B(param[0]),1);
		parametric[1].coeff_orfanidisEQ(biquadunit::calc_omega(param[4]),param[5],dB_to_linear(param[3]), calc_GB_type_B(param[3]),1);		
		memcpy(lastparam,param,sizeof(lastparam));
	}
}

float EQ2BP_B::get_freq_graph(float f)
{
	return parametric[0].plot_magnitude(f)*parametric[1].plot_magnitude(f);
}

void EQ2BP_B::process(float *data, float pitch)
{	
	calc_coeffs();
	parametric[0].process_block(data);	
	parametric[1].process_block(data);
}*/
	
// EQ 6-band graphic
EQ6B::EQ6B(float *fp) : filter(fp)
{	
	strcpy(filtername,"graphic EQ");
	parameter_count = 6;
	strcpy(ctrllabel[0], "100");				ctrlmode[0] = cm_mod_decibel;
	strcpy(ctrllabel[1], "250");				ctrlmode[1] = cm_mod_decibel;	
	strcpy(ctrllabel[2], "630");				ctrlmode[2] = cm_mod_decibel;	
	strcpy(ctrllabel[3], "1.6k");				ctrlmode[3] = cm_mod_decibel;
	strcpy(ctrllabel[4], "5k");					ctrlmode[4] = cm_mod_decibel;	
	strcpy(ctrllabel[5], "12k");				ctrlmode[5] = cm_mod_decibel;	

	strcpy(ctrlmode_desc[0], str_dbbpdef);	
	strcpy(ctrlmode_desc[1], str_dbbpdef);
	strcpy(ctrlmode_desc[2], str_dbbpdef);	
	strcpy(ctrlmode_desc[3], str_dbbpdef);
	strcpy(ctrlmode_desc[4], str_dbbpdef);	
	strcpy(ctrlmode_desc[5], str_dbbpdef);

	lastparam[0] = -1654646816;
}

void EQ6B::init_params()
{
	assert(param);
	if(!param) return;
	// preset values
	param[0] = 0.0f;
	param[1] = 0.0f;
	param[2] = 0.0f;
	param[3] = 0.0f;
	param[4] = 0.0f;
	param[5] = 0.0f;
}

void EQ6B::calc_coeffs()
{
	assert(param);
	
	if((lastparam[0] != param[0])||(lastparam[1] != param[1])||(lastparam[2] != param[2])||(lastparam[3] != param[3])||(lastparam[4] != param[4])||(lastparam[5] != param[5]))
	{
		double a = pi2*samplerate_inv;		
		const double bw = 3;

		parametric[0].coeff_peakEQ(100*a,bw,param[0]);
		parametric[1].coeff_peakEQ(250*a,bw,param[1]);
		parametric[2].coeff_peakEQ(630*a,bw,param[2]);
		parametric[3].coeff_peakEQ(1600*a,bw,param[3]);
		parametric[4].coeff_peakEQ(5000*a,bw,param[4]);
		parametric[5].coeff_peakEQ(12000*a,bw,param[5]);
		memcpy(lastparam,param,sizeof(lastparam));
	}
}

void EQ6B::process(float *datain, float *dataout, float pitch)
{
	calc_coeffs();
	parametric[0].process_block_to(datain,dataout);	
	parametric[1].process_block(dataout);
	parametric[2].process_block(dataout);
	parametric[3].process_block(dataout);
	parametric[4].process_block(dataout);
	parametric[5].process_block(dataout);
}
void EQ6B::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch)
{
	calc_coeffs();
	parametric[0].process_block_to(datainL,datainR,dataoutL,dataoutR);	
	parametric[1].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);
	parametric[2].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);	
	parametric[3].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);
	parametric[4].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);	
	parametric[5].process_block_to(dataoutL,dataoutR,dataoutL,dataoutR);
}

float EQ6B::get_freq_graph(float f)
{
	return parametric[0].plot_magnitude(f)*parametric[1].plot_magnitude(f)*parametric[2].plot_magnitude(f)*parametric[3].plot_magnitude(f)*parametric[4].plot_magnitude(f)*parametric[5].plot_magnitude(f);
}
	
// LP2/HP2 morph
LP2HP2_morph::LP2HP2_morph(float *fp) : filter(fp)
{	
	strcpy(filtername,"LP2/HP2 morph");
	parameter_count = 3;
	strcpy(ctrllabel[0], "cutoff");			ctrlmode[0] = cm_frequency_audible;
	strcpy(ctrllabel[1], "resonance");		ctrlmode[1] = cm_percent;	
	strcpy(ctrllabel[2], "HP morph");		ctrlmode[2] = cm_percent;	
}

void LP2HP2_morph::init_params()
{
	assert(param);
	if(!param) return;
	// preset values
	param[0] = 5.5f;
	param[1] = 0.0f;
	param[2] = 0.0f;
}

void LP2HP2_morph::calc_coeffs()
{
	assert(param);	
	double omega = pi2 * min(0.499, 440.0*powf(2.f,param[0])*samplerate_inv);
	double q = M_SQRT1_2/(1.0-limit_range(param[1],0.f,0.999f));

	f.coeff_LPHPmorph(omega,q,param[2]);	
}

float LP2HP2_morph::get_freq_graph(float f)
{
	return this->f.plot_magnitude(f);
}

void LP2HP2_morph::process(float *data, float pitch)
{		
	calc_coeffs();
	f.process_block(data);	
}
	
// COMB2		IIR-based comb filter
COMB2::COMB2(float *fp) : filter(fp)
{	
	strcpy(filtername,"COMB (IIR)");
	parameter_count = 3;
	strcpy(ctrllabel[0], "frequency");		ctrlmode[0] = cm_frequency_audible;
	strcpy(ctrllabel[1], "resonance");		ctrlmode[1] = cm_percent;	
	strcpy(ctrllabel[2], "feedback");		ctrlmode[2] = cm_percent_bipolar;	
	feedback = 0;
}

void COMB2::init_params()
{
	assert(param);
	if(!param) return;
	// preset values
	param[0] = 0.0f;
	param[1] = 0.0f;
	param[2] = 0.0f;
}

void COMB2::process(float *data, float pitch)
{	
	assert(param);	
	
	double omega = pi2 * min(0.499, 440.0*powf(2,param[0])*samplerate_inv);
	double q = 1.0 / (1.02 - clamp01(param[1]));
	fbval.newValue(limit_range(param[2],-0.99f,0.99f));

	f.coeff_APF(omega,q);	
	int k;
	for(k=0; k<block_size; k++)
	{
		feedback = f.process_sample(data[k] + feedback);		
		data[k] += feedback;
		feedback *= fbval.v;
		fbval.process();
	}
	// feedback ska bort egentligen
}

