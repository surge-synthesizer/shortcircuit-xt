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
#include "mathtables.h"
#include "resampling.h"
#include "util/tools.h"
#include "util/unitconversion.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <math.h>
using std::max;
using std::min;

extern float SincTableF32[(FIRipol_M + 1) * FIRipol_N];
extern float SincOffsetF32[(FIRipol_M)*FIRipol_N];

/*	BF bitfucker		*/

BF::BF(float *fp) : filter(fp)
{
    strcpy(filtername, ("bitfucker"));
    parameter_count = 5;
    strcpy(ctrllabel[0], ("samplerate"));
    ctrlmode[0] = cm_frequency_samplerate;
    strcpy(ctrllabel[1], ("bitdepth"));
    ctrlmode[1] = cm_bitdepth_16;
    strcpy(ctrllabel[2], ("zeropoint"));
    ctrlmode[2] = cm_percent;
    strcpy(ctrllabel[3], ("cutoff"));
    ctrlmode[3] = cm_frequency_audible;
    strcpy(ctrllabel[4], ("resonance"));
    ctrlmode[4] = cm_percent;
    for (int c = 0; c < 2; c++)
    {
        time[c] = 0;
        postslew[c] = 0;
        level[c] = 0;
    }

    strcpy(ctrlmode_desc[0], ("f,-5,0.01,7.5,5,Hz"));
    strcpy(ctrlmode_desc[1], str_percentdef);
    strcpy(ctrlmode_desc[2], str_percentdef);
    strcpy(ctrlmode_desc[3], str_freqdef);
    strcpy(ctrlmode_desc[4], str_percentdef);

    lp_params[0] = 1;
    lp_params[1] = 0;
    if (fp)
    {
        lp_params[0] = fp[3];
        lp_params[1] = fp[4];
    }
    // lp = new LP2B(lp_params);
#if MAC
    lp = (LP2B *)malloc(sizeof(LP2B));
#else
#if WIN
    lp = (LP2B *)_aligned_alloc(16, sizeof(LP2B));
#else
    lp = (LP2B *)std::aligned_alloc(16, sizeof(LP2B));
#endif
#endif

    new (lp) LP2B(lp_params);
}

BF::~BF()
{
    // delete lp;
    free(lp);
}

void BF::init_params()
{
    if (!param)
        return;
    // preset values
    param[0] = 7.5f;
    param[1] = 1.0f;
    param[2] = 0.0f;
    param[3] = 5.5f;
    param[4] = 0.0f;
}

void BF::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch)
{
    float t = samplerate_inv * 440 * note_to_pitch(12 * param[0]);
    float bd = 16.f * std::min(1.f, std::max(0.f, param[1]));
    float b = powf(2, bd), b_inv = 1 / b;

    lp_params[0] = param[3];
    lp_params[1] = param[4];

    int k;
    for (k = 0; k < BLOCK_SIZE; k++)
    {
        float inputL = datainL[k];
        float inputR = datainR[k];
        time[0] -= t;
        time[1] -= t;
        if (time[0] < 0.f)
        {
            float val = inputL * b;
            val += param[2];
            level[0] = (float)((int)val) * b_inv;
            time[0] += 1.0f;
            time[0] = std::max(time[0], 0.f);
        }
        if (time[1] < 0.f)
        {
            float val = inputR * b;
            val += param[2];
            level[1] = (float)((int)val) * b_inv;
            time[1] += 1.0f;
            time[1] = std::max(time[1], 0.f);
        }
        dataoutL[k] = level[0];
        dataoutR[k] = level[1];
    }
    lp->process_stereo(dataoutL, dataoutR, dataoutL, dataoutR, 0);
}
void BF::process(float *datain, float *dataout, float pitch)
{
    float t = samplerate_inv * 440 * note_to_pitch(12 * param[0]);
    float bd = 16.f * std::min(1.f, std::max(0.f, param[1]));

    float b = powf(2.f, bd), b_inv = 1 / b;

    lp_params[0] = param[3];
    lp_params[1] = param[4];

    int k;
    for (k = 0; k < BLOCK_SIZE; k++)
    {
        float inputL = datain[k];
        time[0] -= t;
        if (time[0] < 0.f)
        {
            float val = inputL * b;
            val += param[2];
            level[0] = (float)((int)val) * b_inv;
            time[0] += 1.0f;
            time[0] = max(time[0], 0.f);
        }

        dataout[k] = level[0];
    }
    lp->process(dataout, dataout, 0);
}

/*	OD overdrive		*/

OD::OD(float *fp) : filter(fp)
{
    strcpy(filtername, ("murder OD"));
    parameter_count = 5;
    strcpy(ctrllabel[0], ("drive"));
    ctrlmode[0] = cm_percent;
    strcpy(ctrllabel[1], ("EQ freq"));
    ctrlmode[1] = cm_frequency_audible;
    strcpy(ctrllabel[2], ("EQ gain"));
    ctrlmode[2] = cm_decibel;
    // eq Q
    strcpy(ctrllabel[3], ("Post cutoff"));
    ctrlmode[3] = cm_frequency_audible;
    strcpy(ctrllabel[4], ("Post reso"));
    ctrlmode[4] = cm_percent;
    time = 0;
    postslew = 0;
    level = 0;

    lp_params[0] = 5.5;
    lp_params[1] = 0;
    pk_params[0] = 0.5;
    pk_params[1] = 0.3;
    if (fp)
    {
        lp_params[0] = fp[3];
        lp_params[1] = fp[4];
        pk_params[0] = fp[1];
    }
    lp2a = std::make_unique<LP2A>(lp_params);
    peak = std::make_unique<PKA>(pk_params);
}

OD::~OD() {}

void OD::init_params()
{
    if (!param)
        return;
    // preset values
    param[0] = 0.0f;
    param[1] = 0.5f;
    param[2] = 0.0f;
    param[3] = 5.5f;
    param[4] = 0.0f;
}

void OD::process(float *data, float pitch)
{
    lp_params[0] = param[3];
    lp_params[1] = param[4];
    pk_params[0] = param[1];

    float pk_amp = db_to_linear(param[2]) - 1;
    float drive = 1 - param[0];
    drive = std::max(0.f, std::min(drive, 1.f));

    // peak filter
    float pkbuffer[BLOCK_SIZE];
    memcpy(pkbuffer, data, BLOCK_SIZE * sizeof(float));
    peak->process(pkbuffer, pkbuffer, 0);

    int k;
    for (k = 0; k < BLOCK_SIZE; k++)
    {
        float input = data[k] + pkbuffer[k] * pk_amp;

        int *ip_intcast = (int *)&input;
        int polarity = *ip_intcast & 0x80000000;
        (*ip_intcast) = (*ip_intcast) & 0x7fffffff;
        input = powf(input, drive);
        (*ip_intcast) = (*ip_intcast) | polarity;

        data[k] = input;
    }
    lp2a->process(data, data, 0);
}

/*	treemonster		*/

treemonster::treemonster(float *fp, int *ip) : filter(fp, 0, true, ip)
{
    strcpy(filtername, ("treemonster"));
    parameter_count = 4;
    strcpy(ctrllabel[0], ("threshold"));
    ctrlmode[0] = cm_mod_decibel;
    strcpy(ctrllabel[1], ("pitch"));
    ctrlmode[1] = cm_mod_pitch;
    strcpy(ctrllabel[2], ("mode blend"));
    ctrlmode[2] = cm_percent;
    strcpy(ctrllabel[3], ("lo/hi cut"));
    ctrlmode[3] = cm_frequency_audible;

    strcpy(ctrlmode_desc[0], str_dbmoddef);
    strcpy(ctrlmode_desc[1], str_mpitch);
    strcpy(ctrlmode_desc[2], str_percentdef);
    strcpy(ctrlmode_desc[3], str_freqdef);

    lastval[0] = 0.f;
    lastval[1] = 0.f;
    length[0] = 0.f;
    length[1] = 0.f;
    osc[0].set_rate(0.0);
    osc[1].set_rate(0.0);
    gain[0].set_blocksize(BLOCK_SIZE);
    gain[1].set_blocksize(BLOCK_SIZE);
}

treemonster::~treemonster() {}

void treemonster::init_params()
{
    if (!param)
        return;
    // preset values
    param[0] = -96.0f;
    param[1] = 0.0f;
    param[2] = 0.0f;
    param[3] = -5.0f;
    iparam[0] = 0;
}

void treemonster::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                                 float pitch)
{
    // TODO powf is used, it is not fast
    // makes it possible to change the pitch continuously and not only on triggers
    gain[0].set_target(limit_range(param[2], 0.f, 1.f));

    float tbuf alignas(16)[2][BLOCK_SIZE];
    if (iparam[0])
        locut.coeff_LP2B(biquadunit::calc_omega(param[3]), 0.707);
    else
        locut.coeff_HP(biquadunit::calc_omega(param[3]), 0.707);

    locut.process_block_to(datainL, datainR, tbuf[0], tbuf[1]);

    for (int k = 0; k < BLOCK_SIZE; k++)
    {
        if ((lastval[0] < 0.f) && (tbuf[0][k] >= 0.f))
        {
            if (tbuf[0][k] > db_to_linear(param[0]))
                osc[0].set_rate((M_PI / std::max(2.f, length[0])) *
                                powf(2.0, param[1] * (1 / 12.f)));
            length[0] = 0.f;
        }
        if ((lastval[1] < 0.f) && (tbuf[1][k] >= 0.f))
        {
            if (tbuf[1][k] > db_to_linear(param[0]))
                osc[1].set_rate((M_PI / std::max(2.f, length[1])) *
                                powf(2.0, param[1] * (1 / 12.f)));
            length[1] = 0.f;
        }
        osc[0].process();
        osc[1].process();
        dataoutL[k] = osc[0].r;
        dataoutR[k] = osc[1].r;
        length[0] += 1.0f;
        length[1] += 1.0f;
        lastval[0] = tbuf[0][k];
        lastval[1] = tbuf[1][k];
    }

    mul_block(dataoutL, datainL, tbuf[0], BLOCK_SIZE_QUAD);
    mul_block(dataoutR, datainR, tbuf[1], BLOCK_SIZE_QUAD);

    gain[0].fade_2_blocks_to(tbuf[0], dataoutL, tbuf[1], dataoutR, dataoutL, dataoutR,
                             BLOCK_SIZE_QUAD);
}

void treemonster::process(float *datain, float *dataout, float pitch)
{
    gain[0].set_target(limit_range(param[2], 0.f, 1.f));

    float tbuf alignas(16)[BLOCK_SIZE];
    if (iparam[0])
        locut.coeff_LP2B(biquadunit::calc_omega(param[3]), 0.707);
    else
        locut.coeff_HP(biquadunit::calc_omega(param[3]), 0.707);
    locut.process_block_to(datain, tbuf);

    for (int k = 0; k < BLOCK_SIZE; k++)
    {
        if ((lastval[0] < 0.f) && (tbuf[k] >= 0.f))
        {
            if (tbuf[k] > db_to_linear(param[0]))
                osc[0].set_rate((M_PI / max(2.f, length[0])) * powf(2.0, param[1] * (1 / 12.f)));
            length[0] = 0.f;
        }
        osc[0].process();
        dataout[k] = osc[0].r;
        length[0] += 1.0f;
        lastval[0] = tbuf[k];
    }
    mul_block(dataout, datain, tbuf, BLOCK_SIZE_QUAD);
    gain[0].fade_block_to(tbuf, dataout, dataout, BLOCK_SIZE_QUAD);
}

int treemonster::get_ip_count() { return 1; }

const char *treemonster::get_ip_label(int ip_id)
{
    if (ip_id == 0)
        return ("filter");
    return ("");
}

int treemonster::get_ip_entry_count(int ip_id) { return 2; }
const char *treemonster::get_ip_entry_label(int ip_id, int c_id)
{
    if (ip_id == 0)
    {
        switch (c_id)
        {
        case 0:
            return ("locut");
        case 1:
            return ("hicut");
        }
    }
    return ("");
}

/*	clipper		*/

clipper::clipper(float *fp) : filter(fp)
{
    strcpy(filtername, ("clipper"));
    parameter_count = 2;
    strcpy(ctrllabel[0], ("drive"));
    ctrlmode[0] = cm_mod_decibel;
    strcpy(ctrllabel[1], ("threshold"));
    ctrlmode[1] = cm_mod_decibel;

    strcpy(ctrlmode_desc[0], str_dbmoddef);
    strcpy(ctrlmode_desc[1], str_dbmoddef);
}

clipper::~clipper() {}

void clipper::init_params()
{
    if (!param)
        return;
    // preset values
    param[0] = 0.0f;
    param[1] = -6.0f;
}

void clipper::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                             float pitch)
{
    pregain.set_target(db_to_linear(param[0] - param[1]));
    postgain.set_target(db_to_linear(param[1]));

    pregain.multiply_2_blocks_to(datainL, datainR, dataoutL, dataoutR, BLOCK_SIZE_QUAD);
    hardclip_block(dataoutL, BLOCK_SIZE_QUAD);
    hardclip_block(dataoutR, BLOCK_SIZE_QUAD);
    postgain.multiply_2_blocks(dataoutL, dataoutR, BLOCK_SIZE_QUAD);
}

void clipper::process(float *datain, float *dataout, float pitch)
{
    pregain.set_target(db_to_linear(param[0] - param[1]));
    postgain.set_target(db_to_linear(param[1]));

    pregain.multiply_block_to(datain, dataout, BLOCK_SIZE_QUAD);
    hardclip_block(dataout, BLOCK_SIZE_QUAD);
    postgain.multiply_block(dataout, BLOCK_SIZE_QUAD);
}

/*	distortion		*/

fdistortion::fdistortion(float *fp) : filter(fp), pre(4, false), post(4, false)
{
    strcpy(filtername, ("distortion"));
    parameter_count = 1;
    strcpy(ctrllabel[0], ("drive"));
    ctrlmode[0] = cm_mod_decibel;

    strcpy(ctrlmode_desc[0], str_dbmoddef);

    gain.set_blocksize(BLOCK_SIZE << 1); // 2X oversampling
}

fdistortion::~fdistortion() {}

void fdistortion::init_params()
{
    if (!param)
        return;
    // preset values
    param[0] = 0.0f;
}

void fdistortion::process(float *datain, float *dataout, float pitch)
{
    process_stereo(datain, datain, dataout, 0, pitch);
}
void fdistortion::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                                 float pitch)
{
    gain.set_target(2.f * db_to_linear(param[0]));
    float osbuffer alignas(16)[2][BLOCK_SIZE * 2];

    pre.process_block_U2(datainL, datainR, osbuffer[0], osbuffer[1], BLOCK_SIZE << 1);

    if (dataoutR)
    {
        gain.multiply_2_blocks(osbuffer[0], osbuffer[1], BLOCK_SIZE_QUAD << 1);
        tanh7_block(osbuffer[0], BLOCK_SIZE_QUAD << 1);
        tanh7_block(osbuffer[1], BLOCK_SIZE_QUAD << 1);
    }
    else
    {
        gain.multiply_block(osbuffer[0], BLOCK_SIZE_QUAD << 1);
        tanh7_block(osbuffer[0], BLOCK_SIZE_QUAD << 1);
    }

    post.process_block_D2(osbuffer[0], osbuffer[1], BLOCK_SIZE << 1, dataoutL, dataoutR);
}

/*void fdistortion::process(float *data, float pitch)
{
        gain.newValue(db_to_linear(param[0]));

        float osbuffer[BLOCK_SIZE*2];

        int k;
        // upsample
        for(k=0; k<BLOCK_SIZE; k++)
        {
                osbuffer[(k<<1)] = data[k];
                osbuffer[(k<<1)+1] = data[k];
        }

        for(k=0; k<(BLOCK_SIZE<<1); k++)
        {
                osbuffer[k] = pre.process(osbuffer[k]);
                //osbuffer[k] = tanh_fast(gain.v*osbuffer[k]);
                osbuffer[k] = tanh_turbo(gain.v*osbuffer[k]);
                osbuffer[k] = post.process(osbuffer[k]);
                gain.process();
        }

        // downsample
        for(k=0; k<BLOCK_SIZE; k++)
        {
                data[k] = osbuffer[(k<<1)];
        }
        pre.flush_denormals();
        post.flush_denormals();
}*/

/*	microgate		*/

microgate::microgate(float *fp) : filter(fp)
{
    strcpy(filtername, ("microgate"));
    parameter_count = 4;
    strcpy(ctrllabel[0], ("hold"));
    ctrlmode[0] = cm_time1s;
    strcpy(ctrllabel[1], ("loop"));
    ctrlmode[1] = cm_percent;
    strcpy(ctrllabel[2], ("threshold"));
    ctrlmode[2] = cm_decibel;
    strcpy(ctrllabel[3], ("reduction"));
    ctrlmode[3] = cm_decibel;

    strcpy(ctrlmode_desc[0], str_timedef);
    strcpy(ctrlmode_desc[1], str_percentdef);
    strcpy(ctrlmode_desc[2], str_dbdef);
    strcpy(ctrlmode_desc[3], str_dbdef);

    gate_state = false;
    holdtime = 0;
    gate_zc_sync[0] = false;
    onesampledelay[0] = -1;
    bufpos[0] = 0;
    buflength[0] = 0;
    is_recording[0] = false;
    gate_zc_sync[1] = false;
    onesampledelay[1] = -1;
    bufpos[1] = 0;
    buflength[1] = 0;
    is_recording[1] = false;
    int i;
    for (i = 0; i < mg_bufsize; i++)
    {
        loopbuffer[0][i] = 0.f;
        loopbuffer[1][i] = 0.f;
    }
}

microgate::~microgate() {}

void microgate::init_params()
{
    if (!param)
        return;
    // preset values
    param[0] = -3.0f;
    param[1] = 0.5f;
    param[2] = -12.0f;
    param[3] = -96.0f;
}

void microgate::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                               float pitch)
{
    float threshold = db_to_linear(param[2]);
    float reduction = db_to_linear(param[3]);
    int ihtime = (int)(float)(samplerate * note_to_pitch(12 * param[0]));

    copy_block(datainL, dataoutL, BLOCK_SIZE_QUAD);
    copy_block(datainR, dataoutR, BLOCK_SIZE_QUAD);

    int k;
    for (k = 0; k < BLOCK_SIZE; k++)
    {
        float input = max(fabs(datainL[k]), fabs(datainR[k]));

        if ((input > threshold) && !gate_state)
        {
            holdtime = ihtime;
            gate_state = true;
            is_recording[0] = true;
            bufpos[0] = 0;
            is_recording[1] = true;
            bufpos[1] = 0;
        }

        if (holdtime < 0)
            gate_state = false;

        if ((!(onesampledelay[0] * datainL[k] > 0)) && (datainL[k] > 0))
        {
            gate_zc_sync[0] = gate_state;
            int looplimit = (int)(float)(4 + 3900 * param[1] * param[1]);
            if (bufpos[0] > looplimit)
            {
                is_recording[0] = false;
                buflength[0] = bufpos[0];
            }
        }
        if ((!(onesampledelay[1] * datainR[k] > 0)) && (datainR[k] > 0))
        {
            gate_zc_sync[1] = gate_state;
            int looplimit = (int)(float)(4 + 3900 * param[1] * param[1]);
            if (bufpos[1] > looplimit)
            {
                is_recording[1] = false;
                buflength[1] = bufpos[1];
            }
        }
        onesampledelay[0] = datainL[k];
        onesampledelay[1] = datainR[k];

        if (gate_zc_sync[0])
        {
            if (is_recording[0])
            {
                loopbuffer[0][bufpos[0]++ & (mg_bufsize - 1)] = datainL[k];
            }
            else
            {
                dataoutL[k] = loopbuffer[0][bufpos[0] & (mg_bufsize - 1)];
                bufpos[0]++;
                if (bufpos[0] >= buflength[0])
                    bufpos[0] = 0;
            }
        }
        else
            dataoutL[k] *= reduction;

        if (gate_zc_sync[1])
        {
            if (is_recording[1])
            {
                loopbuffer[1][bufpos[1]++ & (mg_bufsize - 1)] = datainR[k];
            }
            else
            {
                dataoutR[k] = loopbuffer[1][bufpos[1] & (mg_bufsize - 1)];
                bufpos[1]++;
                if (bufpos[1] >= buflength[1])
                    bufpos[1] = 0;
            }
        }
        else
            dataoutR[k] *= reduction;

        holdtime--;
    }
}

void microgate::process(float *datain, float *dataout, float pitch)
{
    float threshold = db_to_linear(param[2]);
    float reduction = db_to_linear(param[3]);
    int ihtime = (int)(float)(samplerate * note_to_pitch(12 * param[0]));

    copy_block(datain, dataout, BLOCK_SIZE_QUAD);

    int k;
    for (k = 0; k < BLOCK_SIZE; k++)
    {
        float input = fabs(datain[k]);

        if ((input > threshold) && !gate_state)
        {
            holdtime = ihtime;
            gate_state = true;
            is_recording[0] = true;
            bufpos[0] = 0;
        }

        if (holdtime < 0)
            gate_state = false;

        if ((!(onesampledelay[0] * datain[k] > 0)) && (datain[k] > 0))
        {
            gate_zc_sync[0] = gate_state;
            int looplimit = (int)(float)(4 + 3900 * param[1] * param[1]);
            if (bufpos[0] > looplimit)
            {
                is_recording[0] = false;
                buflength[0] = bufpos[0];
            }
        }
        onesampledelay[0] = datain[k];

        if (gate_zc_sync[0])
        {
            if (is_recording[0])
            {
                loopbuffer[0][bufpos[0]++ & (mg_bufsize - 1)] = datain[k];
            }
            else
            {
                dataout[k] = loopbuffer[0][bufpos[0] & (mg_bufsize - 1)];
                bufpos[0]++;
                if (bufpos[0] >= buflength[0])
                    bufpos[0] = 0;
            }
        }
        else
            dataout[k] *= reduction;
        holdtime--;
    }
}

// fslewer slew rate thingy
fslewer::fslewer(float *fp) : filter(fp)
{
    strcpy(filtername, ("slewer"));
    parameter_count = 6;
    strcpy(ctrllabel[0], ("pre:gain"));
    ctrlmode[0] = cm_mod_decibel;
    strcpy(ctrllabel[1], ("pre:freq"));
    ctrlmode[1] = cm_frequency_audible;
    strcpy(ctrllabel[2], ("pre:BW"));
    ctrlmode[2] = cm_eq_bandwidth;
    strcpy(ctrllabel[3], ("slew rate"));
    ctrlmode[3] = cm_frequency_audible;
    strcpy(ctrllabel[4], ("post:gain"));
    ctrlmode[4] = cm_mod_decibel;
    strcpy(ctrllabel[5], ("post:freq"));
    ctrlmode[5] = cm_frequency_audible;

    strcpy(ctrlmode_desc[0], str_dbmoddef);
    strcpy(ctrlmode_desc[1], str_freqdef);
    strcpy(ctrlmode_desc[2], str_bwdef);
    strcpy(ctrlmode_desc[3], str_freqdef);
    strcpy(ctrlmode_desc[4], str_dbmoddef);
    strcpy(ctrlmode_desc[5], str_freqdef);

    v[0] = 0;
    v[1] = 0;
    lastparam[1] = -165464684.0f;
}

fslewer::~fslewer() {}

void fslewer::calc_coeffs()
{
    assert(param);
    rate.newValue(samplerate_inv * 440 * note_to_pitch(12 * param[3]));

    if ((lastparam[0] != param[0]) || (lastparam[1] != param[1]) || (lastparam[2] != param[2]) ||
        (lastparam[4] != param[4]) || (lastparam[5] != param[5]))
    {
        bq[0].coeff_orfanidisEQ(biquadunit::calc_omega(param[1]), param[2], dB_to_linear(param[0]),
                                dB_to_linear(param[0] * 0.5), 1);
        bq[1].coeff_orfanidisEQ(biquadunit::calc_omega(param[5]), 2, dB_to_linear(param[4]),
                                dB_to_linear(param[4] * 0.5), 1);
        memcpy(lastparam, param, sizeof(lastparam));
    }
}

float fslewer::get_freq_graph(float f) { return bq[0].plot_magnitude(f) * bq[1].plot_magnitude(f); }

void fslewer::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = 12.0f;
    param[1] = -2.0f;
    param[2] = 0.5f;
    param[3] = -1.0f;
    param[4] = 1.0f;
    param[5] = 5.5f;
}

void fslewer::process(float *datain, float *dataout, float pitch)
{
    assert(param);
    calc_coeffs();

    bq[0].process_block_to(datain, dataout);

    for (int k = 0; k < BLOCK_SIZE; k++)
    {
        if (dataout[k] > v[0])
            v[0] = min(dataout[k], v[0] + rate.v);
        else
            v[0] = max(dataout[k], v[0] - rate.v);

        dataout[k] = v[0];
    }
    bq[1].process_block(dataout);
}
void fslewer::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                             float pitch)
{
    assert(param);
    calc_coeffs();

    bq[0].process_block_to(datainL, datainR, dataoutL, dataoutR);

    for (int k = 0; k < BLOCK_SIZE; k++)
    {
        if (dataoutL[k] > v[0])
            v[0] = min(dataoutL[k], v[0] + rate.v);
        else
            v[0] = max(dataoutL[k], v[0] - rate.v);
        dataoutL[k] = v[0];

        if (dataoutR[k] > v[1])
            v[1] = min(dataoutR[k], v[1] + rate.v);
        else
            v[1] = max(dataoutR[k], v[1] - rate.v);
        dataoutR[k] = v[1];
    }
    bq[1].process_block_to(dataoutL, dataoutR, dataoutL, dataoutR);
}
