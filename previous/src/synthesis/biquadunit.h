/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#pragma once

#include "dspmodules.h"
#include "globals.h"
#include "mathtables.h"
#include <vt_dsp/basic_dsp.h>

#define USE_SSE2 1

const double minBW = 0.0001;

#if USE_SSE2
const __m128d vl_lp = _mm_set1_pd(0.004);
const __m128d vl_lpinv = _mm_set1_pd(1.0 - 0.004);
#else
const double d_lp = 0.004;
const double d_lpinv = 1.0 - 0.004;
#endif

union alignas(16) vdouble
{
    __m128d v;
    double d[2];
};

class alignas(16) vlag
{
  public:
    vdouble v, target_v;
    vlag() {}

    void init()
    {
        v.v = _mm_setzero_pd();
        target_v.v = _mm_setzero_pd();
    }
    inline void process()
    {
        v.v = _mm_add_sd(_mm_mul_sd(v.v, vl_lpinv), _mm_mul_sd(target_v.v, vl_lp));
        v.v = _mm_unpacklo_pd(v.v, v.v);
    }

    inline void newValue(double f) { target_v.d[0] = f; }
    inline void instantize() { v = target_v; }
    inline void startValue(double f)
    {
        target_v.d[0] = f;
        v.d[0] = f;
    }
};

class alignas(16) biquadunit
{
    vlag a1, a2, b0, b1, b2;
    vdouble reg0, reg1;
    // Align16 double reg0R,reg1R;
  public:
    biquadunit();
    void coeff_LP(double omega, double Q);
    void coeff_LP2B(double omega, double Q);
    void coeff_HP(double omega, double Q);
    void coeff_LP_with_BW(double omega, double BW);
    void coeff_HP_with_BW(double omega, double BW);
    void coeff_BP2A(double omega, double Q);
    void coeff_BP2AQ(double omega, double Q);
    void coeff_PKA(double omega, double Q);
    void coeff_NOTCH(double omega, double Q);
    void coeff_NOTCHQ(double omega, double Q);
    void coeff_peakEQ(double omega, double BW, double gain);
    void coeff_LPHPmorph(double omega, double Q, double morph);
    void coeff_APF(double omega, double Q);
    void coeff_orfanidisEQ(double omega, double BW, double pgaindb, double bgaindb, double zgain);
    void coeff_same_as_last_time();
    void coeff_instantize();
    void coeff_copy(biquadunit *);

    void process_block(float *data);
    // void process_block_SSE2(float *data);
    void process_block(float *dataL, float *dataR);
    // void process_block_SSE2(float *dataL,float *dataR);
    void process_block_to(float *, float *);
    void process_block_to(float *dataL, float *dataR, float *dstL, float *dstR);
    // void process_block_to_SSE2(float *dataL,float *dataR, float *dstL,float *dstR);
    void process_block_slowlag(float *dataL, float *dataR);
    // void process_block_slowlag_SSE2(float *dataL,float *dataR);
    void process_block(double *data);
    // void process_block_SSE2(double *data);

    void process_block_DF2SOFTCLIP(float *data);

    inline float process_sample(float input)
    {
        a1.process();
        a2.process();
        b0.process();
        b1.process();
        b2.process();

        double op;

        op = input * b0.v.d[0] + reg0.d[0];
        reg0.d[0] = input * b1.v.d[0] - a1.v.d[0] * op + reg1.d[0];
        reg1.d[0] = input * b2.v.d[0] - a2.v.d[0] * op;

        return (float)op;
    }

    inline __m128d process_sample_sd(__m128d input)
    {
        a1.process();
        a2.process();
        b0.process();
        b1.process();
        b2.process();
        __m128d op0 = _mm_add_sd(reg0.v, _mm_mul_sd(b0.v.v, input));
        reg0.v = _mm_sub_sd(_mm_add_sd(_mm_mul_sd(b1.v.v, input), reg1.v), _mm_mul_sd(a1.v.v, op0));
        reg1.v = _mm_sub_sd(_mm_mul_sd(b2.v.v, input), _mm_mul_sd(a2.v.v, op0));
        return op0;
    }

    inline void process_sample_nolag(float &L, float &R)
    {
        double op;

        op = L * b0.v.d[0] + reg0.d[0];
        reg0.d[0] = L * b1.v.d[0] - a1.v.d[0] * op + reg1.d[0];
        reg1.d[0] = L * b2.v.d[0] - a2.v.d[0] * op;
        L = (float)op;

        op = R * b0.v.d[0] + reg0.d[1];
        reg0.d[1] = R * b1.v.d[0] - a1.v.d[0] * op + reg1.d[1];
        reg1.d[1] = R * b2.v.d[0] - a2.v.d[0] * op;
        R = (float)op;
    }

    inline void process_sample_nolag(float &L, float &R, float &Lout, float &Rout)
    {
        double op;

        op = L * b0.v.d[0] + reg0.d[0];
        reg0.d[0] = L * b1.v.d[0] - a1.v.d[0] * op + reg1.d[0];
        reg1.d[0] = L * b2.v.d[0] - a2.v.d[0] * op;
        Lout = (float)op;

        op = R * b0.v.d[0] + reg0.d[1];
        reg0.d[1] = R * b1.v.d[0] - a1.v.d[0] * op + reg1.d[1];
        reg1.d[1] = R * b2.v.d[0] - a2.v.d[0] * op;
        Rout = (float)op;
    }

    inline void process_sample_nolag_noinput(float &Lout, float &Rout)
    {
        double op;

        op = reg0.d[0];
        reg0.d[0] = -a1.v.d[0] * op + reg1.d[0];
        reg1.d[0] = -a2.v.d[0] * op;
        Lout = (float)op;

        op = reg0.d[1];
        reg0.d[1] = -a1.v.d[0] * op + reg1.d[1];
        reg1.d[1] = -a2.v.d[0] * op;
        Rout = (float)op;
    }

    // static double calc_omega(double scfreq){ return (2*3.14159265358979323846) * min(0.499,
    // 440*powf(2,scfreq)*samplerate_inv); }
    double calc_omega_from_Hz(double Hz)
    {
        return (2 * 3.14159265358979323846) * Hz * samplerate_inv;
    }
    static double calc_omega(double scfreq)
    {
        return (2 * 3.14159265358979323846) * 440 * note_to_pitch(12 * scfreq) * samplerate_inv;
    }
    static double calc_v1_Q(double reso) { return 1 / (1.02 - limit_range(reso, 0.0, 1.0)); }
    // inline void process_block_stereo(float *dataL,float *dataR);
    // inline void process_block(double *data);
    // inline double process_sample(double sample);
    void setBlockSize(int bs);
    void suspend()
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

    float plot_magnitude(float f);

  protected:
    void set_coef(double a0, double a1, double a2, double b0, double b1, double b2);
    bool first_run;
};
