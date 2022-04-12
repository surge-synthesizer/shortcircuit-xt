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

#include "shortcircuit2_RIFF_conversion.h"

int RIFF_FILTER_Type_To_Internal(int Type)
{
    switch (Type)
    {
    case RFT_None:
        return ft_none;
    case RFT_SBQ:
        return ft_biquadSBQ;
    case RFT_Moog:
        return ft_moogLP4sat;
    case RFT_EQ_2Band:
        return ft_eq_2band_parametric_A;
    case RFT_EQ_6BandGraphic:
        return ft_eq_6band;
        //	case RFT_MorphEQ: return ft_morpheq;
    case RFT_Limiter:
        return ft_fx_limiter;
    case RFT_Gate:
        return ft_fx_gate;
    case RFT_MicroGate:
        return ft_fx_microgate;
    case RFT_Distortion:
        return ft_fx_distortion1;
    case RFT_DigiLofi:
        return ft_fx_bitfucker;
    case RFT_Clipper:
        return ft_fx_clipper;
    case RFT_Slewer:
        return ft_fx_slewer;
    case RFT_RingMod:
        return ft_fx_ringmod;
    case RFT_FreqShift:
        return ft_fx_freqshift;
    case RFT_PhaseMod:
        return ft_fx_phasemod;
    case RFT_Delay:
        return ft_e_delay;
    case RFT_Reverb:
        return ft_e_reverb;
    case RFT_Chorus:
        return ft_e_chorus;
    case RFT_Phaser:
        return ft_e_phaser;
    case RFT_Rotary:
        return ft_e_rotary;
    case RFT_FauxStereo:
        return ft_e_fauxstereo;
    case RFT_FSFlange:
        return ft_e_fsflange;
    case RFT_FSDelay:
        return ft_e_fsdelay;
    case RFT_SawOsc:
        return ft_osc_saw;
    case RFT_SinOsc:
        return ft_osc_sin;
    case RFT_PulseOsc:
        return ft_osc_pulse;
    case RFT_PulseOscSync:
        return ft_osc_pulse_sync;
    case RFT_Treemonster:
        return ft_fx_treemonster;
    case RFT_Comb1:
        return ft_comb1;
    case RFT_StereoTools:
        return ft_fx_stereotools;
    };
    return ft_none;
}

int RIFF_FILTER_Type_From_Internal(int Type)
{
    switch (Type)
    {
    case ft_none:
        return RFT_None;
    case ft_biquadSBQ:
        return RFT_SBQ;
    case ft_moogLP4sat:
        return RFT_Moog;
    case ft_eq_2band_parametric_A:
        return RFT_EQ_2Band;
    case ft_eq_6band:
        return RFT_EQ_6BandGraphic;
        //	case ft_morpheq: return RFT_MorphEQ;
    case ft_fx_limiter:
        return RFT_Limiter;
    case ft_fx_gate:
        return RFT_Gate;
    case ft_fx_microgate:
        return RFT_MicroGate;
    case ft_fx_distortion1:
        return RFT_Distortion;
    case ft_fx_bitfucker:
        return RFT_DigiLofi;
    case ft_fx_clipper:
        return RFT_Clipper;
    case ft_fx_slewer:
        return RFT_Slewer;
    case ft_fx_ringmod:
        return RFT_RingMod;
    case ft_fx_freqshift:
        return RFT_FreqShift;
    case ft_fx_phasemod:
        return RFT_PhaseMod;
    case ft_e_delay:
        return RFT_Delay;
    case ft_e_reverb:
        return RFT_Reverb;
    case ft_e_chorus:
        return RFT_Chorus;
    case ft_e_phaser:
        return RFT_Phaser;
    case ft_e_rotary:
        return RFT_Rotary;
    case ft_e_fauxstereo:
        return RFT_FauxStereo;
    case ft_e_fsflange:
        return RFT_FSFlange;
    case ft_e_fsdelay:
        return RFT_FSDelay;
    case ft_osc_saw:
        return RFT_SawOsc;
    case ft_osc_sin:
        return RFT_SinOsc;
    case ft_osc_pulse:
        return RFT_PulseOsc;
    case ft_osc_pulse_sync:
        return RFT_PulseOscSync;
    case ft_fx_treemonster:
        return RFT_Treemonster;
    case ft_comb1:
        return RFT_Comb1;
    case ft_fx_stereotools:
        return RFT_StereoTools;
    };
    return ft_none;
}

unsigned char Playmode_To_Internal(unsigned char x)
{
    switch (x)
    {
    case pm_forwardRIFF:
        return pm_forward;
    case pm_forward_loopRIFF:
        return pm_forward_loop;
    case pm_forward_loop_bidirectionalRIFF:
        return pm_forward_loop_bidirectional;
    case pm_forward_loop_until_releaseRIFF:
        return pm_forward_loop_until_release;
    case pm_forward_shotRIFF:
        return pm_forward_shot;
    case pm_forward_releaseRIFF:
        return pm_forward_release;
    case pm_forward_hitpointsRIFF:
        return pm_forward_hitpoints;

    default:
        return pm_forward;
    };
}

unsigned char Playmode_From_Internal(unsigned char x)
{
    switch (x)
    {
    case pm_forward:
        return pm_forwardRIFF;
    case pm_forward_loop:
        return pm_forward_loopRIFF;
    case pm_forward_loop_bidirectional:
        return pm_forward_loop_bidirectionalRIFF;
    case pm_forward_loop_until_release:
        return pm_forward_loop_until_releaseRIFF;
    case pm_forward_shot:
        return pm_forward_shotRIFF;
    case pm_forward_release:
        return pm_forward_releaseRIFF;
    case pm_forward_hitpoints:
        return pm_forward_hitpointsRIFF;

    default:
        return pm_forwardRIFF;
    };
}

void ReadChunkFltD(RIFF_FILTER *F, filterstruct *f)
{
    f->bypass = vt_read_int32LE(F->Flags) & RFF_Bypass;
    f->type = RIFF_FILTER_Type_To_Internal(vt_read_int32LE(F->Type));
    f->mix = vt_read_float32LE(F->Mix);

    for (int p = 0; p < n_filter_parameters; p++)
    {
        f->p[p] = vt_read_float32LE(F->P[p]);
    }
    for (int p = 0; p < n_filter_iparameters; p++)
        f->ip[p] = vt_read_int32LE(F->IP[p]);
}

void WriteChunkFltD(RIFF_FILTER *F, filterstruct *f)
{
    memset(F, 0, sizeof(RIFF_FILTER));
    F->Type = vt_write_int32LE(RIFF_FILTER_Type_From_Internal(f->type));
    F->Flags = vt_write_int32LE(f->bypass ? RFF_Bypass : 0);
    F->Mix = vt_write_float32LE(f->mix);
    for (int p = 0; p < n_filter_parameters; p++)
    {
        F->PFlags[p] = 0;
        F->P[p] = vt_write_float32LE(f->p[p]);
    }
    for (int p = 0; p < n_filter_iparameters; p++)
        F->IP[p] = vt_write_int32LE(f->ip[p]);
}

void ReadChunkFltB(RIFF_FILTER_BUSSEX *F, sample_multi *m, int i)
{
    m->filter_pregain[i] = vt_read_float32LE(F->PreGain);
    m->filter_postgain[i] = vt_read_float32LE(F->PostGain);
    m->filter_output[i] = F->Output;
}
void WriteChunkFltB(RIFF_FILTER_BUSSEX *F, sample_multi *m, int i)
{
    F->PreGain = vt_write_float32LE(m->filter_pregain[i]);
    F->PostGain = vt_write_float32LE(m->filter_postgain[i]);
    F->Output = m->filter_output[i];
    F->UD[0] = 0;
    F->UD[1] = 0;
    F->UD[2] = 0;
}

void ReadChunkAuxB(RIFF_AUX_BUSS *E, aux_buss *e)
{
    e->level = vt_read_float32LE(E->Level);
    e->balance = vt_read_float32LE(E->Balance);
    e->outmode = E->OutMode;
    e->output = E->Output;
}

void WriteChunkAuxB(RIFF_AUX_BUSS *E, aux_buss *e)
{
    E->Level = vt_write_float32LE(e->level);
    E->Balance = vt_write_float32LE(e->balance);
    E->OutMode = e->outmode;
    E->Output = e->output;
    E->UD0[0] = 0;
    E->UD0[1] = 0;
}

void ReadChunkNCen(RIFF_NC_ENTRY *E, nc_entry *e, modmatrix *mm)
{
    e->source = mm->SourceRiffIDToInternal(E->Source);
    e->low = E->Low;
    e->high = E->High;
}

void WriteChunkNCen(RIFF_NC_ENTRY *E, nc_entry *e, modmatrix *mm)
{
    E->Source = mm->SourceInternalToRiffID(e->source);
    E->Low = e->low;
    E->High = e->high;
    E->UD0 = 0;
}

void ReadChunkMMen(RIFF_MM_ENTRY *E, mm_entry *e, modmatrix *mm)
{
    e->active = E->Active;
    e->strength = vt_read_float32LE(E->Strength);
    e->curve = E->Curve;
    e->source = mm->SourceRiffIDToInternal(E->Source1);
    e->source2 = mm->SourceRiffIDToInternal(E->Source2);
    e->destination = mm->DestinationRiffIDToInternal(E->Destination);
}

void WriteChunkMMen(RIFF_MM_ENTRY *E, mm_entry *e, modmatrix *mm)
{
    E->Active = e->active;
    E->Strength = vt_write_float32LE(e->strength);
    E->Curve = e->curve;
    E->Source1 = mm->SourceInternalToRiffID(e->source);
    E->Source2 = mm->SourceInternalToRiffID(e->source2);
    E->Destination = mm->DestinationInternalToRiffID(e->destination);
    E->UD0[0] = 0;
    E->UD0[1] = 0;
    E->UD0[2] = 0;
}

void ReadChunkAHDS(RIFF_AHDSR *E, envelope_AHDSR *e)
{
    e->attack = vt_read_float32LE(E->Attack);
    e->hold = vt_read_float32LE(E->Hold);
    e->decay = vt_read_float32LE(E->Decay);
    e->sustain = vt_read_float32LE(E->Sustain);
    e->release = vt_read_float32LE(E->Release);
    e->shape[0] = vt_read_float32LE(E->Shape[0]);
    e->shape[1] = vt_read_float32LE(E->Shape[1]);
    e->shape[2] = vt_read_float32LE(E->Shape[2]);
}

void WriteChunkAHDS(RIFF_AHDSR *E, envelope_AHDSR *e)
{
    E->Attack = vt_write_float32LE(e->attack);
    E->Hold = vt_write_float32LE(e->hold);
    E->Decay = vt_write_float32LE(e->decay);
    E->Sustain = vt_write_float32LE(e->sustain);
    E->Release = vt_write_float32LE(e->release);
    E->Shape[0] = vt_write_float32LE(e->shape[0]);
    E->Shape[1] = vt_write_float32LE(e->shape[1]);
    E->Shape[2] = vt_write_float32LE(e->shape[2]);
}

void ReadChunkSLFO(RIFF_LFO *E, steplfostruct *e)
{
    e->rate = vt_read_float32LE(E->Rate);
    e->smooth = vt_read_float32LE(E->Smooth);
    e->shuffle = vt_read_float32LE(E->Shuffle);
    for (int j = 0; j < 32; j++)
        e->data[j] = vt_read_float32LE(E->StepData[j]);

    unsigned int f = vt_read_int32LE(E->Flags);
    e->onlyonce = (f & RLF_OnlyOnce) ? 1 : 0;
    e->cyclemode = (f & RLF_CycleMode) ? 1 : 0;
    e->temposync = (f & RLF_TempoSync) ? 1 : 0;

    e->triggermode = E->TriggerMode;
    e->repeat = E->Repeat;
}

void WriteChunkSLFO(RIFF_LFO *E, steplfostruct *e)
{
    E->Rate = vt_write_float32LE(e->rate);
    E->Smooth = vt_write_float32LE(e->smooth);
    E->Shuffle = vt_write_float32LE(e->shuffle);
    for (int j = 0; j < 32; j++)
        E->StepData[j] = vt_write_float32LE(e->data[j]);

    E->Flags =
        vt_write_int32LE((e->onlyonce ? RLF_OnlyOnce : 0) | (e->cyclemode ? RLF_CycleMode : 0) |
                         (e->temposync ? RLF_TempoSync : 0));
    E->TriggerMode = e->triggermode;
    E->Repeat = e->repeat;
    E->UD[0] = 0;
    E->UD[1] = 0;
}

void ReadChunkHtPt(RIFF_HITPOINT *E, hitpoint *e)
{
    e->muted = vt_write_int32LE(E->Flags) & 1;
    e->start_sample = vt_read_int32LE(E->StartSample);
    e->end_sample = vt_read_int32LE(E->EndSample);
    e->env = vt_read_float32LE(E->Env);
}

void WriteChunkHtPt(RIFF_HITPOINT *E, hitpoint *e)
{
    E->Flags = vt_write_int32LE(e->muted ? 1 : 0);
    E->StartSample = vt_write_int32LE(e->start_sample);
    E->EndSample = vt_write_int32LE(e->end_sample);
    E->Env = vt_write_float32LE(e->env);
}

void ReadChunkZonD(RIFF_ZONE *Z, sample_zone *z)
{
    z->loop_start = vt_read_int32LE(Z->LoopStart);
    z->loop_end = vt_read_int32LE(Z->LoopEnd);
    z->sample_start = vt_read_int32LE(Z->SampleStart);
    z->sample_stop = vt_read_int32LE(Z->SampleStop);
    z->loop_crossfade_length = vt_read_int32LE(Z->LoopCrossfadeLength);

    z->part = Z->Part;
    z->layer = Z->Layer;
    z->playmode = Playmode_To_Internal(Z->Playmode);
    z->key_root = Z->KeyRoot;

    z->key_low = Z->KeyLow;
    z->key_high = Z->KeyHigh;
    z->key_low_fade = Z->KeyLowFade;
    z->key_high_fade = Z->KeyHighFade;

    z->velocity_low = Z->VelocityLow;
    z->velocity_high = Z->VelocityHigh;
    z->velocity_low_fade = Z->VelocityLowFade;
    z->velocity_high_fade = Z->VelocityHighFade;

    z->transpose = Z->Transpose;
    z->pitch_bend_depth = Z->PitchBendDepth;

    z->mute = vt_read_int32LE(Z->Flags) & RZF_Mute;
    z->reverse = vt_read_int32LE(Z->Flags) & RZF_Reverse;
    z->ignore_part_polymode = vt_read_int32LE(Z->Flags) & RZF_IgnorePartPolymode;

    z->mute_group = Z->MuteGroup;
    z->n_hitpoints = Z->N_Hitpoints;

    z->finetune = vt_read_float32LE(Z->FineTune);
    z->pitchcorrection = vt_read_float32LE(Z->PitchCorrection);
    z->velsense = vt_read_float32LE(Z->VelSense);
    z->keytrack = vt_read_float32LE(Z->Keytrack);
    z->pre_filter_gain = vt_read_float32LE(Z->PreFilterGain);
    z->lag_generator[0] = vt_read_float32LE(Z->LagGenerator[0]);
    z->lag_generator[1] = vt_read_float32LE(Z->LagGenerator[1]);
}

void WriteChunkZonD(RIFF_ZONE *Z, sample_zone *z, int SampleID)
{
    memset(Z, 0, sizeof(RIFF_ZONE));

    Z->SampleID = vt_write_int32LE(SampleID);
    Z->LoopStart = vt_write_int32LE(z->loop_start);
    Z->LoopEnd = vt_write_int32LE(z->loop_end);
    Z->SampleStart = vt_write_int32LE(z->sample_start);
    Z->SampleStop = vt_write_int32LE(z->sample_stop);
    Z->LoopCrossfadeLength = vt_write_int32LE(z->loop_crossfade_length);

    Z->Part = z->part;
    Z->Layer = z->layer;
    Z->Playmode = Playmode_From_Internal(z->playmode);
    Z->KeyRoot = z->key_root;

    Z->KeyLow = z->key_low;
    Z->KeyLowFade = z->key_low_fade;
    Z->KeyHigh = z->key_high;
    Z->KeyHighFade = z->key_high_fade;

    Z->VelocityLow = z->velocity_low;
    Z->VelocityLowFade = z->velocity_low_fade;
    Z->VelocityHigh = z->velocity_high;
    Z->VelocityHighFade = z->velocity_high_fade;

    Z->Transpose = z->transpose;
    Z->PitchBendDepth = z->pitch_bend_depth;
    Z->Flags = vt_write_int32LE((z->mute ? RZF_Mute : 0) |
                                (z->ignore_part_polymode ? RZF_IgnorePartPolymode : 0) |
                                (z->reverse ? RZF_Reverse : 0));
    Z->MuteGroup = z->mute_group;

    Z->N_Hitpoints = z->n_hitpoints;

    Z->UD0[0] = 0;
    Z->UD0[1] = 0;
    Z->UD0[2] = 0;
    Z->UD0[3] = 0;
    Z->UD0[4] = 0;

    Z->FineTune = vt_write_float32LE(z->finetune);
    Z->PitchCorrection = vt_write_float32LE(z->pitchcorrection);
    Z->VelSense = vt_write_float32LE(z->velsense);
    Z->Keytrack = vt_write_float32LE(z->keytrack);
    Z->PreFilterGain = vt_write_float32LE(z->pre_filter_gain);
    Z->LagGenerator[0] = vt_write_float32LE(z->lag_generator[0]);
    Z->LagGenerator[1] = vt_write_float32LE(z->lag_generator[1]);
}

void WriteChunkParD(RIFF_PART *P, sample_part *p)
{
    P->MIDIChannel = p->MIDIchannel;
    P->Transpose = p->transpose;
    P->Formant = p->formant;
    P->PolyMode = p->polymode;
    P->PolyLimit = p->polylimit;
    P->PortamentoMode = p->portamento_mode;
    P->VelSplitLayerCount = p->vs_layercount;
    P->Flags = vt_write_int32LE((p->vs_xf_equality ? RPF_XFadeEqualPower : 0) |
                                (p->zonelist_mode ? RPF_DisplayMode1 : 0));
    P->FineTune = vt_write_float32LE(0.f); // unused atm
    P->Portamento = vt_write_float32LE(p->portamento);
    P->PreFilterGain = vt_write_float32LE(p->pfg);
    P->VelSplitDistribution = vt_write_float32LE(p->vs_distribution);
    P->VelSplitXFade = vt_write_float32LE(p->vs_xfade);
    P->Undefined = 0;
}

void ReadChunkParD(RIFF_PART *P, sample_part *p)
{
    p->MIDIchannel = P->MIDIChannel;
    p->transpose = P->Transpose;
    p->formant = P->Formant;
    p->polymode = P->PolyMode;
    p->polylimit = P->PolyLimit;
    p->portamento_mode = P->PortamentoMode;
    p->vs_layercount = P->VelSplitLayerCount;
    p->vs_xf_equality = (vt_read_int32LE(P->Flags) & RPF_XFadeEqualPower) ? 1 : 0;
    p->zonelist_mode = (vt_read_int32LE(P->Flags) & RPF_DisplayMode1) ? 1 : 0;
    p->portamento = vt_read_float32LE(P->Portamento);
    p->pfg = vt_read_float32LE(P->PreFilterGain);
    p->vs_distribution = vt_read_float32LE(P->VelSplitDistribution);
    p->vs_xfade = vt_read_float32LE(P->VelSplitXFade);
}

void WriteChunkCtrD(RIFF_CONTROLLER *e, sample_part *p, int id)
{
    e->State = vt_write_float32LE(p->userparameter[id]);
    e->Flags = vt_write_int32LE(p->userparameterpolarity[id] ? RCF_Bipolar : 0);
}

void ReadChunkCtrD(RIFF_CONTROLLER *e, sample_part *p, int id)
{
    p->userparameter[id] = vt_read_float32LE(e->State);
    p->userparameterpolarity[id] = (vt_read_int32LE(e->Flags) & RCF_Bipolar) ? 1 : 0;
}