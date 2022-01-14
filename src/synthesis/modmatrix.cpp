//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "modmatrix.h"
#include "configuration.h"
#include "controllers.h"
#include "loaders/shortcircuit2_RIFF_format.h"
#include "parameterids.h"
#include "sampler_state.h"
#include "sampler_voice.h"
#include "synthesis/filter.h"
#include <algorithm>
#include <cstring>
#include <vt_dsp/basic_dsp.h>
#include <vt_util/vt_string.h>
using std::max;
using std::min;

float one = 1.0f;

const float randscale = (2.f / (float)RAND_MAX);

int modmatrix::get_destination_value_int(int id) { return Float2Int(fdst[id]); }

modmatrix::modmatrix() {}

void modmatrix::assign(configuration *conf, sample_zone *zone, sample_part *part,
                       sampler_voice *voice, float *control, float *automation, timedata *td)
{
    // zero mem
    memset(fdst, 0, sizeof(float) * md_num_destinations);
    alternate = 1.f;

    this->zone = zone;
    this->part = part;
    this->voice = voice;
    this->control = control;
    this->automation = automation;

    add_source(RMS_None, "none", 0);
    // voice sources
    if (zone) // only available for the zone
    {
        add_source(RMS_Keytrack, "keytrack", voice ? &voice->keytrack : 0);
        add_source(RMS_Velocity, "velocity", voice ? &voice->fvelocity : 0);
        add_source(RMS_Modulator1, "AEG", voice ? &voice->AEG.output : 0);
        ss_id[ss_EG2] = (int)src.size();
        add_source(RMS_Modulator2, "EG2", voice ? &voice->EG2.output : 0);
        ss_id[ss_LFO1] = (int)src.size();
        add_source(RMS_Modulator3, "stepLFO1", voice ? &voice->stepLFO[0].output : 0);
        ss_id[ss_LFO2] = (int)src.size();
        add_source(RMS_Modulator4, "stepLFO2", voice ? &voice->stepLFO[1].output : 0);
        ss_id[ss_LFO3] = (int)src.size();
        add_source(RMS_Modulator5, "stepLFO3", voice ? &voice->stepLFO[2].output : 0);
        add_source(RMS_SliceEnv, "slice_env", voice ? &voice->slice_env : 0, "Slice Envelope");
        add_source(RMS_Random, "random+", voice ? &voice->random : 0);
        add_source(RMS_RandomBP, "random +/-", voice ? &voice->randombp : 0);
        add_source(RMS_Gate, "gate", voice ? &voice->fgate : 0);
        add_source(RMS_Time, "time", voice ? &voice->time : 0, "Time (s)");
        add_source(RMS_TimeMinutes, "time60", voice ? &voice->time60 : 0, "Time (m)");
        ss_id[ss_lag0] = (int)src.size();
        add_source(RMS_LagGenerator1, "lag1", voice ? &voice->lag[0] : 0);
        ss_id[ss_lag1] = (int)src.size();
        add_source(RMS_LagGenerator2, "lag2", voice ? &voice->lag[1] : 0);
        ss_id[ss_envf] = (int)src.size();
        // add_source("envfollow",		voice ? &voice->envelope_follower : 0);
        add_source(RMS_WithinLoop, "loop_gate", voice ? &voice->loop_gate : 0, "Is Within Loop");
        add_source(RMS_LoopPos, "loop_pos", voice ? &voice->loop_pos : 0, "Position In Loop");
        add_source(RMS_Filter1ModOut, "f1modout", voice ? &voice->filter_modout[0] : 0,
                   "Filter 1 Mod Out");
        add_source(RMS_Filter2ModOut, "f2modout", voice ? &voice->filter_modout[1] : 0,
                   "Filter 2 Mod Out");
    }

    add_source(RMS_Noise, "noise", &noisegen, "noise");
    add_source(RMS_Alternate, "alternate", voice ? &voice->alternate : &alternate, "Alternate");

    add_source(RMS_PosBeat, "pos_beat", td ? &td->pos_in_beat : 0, "Sync (1 Beat)");
    add_source(RMS_Pos2Beats, "pos_2beats", td ? &td->pos_in_2beats : 0, "Sync (2 Beats)");
    add_source(RMS_PosBar, "pos_bar", td ? &td->pos_in_bar : 0, "Sync (1 Bar)");
    add_source(RMS_Pos2Bars, "pos_2bars", td ? &td->pos_in_2bars : 0, "Sync (2 Bars)");
    add_source(RMS_Pos4Bars, "pos_4bars", td ? &td->pos_in_4bars : 0, "Sync (4 Bars)");

    add_source(RMS_One, "constant1", &one, "One");
    // automation & channel modulation sources
    add_source(RMS_PitchBend, "pitchbend", control ? &control[c_pitch_bend] : 0, "Pitch Bend");
    // add_source("pb_up",			&pb_up,"pitchbend up");
    // add_source("pb_down",		&pb_down,"pitchbend down");
    add_source(RMS_ChAftertouch, "channelAT", control ? &control[c_channel_aftertouch] : 0,
               "Ch Aftertouch");
    add_source(RMS_ModulationWheel, "modwheel", control ? &control[c_modwheel] : 0);
    int i;
    for (i = 0; i < n_custom_controllers; i++)
    {
        char stnice[namelen], st[namelen];
        /*if (conf)
                sprintf(stnice,"c%i:%s",i+1,conf->MIDIcontrol[i].name);
        else
                sprintf(stnice,"c%i:",i+1);*/
        sprintf(stnice, "c%i:%s", i + 1, part ? part->userparametername[i] : "");
        sprintf(st, "c%i", i + 1);
        add_source(RMS_Ctrl1 + i, st, part ? &part->userparameter_smoothed[i] : 0, stnice);
    }
    /*for(i=0; i<n_automation_parameters; i++)
    {
            char st[namelen];
            sprintf(st,"auto%02i",i);
            add_source(st,	automation ? &automation[i] : 0);
    }	*/

    // destinations

    add_destination(RMD_None, md_none, "}none", 0);
    if (zone)
    {
        // zone-level matrix
        add_destination(RMD_Pitch, md_pitch, "pitch", cm_mod_pitch);
        add_destination(RMD_Rate, md_rate, "rate", cm_mod_percent, "rate (linear)");
        add_destination(RMD_Amplitude, md_amplitude, "amplitude", cm_mod_decibel);
        add_destination(RMD_PreFilterGain, md_prefilter_gain, "pfg", cm_mod_decibel, "p.f. gain");
        add_destination(RMD_Balance, md_pan, "pan", cm_mod_percent);
        add_destination(RMD_AmplitudeAux1, md_aux_level, "aux_level", cm_mod_decibel, "aux level");
        add_destination(RMD_BalanceAux1, md_aux_balance, "aux_balance", cm_mod_percent,
                        "aux balance");
        add_destination(RMD_AmplitudeAux2, md_aux2_level, "aux2_level", cm_mod_decibel,
                        "aux2 level");
        add_destination(RMD_BalanceAux2, md_aux2_balance, "aux2_balance", cm_mod_percent,
                        "aux2 balance");

        add_destination(RMD_Filter1Mix, md_filter1mix, "f1mix", cm_mod_percent, "filter 1 mix");
        add_destination(RMD_Filter2Mix, md_filter2mix, "f2mix", cm_mod_percent, "filter 2 mix");

        for (unsigned int fs = 0; fs < 2; fs++)
        {
            filter *tempf = 0;
            if (zone)
                tempf = spawn_filter(zone->Filter[fs].type, zone->Filter[fs].p, zone->Filter[fs].ip,
                                     0, false);

            for (unsigned int fp = 0; fp < 6; fp++)
            {
                int ct = 0;
                char txt[namelen];
                char idname[namelen];
                sprintf(idname, "f%ip%i", fs + 1, fp + 1);
                if (tempf)
                    sprintf(txt, "f%i:%s", fs + 1, tempf->get_parameter_label(fp));
                else
                    sprintf(txt, "f%i: ---", fs + 1);
                if (tempf)
                    ct = tempf->get_parameter_ctrlmode(fp);

                ct = get_mod_mode(ct);

                add_destination(RMD_Filter1Param0 + 0x10 * fs + fp,
                                md_filter1prm0 + fs * (md_filter2prm0 - md_filter1prm0) + fp,
                                idname, ct, txt);
            }
            spawn_filter_release(tempf);
        }

        add_destination(RMD_SampleStart, md_sample_start, "samplestart", cm_mod_percent,
                        "zone start");
        add_destination(RMD_LoopStart, md_loop_start, "loopstart", cm_mod_percent, "loop start");
        add_destination(RMD_LoopLength, md_loop_length, "looplength", cm_mod_percent,
                        "loop length");

        // add_destination(md_granularpos,"granular",cm_mod_percent,"granular pos");

        add_destination(RMD_AEGAttack, md_AEG_a, "eg1a", cm_mod_time, "AEG attack");
        add_destination(RMD_AEGHold, md_AEG_h, "eg1h", cm_mod_time, "AEG hold");
        add_destination(RMD_AEGDecay, md_AEG_d, "eg1d", cm_mod_time, "AEG decay");
        add_destination(RMD_AEGSustain, md_AEG_s, "eg1s", cm_mod_percent, "AEG sustain");
        add_destination(RMD_AEGRelease, md_AEG_r, "eg1r", cm_mod_time, "AEG release");
        add_destination(RMD_EG2Attack, md_EG2_a, "eg2a", cm_mod_time, "EG2 attack");
        add_destination(RMD_EG2Hold, md_EG2_h, "eg2h", cm_mod_time, "EG2 hold");
        add_destination(RMD_EG2Decay, md_EG2_d, "eg2d", cm_mod_time, "EG2 decay");
        add_destination(RMD_EG2Sustain, md_EG2_s, "eg2s", cm_mod_percent, "EG2 sustain");
        add_destination(RMD_EG2Release, md_EG2_r, "eg2r", cm_mod_time, "EG2 release");

        add_destination(RMD_LFO1Rate, md_LFO1_rate, "lfo1rate", cm_mod_freq, "stepLFO1 rate");
        add_destination(RMD_LFO2Rate, md_LFO2_rate, "lfo2rate", cm_mod_freq, "stepLFO2 rate");
        add_destination(RMD_LFO3Rate, md_LFO3_rate, "lfo3rate", cm_mod_freq, "stepLFO3 rate");

        add_destination(RMD_LagGenerator1, md_lag0, "lag1", cm_mod_percent, "lag 1");
        add_destination(RMD_LagGenerator2, md_lag1, "lag2", cm_mod_percent, "lag 2");

        /*for(i=0; i<mm_entries; i++)
        {
                char label[namelen],id[namelen];
                sprintf(label,"mod slot #%i",i+1);
                sprintf(id,"mm%i",i);
                int ct = cm_mod_percent;
                if (zone)
                {
                        int d = zone->mm[i].destination;
                        if((d>0)&&(d<(int)dst.size())) ct = dst[d].ctrlmode;
                }
                add_destination(md_MM0_depth+i,id,ct,label);
        }*/
    }
    else
    {
        // part-level matrix
        add_destination(RMD_PartAmplitude, md_part_amplitude, "amplitude", cm_mod_decibel);
        add_destination(RMD_PartPreFilterGain, md_part_prefilter_gain, "pfg", cm_mod_decibel,
                        "p.f. gain");
        add_destination(RMD_PartBalance, md_part_pan, "balance", cm_mod_percent);
        add_destination(RMD_PartAmplitudeAux1, md_part_aux_level, "aux_level", cm_mod_decibel,
                        "aux level");
        add_destination(RMD_PartBalanceAux1, md_part_aux_balance, "aux_balance", cm_mod_percent,
                        "aux balance");
        add_destination(RMD_PartAmplitudeAux2, md_part_aux2_level, "aux2_level", cm_mod_decibel,
                        "aux2 level");
        add_destination(RMD_PartBalanceAux2, md_part_aux2_balance, "aux2_balance", cm_mod_percent,
                        "aux2 balance");

        add_destination(RMD_PartFilter1Mix, md_part_filter1mix, "f1mix", cm_mod_percent,
                        "filter 1 mix");
        add_destination(RMD_PartFilter2Mix, md_part_filter2mix, "f2mix", cm_mod_percent,
                        "filter 2 mix");

        for (unsigned int fs = 0; fs < 2; fs++)
        {
            filter *tempf = 0;
            if (part)
                tempf = spawn_filter(part->Filter[fs].type, part->Filter[fs].p, part->Filter[fs].ip,
                                     0, false);

            for (unsigned int fp = 0; fp < n_filter_parameters; fp++)
            {
                int ct = 0;
                char txt[namelen];
                char idname[namelen];
                sprintf(idname, "f%ip%i", fs + 1, fp + 1);
                if (tempf)
                    sprintf(txt, "f%i:%s", fs + 1, tempf->get_parameter_label(fp));
                else
                    sprintf(txt, "f%i: ---", fs + 1);
                if (tempf)
                    ct = tempf->get_parameter_ctrlmode(fp);

                ct = get_mod_mode(ct);

                add_destination(RMD_PartFilter1Param0 + 0x10 * fs + fp,
                                md_part_filter1prm0 +
                                    fs * (md_part_filter2prm0 - md_part_filter1prm0) + fp,
                                idname, ct, txt);
            }
            spawn_filter_release(tempf);
        }
    }
}

int modmatrix::is_source_used(int source) // for CPU saving purposes
{
    if ((source < 0) || (source > num_switchable_sources))
        return 0;
    if (!zone)
        return 0;
    int sid = ss_id[source];
    for (int i = 0; i < mm_entries; i++)
    {
        if ((zone->mm[i].source == sid) || (zone->mm[i].source2 == sid))
            return 0xffffffff;
    }
    return 0;
}

void modmatrix::add_destination(unsigned char RIFFID, int id, const char *name, int control_type,
                                const char *dispname)
{
    vtCopyString(dst[id].id_name, name, namelen);

    if (dispname)
        vtCopyString(dst[id].display_name, dispname, namelen);
    else
        vtCopyString(dst[id].display_name, name, namelen);

    dst[id].ctrlmode = control_type;
    dst[id].RIFFID = RIFFID;
}

modmatrix::~modmatrix() {}

void modmatrix::add_source(unsigned char RIFFID, const char *name, float *fptr,
                           const char *dispname)
{
    mm_src t;
    vtCopyString(t.id_name, name, namelen);

    if (dispname)
        vtCopyString(t.display_name, dispname, namelen);
    else
        vtCopyString(t.display_name, name, namelen);

    t.fptr = fptr;
    t.RIFFID = RIFFID;
    src.push_back(t);
}

int modmatrix::get_n_sources() { return (int)src.size(); }
int modmatrix::get_n_destinations()
{
    // return (int)dst.size();
    if (zone)
        return md_num_zone_destinations;
    else
        return md_num_part_destinations;
}

bool modmatrix::check_NC(sample_zone *z)
{
    // perform per-note variable updates
    alternate *= -1.f;

    // checks NCs
    for (int i = 0; i < nc_entries; i++)
    {
        if (src[z->nc[i].source].fptr)
        {
            int val = (int)(float)(*src[z->nc[i].source].fptr * 127.f);
            if ((val < z->nc[i].low) || (val > z->nc[i].high))
                return false;
        }
    }
    unsigned int layer = z->layer & (num_layers - 1);
    for (int i = (layer * num_layer_ncs); i < (layer * num_layer_ncs + num_layer_ncs); i++)
    {
        if (src[part->nc[i].source].fptr)
        {
            int val = (int)(float)(*src[part->nc[i].source].fptr * 127.f);
            if ((val < part->nc[i].low) || (val > part->nc[i].high))
                return false;
        }
    }

    return true;
}

float do_curve(unsigned int curve, float x)
{
    if (!curve)
        return x;

    switch (curve)
    {
    case mmc_square:
        x = x * x;
        break;
    case mmc_cube:
        x = x * x * x;
        break;
    case mmc_squareroot:
        x = sqrt(max(0.00000000000001f, x));
        break;
    case mmc_cuberoot:
        x = powf(max(0.00000000000001f, x), 1.f / 3.f);
        break;
    case mmc_reverse:
        x = 1.f - x;
        break;
    case mmc_abs:
        x = fabs(x);
        break;
    case mmc_pos:
        x = max(0.f, x);
        break;
    case mmc_neg:
        x = min(0.f, x);
        break;
    case mmc_switch:
        x = (x > 0.f) ? 1 : 0;
        break;
    case mmc_switchi:
        x = (x < 0.f) ? 1 : 0;
        break;
    case mmc_switchh:
        x = (x > 0.5f) ? 1 : 0;
        break;
    case mmc_switchhi:
        x = (x < 0.5f) ? 1 : 0;
        break;
    case mmc_polarity:
        x = (x > 0.f) ? 1 : -1;
        break;
    case mmc_up2bp:
        x = 2.f * x - 1.f;
        break;
    case mmc_bp2up:
        x = 0.5f * x + 0.5f;
        break;
    case mmc_tri:
        x = fabs(1.f - 2.f * x);
        break;
    case mmc_tribp:
        x = 2.f * fabs(1.f - 2.f * x) - 1.f;
        break;
    };
    return x;
}

void modmatrix::process_part()
{
    //	pb_up = max(0,control[c_pitch_bend]);
    //	pb_down = max(0,-control[c_pitch_bend]);
    noisegen = ((float)rand() * randscale) - 1.f;

    fdst[md_part_amplitude] = part->aux[0].level;
    fdst[md_part_pan] = part->aux[0].balance;
    fdst[md_part_aux_level] = part->aux[1].level;
    fdst[md_part_aux_balance] = part->aux[1].balance;
    fdst[md_part_aux2_level] = part->aux[2].level;
    fdst[md_part_aux2_balance] = part->aux[2].balance;
    fdst[md_part_prefilter_gain] = part->pfg;

    if (part->Filter[0].type && !part->Filter[0].bypass)
    {
        fdst[md_part_filter1mix] = part->Filter[0].mix;
        fdst[md_part_filter1prm0] = part->Filter[0].p[0];
        fdst[md_part_filter1prm1] = part->Filter[0].p[1];
        fdst[md_part_filter1prm2] = part->Filter[0].p[2];
        fdst[md_part_filter1prm3] = part->Filter[0].p[3];
        fdst[md_part_filter1prm4] = part->Filter[0].p[4];
        fdst[md_part_filter1prm5] = part->Filter[0].p[5];
        fdst[md_part_filter1prm6] = part->Filter[0].p[6];
        fdst[md_part_filter1prm7] = part->Filter[0].p[7];
        fdst[md_part_filter1prm8] = part->Filter[0].p[8];
    }
    if (part->Filter[1].type && !part->Filter[1].bypass)
    {
        fdst[md_part_filter2mix] = part->Filter[1].mix;
        fdst[md_part_filter2prm0] = part->Filter[1].p[0];
        fdst[md_part_filter2prm1] = part->Filter[1].p[1];
        fdst[md_part_filter2prm2] = part->Filter[1].p[2];
        fdst[md_part_filter2prm3] = part->Filter[1].p[3];
        fdst[md_part_filter2prm4] = part->Filter[1].p[4];
        fdst[md_part_filter2prm5] = part->Filter[1].p[5];
        fdst[md_part_filter2prm6] = part->Filter[1].p[6];
        fdst[md_part_filter2prm7] = part->Filter[1].p[7];
        fdst[md_part_filter2prm8] = part->Filter[1].p[8];
    }

    for (int i = 0; i < mm_part_entries; i++)
    {
        if (part->mm[i].destination && (part->mm[i].source || part->mm[i].source2) &&
            (part->mm[i].active) && (src[part->mm[i].source].fptr))
        {
            int dest = part->mm[i].destination;
            int curve = part->mm[i].curve;
            float strength = part->mm[i].strength; // + fdst[md_MM0_depth+i];
            float tmodulation = *src[part->mm[i].source].fptr;
            if (src[part->mm[i].source2].fptr)
                tmodulation *= *src[part->mm[i].source2].fptr;

            if (curve)
                tmodulation = do_curve(curve, tmodulation);
            tmodulation *= strength;

            if (curve >= mmc_quantitize1)
            {
                float qval = 1.f + (curve - mmc_quantitize1);
                tmodulation = qval * floor(0.5f + tmodulation / qval);
            }

            fdst[dest] += tmodulation;
        }
    }
}

void modmatrix::process()
{
    // calculate special controllers that only exist within the modmatrix
    // pb_up = max(0,control[c_pitch_bend]);
    // pb_down = max(0,-control[c_pitch_bend]);
    noisegen = ((float)rand() * randscale) - 1.f;

    if (!zone)
        return;
    if (!voice)
        return;
    int i;
    // init destinations
    // memset(fdst,0,sizeof(float)*md_num_destinations);

    fdst[md_pitch] = 0.0f;
    fdst[md_rate] = 1.0f;

    fdst[md_amplitude] = zone->aux[0].level;
    fdst[md_prefilter_gain] = zone->pre_filter_gain;
    fdst[md_pan] = zone->aux[0].balance;
    fdst[md_aux_level] = zone->aux[1].level;
    fdst[md_aux_balance] = zone->aux[1].balance;
    fdst[md_aux2_level] = zone->aux[2].level;
    fdst[md_aux2_balance] = zone->aux[2].balance;

    fdst[md_lag0] = 0.0f;
    fdst[md_lag1] = 0.0f;

    fdst[md_AEG_a] = zone->AEG.attack;
    fdst[md_AEG_h] = zone->AEG.hold;
    fdst[md_AEG_d] = zone->AEG.decay;
    fdst[md_AEG_s] = zone->AEG.sustain;
    fdst[md_AEG_r] = zone->AEG.release;

    {
        fdst[md_EG2_a] = zone->EG2.attack;
        fdst[md_EG2_h] = zone->EG2.hold;
        fdst[md_EG2_d] = zone->EG2.decay;
        fdst[md_EG2_s] = zone->EG2.sustain;
        fdst[md_EG2_r] = zone->EG2.release;
    }

    if (zone->Filter[0].type && !zone->Filter[0].bypass)
    {
        fdst[md_filter1mix] = zone->Filter[0].mix;
        fdst[md_filter1prm0] = zone->Filter[0].p[0];
        fdst[md_filter1prm1] = zone->Filter[0].p[1];
        fdst[md_filter1prm2] = zone->Filter[0].p[2];
        fdst[md_filter1prm3] = zone->Filter[0].p[3];
        fdst[md_filter1prm4] = zone->Filter[0].p[4];
        fdst[md_filter1prm5] = zone->Filter[0].p[5];
    }
    if (zone->Filter[1].type && !zone->Filter[1].bypass)
    {
        fdst[md_filter2mix] = zone->Filter[1].mix;
        fdst[md_filter2prm0] = zone->Filter[1].p[0];
        fdst[md_filter2prm1] = zone->Filter[1].p[1];
        fdst[md_filter2prm2] = zone->Filter[1].p[2];
        fdst[md_filter2prm3] = zone->Filter[1].p[3];
        fdst[md_filter2prm4] = zone->Filter[1].p[4];
        fdst[md_filter2prm5] = zone->Filter[1].p[5];
    }

    fdst[md_LFO1_rate] = zone->LFO[0].rate;
    fdst[md_LFO2_rate] = zone->LFO[1].rate;
    fdst[md_LFO3_rate] = zone->LFO[2].rate;

    fdst[md_sample_start] = (float)(zone->sample_start);
    fdst[md_loop_start] = (float)(zone->loop_start);
    fdst[md_loop_length] = (float)(int)(zone->loop_end - zone->loop_start);

    // process mm depth slots
    /*for(i=0; i<mm_entries; i++)
    {
            if((zone->mm[i].destination >=
    md_MM0_depth)&&(zone->mm[i].active)&&(src[zone->mm[i].source].fptr))
            {
                    float strength = zone->mm[i].strength + fdst[md_MM0_depth+i];
                    fdst[zone->mm[i].destination] += strength * *src[zone->mm[i].source].fptr;
            }
    }*/

    for (i = 0; i < mm_entries; i++)
    {
        if (zone->mm[i].destination && (zone->mm[i].source || zone->mm[i].source2) &&
            (zone->mm[i].active) && (src[zone->mm[i].source].fptr))
        {
            int dest = zone->mm[i].destination;
            int curve = zone->mm[i].curve;
            float strength = zone->mm[i].strength; // + fdst[md_MM0_depth+i];
            float tmodulation = *src[zone->mm[i].source].fptr;
            if (src[zone->mm[i].source2].fptr)
                tmodulation *= *src[zone->mm[i].source2].fptr;

            if (curve)
                tmodulation = do_curve(curve, tmodulation);

            tmodulation *= strength;

            if (curve >= mmc_quantitize1)
            {
                float qval = 1.f + (curve - mmc_quantitize1);
                tmodulation = qval * floor(0.5f + tmodulation / qval);
            }

            switch (dest)
            {
            case md_sample_start:
            case md_loop_start:
            case md_loop_length:
                fdst[dest] += zone->sample_stop * tmodulation;
                break;
            default:
                fdst[dest] += tmodulation;
            };
        }
    }
}

int get_mm_source_id(const char *txt)
{
    modmatrix mm;
    mm.assign(0, 0, 0);
    int k, n = mm.get_n_sources();
    for (k = 0; k < n; k++)
    {
        if (!strcmp(txt, mm.get_source_idname(k)))
            return k;
    }
    return 0;
}

int get_mm_dest_id(const char *txt)
{
    modmatrix mm;
    mm.assign(0, 0, 0);
    int k, n = mm.get_n_destinations();
    for (k = 0; k < n; k++)
    {
        if (!strcmp(txt, mm.get_destination_idname(k)))
            return k;
    }
    return 0;
}

int modmatrix::SourceRiffIDToInternal(unsigned char ID)
{
    int k, n = get_n_sources();
    for (k = 0; k < n; k++)
    {
        if (ID == get_source_RIFFID(k))
            return k;
    }
    return 0;
}
unsigned char modmatrix::SourceInternalToRiffID(unsigned int ID)
{
    if (ID < get_n_sources())
        return get_source_RIFFID(ID) & 0xff;
    return 0;
}
int modmatrix::DestinationRiffIDToInternal(unsigned char ID)
{
    int k, n = get_n_destinations();
    for (k = 0; k < n; k++)
    {
        if (ID == get_destination_RIFFID(k))
            return k;
    }
    return 0;
}
unsigned char modmatrix::DestinationInternalToRiffID(unsigned int ID)
{
    if (ID < get_n_destinations())
        return get_destination_RIFFID(ID);
    return 0;
}
