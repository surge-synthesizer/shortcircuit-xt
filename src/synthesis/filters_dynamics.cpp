//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "filter_defs.h"
#include "resampling.h"
#include "tools.h"
#include "parameterids.h"

#include <math.h>
#include <cmath>
#include <cstring>
#include <algorithm>
using std::max;
using std::min;

/*	limiter		*/

limiter::limiter(float *fp, int *ip) : filter(fp,0,true,ip)
{	
	strcpy(filtername,"limiter");
	parameter_count = 4;
	strcpy(ctrllabel[0], "input gain");				ctrlmode[0] = cm_mod_decibel;
	strcpy(ctrllabel[1], "attack");					ctrlmode[1] = cm_time1s;	
	strcpy(ctrllabel[2], "release");				ctrlmode[2] = cm_time1s;		
	strcpy(ctrllabel[3], "initial");				ctrlmode[3] = cm_percent;
	strcpy(ctrllabel[4], "SC freq");				ctrlmode[4] = cm_frequency_audible;
	strcpy(ctrllabel[5], "SC gain");				ctrlmode[5] = cm_decibel;
	strcpy(ctrlmode_desc[0], "f,0,0.1,96,0,dB");
	//strcpy(ctrlmode_desc[1], "f,-10,0.025,-2.7,4,s");
	//strcpy(ctrlmode_desc[2], "f,-8,0.025,0,4,s");		
	strcpy(ctrlmode_desc[1], str_percentbpdef);
	strcpy(ctrlmode_desc[2], str_percentbpdef);
	strcpy(ctrlmode_desc[3], str_percentdef);
	strcpy(ctrlmode_desc[4], str_freqdef);
	strcpy(ctrlmode_desc[5], str_dbmoddef);
	lastparam[1] = -1000.f;
	lastparam[2] = -1000.f;
}

void limiter::init_params()
{
	if(!param) return;
	// preset values
	param[0] = 0.0f;
	param[1] = 0.f;
	param[2] = 0.f;
	param[3] = 1.0f;
	param[4] = 0.0f;
	param[5] = 0.0f;
	iparam[0] = 1;
}

void limiter::suspend()
{
	init();
}
void limiter::init()
{
	float t = (1.f-param[3]);
	ef = t*t * db_to_linear(param[0]);
	if(iparam[1]) ef = ef*ef;
}

int	limiter::get_ip_count()
{
	return 2;
}
const char*	limiter::get_ip_label(int ip_id)
{
	return "type";
}
int	limiter::get_ip_entry_count(int ip_id)
{
	return 2;
}
const char*	limiter::get_ip_entry_label(int ip_id, int c_id)
{
	if(ip_id == 0)
	{
		if(c_id == 1) return "FB";
		else return "FF";
	}
	if(c_id == 1) return "RMS";
	else return "Peak";
}


void limiter::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch)
{		
	pregain.set_target(db_to_linear(param[0]) * 3.f);
	
	float am = 1.0f - 0.99f * param[1];
	float rm = 1.0f - 0.99f * param[2];
	at = 1.f - 0.001f*am*am;
	re = 1.f - 0.001f * rm*rm;

	// TODO sidechain.. disabled atm (behöver listig strategi för feedbaackläget.. kan vara bättre med separat filter för det)
	//Align16 float sidechainL[block_size];
	//Align16 float sidechainR[block_size];
	//bq.coeff_peakEQ(bq.calc_omega(param[4]),1,param[5]);	
	
	/*if(lastparam[1] != param[1])
	{
		at = 2-cos(pi*note_to_pitch(-12*param[1])*samplerate_inv),
		at -= sqrt(at*at - 1.f);		
		at = min(0.999999f,at);
		lastparam[1] = param[1];
	}	
	if(lastparam[2] != param[2])
	{
		re = 2-cos(pi*note_to_pitch(-12*param[2])*samplerate_inv);
		re -= sqrt(re*re - 1.f);
		lastparam[2] = param[2];
	}*/
		
	if(iparam[0])
	{
		// feedback design		
		__m128 postgainv = _mm_rsqrt_ss(pregain.target);		
		postgain.set_target(postgainv);
		pregain.multiply_2_blocks_to(datainL,datainR,dataoutL,dataoutR,block_size_quad);			

		for(unsigned int k=0; k<block_size; k++)
		{				
			__m128 xL = _mm_load_ss(dataoutL+k);
			__m128 xR = _mm_load_ss(dataoutR+k);						
			__m128 mef = _mm_max_ss(_mm_load_ss(&ef),m128_one);
			if(iparam[1]) mef = _mm_rsqrt_ss(mef);
			else mef = _mm_rcp_ss(mef);			
			xL = _mm_mul_ss(xL,mef);
			xR = _mm_mul_ss(xR,mef);
			__m128 x = _mm_mul_ss(_mm_add_ss(xL,xR), m128_half);

			__m128 ef_nv;
			if(iparam[1]) ef_nv = _mm_mul_ss(x,x); 
			else ef_nv = abs_ps(x);
			__m128 ef_c = _mm_load_ss(&ef);
			__m128 p;
			if(_mm_comigt_ss(ef_nv,ef_c))	//float p = (ef_nv>ef)?at:re;
				p = _mm_load_ss(&at);
			else p = _mm_load_ss(&re);
			
			// ef = ef*p + ef_nv*(1-p);	
			ef_c = _mm_mul_ss(p,ef_c);
			p = _mm_sub_ss(m128_one,p);
			ef_c = _mm_add_ss(ef_c,_mm_mul_ss(p,ef_nv));
			_mm_store_ss(&ef,ef_c);
			_mm_store_ss(dataoutL+k,xL);
			_mm_store_ss(dataoutR+k,xR);
		}	
		postgain.multiply_2_blocks(dataoutL,dataoutR,block_size_quad);
	}
	else
	{
		// feedforward design		
		pregain.multiply_2_blocks_to(datainL,datainR,dataoutL,dataoutR,block_size_quad);
		
		//bq.process_block_to(dataoutL,dataoutR,sidechainL,sidechainR);

		const __m128 postg = _mm_set_ss(0.3333f);

		for(unsigned int k=0; k<block_size; k++)
		{				
			__m128 xL = _mm_load_ss(dataoutL+k);
			__m128 xR = _mm_load_ss(dataoutR+k);
			__m128 x = _mm_mul_ss(_mm_add_ss(xL,xR), m128_half);

			__m128 ef_nv;
			if(iparam[1]) ef_nv = _mm_mul_ss(x,x); 
			else ef_nv = abs_ps(x);
			__m128 ef_c = _mm_load_ss(&ef);
			__m128 p;
			if(_mm_comigt_ss(ef_nv,ef_c))	//float p = (ef_nv>ef)?at:re;
				p = _mm_load_ss(&at);
			else p = _mm_load_ss(&re);
						
			ef_c = _mm_mul_ss(p,ef_c);	// ef = ef*p + ef_nv*(1-p);	
			p = _mm_sub_ss(m128_one,p);
			ef_c = _mm_add_ss(ef_c,_mm_mul_ss(p,ef_nv));
			_mm_store_ss(&ef,ef_c);

			__m128 mef = _mm_max_ss(_mm_load_ss(&ef),m128_one);	
			if(iparam[1]) mef = _mm_rsqrt_ss(mef);
			else mef = _mm_rcp_ss(mef);	
			// SC edit
			xL = _mm_load_ss(dataoutL+k);
			xR = _mm_load_ss(dataoutR+k);
			xL = _mm_mul_ss(postg,_mm_mul_ss(xL,mef));
			xR = _mm_mul_ss(postg,_mm_mul_ss(xR,mef));

			_mm_store_ss(dataoutL+k,xL);
			_mm_store_ss(dataoutR+k,xR);
		}			
	}
}

void limiter::process(float *datain, float *dataout, float pitch)
{	
	pregain.set_target(db_to_linear(param[0]) * 3.f);
		
	float am = 1.0f - 0.99f * param[1];
	float rm = 1.0f - 0.99f * param[2];
	at = 1.f - 0.001f*am*am;
	re = 1.f - 0.001f * rm*rm;

	/*if(lastparam[1] != param[1])
	{
		at = 2-cos(pi*note_to_pitch(-12*param[1])*samplerate_inv),
		at -= sqrt(at*at - 1.f);		
		lastparam[1] = param[1];
	}	
	if(lastparam[2] != param[2])
	{
		re = 2-cos(pi*note_to_pitch(-12*param[2])*samplerate_inv);
		re -= sqrt(re*re - 1.f);
		lastparam[2] = param[2];
	}*/
		
	if(iparam[0])
	{
		// feedback design		
		__m128 postgainv = _mm_rsqrt_ss(pregain.target);		
		postgain.set_target(postgainv);
		pregain.multiply_block_to(datain,dataout,block_size_quad);

		for(unsigned int k=0; k<block_size; k++)
		{				
			__m128 x = _mm_load_ss(dataout+k);
			__m128 mef = _mm_max_ss(_mm_load_ss(&ef),m128_one);
			if(iparam[1]) x = _mm_mul_ss(x,_mm_rsqrt_ss(mef));	
			else x = _mm_mul_ss(x,_mm_rcp_ss(mef));	

			__m128 ef_nv;
			if(iparam[1]) ef_nv = _mm_mul_ss(x,x); 
			else ef_nv = abs_ps(x);
			__m128 ef_c = _mm_load_ss(&ef);
			__m128 p;
			if(_mm_comigt_ss(ef_nv,ef_c))	//float p = (ef_nv>ef)?at:re;
				p = _mm_load_ss(&at);
			else p = _mm_load_ss(&re);
			
			// ef = ef*p + ef_nv*(1-p);	
			ef_c = _mm_mul_ss(p,ef_c);
			p = _mm_sub_ss(m128_one,p);
			ef_c = _mm_add_ss(ef_c,_mm_mul_ss(p,ef_nv));
			_mm_store_ss(&ef,ef_c);
			_mm_store_ss(dataout+k,x);
		}	
		postgain.multiply_block(dataout,block_size_quad);
	}
	else
	{
		// feedforward design		
		pregain.multiply_block_to(datain,dataout,block_size_quad);
		const __m128 postg = _mm_set_ss(0.3333f);

		for(unsigned int k=0; k<block_size; k++)
		{				
			__m128 x = _mm_load_ss(dataout+k);											
			
			__m128 ef_nv;
			if(iparam[1]) ef_nv = _mm_mul_ss(x,x); 
			else ef_nv = abs_ps(x);
			__m128 ef_c = _mm_load_ss(&ef);
			__m128 p;
			if(_mm_comigt_ss(ef_nv,ef_c))	//float p = (ef_nv>ef)?at:re;
				p = _mm_load_ss(&at);
			else p = _mm_load_ss(&re);
						
			ef_c = _mm_mul_ss(p,ef_c);	// ef = ef*p + ef_nv*(1-p);	
			p = _mm_sub_ss(m128_one,p);
			ef_c = _mm_add_ss(ef_c,_mm_mul_ss(p,ef_nv));
			_mm_store_ss(&ef,ef_c);

			__m128 mef = _mm_max_ss(_mm_load_ss(&ef),m128_one);	
			if(iparam[1]) x = _mm_mul_ss(x,_mm_rsqrt_ss(mef));	
			else x = _mm_mul_ss(x,_mm_rcp_ss(mef));	
			x = _mm_mul_ss(postg,x);

			_mm_store_ss(dataout+k,x);
		}			
	}
}

/*	gate		*/

gate::gate(float *fp) : filter(fp)
{	
	strcpy(filtername,"gate");
	parameter_count = 4;
	strcpy(ctrllabel[0], "hold");				ctrlmode[0] = cm_time1s;
	strcpy(ctrllabel[1], "threshold");			ctrlmode[1] = cm_decibel;	
	strcpy(ctrllabel[2], "reduction");			ctrlmode[2] = cm_decibel;
	strcpy(ctrllabel[3], "hysteresis");			ctrlmode[3] = cm_decibel;
	strcpy(ctrlmode_desc[0], "f,-10,0.025,0,4,s");
	strcpy(ctrlmode_desc[1], "f,-60,0.1,0,0,dB");
	strcpy(ctrlmode_desc[2], "f,-96,0.1,0,0,dB");	
	strcpy(ctrlmode_desc[3], "f,0,0.02,9,0,dB");	
	gate_state = false;	
	gate_zc_sync[0] = false;
	gate_zc_sync[1] = false;
	holdtime = 0;
	onesampledelay[0] = -1;
	onesampledelay[1] = -1;
}

gate::~gate()
{

}

void gate::init_params()
{
	if(!param) return;
	// preset values
	param[0] = -3.0f;
	param[1] = -12.0f;
	param[2] = -96.0f;
}


void gate::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch)
{		
	float threshold_lo = db_to_linear(param[1] - param[3]);
	float threshold_hi = db_to_linear(param[1] + param[3]);
	float reduction = db_to_linear(param[2]);
	int ihtime = (int)(float)(samplerate * powf(2,param[0]));

	copy_block(datainL,dataoutL,block_size_quad);
	copy_block(datainR,dataoutR,block_size_quad);

	int k;
	for(k=0; k<block_size; k++)
	{	
		float input = max(fabs(datainL[k]),fabs(datainR[k]));
		
		if (input>(gate_state?threshold_lo:threshold_hi))
		{
			holdtime = ihtime;
			gate_state = true;
		}				

		if(holdtime<0) gate_state = false;
		if(!(onesampledelay[0]*datainL[k] > 0)) gate_zc_sync[0] = gate_state;
		if(!(onesampledelay[1]*datainR[k] > 0)) gate_zc_sync[1] = gate_state;

		if (!gate_zc_sync[0]) dataoutL[k] *= reduction;
		if (!gate_zc_sync[1]) dataoutR[k] *= reduction;
		
		holdtime--;
		onesampledelay[0] = datainL[k];
		onesampledelay[1] = datainR[k];
	}
}

void gate::process(float *datain, float *dataout, float pitch)
{		
	float threshold_lo = db_to_linear(param[1] - param[3]);
	float threshold_hi = db_to_linear(param[1] + param[3]);
	float reduction = db_to_linear(param[2]);
	int ihtime = (int)(float)(samplerate * powf(2,param[0]));

	copy_block(datain,dataout,block_size_quad);

	int k;
	for(k=0; k<block_size; k++)
	{	
		float input = datain[k];
		
		int *ip_intcast = (int*) &input;
		int polarity = *ip_intcast & 0x80000000;
		(*ip_intcast) = (*ip_intcast) & 0x7fffffff;
		if (input>(gate_state?threshold_lo:threshold_hi))
		{
			holdtime = ihtime;
			gate_state = true;
		}				

		if(holdtime<0) gate_state = false;
		if(!(onesampledelay[0]*datain[k] > 0)) gate_zc_sync[0] = gate_state;

		if (!gate_zc_sync[0]) dataout[k] = reduction*datain[k];		
		
		holdtime--;
		onesampledelay[0] = datain[k];
	}	
}

/*	exciter		*/

/*fexciter::fexciter(float *fp) : filter(fp), pre(false), post(false)
{	
	strcpy(filtername,"exciter");
	parameter_count = 1;
	strcpy(ctrllabel[0], "amount");				ctrlmode[0] = cm_mod_decibel;	

	strcpy(ctrlmode_desc[0], str_dbmoddef);	
	
	amount.setBlockSize(block_size*2);	// 2X oversampling
		
	reg = 0;
}

fexciter::~fexciter()
{	
}

void fexciter::init_params()
{
	if(!param) return;
	// preset values
	param[0] = 10.0f;	
}

void fexciter::process(float *data, float pitch)
{	
	amount.newValue(db_to_linear(param[0]));

	float osbuffer[block_size*2];

	int k;
	// upsample
	for(k=0; k<block_size; k++)
	{
		osbuffer[(k<<1)] = data[k];
		osbuffer[(k<<1)+1] = data[k];
	}	

	for(k=0; k<(block_size<<1); k++)
	{
		float in = pre.process(osbuffer[k]);		
		float out = in + amount.v*(in-reg)*in;
		reg = in;
		osbuffer[k] = post.process(out);
		amount.process();
	}

	// downsample
	for(k=0; k<block_size; k++)
	{
		data[k] = osbuffer[(k<<1)];		
	}
}*/

/*	stereotools		*/

stereotools::stereotools(float *fp, int* ip) : filter(fp,0,true,ip)
{	
	strcpy(filtername,"stereo tools");
	parameter_count = 2;
	strcpy(ctrllabel[0], "center");				ctrlmode[0] = cm_percent_bipolar;
	strcpy(ctrllabel[1], "width");				ctrlmode[1] = cm_percent_bipolar;	

	strcpy(ctrlmode_desc[0], str_percentbpdef);
	strcpy(ctrlmode_desc[1], "f,-2,0.005,2,1,%");	

	ampL.set_blocksize(block_size);
	ampR.set_blocksize(block_size);
	width.set_blocksize(block_size);		
}

stereotools::~stereotools()
{

}

void stereotools::init_params()
{
	if(!param) return;	
	param[0] = 0.0f;
	param[1] = 1.0f;
	iparam[0] = 0;
}

void stereotools::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch)
{		
	width.set_target_smoothed(clamp1bp(param[1]));
	ampL.set_target_smoothed(clamp1bp(1 - param[0]));
	ampR.set_target_smoothed(clamp1bp(1 + param[0])*((iparam[0]==1)?-1.f:1.f));

	Align16 float M[block_size],S[block_size];

	if(iparam[0]==2)
	{
		copy_block(datainL,M,block_size_quad);
		copy_block(datainR,S,block_size_quad);		
	}
	else
	{
		encodeMS(datainL,datainR,M,S,block_size_quad);
	}

	width.multiply_block(S,block_size_quad);
	decodeMS(M,S,dataoutL,dataoutR,block_size_quad);
	ampL.multiply_block(dataoutL,block_size_quad);
	ampR.multiply_block(dataoutR,block_size_quad);
}

void stereotools::process(float *datain, float *dataout, float pitch)
{		
	copy_block(datain,dataout,block_size_quad);	// do nothing
}


int stereotools::get_ip_count()
{
	return 1;
}

const char* stereotools::get_ip_label(int ip_id)
{ 
	if(ip_id == 0) return "mode";
	return ""; 
}

int stereotools::get_ip_entry_count(int ip_id)
{
	return 3;	
}
const char* stereotools::get_ip_entry_label(int ip_id, int c_id)
{
	if(ip_id == 0)
	{
		switch(c_id)
		{
		case 0:
			return "width";
		case 1:
			return "L+ R-";
		case 2:
			return "MSdec";		
		}
	}
	return "";
}
