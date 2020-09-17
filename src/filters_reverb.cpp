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


/* reverb			*/

const float db60 = powf(10.f,0.05f*-60.f);

enum revparam
{	
	rp_predelay = 0,
	//rp_shape,	// as iparam instead
	rp_roomsize,
	rp_decaytime,
	//rp_modrate,
	//rp_moddepth,
	rp_damping,
	//rp_variation,
	rp_locut,
	rp_freq1,
	rp_gain1,	
	rp_hicut,
	//rp_mix,
	rp_width,
};

reverb::reverb(float *fp, int *ip) 
: filter(fp,0,true,ip)
{
	strcpy(filtername,"Reverb");
	parameter_count = 9;	
	strcpy(ctrllabel[0], "Pre-Delay");			
	strcpy(ctrllabel[1], "Room Size");			
	strcpy(ctrllabel[2], "Decay Time");			
	strcpy(ctrllabel[3], "HF Damp");			
	strcpy(ctrllabel[4], "Low Cut");
	strcpy(ctrllabel[5], "EQ Freq");
	strcpy(ctrllabel[6], "EQ Gain");
	strcpy(ctrllabel[7], "High Cut");
	strcpy(ctrllabel[8], "Width");
			
	strcpy(ctrlmode_desc[0], "f,-7,0.025,0,4,s");	
	strcpy(ctrlmode_desc[1], str_percentdef);
	strcpy(ctrlmode_desc[2], "f,-4,0.025,7,4,s");	
	strcpy(ctrlmode_desc[3], str_percentdef);
	strcpy(ctrlmode_desc[4], str_freqdef);	
	strcpy(ctrlmode_desc[5], str_freqdef);
	strcpy(ctrlmode_desc[6], str_dbbpdef);	
	strcpy(ctrlmode_desc[7], str_freqdef);
	strcpy(ctrlmode_desc[8], str_dbbpdef);
}

reverb::~reverb()
{
}

void reverb::init_params()
{
	assert(param);
	if(!param) return;
	// preset values
	param[rp_predelay] = -5.5f;			
	param[rp_roomsize] = 0.42f;
	param[rp_decaytime] = 0.5f;	
	param[rp_damping] = 0.2f;
	param[rp_freq1] = 0.0f;
	param[rp_gain1] = -4.0f;
	param[rp_locut] = -2.0f;
	param[rp_hicut] = 3.5f;	
	param[rp_width] = -3.0f;	
	if(!iparam) return;
	iparam[0] = 2;
}

void reverb::init()
{		
	band1.coeff_peakEQ(band1.calc_omega(param[rp_freq1]),2,param[rp_gain1]);
	locut.coeff_HP(locut.calc_omega(param[rp_locut]),0.5);
	hicut.coeff_LP2B(hicut.calc_omega(param[rp_hicut]),0.5);
	band1.coeff_instantize();
	locut.coeff_instantize();
	hicut.coeff_instantize();
	band1.suspend();
	locut.suspend();
	hicut.suspend();
		
	b = 0;

	clear_buffers();
	loadpreset(0);
	modphase = 0;
	update_rsize();	

	width.set_target(1.f);	// borde bli mest smooth
	width.instantize();
	
	for(int t=0; t<rev_taps; t++)
	{
		float x = (float) t / (rev_taps - 1.f);
		float xbp = -1.f + 2.f*x;	

		out_tap[t] = 0;
		delay_pan_L[t] = sqrt(0.5 - 0.495*xbp);
		delay_pan_R[t] = sqrt(0.5 + 0.495*xbp);								
	}
	delay_pos = 0;

}

void reverb::clear_buffers()
{	
	clear_block(predelay,max_rev_dly>>2);
	clear_block(delay,(rev_taps*max_rev_dly)>>2);
}

void reverb::loadpreset(int id)
{
	shape = id;

	//clear_buffers();

	switch(id)
	{
	case 0:
		delay_time[0] = 1339934;	
		delay_time[1] = 962710;
		delay_time[2] = 1004427;
		delay_time[3] = 1103966;
		delay_time[4] = 1198575;
		delay_time[5] = 1743348;
		delay_time[6] =	1033425;
		delay_time[7] =	933313;
		delay_time[8] =	949407;
		delay_time[9] =	1402754;
		delay_time[10] = 1379894;
		delay_time[11] = 1225304;
		delay_time[12] = 1135598;
		delay_time[13] = 1402107;
		delay_time[14] = 956152;
		delay_time[15] = 1137737;
		break;
	case 1:
		delay_time[0] = 1265607 ;
		delay_time[1] = 844703 ;
		delay_time[2] = 856159 ;
		delay_time[3] = 1406425 ;
		delay_time[4] = 786608 ;
		delay_time[5] = 1163557 ;
		delay_time[6] = 1091206 ;
		delay_time[7] = 1129434 ;
		delay_time[8] = 1270379 ;
		delay_time[9] = 896997 ;
		delay_time[10] = 1415393 ;
		delay_time[11] = 782808 ;
		delay_time[12] = 868582 ;
		delay_time[13] = 1234463 ;
		delay_time[14] = 1000336 ;
		delay_time[15] = 968299 ;
		break;
	case 2:
		delay_time[0] = 1293101 ;
		delay_time[1] = 1334867 ;
		delay_time[2] = 1178781 ;
		delay_time[3] = 1850949 ;
		delay_time[4] = 1663760 ;
		delay_time[5] = 1982922 ;
		delay_time[6] = 1211021 ;
		delay_time[7] = 1824481 ;
		delay_time[8] = 1520266 ;
		delay_time[9] = 1351822 ;
		delay_time[10] = 1102711 ;
		delay_time[11] = 1513696 ;
		delay_time[12] = 1057618 ;
		delay_time[13] = 1671799 ;
		delay_time[14] = 1406360 ;
		delay_time[15] = 1170468 ;
		break;
	case 3:
		delay_time[0] = 1833435 ;
		delay_time[1] = 2462309 ;
		delay_time[2] = 2711583 ;
		delay_time[3] = 2219764 ;
		delay_time[4] = 1664194 ;
		delay_time[5] = 2109157 ;
		delay_time[6] = 1626137 ;
		delay_time[7] = 1434473 ;
		delay_time[8] = 2271242 ;
		delay_time[9] = 1621375 ;
		delay_time[10] = 1831218 ;
		delay_time[11] = 2640903 ;
		delay_time[12] = 1577737 ;
		delay_time[13] = 1871624 ;
		delay_time[14] = 2439164 ;
		delay_time[15] = 1427343 ;
		break;
	}

	for(int t=0; t<rev_taps; t++)
	{		
		delay_time[t] = (int)((float)(2.f * param[rp_roomsize]) * delay_time[t]);
	}
	lastf[rp_roomsize] = param[rp_roomsize];
	update_rtime();
}

void reverb::update_rtime()
{	
	int max_dt=0;
	for(int t=0; t<rev_taps; t++) 
	{
		delay_fb[t] = powf(db60, delay_time[t] / (256.f * samplerate * powf(2.f,param[rp_decaytime])));
		max_dt = std::max(max_dt,delay_time[t]);
	}
	lastf[rp_decaytime] = param[rp_decaytime];	
	float t = inv_block_size * ((float)(max_dt>>8) + samplerate * powf(2.f,param[rp_decaytime]) * 2.f);	// *2 is to get the db120 time
	ringout_time = (int)t;
}
void reverb::update_rsize()
{
	//memset(delay,0,rev_taps*max_rev_dly*sizeof(float));
	
	loadpreset(shape);

	
//	lastf[rp_variation] = *f[rp_variation];	
	//update_rtime();
}

void reverb::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch)
{				
	Align16 float wetL[block_size],wetR[block_size];	
	
	if(iparam[0] != shape) loadpreset(iparam[0]);
	if((b == 0)&&(fabs(param[rp_roomsize] - lastf[rp_roomsize]) > 0.001f)) loadpreset(shape);
	if(fabs(param[rp_decaytime] - lastf[rp_decaytime]) > 0.001f) update_rtime();		
	
	// do more seldom
	if(b == 0)
	{
		band1.coeff_peakEQ(band1.calc_omega(param[rp_freq1]),2,param[rp_gain1]);
		locut.coeff_HP(locut.calc_omega(param[rp_locut]),0.5);
		hicut.coeff_LP2B(hicut.calc_omega(param[rp_hicut]),0.5);
	}
	b = (b+1)&31;
		
	width.set_target_smoothed(db_to_linear(param[rp_width]));

	int pdtime = (int)(float)samplerate * note_to_pitch(12* param[rp_predelay]);
	const __m128 one4 = _mm_set1_ps(1.f);
	__m128 damp4 = _mm_load1_ps(&param[rp_damping]);	
	__m128 damp4m1 = _mm_sub_ps(one4, damp4);

	for(int k=0; k<block_size; k++)
	{						
		for(int t=0; t<rev_taps; t+=4)
		{
			int dp = (delay_pos - (delay_time[t]>>8));
			//float newa = delay[t + ((dp & (max_rev_dly-1))<<rev_tap_bits)];			
			__m128 newa = _mm_load_ss(&delay[t + ((dp & (max_rev_dly-1))<<rev_tap_bits)]);
			dp = (delay_pos - (delay_time[t+1]>>8));
			__m128 newb = _mm_load_ss(&delay[t+1 + ((dp & (max_rev_dly-1))<<rev_tap_bits)]);
			dp = (delay_pos - (delay_time[t+2]>>8));			
			newa = _mm_unpacklo_ps(newa,newb); // a,b,0,0
			__m128 newc = _mm_load_ss(&delay[t+2 + ((dp & (max_rev_dly-1))<<rev_tap_bits)]);
			dp = (delay_pos - (delay_time[t+3]>>8));
			__m128 newd = _mm_load_ss(&delay[t+3 + ((dp & (max_rev_dly-1))<<rev_tap_bits)]);
			newc = _mm_unpacklo_ps(newc,newd); // c,d,0,0
			__m128 new4 = _mm_movelh_ps(newa,newc);	// a,b,c,d

			__m128 out_tap4 = _mm_load_ps(&out_tap[t]);
			out_tap4 = _mm_add_ps(_mm_mul_ps(out_tap4,damp4),_mm_mul_ps(new4,damp4m1));
			_mm_store_ps(&out_tap[t],out_tap4);
			//out_tap[t] = *f[rp_damping]*out_tap[t] + (1- *f[rp_damping])*newa;
		}

		__m128 fb = _mm_add_ps(_mm_add_ps(_mm_load_ps(out_tap),_mm_load_ps(out_tap+4)),_mm_add_ps(_mm_load_ps(out_tap+8),_mm_load_ps(out_tap+12)));
		fb = sum_ps_to_ss(fb);
		/*for(int t=0; t<rev_taps; t+=4)
		{
			fb += out_tap[t] + out_tap[t+1] + out_tap[t+2] + out_tap[t+3];
		}*/			
		
		const __m128 ca = _mm_set_ss(((float)(-(2.f)/rev_taps)));
		//fb =  ca * fb + predelay[(delay_pos - pdtime)&(max_rev_dly-1)];		
		fb = _mm_add_ss(_mm_mul_ss(ca,fb),_mm_load_ss(&predelay[(delay_pos - pdtime)&(max_rev_dly-1)]));				
		
		delay_pos = (delay_pos+1) & (max_rev_dly-1);
				
		predelay[delay_pos] = 0.5f*(datainL[k] + datainR[k]);
		//__m128 fb4 = _mm_load1_ps(&fb);
		__m128 fb4 = _mm_shuffle_ps(fb,fb,0);
		 			
		__m128 L=_mm_setzero_ps(),R=_mm_setzero_ps();
		for(int t=0; t<rev_taps; t+=4)
		{						
			__m128 ot = _mm_load_ps(&out_tap[t]);
			__m128 dfb = _mm_load_ps(&delay_fb[t]);
			__m128 a = _mm_mul_ps(dfb,_mm_add_ps(fb4,ot));
			_mm_store_ps(&delay[(delay_pos<<rev_tap_bits) + t],a);			
			L = _mm_add_ps(L,_mm_mul_ps(ot,_mm_load_ps(&delay_pan_L[t])));
			R = _mm_add_ps(R,_mm_mul_ps(ot,_mm_load_ps(&delay_pan_R[t])));			
		}						
		L = sum_ps_to_ss(L);
		R = sum_ps_to_ss(R);
		_mm_store_ss(&wetL[k],L);
		_mm_store_ss(&wetR[k],R);		
	}	
	locut.process_block_slowlag(wetL,wetR);
	band1.process_block_slowlag(wetL,wetR);
	hicut.process_block_slowlag(wetL,wetR);

	// scale width
	Align16 float M[block_size],S[block_size];
	encodeMS(wetL,wetR,M,S,block_size_quad);
	width.multiply_block(S,block_size_quad);
	decodeMS(M,S,dataoutL,dataoutR,block_size_quad);
	
	//mix.fade_2_blocks_to(dataL,wetL,dataR,wetR,dataL,dataR, block_size_quad);
}

void reverb::suspend()
{ 
	init();
}


int		reverb::get_ip_count(){ return 1; }
const char*	reverb::get_ip_label(int ip_id){ return ("Shape"); }
int		reverb::get_ip_entry_count(int ip_id){ return 4; }
const char*	reverb::get_ip_entry_label(int ip_id, int c_id)
{
	switch(c_id)
	{
	case 0: return "S1"; break;
	case 1: return "S2"; break;
	case 2: return "S3"; break;
	case 3: return "S4"; break;
	}
	return "-";
}