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
#include <cstring>

using namespace std;

/* fauxstereo	*/
fauxstereo::fauxstereo(float *ep, int *ip) : filter(ep)
{
    strcpy(filtername, "faux stereo");
    parameter_count = 3;
    strcpy(ctrllabel[0], "amplitude");
    ctrlmode[0] = cm_decibel;
    strcpy(ctrllabel[1], "delay");
    ctrlmode[1] = cm_time1s;
    strcpy(ctrllabel[2], "source (M/S)");
    ctrlmode[2] = cm_percent;

    strcpy(ctrlmode_desc[0], str_dbbpdef);
    strcpy(ctrlmode_desc[1], ("f,-10,0.05,-5,4,s"));
    strcpy(ctrlmode_desc[2], str_percentdef);

    combfilter = new COMB3(fp);
}

fauxstereo::~fauxstereo() { delete combfilter; }

void fauxstereo::init_params()
{
    if (!param)
        return;
    // preset values
    param[0] = -9.0f;
    param[1] = -6.0f;
    param[2] = 0.0f;
}
void fauxstereo::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                                float pitch)
{
    fp[0] = param[1];
    fp[1] = 0;
    l_amplitude.set_target_smoothed(db_to_linear(param[0]));
    l_source.set_target_smoothed(clamp01(param[2]));

    float mid alignas(16)[BLOCK_SIZE], side alignas(16)[BLOCK_SIZE],
        side_comb alignas(16)[BLOCK_SIZE];

    encodeMS(datainL, datainR, mid, side, BLOCK_SIZE_QUAD);
    l_source.fade_block_to(mid, side, side_comb, BLOCK_SIZE_QUAD);

    combfilter->process(side_comb, 0);

    l_amplitude.MAC_block_to(side_comb, side, BLOCK_SIZE_QUAD);
    decodeMS(mid, side, dataoutL, dataoutR, BLOCK_SIZE_QUAD);
}

/*	fs_flange			*/

fs_flange::fs_flange(float *ep, int *pi) : filter(ep)
{
    strcpy(filtername, "freqshift flange");
    parameter_count = 3;
    // strcpy(ctrllabel[0], "dry");			ctrlmode[0] = cm_decibel;
    // strcpy(ctrllabel[1], "wet");			ctrlmode[1] = cm_decibel;
    strcpy(ctrllabel[0], "shift");
    ctrlmode[0] = cm_frequency_hz_bi;
    strcpy(ctrllabel[1], "rel. shift R");
    ctrlmode[1] = cm_mod_percent;
    strcpy(ctrllabel[2], "feedback");
    ctrlmode[2] = cm_decibel;

    freqshift[0] = spawn_filter(ft_fx_freqshift, f_fs[0], i_fs[0], 0, false);
    freqshift[1] = spawn_filter(ft_fx_freqshift, f_fs[1], i_fs[1], 0, false);

    // strcpy(ctrlmode_desc[0], str_dbdef);
    // strcpy(ctrlmode_desc[1], str_dbdef);
    strcpy(ctrlmode_desc[0], "f,-20,0.01,20,0,kHz");
    strcpy(ctrlmode_desc[1], str_percentbpdef);
    strcpy(ctrlmode_desc[2], str_dbdef);

    memset(fs_buf, 0, 2 * BLOCK_SIZE * sizeof(float));
}

fs_flange::~fs_flange()
{
    spawn_filter_release(freqshift[0]);
    spawn_filter_release(freqshift[1]);
}

void fs_flange::init_params()
{
    if (!param)
        return;
    // preset values
    // param[0] = -3;
    // param[1] = -3;
    param[0] = -1.18f;
    param[1] = 0.99f;
    param[2] = -2.7f;
}

void fs_flange::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                               float pitch)
{
    feedback.newValue(db_to_linear(param[2]));

    int k;
    for (k = 0; k < BLOCK_SIZE; k++)
    {
        fs_buf[0][k] =
            (float)tanh_fast(datainL[k] + feedback.v * fs_buf[0][k]); // denormal honk i fs_buf?
        fs_buf[1][k] = (float)tanh_fast(datainR[k] + feedback.v * fs_buf[1][k]);
        feedback.process();
    }

    float freqL = 0.001f * param[0];
    float freqR = freqL * param[1];
    f_fs[0][0] = freqL;
    f_fs[1][0] = freqR;
    f_fs[0][1] = 1.f;
    f_fs[1][1] = 1.f;

    freqshift[0]->process(fs_buf[0], fs_buf[0], 0);
    freqshift[1]->process(fs_buf[1], fs_buf[1], 0);

    for (k = 0; k < BLOCK_SIZE; k++)
    {
        dataoutL[k] = fs_buf[0][k];
        dataoutR[k] = fs_buf[1][k];
    }
}

/* freqshiftdelay	*/

const int dltemp = 0x40000;

freqshiftdelay::freqshiftdelay(float *ep, int *ip) : filter(ep)
{
    strcpy(filtername, "freqshift delay");
    parameter_count = 3;
    strcpy(ctrllabel[0], "time");
    ctrlmode[0] = cm_time1s;
    strcpy(ctrllabel[1], "feedback");
    ctrlmode[1] = cm_decibel;
    strcpy(ctrllabel[2], "freq shift");
    ctrlmode[2] = cm_frequency_khz_bi;

    strcpy(ctrlmode_desc[0], str_timedef);
    strcpy(ctrlmode_desc[1], str_dbdef);
    strcpy(ctrlmode_desc[2], "f,-20,0.01,20,0,kHz");

    buffer = new float[dltemp];

    bufferlength = dltemp;

    memset(buffer, dltemp, dltemp * sizeof(float));

    wpos = 0;

    freqshift = spawn_filter(ft_fx_freqshift, f_fs, i_fs, 0, false);

    if (ep)
    {
        delaytime_filtered = (float)(samplerate * powf(2, param[0]));
    }
}

freqshiftdelay::~freqshiftdelay()
{
    spawn_filter_release(freqshift);
    delete buffer;
}

void freqshiftdelay::init_params()
{
    if (!param)
        return;
    // preset values
    param[0] = -1.0f;
    param[1] = -15.f;
    param[2] = 1.0f;
    delaytime_filtered = (float)(samplerate * powf(2, param[0]));
}

void freqshiftdelay::suspend()
{
    memset(buffer, dltemp, dltemp * sizeof(float));
    wpos = 0;
}

void freqshiftdelay::process_stereo(float *datainL, float *datainR, float *dataoutL,
                                    float *dataoutR, float pitch)
{
    f_fs[0] = param[2];
    f_fs[1] = 1.f;

    float feedback = db_to_linear(param[1]);
    float delaytime;

    delaytime = (float)(samplerate * powf(2, param[0]));

    float tbuffer[BLOCK_SIZE];

    int k;
    for (k = 0; k < BLOCK_SIZE; k++)
    {
        const float a = 0.0001f;
        delaytime_filtered = (1 - a) * delaytime_filtered + a * delaytime;
        int i_dtime = max((int)BLOCK_SIZE, min((int)delaytime_filtered, dltemp - 1));

        int rp = (wpos - i_dtime + k) & (dltemp - 1);
        tbuffer[k] = buffer[rp];
    }

    freqshift->process(tbuffer, tbuffer, 0);

    for (k = 0; k < BLOCK_SIZE; k++)
    {
        int wp = (wpos + k) & (dltemp - 1);

        buffer[wp] = 0.5f * (datainL[k] + datainR[k]) + (float)tanh_fast(tbuffer[k] * feedback);

        dataoutL[k] = tbuffer[k];
        dataoutR[k] = tbuffer[k];
    }

    wpos += BLOCK_SIZE;
    wpos = wpos & (dltemp - 1);
}
