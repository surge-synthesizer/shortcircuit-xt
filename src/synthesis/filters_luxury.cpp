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
#include <assert.h>

#include <algorithm>
using std::max;
using std::min;

#include <cmath>
#include <math.h>

#include <cstring>

/*	LP4M		*/
// moog ladder filter emulation

const int LP4M_oversampling = 2;

LP4M_sat::LP4M_sat(float *fp, int *ip)
    : filter(fp, 0, true, ip), pre_filter(4, true), post_filter(4, true)
{
    strcpy(filtername, "LP4M saturated");
    parameter_count = 3;
    strcpy(ctrllabel[0], "cutoff");
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], "resonance");
    ctrlmode[1] = cm_percent;
    strcpy(ctrllabel[2], "drive");
    ctrlmode[2] = cm_mod_decibel;

    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_percentdef);
    strcpy(ctrlmode_desc[2], str_dbmoddef);

    memset(reg, 0, sizeof(float) * 10);

    g.setBlockSize(LP4M_oversampling * block_size);
    r.setBlockSize(LP4M_oversampling * block_size);
    gain.set_blocksize(block_size);

    first_run = true;
}

LP4M_sat::~LP4M_sat() {}

void LP4M_sat::init_params()
{
    assert(param);
    if (!param)
        return;
    // preset values
    param[0] = 5.5f;
    param[1] = 0.0f;
    param[2] = 0.0f;
    iparam[0] = 3;
}

int LP4M_sat::get_ip_count() { return 1; }

const char *LP4M_sat::get_ip_label(int ip_id)
{
    if (ip_id == 0)
        return "order";
    return "";
}

int LP4M_sat::get_ip_entry_count(int ip_id)
{
    return 4; // 4 poles
}

const char *LP4M_sat::get_ip_entry_label(int ip_id, int c_id)
{
    if (ip_id == 0)
    {
        switch (c_id)
        {
        case 0:
            return "1-pole";
        case 1:
            return "2-pole";
        case 2:
            return "3-pole";
        case 3:
            return "4-pole";
        }
    }
    return "";
}

void LP4M_sat::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                              float pitch)
{
    assert(datainL);
    assert(datainR);
    assert(param);
    double gg = limit_range(
        (440.0 * (double)note_to_pitch(12.0 * param[0]) * (double)samplerate_inv * 0.5), 0.0, 0.25);

    double t_b1 = 1.0 - exp(-2.0 * M_PI * gg);
    g.newValue(t_b1);
    double q =
        min(2.15 * limit_range((double)param[1], 0.0, 1.0), 0.5 / (t_b1 * t_b1 * t_b1 * t_b1));
    r.newValue(q);

    gain.set_target_smoothed(2.f * db_to_linear(param[2]) * (3.0 / (3.0 - q)));

    float *ypoleL = &reg[limit_range(iparam[0], 0, 3)];
    float *ypoleR = &reg[5 + limit_range(iparam[0], 0, 3)];

    Align16 float dataOS[2][block_size << 1];

    gain.multiply_2_blocks_to(datainL, datainR, dataOS[0], dataOS[1], block_size_quad);
    pre_filter.process_block_U2(dataOS[0], dataOS[1], dataOS[0], dataOS[1], block_size << 1);

    for (int k = 0; k < (block_size << 1); k++)
    {
        float inL = dataOS[0][k];
        float inR = dataOS[1][k];
        reg[0] = reg[0] + g.v * (((inL - r.v * (reg[3] + reg[4]))) - reg[0]);
        reg[5] = reg[5] + g.v * (((inR - r.v * (reg[8] + reg[9]))) - reg[5]);
        reg[0] = 8.0 * softclip_double(0.125 * reg[0]);
        reg[5] = 8.0 * softclip_double(0.125 * reg[5]);
        reg[1] = reg[1] + g.v * (reg[0] - reg[1]);
        reg[6] = reg[6] + g.v * (reg[5] - reg[6]);

        reg[2] = reg[2] + g.v * (reg[1] - reg[2]);
        reg[7] = reg[7] + g.v * (reg[6] - reg[7]);

        reg[4] = reg[3];
        reg[9] = reg[8];

        reg[3] = reg[3] + g.v * (reg[2] - reg[3]);
        reg[8] = reg[8] + g.v * (reg[7] - reg[8]);
        dataOS[0][k] = *ypoleL;
        dataOS[1][k] = *ypoleR;

        g.process();
        r.process();
    }

    post_filter.process_block_D2(dataOS[0], dataOS[1], block_size << 1, dataoutL, dataoutR);
}

void LP4M_sat::process(float *datain, float *dataout, float pitch)
{
    assert(datain);
    assert(param);
    double gg = limit_range(
        (440.0 * (double)note_to_pitch(12.0 * param[0]) * (double)samplerate_inv * 0.5), 0.0, 0.25);

    double t_b1 = 1 - exp(-2.0 * M_PI * gg);
    g.newValue(t_b1);
    double q =
        min(2.15 * limit_range((double)param[1], 0.0, 1.0), 0.5 / (t_b1 * t_b1 * t_b1 * t_b1));
    r.newValue(q);

    gain.set_target(2.f * db_to_linear(param[2]) * (3.0 / (3.0 - q)));

    float *ypole = &reg[limit_range(iparam[0], 0, 3)];

    Align16 float dataOS[2][block_size << 1];

    gain.multiply_block_to(datain, dataOS[0], block_size_quad);
    clear_block(dataOS[1], block_size_quad);
    pre_filter.process_block_U2(dataOS[0], dataOS[1], dataOS[0], dataOS[1], block_size << 1);

    for (int k = 0; k < (block_size << 1); k++)
    {
        float in = dataOS[0][k];
        reg[0] = reg[0] + g.v * (((in - r.v * (reg[3] + reg[4]))) - reg[0]);
        reg[0] = 8.0 * softclip_double(0.125 * reg[0]);
        reg[1] = reg[1] + g.v * (reg[0] - reg[1]);
        reg[2] = reg[2] + g.v * (reg[1] - reg[2]);
        reg[4] = reg[3];
        reg[3] = reg[3] + g.v * (reg[2] - reg[3]);
        dataOS[0][k] = *ypole;

        g.process();
        r.process();
    }

    post_filter.process_block_D2(dataOS[0], dataOS[1], block_size << 1, dataout);
}

/* moog 4-pole SVF without saturation	*/

/*
LP4M::LP4M(float *fp) : filter(fp)
{
        strcpy(filtername,"LP4M");
        parameter_count = 3;
        strcpy(ctrllabel[0], "cutoff");			ctrlmode[0] = cm_frequency_audible;
        strcpy(ctrllabel[1], "resonance");		ctrlmode[1] = cm_percent;
        strcpy(ctrllabel[2], "poles");			ctrlmode[2] = cm_count4;

        strcpy(ctrlmode_desc[0], str_freqdef);
        strcpy(ctrlmode_desc[1], str_percentdef);
        strcpy(ctrlmode_desc[2], "f,1,0.1,4,0,");

        ya = 0;
        yb = 0;
        yc = 0;
        yd = 0;
        wa = 0;
        wb = 0;
        wc = 0;
        ya_d = 0;
        yb_d = 0;
        yc_d = 0;
        yd_d = 0;
        wa_d = 0;
        wb_d = 0;
        wc_d = 0;

        g.setBlockSize(LP4M_oversampling*block_size);
        r.setBlockSize(LP4M_oversampling*block_size);

        first_run = true;
}

LP4M::~LP4M()
{
}

void LP4M::init_params()
{
        assert(param);
        if(!param) return;
        // preset values
        param[0] = 5.5f;
        param[1] = 0.0f;
        param[2] = 4.0f;
}

void LP4M::process(float *data, float pitch)
{
        assert(data);
        assert(param);
        double gg =
limit_range((440*pow((double)2,(double)param[0])*(double)samplerate_inv*0.5),0,0.25);

        g.newValue(1 - exp(-k2PI*gg));
        r.newValue(2*limit_range(param[1],0,1));

        int pole = max(1,min(4,(int)param[2]));
        double *ypole;
        switch(pole)
        {
        case 1:
                ypole = &ya;
                break;
        case 2:
                ypole = &yb;
                break;
        case 3:
                ypole = &yc;
                break;
        default:
                ypole = &yd;
                break;
        };

        double dataOS[block_size*LP4M_oversampling];

        int k;
        for(k=0; k<block_size; k++)
        {
                dataOS[2*k] = data[k];
                dataOS[2*k+1] = data[k];
        }

        int bsos = block_size*LP4M_oversampling;
        for(k=0; k<bsos; k++)
        {
                double in = dataOS[k];

                ya = ya + g.v*(((in - r.v*(yd+yd_d))) - ya);
                ya = tanh_turbo(ya);
                yb = yb + g.v*(ya - yb);
                yc = yc + g.v*(yb - yc);
                yd_d = yd;
                yd = yd + g.v*(yc - yd);
                dataOS[k] = *ypole;

                g.process();
                r.process();

        }

        for(k=0; k<block_size; k++)
        {
                data[k] = 0.5*(dataOS[2*k]+dataOS[2*k+1]);
        }
}*/