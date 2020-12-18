#include "biquadunit.h"
#include "globals.h"
#include "unitconversion.h"
#include <complex>
#include "unitconversion.h"
#include <vt_dsp/basic_dsp.h>
#include <cmath>
#include <math.h>
#include <algorithm>

using namespace std;

inline double square(double x){ return x*x; }

biquadunit::biquadunit()
{
	reg0.d[0] = 0;
	reg1.d[0] = 0;
	reg0.d[1] = 0;
	reg1.d[1] = 0;
	first_run = true;

	a1.init();
	a2.init();	
	b0.init();
	b1.init();
	b2.init();		
}

void biquadunit::coeff_copy(biquadunit* bq)
{
	set_coef(1,bq->a1.target_v.d[0],bq->a2.target_v.d[0],bq->b0.target_v.d[0],bq->b1.target_v.d[0],bq->b2.target_v.d[0]);
}

const double pi1_with_margin = M_PI * 0.999;

void biquadunit::coeff_LP(double omega, double Q)
{	
	if (omega > pi1_with_margin) omega = pi1_with_margin;

	{
		double	cosi = cos(omega),
				sinu = sin(omega),		
				alpha = sinu/(2*Q),			
				b0 = (1 - cosi)*0.5,
				b1 =  1 - cosi,
				b2 = (1 - cosi)*0.5,
				a0 =  1 + alpha,
				a1 = -2*cosi,
				a2 = 1 - alpha;

		set_coef(a0,a1,a2,b0,b1,b2);
	}	
}

void biquadunit::coeff_LP2B(double omega, double Q)
{	
	if (omega > pi1_with_margin) omega = pi1_with_margin;

	{						
		double w_sq = omega*omega;		
		double den = (w_sq*w_sq) + (pi1*pi1*pi1*pi1) + w_sq*(pi1*pi1)*(1/Q-2);
		double G1 = std::min(1.0,sqrt((w_sq*w_sq)/den) * 0.5);

		double	cosi = cos(omega),
		sinu = sin(omega),		
		//alpha = sinu*sinh((log(2.0)/2.0) * (BW) * omega / sinu),
		alpha = sinu/(2*Q),
		//G1 = 0.05, //powf(2,-log(pi1/omega)/log(2.0)),
		// sätt aa till 6 db

		A = 2*sqrt(G1)*sqrt(2-G1),		
		b0 = (1 - cosi + G1*(1+cosi) + A*sinu)*0.5,
		b1 =  (1 - cosi - G1*(1+cosi)),
		b2 = (1 - cosi + G1*(1+cosi) - A*sinu)*0.5,
		a0 =  (1 + alpha),
		a1 = -2*cosi,
		a2 = 1 - alpha;

		set_coef(a0,a1,a2,b0,b1,b2);
	}
}

void biquadunit::coeff_HP(double omega, double Q)
{	
	if (omega > pi1_with_margin) omega = pi1_with_margin;

	{
		double	cosi = cos(omega),
				sinu = sin(omega),		
				alpha = sinu/(2*Q),			
				b0 = (1 + cosi)*0.5,
				b1 =  -(1 + cosi),
				b2 = (1 + cosi)*0.5,
				a0 =  1 + alpha,
				a1 = -2*cosi,
				a2 = 1 - alpha;

		set_coef(a0,a1,a2,b0,b1,b2);
	}
}

void biquadunit::coeff_BP2AQ(double omega, double Q)
{
	if (omega > pi1_with_margin) omega = pi1_with_margin;

	{
		double	cosi = cos(omega),
			sinu = sin(omega),
			alpha = sinu/(2*Q),
			b0 = alpha,		
			b2 = -alpha,
			a0 =  1 + alpha,
			a1 = -2*cosi,
			a2 =  1 - alpha;	

		set_coef(a0,a1,a2,b0,0,b2);
	}
}

void biquadunit::coeff_BP2A(double omega, double BW)
{
	if (omega > pi1_with_margin) omega = pi1_with_margin;
		double	cosi = cos(omega),
			sinu = sin(omega),
			q = 1 / (0.02 + 30*BW*BW),
			alpha = sinu/(2*q),
			b0 = alpha,		
			b2 = -alpha,
			a0 =  1 + alpha,
			a1 = -2*cosi,
			a2 =  1 - alpha;	

	set_coef(a0,a1,a2,b0,0,b2);
}

void biquadunit::coeff_PKA(double omega, double QQ)
{
	if (omega > pi1_with_margin) omega = pi1_with_margin;

	double	cosi = cos(omega),
			sinu = sin(omega),
			reso = limit_range(QQ,0.0,1.0),
			q = 0.1 + 10*reso*reso,
			alpha = sinu/(2*q),
			b0 = q*alpha,		
			b2 = -q*alpha,
			a0 =  1 + alpha,
			a1 = -2*cosi,
			a2 =  1 - alpha;

	set_coef(a0,a1,a2,b0,0,b2);
}

void biquadunit::coeff_NOTCHQ(double omega, double Q)
{
	if (omega > pi1_with_margin) omega = pi1_with_margin;

	{
		double	cosi = cos(omega),
			sinu = sin(omega),
			alpha = sinu/(2*Q),
			b0 = 1,
			b1 = -2*cosi,
			b2 = 1,
			a0 =  1 + alpha,
			a1 = -2*cosi,
			a2 =  1 - alpha;

		set_coef(a0,a1,a2,b0,b1,b2);
	}
}

void biquadunit::coeff_NOTCH(double omega, double BW)
{	

//	TODO byta ut orfanidis mot vanlig (buggar i detta caset)
	//assert(0);
	//coeff_orfanidisEQ(omega, q, 0, M_SQRT1_2, 1);
	if (omega > pi1_with_margin) omega = pi1_with_margin;

	{	
		double	cosi = cos(omega),
			sinu = sin(omega),
			reso = limit_range(BW,0.0,1.0),
			//q = 10*reso*reso*reso,
			q = 1 / (0.02 + 30*reso*reso),
			alpha = sinu/(2*q),
			b0 = 1,
			b1 = -2*cosi,
			b2 = 1,
			a0 =  1 + alpha,
			a1 = -2*cosi,
			a2 =  1 - alpha;

		set_coef(a0,a1,a2,b0,b1,b2);
	}
}

void biquadunit::coeff_LP_with_BW(double omega, double BW)
{
	coeff_LP(omega,1/BW);
}

void biquadunit::coeff_HP_with_BW(double omega, double BW)
{
	coeff_HP(omega,1/BW);
}	

void biquadunit::coeff_LPHPmorph(double omega, double Q, double morph)
{
	if (omega > pi1_with_margin) omega = pi1_with_margin;

	double	HP = limit_range(morph,0.0,1.0),
			LP = 1-HP,
			BP = LP*HP;
	HP *= HP;
	LP *= LP;
	
	double	cosi = cos(omega),
			sinu = sin(omega),		
			alpha = sinu/(2*Q),			
			b0 = alpha,
			b1 = 0,
			b2 = -alpha,
			a0 =  1 + alpha,
			a1 = -2*cosi,
			a2 = 1 - alpha;

	set_coef(a0,a1,a2,b0,b1,b2);
}

void biquadunit::coeff_APF(double omega, double Q)
{
	if (omega > pi1_with_margin) omega = pi1_with_margin;

	double	cosi = cos(omega),
			sinu = sin(omega),		
			alpha = sinu/(2*Q),			
			b0 = (1 - alpha),
			b1 =  -2*cosi,
			b2 = (1 + alpha),
			a0 =  (1 + alpha),
			a1 = -2*cosi,
			a2 = (1 - alpha);

	set_coef(a0,a1,a2,b0,b1,b2);
}

void biquadunit::coeff_peakEQ(double omega, double BW, double gain)
{		
	coeff_orfanidisEQ(omega, BW, dB_to_linear(gain), dB_to_linear(gain*0.5), 1);
}

void biquadunit::coeff_orfanidisEQ(double omega, double BW, double G, double GB, double G0)
{			
	double limit = 0.95;
	double w0 = omega; //min(pi1-0.000001,omega);	
	BW = std::max(minBW,BW);
	double Dww = 2*w0*sinh((log(2.0)/2.0)*BW);		// sinh = (e^x - e^-x)/2

	double gainscale = 1; 
	//if(omega>pi1) gainscale = 1 / (1 + (omega-pi1)*(4/Dw));	

	if (abs(G - G0)>0.00001)
	{
		double F = abs(G*G - GB*GB);
		double G00 = abs(G*G - G0*G0);
		double F00 = abs(GB*GB - G0*G0);
		double num = G0*G0 * square(w0*w0 - (pi1*pi1)) + G*G * F00 * (pi1*pi1) * Dww*Dww / F;
		double den = square(w0*w0 - pi1*pi1) + F00 * pi1*pi1 * Dww*Dww / F;
		double G1 = sqrt(num/den);			

		if(omega > pi1)
		{
			G = G1 * 0.9999;
			w0 = pi1 - 0.00001;
			G00 = abs(G*G - G0*G0);
			F00 = abs(GB*GB - G0*G0);
		}

		double G01 = abs(G*G - G0*G1);
		double G11 = abs(G*G - G1*G1);
		double F01 = abs(GB*GB - G0*G1);
		double F11 = abs(GB*GB - G1*G1);	// blir wacko ?
		double W2 = sqrt(G11 / G00) * square(tan(w0/2));
		double w_lower = w0 * powf(2,-0.5*BW);		
		double w_upper = 2*atan(sqrt(F00/F11)*sqrt(G11/G00)*square(tan(w0/2))/tan(w_lower/2));
		double Dw = abs(w_upper - w_lower);				
		double DW = (1 + sqrt(F00 / F11) * W2) * tan(Dw/2);		

		double C = F11 * DW*DW - 2 * W2 * (F01 - sqrt(F00 * F11));
		double D = 2 * W2 * (G01 - sqrt(G00 * G11));
		double A = sqrt((C + D) / F);
		double B = sqrt((G*G * C + GB*GB * D) / F);
		double	a0 = (1 + W2 + A),
			a1 = -2*(1 - W2),
			a2 = (1 + W2 - A),
			b0 = (G1 + G0*W2 + B),
			b1 = -2*(G1 - G0*W2),
			b2 = (G1 - B + G0*W2);

		//if (c) sprintf(c,"G0: %f G: %f GB: %f G1: %f ",linear_to_dB(G0),linear_to_dB(G),linear_to_dB(GB),linear_to_dB(G1));

		set_coef(a0,a1,a2,b0,b1,b2);
	}
	else 
	{
		set_coef(1,0,0,1,0,0);
	}
}


void biquadunit::coeff_same_as_last_time()
{
	// ifall man skulle byta ipol så sätt dv = 0 här
}

void biquadunit::coeff_instantize()
{
	a1.instantize();
	a2.instantize();
	b0.instantize();
	b1.instantize();
	b2.instantize();
}

void biquadunit::set_coef(double a0,double a1,double a2,double b0,double b1,double b2)
{
	double a0inv = 1/a0;

	b0 *= a0inv;	b1 *= a0inv;	b2 *= a0inv;		a1 *= a0inv;	a2 *= a0inv;	
	if(first_run)
	{
		this->a1.startValue(a1);		this->a2.startValue(a2);
		this->b0.startValue(b0); 		this->b1.startValue(b1); 		this->b2.startValue(b2);
		first_run = false;
	}
	this->a1.newValue(a1);	this->a2.newValue(a2);
	this->b0.newValue(b0); 	this->b1.newValue(b1);	this->b2.newValue(b2);
}

#if USE_SSE2

void biquadunit::process_block(double *data)
{	
	for(int k=0; k<block_size; k+=2)
	{		
		__m128d input = _mm_load_sd(data+k);
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op0 = _mm_add_sd(reg0.v,_mm_mul_sd(b0.v.v, input));
		reg0.v = _mm_sub_sd(_mm_add_sd(_mm_mul_sd(b1.v.v, input), reg1.v), _mm_mul_sd(a1.v.v, op0));
		reg1.v = _mm_sub_sd(_mm_mul_sd(b2.v.v, input), _mm_mul_sd(a2.v.v, op0));
		_mm_store_sd(data+k,op0);

		input = _mm_load_sd(data+k);
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		op0 = _mm_add_sd(reg0.v,_mm_mul_sd(b0.v.v, input));
		reg0.v = _mm_sub_sd(_mm_add_sd(_mm_mul_sd(b1.v.v, input), reg1.v), _mm_mul_sd(a1.v.v, op0));
		reg1.v = _mm_sub_sd(_mm_mul_sd(b2.v.v, input), _mm_mul_sd(a2.v.v, op0));
		_mm_store_sd(data+k,op0);
	}
}

void biquadunit::process_block(float *data)
{	
	for(int k=0; k<block_size; k+=4)
	{
		// load
		__m128 vl = _mm_load_ps(data + k);						
		// first
		__m128d input = _mm_cvtss_sd (input, vl);
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op0 = _mm_add_sd(reg0.v,_mm_mul_sd(b0.v.v, input));
		reg0.v = _mm_sub_sd(_mm_add_sd(_mm_mul_sd(b1.v.v, input), reg1.v), _mm_mul_sd(a1.v.v, op0));
		reg1.v = _mm_sub_sd(_mm_mul_sd(b2.v.v, input), _mm_mul_sd(a2.v.v, op0));
		
		// second
		input = _mm_cvtss_sd (input, _mm_shuffle_ps(vl,vl,_MM_SHUFFLE(0, 0, 0, 1)));
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op1 = _mm_add_sd(reg0.v,_mm_mul_sd(b0.v.v, input));
		reg0.v = _mm_sub_sd(_mm_add_sd(_mm_mul_sd(b1.v.v, input), reg1.v), _mm_mul_sd(a1.v.v, op1));
		reg1.v = _mm_sub_sd(_mm_mul_sd(b2.v.v, input), _mm_mul_sd(a2.v.v, op1));

		// third
		input = _mm_cvtss_sd (input, _mm_shuffle_ps(vl,vl,_MM_SHUFFLE(0, 0, 0, 2)));
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op2 = _mm_add_sd(reg0.v,_mm_mul_sd(b0.v.v, input));
		reg0.v = _mm_sub_sd(_mm_add_sd(_mm_mul_sd(b1.v.v, input), reg1.v), _mm_mul_sd(a1.v.v, op2));
		reg1.v = _mm_sub_sd(_mm_mul_sd(b2.v.v, input), _mm_mul_sd(a2.v.v, op2));

		// fourth
		input = _mm_cvtss_sd (input, _mm_shuffle_ps(vl,vl,_MM_SHUFFLE(0, 0, 0, 3)));
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op3 = _mm_add_sd(reg0.v,_mm_mul_sd(b0.v.v, input));
		reg0.v = _mm_sub_sd(_mm_add_sd(_mm_mul_sd(b1.v.v, input), reg1.v), _mm_mul_sd(a1.v.v, op3));
		reg1.v = _mm_sub_sd(_mm_mul_sd(b2.v.v, input), _mm_mul_sd(a2.v.v, op3));

		// store
		__m128 sl = _mm_cvtpd_ps(_mm_unpacklo_pd(op0,op1)); 
		sl = _mm_movelh_ps(sl,_mm_cvtpd_ps(_mm_unpacklo_pd(op2,op3))); 
		_mm_store_ps(data+k,sl);		
	}
}

void biquadunit::process_block_to(float *data, float *dataout)
{	
	for(int k=0; k<block_size; k+=4)
	{
		// load
		__m128 vl = _mm_load_ps(data + k);						
		// first
		__m128d input = _mm_cvtss_sd (input, vl);
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op0 = _mm_add_sd(reg0.v,_mm_mul_sd(b0.v.v, input));
		reg0.v = _mm_sub_sd(_mm_add_sd(_mm_mul_sd(b1.v.v, input), reg1.v), _mm_mul_sd(a1.v.v, op0));
		reg1.v = _mm_sub_sd(_mm_mul_sd(b2.v.v, input), _mm_mul_sd(a2.v.v, op0));
		
		// second
		input = _mm_cvtss_sd (input, _mm_shuffle_ps(vl,vl,_MM_SHUFFLE(0, 0, 0, 1)));
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op1 = _mm_add_sd(reg0.v,_mm_mul_sd(b0.v.v, input));
		reg0.v = _mm_sub_sd(_mm_add_sd(_mm_mul_sd(b1.v.v, input), reg1.v), _mm_mul_sd(a1.v.v, op1));
		reg1.v = _mm_sub_sd(_mm_mul_sd(b2.v.v, input), _mm_mul_sd(a2.v.v, op1));

		// third
		input = _mm_cvtss_sd (input, _mm_shuffle_ps(vl,vl,_MM_SHUFFLE(0, 0, 0, 2)));
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op2 = _mm_add_sd(reg0.v,_mm_mul_sd(b0.v.v, input));
		reg0.v = _mm_sub_sd(_mm_add_sd(_mm_mul_sd(b1.v.v, input), reg1.v), _mm_mul_sd(a1.v.v, op2));
		reg1.v = _mm_sub_sd(_mm_mul_sd(b2.v.v, input), _mm_mul_sd(a2.v.v, op2));

		// fourth
		input = _mm_cvtss_sd (input, _mm_shuffle_ps(vl,vl,_MM_SHUFFLE(0, 0, 0, 3)));
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op3 = _mm_add_sd(reg0.v,_mm_mul_sd(b0.v.v, input));
		reg0.v = _mm_sub_sd(_mm_add_sd(_mm_mul_sd(b1.v.v, input), reg1.v), _mm_mul_sd(a1.v.v, op3));
		reg1.v = _mm_sub_sd(_mm_mul_sd(b2.v.v, input), _mm_mul_sd(a2.v.v, op3));

		// store
		__m128 sl = _mm_cvtpd_ps(_mm_unpacklo_pd(op0,op1)); 
		sl = _mm_movelh_ps(sl,_mm_cvtpd_ps(_mm_unpacklo_pd(op2,op3))); 
		_mm_store_ps(dataout+k,sl);		
	}
}

void biquadunit::process_block(float *dataL,float *dataR)
{	
	for(int k=0; k<block_size; k+=4)
	{
		// load
		__m128 vl = _mm_load_ps(dataL + k);
		__m128 vr = _mm_load_ps(dataR + k);
		__m128 v0 = _mm_unpacklo_ps(vl,vr);
		
		// first
		__m128d input = _mm_cvtps_pd(v0);
		v0 = _mm_movehl_ps(v0,v0);		
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();

		__m128d op0 = _mm_add_pd(reg0.v,_mm_mul_pd(b0.v.v, input));
		reg0.v = _mm_sub_pd(_mm_add_pd(_mm_mul_pd(b1.v.v, input), reg1.v), _mm_mul_pd(a1.v.v, op0));
		reg1.v = _mm_sub_pd(_mm_mul_pd(b2.v.v, input), _mm_mul_pd(a2.v.v, op0));

		
		// second
		input = _mm_cvtps_pd(v0);
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op1 = _mm_add_pd(reg0.v,_mm_mul_pd(b0.v.v, input));
		reg0.v = _mm_sub_pd(_mm_add_pd(_mm_mul_pd(b1.v.v, input), reg1.v), _mm_mul_pd(a1.v.v, op1));
		reg1.v = _mm_sub_pd(_mm_mul_pd(b2.v.v, input), _mm_mul_pd(a2.v.v, op1));


		// third
		v0 = _mm_unpackhi_ps(vl,vr);
		input = _mm_cvtps_pd(v0);		
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op2 = _mm_add_pd(reg0.v,_mm_mul_pd(b0.v.v, input));
		reg0.v = _mm_sub_pd(_mm_add_pd(_mm_mul_pd(b1.v.v, input), reg1.v), _mm_mul_pd(a1.v.v, op2));
		reg1.v = _mm_sub_pd(_mm_mul_pd(b2.v.v, input), _mm_mul_pd(a2.v.v, op2));

		// fourth
		v0 = _mm_movehl_ps(v0,v0);
		input = _mm_cvtps_pd(v0);		
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op3 = _mm_add_pd(reg0.v,_mm_mul_pd(b0.v.v, input));
		reg0.v = _mm_sub_pd(_mm_add_pd(_mm_mul_pd(b1.v.v, input), reg1.v), _mm_mul_pd(a1.v.v, op3));
		reg1.v = _mm_sub_pd(_mm_mul_pd(b2.v.v, input), _mm_mul_pd(a2.v.v, op3));

		// store
		__m128 sl = _mm_cvtpd_ps(_mm_unpacklo_pd(op0,op1)); 
		sl = _mm_movelh_ps(sl,_mm_cvtpd_ps(_mm_unpacklo_pd(op2,op3))); 
		_mm_store_ps(dataL+k,sl);

		__m128 sr = _mm_cvtpd_ps(_mm_unpackhi_pd(op0,op1)); 
		sr = _mm_movelh_ps(sr,_mm_cvtpd_ps(_mm_unpackhi_pd(op2,op3))); 
		_mm_store_ps(dataR+k,sr);
	}
}

void biquadunit::process_block_slowlag(float *dataL,float *dataR)
{	
	a1.process();	a2.process();	b0.process();	b1.process();	b2.process();

	for(int k=0; k<block_size; k+=4)
	{
		// load
		__m128 vl = _mm_load_ps(dataL + k);
		__m128 vr = _mm_load_ps(dataR + k);
		__m128 v0 = _mm_unpacklo_ps(vl,vr);
		
		// first
		__m128d input = _mm_cvtps_pd(v0);
		v0 = _mm_movehl_ps(v0,v0);				

		__m128d op0 = _mm_add_pd(reg0.v,_mm_mul_pd(b0.v.v, input));
		reg0.v = _mm_sub_pd(_mm_add_pd(_mm_mul_pd(b1.v.v, input), reg1.v), _mm_mul_pd(a1.v.v, op0));
		reg1.v = _mm_sub_pd(_mm_mul_pd(b2.v.v, input), _mm_mul_pd(a2.v.v, op0));

		
		// second
		input = _mm_cvtps_pd(v0);		
		__m128d op1 = _mm_add_pd(reg0.v,_mm_mul_pd(b0.v.v, input));
		reg0.v = _mm_sub_pd(_mm_add_pd(_mm_mul_pd(b1.v.v, input), reg1.v), _mm_mul_pd(a1.v.v, op1));
		reg1.v = _mm_sub_pd(_mm_mul_pd(b2.v.v, input), _mm_mul_pd(a2.v.v, op1));


		// third
		v0 = _mm_unpackhi_ps(vl,vr);
		input = _mm_cvtps_pd(v0);				
		__m128d op2 = _mm_add_pd(reg0.v,_mm_mul_pd(b0.v.v, input));
		reg0.v = _mm_sub_pd(_mm_add_pd(_mm_mul_pd(b1.v.v, input), reg1.v), _mm_mul_pd(a1.v.v, op2));
		reg1.v = _mm_sub_pd(_mm_mul_pd(b2.v.v, input), _mm_mul_pd(a2.v.v, op2));

		// fourth
		v0 = _mm_movehl_ps(v0,v0);
		input = _mm_cvtps_pd(v0);				
		__m128d op3 = _mm_add_pd(reg0.v,_mm_mul_pd(b0.v.v, input));
		reg0.v = _mm_sub_pd(_mm_add_pd(_mm_mul_pd(b1.v.v, input), reg1.v), _mm_mul_pd(a1.v.v, op3));
		reg1.v = _mm_sub_pd(_mm_mul_pd(b2.v.v, input), _mm_mul_pd(a2.v.v, op3));

		// store
		__m128 sl = _mm_cvtpd_ps(_mm_unpacklo_pd(op0,op1)); 
		sl = _mm_movelh_ps(sl,_mm_cvtpd_ps(_mm_unpacklo_pd(op2,op3))); 
		_mm_store_ps(dataL+k,sl);

		__m128 sr = _mm_cvtpd_ps(_mm_unpackhi_pd(op0,op1)); 
		sr = _mm_movelh_ps(sr,_mm_cvtpd_ps(_mm_unpackhi_pd(op2,op3))); 
		_mm_store_ps(dataR+k,sr);
	}
}

void biquadunit::process_block_to(float *dataL,float *dataR, float *dstL,float *dstR)
{	
	for(int k=0; k<block_size; k+=4)
	{
		// load
		__m128 vl = _mm_load_ps(dataL + k);
		__m128 vr = _mm_load_ps(dataR + k);
		__m128 v0 = _mm_unpacklo_ps(vl,vr);
		
		// first
		__m128d input = _mm_cvtps_pd(v0);
		v0 = _mm_movehl_ps(v0,v0);		
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();

		__m128d op0 = _mm_add_pd(reg0.v,_mm_mul_pd(b0.v.v, input));
		reg0.v = _mm_sub_pd(_mm_add_pd(_mm_mul_pd(b1.v.v, input), reg1.v), _mm_mul_pd(a1.v.v, op0));
		reg1.v = _mm_sub_pd(_mm_mul_pd(b2.v.v, input), _mm_mul_pd(a2.v.v, op0));

		
		// second
		input = _mm_cvtps_pd(v0);
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op1 = _mm_add_pd(reg0.v,_mm_mul_pd(b0.v.v, input));
		reg0.v = _mm_sub_pd(_mm_add_pd(_mm_mul_pd(b1.v.v, input), reg1.v), _mm_mul_pd(a1.v.v, op1));
		reg1.v = _mm_sub_pd(_mm_mul_pd(b2.v.v, input), _mm_mul_pd(a2.v.v, op1));


		// third
		v0 = _mm_unpackhi_ps(vl,vr);
		input = _mm_cvtps_pd(v0);		
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op2 = _mm_add_pd(reg0.v,_mm_mul_pd(b0.v.v, input));
		reg0.v = _mm_sub_pd(_mm_add_pd(_mm_mul_pd(b1.v.v, input), reg1.v), _mm_mul_pd(a1.v.v, op2));
		reg1.v = _mm_sub_pd(_mm_mul_pd(b2.v.v, input), _mm_mul_pd(a2.v.v, op2));

		// fourth
		v0 = _mm_movehl_ps(v0,v0);
		input = _mm_cvtps_pd(v0);		
		a1.process();	a2.process();	b0.process();	b1.process();	b2.process();
		__m128d op3 = _mm_add_pd(reg0.v,_mm_mul_pd(b0.v.v, input));
		reg0.v = _mm_sub_pd(_mm_add_pd(_mm_mul_pd(b1.v.v, input), reg1.v), _mm_mul_pd(a1.v.v, op3));
		reg1.v = _mm_sub_pd(_mm_mul_pd(b2.v.v, input), _mm_mul_pd(a2.v.v, op3));

		// store
		__m128 sl = _mm_cvtpd_ps(_mm_unpacklo_pd(op0,op1)); 
		sl = _mm_movelh_ps(sl,_mm_cvtpd_ps(_mm_unpacklo_pd(op2,op3))); 
		_mm_store_ps(dstL+k,sl);

		__m128 sr = _mm_cvtpd_ps(_mm_unpackhi_pd(op0,op1)); 
		sr = _mm_movelh_ps(sr,_mm_cvtpd_ps(_mm_unpackhi_pd(op2,op3))); 
		_mm_store_ps(dstR+k,sr);
	}
}

#else

void biquadunit::process_block(float *data)
{
	/*if(storage->SSE2) process_block_SSE2(data);	
	else*/
	{
		int k;
		for(k=0; k<block_size; k++)
		{	
			a1.process();	a2.process();
			b0.process();	b1.process();	b2.process();

			double input = data[k];
			double op;

			op = input*b0.v.d[0] + reg0.d[0];
			reg0.d[0] = input*b1.v.d[0] - a1.v.d[0]*op + reg1.d[0];
			reg1.d[0] = input*b2.v.d[0] - a2.v.d[0]*op;	

			data[k] = op;
		}
		flush_denormal(reg0.d[0]);
		flush_denormal(reg1.d[0]);
	}
}

void biquadunit::process_block_to(float *data, float *dataout)
{
	/*if(storage->SSE2) process_block_SSE2(data);	
	else*/
	{
		int k;
		for(k=0; k<block_size; k++)
		{	
			a1.process();	a2.process();
			b0.process();	b1.process();	b2.process();

			double input = data[k];
			double op;

			op = input*b0.v.d[0] + reg0.d[0];
			reg0.d[0] = input*b1.v.d[0] - a1.v.d[0]*op + reg1.d[0];
			reg1.d[0] = input*b2.v.d[0] - a2.v.d[0]*op;	

			dataout[k] = op;
		}
		flush_denormal(reg0.d[0]);
		flush_denormal(reg1.d[0]);
	}
}

void biquadunit::process_block_slowlag(float *dataL,float *dataR)
{
	/*if(storage->SSE2) process_block_slowlag_SSE2(dataL,dataR);	
	else*/
	{
		a1.process();	a2.process();
		b0.process();	b1.process();	b2.process();

		int k;
		for(k=0; k<block_size; k++)
		{					
			double input = dataL[k];
			double op;

			op = input*b0.v.d[0] + reg0.d[0];
			reg0.d[0] = input*b1.v.d[0] - a1.v.d[0]*op + reg1.d[0];
			reg1.d[0] = input*b2.v.d[0] - a2.v.d[0]*op;	

			dataL[k] = op;

			input = dataR[k];				
			op = input*b0.v.d[0] + reg0.d[1];
			reg0.d[1] = input*b1.v.d[0] - a1.v.d[0]*op + reg1.d[1];
			reg1.d[1] = input*b2.v.d[0] - a2.v.d[0]*op;	

			dataR[k] = op;
		}
		flush_denormal(reg0.d[0]);
		flush_denormal(reg1.d[0]);
		flush_denormal(reg0.d[1]);
		flush_denormal(reg1.d[1]);
	}
}


void biquadunit::process_block(float *dataL,float *dataR)
{
	/*if(storage->SSE2) process_block_SSE2(dataL,dataR);	
	else*/
	{
		int k;
		for(k=0; k<block_size; k++)
		{	
			a1.process();	a2.process();
			b0.process();	b1.process();	b2.process();

			double input = dataL[k];
			double op;

			op = input*b0.v.d[0] + reg0.d[0];
			reg0.d[0] = input*b1.v.d[0] - a1.v.d[0]*op + reg1.d[0];
			reg1.d[0] = input*b2.v.d[0] - a2.v.d[0]*op;	

			dataL[k] = op;

			input = dataR[k];				
			op = input*b0.v.d[0] + reg0.d[1];
			reg0.d[1] = input*b1.v.d[0] - a1.v.d[0]*op + reg1.d[1];
			reg1.d[1] = input*b2.v.d[0] - a2.v.d[0]*op;	

			dataR[k] = op;
		}
		flush_denormal(reg0.d[0]);
		flush_denormal(reg1.d[0]);
		flush_denormal(reg0.d[1]);
		flush_denormal(reg1.d[1]);
	}
}



void biquadunit::process_block_to(float *dataL,float *dataR, float *dstL,float *dstR)
{
	/*if(storage->SSE2) process_block_to_SSE2(dataL,dataR,dstL,dstR);	
	else*/
	{
		int k;
		for(k=0; k<block_size; k++)
		{	
			a1.process();	a2.process();
			b0.process();	b1.process();	b2.process();

			double input = dataL[k];
			double op;

			op = input*b0.v.d[0] + reg0.d[0];
			reg0.d[0] = input*b1.v.d[0] - a1.v.d[0]*op + reg1.d[0];
			reg1.d[0] = input*b2.v.d[0] - a2.v.d[0]*op;	

			dstL[k] = op;

			input = dataR[k];				
			op = input*b0.v.d[0] + reg0.d[1];
			reg0.d[1] = input*b1.v.d[0] - a1.v.d[0]*op + reg1.d[1];
			reg1.d[1] = input*b2.v.d[0] - a2.v.d[0]*op;	

			dstR[k] = op;
		}
		flush_denormal(reg0.d[0]);
		flush_denormal(reg1.d[0]);
		flush_denormal(reg0.d[1]);
		flush_denormal(reg1.d[1]);
	}
}

void biquadunit::process_block(double *data)
{
	/*if(storage->SSE2) process_block_SSE2(data);	
	else*/
	{
		int k;
		for(k=0; k<block_size; k++)
		{	
			a1.process();	a2.process();
			b0.process();	b1.process();	b2.process();

			double input = data[k];
			double op;

			op = input*b0.v.d[0] + reg0.d[0];
			reg0.d[0] = input*b1.v.d[0] - a1.v.d[0]*op + reg1.d[0];
			reg1.d[0] = input*b2.v.d[0] - a2.v.d[0]*op;	

			data[k] = op;
		}
		flush_denormal(reg0.d[0]);
		flush_denormal(reg1.d[0]);
	}
}
#endif

void biquadunit::process_block_DF2SOFTCLIP(float *data)
{
	{
		int k;
		for(k=0; k<block_size; k++)
		{	
			a1.process();	a2.process();
			b0.process();	b1.process();	b2.process();

			double input = data[k];
			double op;

			op = input*b0.v.d[0] + reg0.d[0];
			op = sin(op-input) + input;
			reg0.d[0] = input*b1.v.d[0] - a1.v.d[0]*op + reg1.d[0];
			reg1.d[0] = input*b2.v.d[0] - a2.v.d[0]*op;	

			data[k] = op;
		}
		flush_denormal(reg0.d[0]);
		flush_denormal(reg1.d[0]);
	}
}

void biquadunit::setBlockSize(int bs)
{
/*	a1.setBlockSize(bs);
	a2.setBlockSize(bs);
	b0.setBlockSize(bs);
	b1.setBlockSize(bs);
	b2.setBlockSize(bs);*/
}

using namespace std;

float biquadunit::plot_magnitude(float f)
{		
	complex <double> ca0(1,0),
					ca1(a1.v.d[0],0),
					ca2(a2.v.d[0],0),
					cb0(b0.v.d[0],0),
					cb1(b1.v.d[0],0),
					cb2(b2.v.d[0],0);
	
	complex<double> i(0,1);
	complex<double> z = exp(-2*3.1415*f*i);	

	complex<double> h = (cb0 + cb1*z + cb2*z*z)/(ca0 + ca1*z + ca2*z*z);

	double r = abs(h);
	return r;
}
