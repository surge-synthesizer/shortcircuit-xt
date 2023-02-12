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

#include "filter_defs.h"
#include "resampling.h"
#include "util/tools.h"
#include "util/unitconversion.h"
#include <assert.h>

#include <algorithm>
#include <cstring>
using std::max;
using std::min;

extern float SincTableF32[(FIRipol_M + 1) * FIRipol_N];
extern float SincOffsetF32[(FIRipol_M)*FIRipol_N];

const double rmsfilter = 0.004;
const double rmsfilterm1 = 1 - rmsfilter;

/*	LP2A		*/
LP2A::LP2A(float *fp) : filter(fp)
{
    strcpy(filtername, ("LP2"));
    parameter_count = 2;
    strcpy(ctrllabel[0], ("cutoff"));
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], ("resonance"));
    ctrlmode[1] = cm_percent;

    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_percentdef);

    lastparam[0] = -1654816816.0f;
}

void LP2A::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = 5.5f;
    param[1] = 0.0f;
}

void LP2A::calc_coeffs()
{
    if ((lastparam[0] != param[0]) || (lastparam[1] != param[1]))
    {
        lp.coeff_LP(biquadunit::calc_omega(param[0]), biquadunit::calc_v1_Q(param[1]));
        memcpy(lastparam, param, sizeof(lastparam));
    }
}
float LP2A::get_freq_graph(float f) { return lp.plot_magnitude(f); }

void LP2A::process(float *data, float pitch)
{
    calc_coeffs();
    lp.process_block(data);
}

/*	LP2B		*/
LP2B::LP2B(float *fp) : filter(fp)
{
    strcpy(filtername, ("LP2B"));
    parameter_count = 2;
    strcpy(ctrllabel[0], ("cutoff"));
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], ("resonance"));
    ctrlmode[1] = cm_percent;

    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_percentdef);

    lastparam[0] = -1654816816.0f;
}

void LP2B::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = 5.5f;
    param[1] = 0.0f;
}

void LP2B::calc_coeffs()
{
    assert(param);
    if ((lastparam[0] != param[0]) || (lastparam[1] != param[1]))
    {
        lp.coeff_LP2B(biquadunit::calc_omega(param[0]),
                      M_SQRT1_2 / (1 - limit_range(param[1], 0.f, 0.999f)));
        memcpy(lastparam, param, sizeof(lastparam));
    }
}
float LP2B::get_freq_graph(float f) { return lp.plot_magnitude(f); }

void LP2B::process(float *datain, float *dataout, float pitch)
{
    calc_coeffs();
    lp.process_block_to(datain, dataout);
}

void LP2B::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                          float pitch)
{
    calc_coeffs();
    lp.process_block_to(datainL, datainR, dataoutL, dataoutR);
}

/*	superbiquad		*/

enum
{
    sb_LP = 0,
    sb_HP,
    sb_BP,
    sb_Notch,
};

superbiquad::superbiquad(float *fp, int *ip, int mode) : filter(fp, 0, true, ip)
{
    strcpy(filtername, ("superbiquad"));
    parameter_count = 2;
    strcpy(ctrllabel[0], ("cutoff"));
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], ("resonance"));
    ctrlmode[1] = cm_percent;

    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_percentdef);

    initmode = mode;

    assert(iparam);
    lastparam[0] = -1654646816.0f;

    // memset(&d,0,sizeof(d));
}

void superbiquad::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = 5.5f;
    param[1] = 0.0f;
    iparam[0] = initmode;
}

void superbiquad::calc_coeffs()
{
    if ((lastparam[0] != param[0]) || (lastparam[1] != param[1]) || (lastiparam[0] != iparam[0]))
    {
        float q = param[1];
        int n = min(4, (iparam[1] + 1));
        for (int i = 1; i < n; i++)
            q *= param[1];

        switch (iparam[0])
        {
        case sb_LP:
            bq[0].coeff_LP2B(biquadunit::calc_omega(param[0]),
                             M_SQRT1_2 / (1 - limit_range(q, 0.f, 0.999f)));
            break;
        case sb_HP:
            bq[0].coeff_HP(biquadunit::calc_omega(param[0]),
                           M_SQRT1_2 / (1 - limit_range(q, 0.f, 0.999f)));
            break;
        case sb_BP:
            bq[0].coeff_BP2AQ(biquadunit::calc_omega(param[0]),
                              M_SQRT1_2 / (1 - limit_range(q, 0.f, 0.999f)));
            // bq[0].coeff_orfanidisEQ(biquadunit::calc_omega(param[0]),1/(1-limit_range(q,0.f,0.999f)),1,
            // (1/sqrt(2.0)),0);
            break;
        case sb_Notch:
            bq[0].coeff_NOTCHQ(biquadunit::calc_omega(param[0]),
                               M_SQRT1_2 / (1 - limit_range(q, 0.f, 0.999f)));
            break;
        }

        bq[1].coeff_copy(&bq[0]);
        bq[2].coeff_copy(&bq[0]);
        bq[3].coeff_copy(&bq[0]);

        lastparam[0] = param[0];
        lastparam[1] = param[1];
        lastiparam[0] = iparam[0];
    }
}
float superbiquad::get_freq_graph(float f)
{
    float r = bq[0].plot_magnitude(f);
    int n = min(4, (iparam[1] + 1));
    for (int i = 1; i < n; i++)
        r *= bq[i].plot_magnitude(f);
    return r;
}

void superbiquad::process(float *datain, float *dataout, float pitch)
{
    calc_coeffs();
    bq[0].process_block_to(datain, dataout);

    // copy_block(datain,dataout,BLOCK_SIZE_QUAD);
    // bq[0].process_block_DF2SOFTCLIP(dataout);

    int n = min(4, (iparam[1] + 1));
    for (int i = 1; i < n; i++)
    {
        softclip_block(dataout, BLOCK_SIZE_QUAD);
        bq[i].process_block(dataout);
    }
    /*
            coeff_LP2_sd(d,param[0],param[1]);

            double tmp[BLOCK_SIZE];
            for(int i=0; i<BLOCK_SIZE; i++)
            {
                    tmp[i] = datain[i];
            }

            iir_lattice_sd(d,tmp,BLOCK_SIZE);

            for(int i=0; i<BLOCK_SIZE; i++)
            {
                    dataout[i] = tmp[i];
            }*/
}

void superbiquad::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                                 float pitch)
{
    calc_coeffs();
    bq[0].process_block_to(datainL, datainR, dataoutL, dataoutR);

    int n = min(4, (iparam[1] + 1));
    for (int i = 1; i < n; i++)
    {
        softclip_block(dataoutL, BLOCK_SIZE_QUAD);
        softclip_block(dataoutR, BLOCK_SIZE_QUAD);
        bq[i].process_block_to(dataoutL, dataoutR, dataoutL, dataoutR);
    }
}

void superbiquad::suspend()
{
    bq[0].suspend();
    bq[1].suspend();
    bq[2].suspend();
    bq[3].suspend();
}

int superbiquad::get_ip_count() { return 2; }

const char *superbiquad::get_ip_label(int ip_id)
{
    if (ip_id == 0)
        return ("mode");
    else if (ip_id == 1)
        return ("order");
    return ("");
}

int superbiquad::get_ip_entry_count(int ip_id)
{
    if (ip_id == 0)
        return 4;
    return 4;
}

const char *superbiquad::get_ip_entry_label(int ip_id, int c_id)
{
    if (ip_id == 0)
    {
        switch (c_id)
        {
        case 0:
            return ("LP");
        case 1:
            return ("HP");
        case 2:
            return ("BP");
        case 3:
            return ("Notch");
        }
    }
    else if (ip_id == 1)
    {
        switch (c_id)
        {
        case 0:
            return ("2-pole");
        case 1:
            return ("4-pole");
        case 2:
            return ("6-pole");
        case 3:
            return ("8-pole");
        }
    }
    return ("");
}

/*	HP2A		*/

HP2A::HP2A(float *fp) : filter(fp)
{
    strcpy(filtername, ("HP2"));
    parameter_count = 2;
    strcpy(ctrllabel[0], ("cutoff"));
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], ("resonance"));
    ctrlmode[1] = cm_percent;
    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_percentdef);

    lastparam[0] = -1654816816.0f;
}

void HP2A::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = -5.0f;
    param[1] = 0.0f;
}

void HP2A::calc_coeffs()
{
    if ((lastparam[0] != param[0]) || (lastparam[1] != param[1]))
    {
        hp.coeff_HP(biquadunit::calc_omega(param[0]), biquadunit::calc_v1_Q(param[1]));
        memcpy(lastparam, param, sizeof(lastparam));
    }
}
float HP2A::get_freq_graph(float f) { return hp.plot_magnitude(f); }

void HP2A::process(float *datain, float *dataout, float pitch)
{
    calc_coeffs();
    hp.process_block_to(datain, dataout);
}

void HP2A::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                          float pitch)
{
    calc_coeffs();
    hp.process_block_to(datainL, datainR, dataoutL, dataoutR);
}

/*	BP2A		*/

BP2A::BP2A(float *fp) : filter(fp)
{
    strcpy(filtername, ("bandpass (old)"));
    parameter_count = 2;
    strcpy(ctrllabel[0], ("frequency"));
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], ("bandwidth"));
    ctrlmode[1] = cm_percent;
    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_bwdef);

    lastparam[0] = -1654646816.0f;
}

void BP2A::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = 0.5f;
    param[1] = 0.2f;
}

void BP2A::calc_coeffs()
{
    if ((lastparam[0] != param[0]) || (lastparam[1] != param[1]))
    {
        bq.coeff_orfanidisEQ(biquadunit::calc_omega(param[0]),
                             1 / (1 / (0.02 + 30 * param[1] * param[1])), 1, M_SQRT1_2, 0);
        memcpy(lastparam, param, sizeof(lastparam));
    }
}
float BP2A::get_freq_graph(float f) { return bq.plot_magnitude(f); }

void BP2A::process(float *data, float pitch)
{
    calc_coeffs();
    bq.process_block(data);
}

/*	BP2B		*/

BP2B::BP2B(float *fp) : filter(fp)
{
    strcpy(filtername, ("bandpass"));
    parameter_count = 2;
    strcpy(ctrllabel[0], ("frequency"));
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], ("bandwidth"));
    ctrlmode[1] = cm_eq_bandwidth;
    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_bwdef);

    lastparam[0] = -1654646816.0f;
}

void BP2B::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = 0.5f;
    param[1] = 0.2f;
}

void BP2B::calc_coeffs()
{
    assert(param);
    if ((lastparam[0] != param[0]) || (lastparam[1] != param[1]))
    {
        bq.coeff_orfanidisEQ(biquadunit::calc_omega(param[0]), param[1], 1, (1 / sqrt(2.0)), 0);
        memcpy(lastparam, param, sizeof(lastparam));
    }
}
float BP2B::get_freq_graph(float f) { return bq.plot_magnitude(f); }

void BP2B::process(float *datain, float *dataout, float pitch)
{
    calc_coeffs();
    bq.process_block_to(datain, dataout);
}

void BP2B::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                          float pitch)
{
    calc_coeffs();
    bq.process_block_to(datainL, datainR, dataoutL, dataoutR);
}

/*	BP2AD		*/

BP2AD::BP2AD(float *fp) : filter(fp)
{
    strcpy(filtername, ("dual bandpass"));
    parameter_count = 4;
    strcpy(ctrllabel[0], ("frequency"));
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], ("bandwidth"));
    ctrlmode[1] = cm_percent;
    strcpy(ctrllabel[2], ("abs. offset"));
    ctrlmode[2] = cm_frequency0_2k;
    strcpy(ctrllabel[3], ("rel. offset"));
    ctrlmode[3] = cm_mod_freq;
    lastparam[0] = -1654646816.0f;

    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_percentdef);
    strcpy(ctrlmode_desc[2], str_percentdef); // todo, this one is weird
    strcpy(ctrlmode_desc[3], ("f,-12,0.01,12,0,oct"));
}

BP2AD::~BP2AD() {}

void BP2AD::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = 0.5f;
    param[1] = 0.2f;
    param[2] = 0.0f;
    param[3] = 0.0f;
}

void BP2AD::calcfreq(float *a, float *b)
{
    assert(param);
    float freq = 440 * powf(2, param[0]);
    float a_diff = 2000 * param[2];
    float r_diff = powf(2, param[3]);
    float freq0 = freq * r_diff + a_diff;
    float freq1 = freq / r_diff - a_diff;
    freq0 = min(20000.f, max(freq0, 20.f));
    *a = (float)(1.442695040889f * log(freq0 / 440));
    freq1 = min(20000.f, max(freq1, 20.f));
    *b = (float)(1.442695040889f * log(freq1 / 440));
    lastparam[0] = -1656816816.0f;
}

void BP2AD::calc_coeffs()
{
    assert(param);
    if ((lastparam[0] != param[0]) || (lastparam[1] != param[1]) || (lastparam[2] != param[2]) ||
        (lastparam[3] != param[3]))
    {
        float freq0, freq1;
        calcfreq(&freq0, &freq1);
        bq[0].coeff_orfanidisEQ(biquadunit::calc_omega(freq0),
                                1 / (1 / (0.02 + 30 * param[1] * param[1])), 1, M_SQRT1_2, 0);
        bq[1].coeff_orfanidisEQ(biquadunit::calc_omega(freq1),
                                1 / (1 / (0.02 + 30 * param[1] * param[1])), 1, M_SQRT1_2, 0);
        memcpy(lastparam, param, sizeof(lastparam));
    }
}

float BP2AD::get_freq_graph(float f) { return bq[0].plot_magnitude(f) + bq[1].plot_magnitude(f); }

void BP2AD::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                           float pitch)
{
    calc_coeffs();
    float d alignas(16)[2][BLOCK_SIZE];
    bq[0].process_block_to(datainL, datainR, d[0], d[1]);
    bq[1].process_block_to(datainL, datainR, dataoutL, dataoutR);
    accumulate_block(d[0], dataoutL, BLOCK_SIZE_QUAD);
    accumulate_block(d[1], dataoutR, BLOCK_SIZE_QUAD);
}

void BP2AD::process(float *datain, float *dataout, float pitch)
{
    calc_coeffs();
    float d alignas(16)[BLOCK_SIZE];
    bq[0].process_block_to(datain, d);
    bq[1].process_block_to(datain, dataout);
    accumulate_block(d, dataout, BLOCK_SIZE_QUAD);
}

/*	PKA		*/

PKA::PKA(float *fp) : filter(fp)
{
    strcpy(filtername, ("peak"));
    parameter_count = 2;
    strcpy(ctrllabel[0], ("frequency"));
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], ("resonance"));
    ctrlmode[1] = cm_percent;
    lastparam[0] = -1654616816.0f;

    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_percentdef);
}

void PKA::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = 0.0f;
    param[1] = 0.2f;
}

void PKA::calc_coeffs()
{
    assert(param);
    if ((lastparam[0] != param[0]) || (lastparam[1] != param[1]))
    {
        float reso = clamp01(param[1]);
        float Q = (0.1 + 10 * reso * reso);
        bq.coeff_orfanidisEQ(biquadunit::calc_omega(param[0]), 1 / Q, Q, Q * M_SQRT1_2, 0);
        memcpy(lastparam, param, sizeof(lastparam));
    }
}
float PKA::get_freq_graph(float f) { return bq.plot_magnitude(f); }

void PKA::process(float *datain, float *dataout, float pitch)
{
    calc_coeffs();
    bq.process_block_to(datain, dataout);
}

void PKA::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                         float pitch)
{
    calc_coeffs();
    bq.process_block_to(datainL, datainR, dataoutL, dataoutR);
}

/*	PKAD		*/

PKAD::PKAD(float *fp) : filter(fp)
{
    strcpy(filtername, ("dual peak"));
    parameter_count = 4;
    strcpy(ctrllabel[0], ("frequency"));
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], ("resonance"));
    ctrlmode[1] = cm_percent;
    strcpy(ctrllabel[2], ("abs. offset"));
    ctrlmode[2] = cm_frequency0_2k;
    strcpy(ctrllabel[3], ("rel. offset"));
    ctrlmode[3] = cm_mod_freq;
    lastparam[0] = -1654616816.0f;

    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_percentdef);
    strcpy(ctrlmode_desc[2], str_percentdef); // todo, this one is weird
    strcpy(ctrlmode_desc[3], ("f,-12,0.01,12,0,oct"));
}

PKAD::~PKAD() {}

void PKAD::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = 0.5f;
    param[1] = 0.2f;
    param[2] = 0.0f;
    param[3] = 0.0f;
}

void PKAD::calcfreq(float *a, float *b)
{
    assert(param);
    float freq = 440 * powf(2, param[0]);
    float a_diff = 2000 * param[2];
    float r_diff = powf(2, param[3]);
    float freq0 = freq * r_diff + a_diff;
    float freq1 = freq / r_diff - a_diff;
    freq0 = min(20000.f, max(freq0, 20.f));
    *a = (float)(1.442695040889f * log(freq0 / 400));
    freq1 = min(20000.f, max(freq1, 20.f));
    *b = (float)(1.442695040889f * log(freq1 / 440));
}

void PKAD::calc_coeffs()
{
    assert(param);
    if ((lastparam[0] != param[0]) || (lastparam[1] != param[1]) || (lastparam[2] != param[2]) ||
        (lastparam[3] != param[3]))
    {
        float freq0, freq1;
        calcfreq(&freq0, &freq1);
        float reso = clamp01(param[1]);
        float Q = (0.1 + 10 * reso * reso);
        bq[0].coeff_orfanidisEQ(biquadunit::calc_omega(freq0), 1 / Q, Q, Q * M_SQRT1_2, 0);
        bq[1].coeff_orfanidisEQ(biquadunit::calc_omega(freq1), 1 / Q, Q, Q * M_SQRT1_2, 0);
        memcpy(lastparam, param, sizeof(lastparam));
    }
}
float PKAD::get_freq_graph(float f) { return bq[0].plot_magnitude(f) + bq[1].plot_magnitude(f); }

void PKAD::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                          float pitch)
{
    calc_coeffs();
    float d alignas(16)[2][BLOCK_SIZE];
    bq[0].process_block_to(datainL, datainR, d[0], d[1]);
    bq[1].process_block_to(datainL, datainR, dataoutL, dataoutR);
    accumulate_block(d[0], dataoutL, BLOCK_SIZE_QUAD);
    accumulate_block(d[1], dataoutR, BLOCK_SIZE_QUAD);
}

void PKAD::process(float *datain, float *dataout, float pitch)
{
    calc_coeffs();
    float d alignas(16)[BLOCK_SIZE];
    bq[0].process_block_to(datain, d);
    bq[1].process_block_to(datain, dataout);
    accumulate_block(d, dataout, BLOCK_SIZE_QUAD);
}
/*	NOTCH		*/

NOTCH::NOTCH(float *fp) : filter(fp)
{
    strcpy(filtername, ("notch"));
    parameter_count = 2;
    strcpy(ctrllabel[0], ("frequency"));
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], ("bandwidth"));
    ctrlmode[1] = cm_percent;
    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_percentdef);
    lastparam[0] = -1654646816.0f;
}

void NOTCH::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = 0.5f;
    param[1] = 0.2f;
}

void NOTCH::calc_coeffs()
{
    if ((lastparam[0] != param[0]) || (lastparam[1] != param[1]))
    {
        bq.coeff_NOTCH(biquadunit::calc_omega(param[0]), param[1]);
        lastparam[0] = param[0];
        lastparam[1] = param[1];
    }
}
float NOTCH::get_freq_graph(float f)
{
    calc_coeffs();
    return bq.plot_magnitude(f);
}

void NOTCH::process(float *datain, float *dataout, float pitch)
{
    calc_coeffs();
    bq.process_block_to(datain, dataout);
}

void NOTCH::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                           float pitch)
{
    calc_coeffs();
    bq.process_block_to(datainL, datainR, dataoutL, dataoutR);
}

/*	LP + HP serial		*/

LPHP_ser::LPHP_ser(float *fp) : filter(fp)
{
    strcpy(filtername, ("LP2+HP2 serial"));
    parameter_count = 4;
    strcpy(ctrllabel[0], ("HP freq"));
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], ("HP reso"));
    ctrlmode[1] = cm_percent;
    strcpy(ctrllabel[2], ("LP freq"));
    ctrlmode[2] = cm_frequency_audible;
    strcpy(ctrllabel[3], ("LP reso"));
    ctrlmode[3] = cm_percent;
    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_percentdef);
    strcpy(ctrlmode_desc[2], str_freqdef);
    strcpy(ctrlmode_desc[3], str_percentdef);
    lastparam[0] = -1654646816.0f;
}

LPHP_ser::~LPHP_ser() {}

void LPHP_ser::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = -1.f;
    param[1] = 0.0f;
    param[2] = 1.0f;
    param[3] = 0.0f;
}

void LPHP_ser::calc_coeffs()
{
    bq[0].coeff_HP(biquadunit::calc_omega(param[0]),
                   M_SQRT1_2 / (1 - limit_range(param[1], 0.f, 0.999f)));
    bq[1].coeff_LP2B(biquadunit::calc_omega(param[2]),
                     M_SQRT1_2 / (1 - limit_range(param[3], 0.f, 0.999f)));
}
float LPHP_ser::get_freq_graph(float f)
{
    return bq[0].plot_magnitude(f) * bq[1].plot_magnitude(f);
}

void LPHP_ser::process(float *datain, float *dataout, float pitch)
{
    calc_coeffs();
    bq[0].process_block_to(datain, dataout);
    bq[1].process_block(dataout);
}

void LPHP_ser::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                              float pitch)
{
    calc_coeffs();
    bq[0].process_block_to(datainL, datainR, dataoutL, dataoutR);
    bq[1].process_block_to(dataoutL, dataoutR, dataoutL, dataoutR);
}

/*	LP + HP parallel		*/

LPHP_par::LPHP_par(float *fp) : filter(fp)
{
    strcpy(filtername, ("LP2+HP2 parallel"));
    parameter_count = 4;
    strcpy(ctrllabel[0], ("LP freq"));
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], ("LP reso"));
    ctrlmode[1] = cm_percent;
    strcpy(ctrllabel[2], ("HP freq"));
    ctrlmode[2] = cm_frequency_audible;
    strcpy(ctrllabel[3], ("HP reso"));
    ctrlmode[3] = cm_percent;
    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_percentdef);
    strcpy(ctrlmode_desc[2], str_freqdef);
    strcpy(ctrlmode_desc[3], str_percentdef);
    lastparam[0] = -1654646816.0f;
}

LPHP_par::~LPHP_par() {}

void LPHP_par::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = -2.f;
    param[1] = 0.0f;
    param[2] = 2.0f;
    param[3] = 0.0f;
}

void LPHP_par::calc_coeffs()
{
    bq[0].coeff_LP(biquadunit::calc_omega(param[0]),
                   M_SQRT1_2 / (1 - limit_range(param[1], 0.f, 0.999f)));
    bq[1].coeff_HP(biquadunit::calc_omega(param[2]),
                   M_SQRT1_2 / (1 - limit_range(param[3], 0.f, 0.999f)));
}
float LPHP_par::get_freq_graph(float f)
{
    return bq[0].plot_magnitude(f) + bq[1].plot_magnitude(f);
}

void LPHP_par::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                              float pitch)
{
    calc_coeffs();
    float d alignas(16)[2][BLOCK_SIZE];
    bq[0].process_block_to(datainL, datainR, d[0], d[1]);
    bq[1].process_block_to(datainL, datainR, dataoutL, dataoutR);
    accumulate_block(d[0], dataoutL, BLOCK_SIZE_QUAD);
    accumulate_block(d[1], dataoutR, BLOCK_SIZE_QUAD);
}

void LPHP_par::process(float *datain, float *dataout, float pitch)
{
    calc_coeffs();
    float d alignas(16)[BLOCK_SIZE];
    bq[0].process_block_to(datain, d);
    bq[1].process_block_to(datain, dataout);
    accumulate_block(d, dataout, BLOCK_SIZE_QUAD);
}
