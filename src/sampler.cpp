#include "sampler.h"
#include "version.h"
#ifdef SCPB
#include "scpb_editor.h"
#endif
#if !TARGET_HEADLESS
#include "shortcircuit_editor2.h"
#endif

#include <iostream>
#include <string>
#include <tinyxml/tinyxml.h>
#if WINDOWS
#include <windows.h>
#else
#include "windows_compat.h"
#endif
#include "globals.h"
#include "synthesis/mathtables.h"
#include "sample.h"
#include "sampler_voice.h"
#if TARGET_VST2
#include <AEffEditor.h>
#include <audioeffectx.h>
#endif
#include "infrastructure/logfile.h"
#include "configuration.h"
#include "interaction_parameters.h"
#include "synthesis/morphEQ.h"

#include <vt_dsp/basic_dsp.h>
#include <vt_util/vt_lockfree.h>
#include <vt_util/vt_string.h>

#include <list>
using std::list;

#include <algorithm>
using std::max;
using std::min;

float samplerate;
float samplerate_inv;
float multiplier_freq2omega;

//-------------------------------------------------------------------------------------------------

void sampler::set_samplerate(float sr)
{
    samplerate = sr;
    samplerate_inv = 1.f / sr;
    multiplier_freq2omega = pi2 * filter_freqrange / samplerate;
    VUidx = 0;
    VUrate = (int)(sr / ((float)block_size * 30.f));
    init_tables(sr, block_size);
}

//-------------------------------------------------------------------------------------------------

sampler::sampler(EditorClass *editor, int NumOutputs, WrapperClass *effect,
                 SC3::Log::LoggingCallback *cb)
    : mLogger(cb), mNumOutputs(NumOutputs)
{
    LOGINFO(mLogger) << "SC3 engine " << SC3::Build::FullVersionStr << std::flush;
    conf = new configuration(mLogger);

#if WINDOWS    
    holdengine = false;
#endif

    mpPreview = new sampler::Preview(&time_data, this);

    // MEQ is disabled
    /*	strcpy(end,"\\morphEQ-preset.xml");
            meq_loader.load(0,path);
            strcpy(end,"\\morphEQ-custom.xml");
            meq_loader.load(1,path);*/

    selected = new multiselect(this);

    ActionBuffer = new vt_LockFree(sizeof(actiondata), 0x4000, 1);

    chunkDataPtr = 0;
    dbSampleListDataPtr = 0;

    AudioHalted = true;

#ifdef SCFREE
    polyphony_cap = 2;
#else
    polyphony_cap = max_voices;
#endif

#if TARGET_VST2
    this->editor = (sc_editor2 *)(editor);
#endif
    //	this->effect = effect;
    uint32_t i, c;
    for (i = 0; i < max_voices; i++)
    {
        voices[i] = (sampler_voice *)_mm_malloc(sizeof(sampler_voice), 16);
        new (voices[i]) sampler_voice(i, &time_data);

        voice_state[i].active = false;
    }

    multi_init();

    assert(n_sampler_parts <= 16);

    for (c = 0; c < n_sampler_parts; c++)
    {
        hold[c] = false;
        for (i = 0; i < n_controllers; i++)
        {
            controllers[c * n_controllers + i] = 0;
            controllers_target[c * n_controllers + i] = 0;
        }
        part_init(c);
        parts[c].MIDIchannel = c;
        partv[c].last_ft[0] = 0;
        partv[c].last_ft[1] = 0;
        partv[c].pFilter[0] = 0;
        partv[c].pFilter[1] = 0;

        partv[c].mm = (modmatrix *)_mm_malloc(sizeof(modmatrix), 16);
        new (partv[c].mm) modmatrix();
        partv[c].mm->assign(conf, 0, &parts[c], 0, &controllers[n_controllers * c], automation,
                            &time_data);
    }

    for (int i = 0; i < 16; i++)
    {
        customcontrollers[i] = "-";
        customcontrollers_bp[i] = false;
    }

    for (i = 0; i < max_samples; i++)
        samples[i] = nullptr;
    for (i = 0; i < max_zones; i++)
        zone_exists[i] = false;

    for (i = 0; i < n_automation_parameters; i++)
        automation[i] = 0;

    for (i = 0; i < (max_outputs); i++)
    {
        vu_peak[i] = 0;
        vu_rms[i] = 0;
        vu_time = 0;
        output_ptr[i << 1] = output[i << 1];
        output_ptr[(i << 1) + 1] = output[(i << 1) + 1];
    }

    memset(keystate, 0, sizeof(keystate));

    editorpart = 0;
    editorlayer = 0;
    polyphony = 0;
    highest_voice_id = 0;
    highest_group_id = 0;
    //	set_headroom(conf->headroom);
    time_data.tempo = 120; // default tempo
    time_data.ppqPos = 0;
    sample_replace_filename[0] = 0;
    toggled_samplereplace = false;
}

//-------------------------------------------------------------------------------------------------

void sampler::multi_init()
{
    memset(&multi, 0, sizeof(multi));

    for (int i = 0; i < 8; i++)
    {
        multiv.pFilter[i] = 0;
        multiv.last_ft[i] = 0;
        multi.filter_output[i] = out_output1;
    }
}

//-------------------------------------------------------------------------------------------------

sampler::~sampler(void)
{
    free_all();
    int i;
    for (i = 0; i < max_voices; i++)
    {
        if (voices[i])
        {
            voices[i]->~sampler_voice();
            _mm_free(voices[i]);
            voices[i] = 0;
        }
    }
    for (int c = 0; c < 16; c++)
    {
        partv[c].mm->~modmatrix();
        _mm_free(partv[c].mm);
    }
    delete conf;
    delete selected;
    delete ActionBuffer;
    delete mpPreview;
    if (chunkDataPtr)
        free(chunkDataPtr);
    if (dbSampleListDataPtr)
        free(dbSampleListDataPtr);
}

bool sampler::loadUserConfiguration(const fs::path &configFile) {
            
    return conf->load(configFile);
    
}

bool sampler::saveUserConfiguration(const fs::path &configFile) { 
    return conf->save(configFile); 
}

bool sampler::zone_exist(int id)
{
    if (id >= max_zones)
        return false;
    if (id < 0)
        return false;
    return zone_exists[id];
}

//-------------------------------------------------------------------------------------------------

void sampler::SetCustomController(int Part, int ControllerIdx, float NormalizedValue)
{
    if (parts[Part].userparameterpolarity[ControllerIdx])
    {
        parts[Part].userparameter[ControllerIdx] = NormalizedValue * 2.f - 1.f;
    }
    else
    {
        parts[Part].userparameter[ControllerIdx] = NormalizedValue;
    }

    if (!wrappers.empty() && (Part == editorpart))
    {
        actiondata ad;
        ad.id = ip_part_userparam_value;
        ad.subid = ControllerIdx;
        ad.actiontype = vga_floatval;
        ad.data.f[0] = parts[Part].userparameter[ControllerIdx];
        postEventsToWrapper(ad);
    }
}

//-------------------------------------------------------------------------------------------------

void sampler::PitchBend(char channel, int value)
{
    for (int p = 0; p < n_sampler_parts; p++)
    {
        if (parts[p].MIDIchannel == channel)
        {
            controllers_target[p * n_controllers + c_pitch_bend] = (float)value * (1.f / 8192.0f);
        }
    }
}

//-------------------------------------------------------------------------------------------------

void sampler::ChannelAftertouch(char channel, int value)
{
    for (int p = 0; p < n_sampler_parts; p++)
    {
        if (parts[p].MIDIchannel == channel)
        {
            controllers_target[p * n_controllers + c_channel_aftertouch] =
                (float)value * (1.f / 127.0f);
        }
    }
}

//-------------------------------------------------------------------------------------------------

void sampler::RPNForPart(int Part, int ControllerIdx, bool IsNRPN, int value)
{
    // search for any nrpn/rpns among the custom controllers
    for (int i = 0; i < n_custom_controllers; i++)
    {
        if ((conf->MIDIcontrol[i].number == ControllerIdx) &&
            ((IsNRPN && (conf->MIDIcontrol[i].type == mct_nrpn)) ||
             (!IsNRPN && (conf->MIDIcontrol[i].type == mct_rpn))))
        {
            SetCustomController(Part, i, (float)value * (1.f / 16384.0f));
        }
    }
}

//-------------------------------------------------------------------------------------------------

void sampler::ChannelControllerForPart(int Part, int cc, int value)
{
    // Check special cases (like modwheel)
    switch (cc)
    {
    case 1: // modulation
        controllers_target[Part * n_controllers + c_modwheel] = value * (1.f / 127.0f);
        break;
    };

    // Check for Custom controllers
    for (int i = 0; i < n_custom_controllers; i++)
    {
        if ((conf->MIDIcontrol[i].type == mct_cc) && (conf->MIDIcontrol[i].number == cc))
        {
            SetCustomController(Part, i, value * (1.f / 127.f));
        }
    }
}

//-------------------------------------------------------------------------------------------------

void sampler::ChannelController(char channel, int cc, int value)
{
    switch (cc)
    {
    case 98: // NRPN LSB
        nrpn[channel][0] = value;
        nrpn_last[channel] = true;
        break;
    case 99: // NRPN MSB
        nrpn[channel][1] = value;
        nrpn_last[channel] = true;
        break;
    case 100: // RPN LSB
        rpn[channel][0] = value;
        nrpn_last[channel] = false;
        break;
    case 101: // RPN MSB
        rpn[channel][1] = value;
        nrpn_last[channel] = false;
        break;
    case 6:
        if (nrpn_last[channel])
            nrpn_v[channel][1] = value;
        else
            rpn_v[channel][1] = value;
        break;
    case 38:
        if (nrpn_last[channel])
            nrpn_v[channel][0] = value;
        else
            rpn_v[channel][0] = value;
        break;
    };

    if ((cc == 6) || (cc == 38)) // handle RPN/NRPNs (untested)
    {
        int value, cnum;
        if (nrpn_last[channel])
        {
            value = (nrpn_v[channel][1] << 7) + nrpn_v[channel][0];
            cnum = (nrpn[channel][1] << 7) + nrpn[channel][0];
        }
        else
        {
            value = (rpn_v[channel][1] << 7) + rpn_v[channel][0];
            cnum = (rpn[channel][1] << 7) + rpn[channel][0];
        }

        for (int p = 0; p < n_sampler_parts; p++)
        {
            if (parts[p].MIDIchannel == channel)
            {
                RPNForPart(p, cnum, nrpn_last[channel], value);
            }
        }
    }
    else
    {
        for (int p = 0; p < n_sampler_parts; p++)
        {
            if (parts[p].MIDIchannel == channel)
            {
                ChannelControllerForPart(p, cc, value);
            }
        }
    }

    if (cc == 64)
    {
        hold[channel] = value > 63; // check hold pedal
        purge_holdbuffer();
    }
}

//-------------------------------------------------------------------------------------------------

void sampler::purge_holdbuffer()
{
    std::lock_guard g(cs_engine);
    int z;
    list<int>::iterator iter = holdbuffer.begin();
    while (1)
    {
        if (iter == holdbuffer.end())
            break;
        z = *iter;
        if (voice_state[z].active && !hold[voice_state[z].channel])
        {
            voices[z]->release(127);
            list<int>::iterator del = iter;
            iter++;
            holdbuffer.erase(del);
        }
        else
            iter++;
    }
    // note: m�ste remova entries n�r noter d�dar sig sj�lv auch
}

//-------------------------------------------------------------------------------------------------

void sampler::update_zone_switches(int z)
{
    if (!zone_exists[z])
        return;

    modmatrix mm;
    mm.assign(conf, &zones[z], 0);
    zones[z].element_active =
        (mm.is_source_used(ss_LFO1) & ve_LFO1) | (mm.is_source_used(ss_LFO2) & ve_LFO2) |
        (mm.is_source_used(ss_LFO3) & ve_LFO3) | (mm.is_source_used(ss_EG2) & ve_EG2);
}

//-------------------------------------------------------------------------------------------------

bool sampler::clone_zone(int zone_id, int *new_z, bool same_key)
{
    if (!zone_exists[zone_id])
    {
        return false;
    }

    // find free zone and create zone object
    int i = GetFreeZoneId();
    if (i < 0)
        return false;

    memcpy(&zones[i], &zones[zone_id], sizeof(sample_zone));

    uint32_t s = zones[zone_id].sample_id;
    // increase refcount for sample
    if (samples[s])
        samples[s]->remember();

    zone_exists[i] = true;

    if (!same_key)
    {
        zones[i].key_root = find_next_free_key(zones[i].part);
        zones[i].key_low = zones[i].key_root;
        zones[i].key_high = zones[i].key_root;
    }

    if (new_z)
        (*new_z) = i;

    return true;
}

//-------------------------------------------------------------------------------------------------

bool sampler::slice_to_zone(int zone_id, int slice)
{
    if (slice < zones[zone_id].n_hitpoints)
    {
        int nz;
        if (clone_zone(zone_id, &nz))
        {
            zones[nz].playmode = pm_forward;
            zones[nz].n_hitpoints = 1;
            zones[nz].sample_start = zones[zone_id].hp[slice].start_sample;
            zones[nz].sample_stop = zones[zone_id].hp[slice].end_sample;
            zones[nz].hp[0].start_sample = zones[nz].sample_start;
            zones[nz].hp[0].end_sample = zones[nz].sample_stop;
            zones[nz].hp[0].env = zones[zone_id].hp[slice].env;

            zones[nz].loop_start = zones[nz].sample_start;
            zones[nz].loop_end = zones[nz].sample_stop;

            sprintf(zones[nz].name, "%s s%i", zones[zone_id].name, slice + 1);
            zones[nz].key_root = zones[zone_id].key_root + slice;
            zones[nz].key_high = zones[nz].key_root;
            zones[nz].key_low = zones[nz].key_root;

            return true;
        }
    }
    return false;
}

//-------------------------------------------------------------------------------------------------

bool sampler::slices_to_zones(int zone_id)
{
    if (zones[zone_id].n_hitpoints > 0)
    {
        int i;
        for (i = 0; i < zones[zone_id].n_hitpoints; i++)
        {
            this->slice_to_zone(zone_id, i);
        }
        zones[zone_id].mute = true;

        return true;
    }
    return false;
}

//-------------------------------------------------------------------------------------------------

bool sampler::get_key_name(char *str, int channel, int key)
{
    for (int z = 0; z < max_zones; z++)
    {
        if (zone_exists[z])
        {
            int p = zones[z].part;
            int zkey = key + parts[p].transpose - parts[p].formant;

            if ((zones[z].key_low <= zkey) && (zones[z].key_high >= zkey) &&
                (parts[p].MIDIchannel == channel))
            {
                vtCopyString(str, zones[z].name, 63);
                return true;
            }
        }
    }
    strcpy(str, "");
    return false;
}

//-------------------------------------------------------------------------------------------------

bool sampler::get_sample_id(const fs::path &filename, int *s_id)
{
    std::lock_guard g(cs_patch);
    if (filename.empty())
    {
        if (s_id)
            *s_id = -1;
        return true; // already exists, one could say, because it should not be charged
    }

    auto fnstr = path_to_string(filename);

    if (!strncmp("loaded", fnstr.c_str(), 6))
    {
        if (s_id)
            *s_id = atoi(fnstr.c_str() + 6);
        return (samples[*s_id & (max_samples - 1)] != NULL);
    }


    int s;
    for (s = 0; s < max_samples; s++)
    {
        if (samples[s])
        {
            if (samples[s]->compare_filename(fnstr.c_str()))
            {
                if (s_id)
                    *s_id = s;
                return true;
            }
        }
    }
    return false;
}

//-------------------------------------------------------------------------------------------------

void sampler::InitZone(int i) { SInitZone(&zones[i]); }

//-------------------------------------------------------------------------------------------------

void sampler::SInitZone(sample_zone *pZone)
{
    memset(pZone, 0, sizeof(sample_zone));

    pZone->database_id = -1;
    pZone->key_low_fade = 0;
    pZone->key_high_fade = 0;
    pZone->velocity_low_fade = 0;
    pZone->velocity_high_fade = 0;

    pZone->transpose = 0;
    pZone->finetune = 0;
    pZone->pitchcorrection = 0;
    pZone->pitch_bend_depth = 2;
    pZone->velocity_low = 0;
    pZone->velocity_high = 127;
    pZone->velsense = -20.f;
    pZone->keytrack = 1.f;
    pZone->pre_filter_gain = 0;
    pZone->AEG.attack = -10.0f;
    pZone->AEG.hold = -10.0f;
    pZone->AEG.decay = 0.f;
    pZone->AEG.sustain = 1.0f;
    pZone->AEG.release = -4.5f;
    pZone->AEG.shape[0] = 0.f;
    pZone->AEG.shape[1] = 1.f;
    pZone->AEG.shape[2] = 1.f;
    pZone->EG2.attack = -2.0f;
    pZone->EG2.hold = -10.0f;
    pZone->EG2.decay = -2.f;
    pZone->EG2.sustain = 1.0f;
    pZone->EG2.release = -5.f;
    //	pZone->ef_attack = 0;
    //	pZone->ef_release = 0;
    pZone->Filter[0].mix = 1.f;
    pZone->Filter[1].mix = 1.f;
    pZone->aux[0].level = 0;
    pZone->aux[0].balance = 0;
    pZone->aux[0].output = 0;
    pZone->aux[1].level = 0;
    pZone->aux[1].balance = 0;
    pZone->aux[1].output = out_fx1;
    pZone->aux[1].outmode = 0;
    pZone->aux[2].level = 0;
    pZone->aux[2].balance = 0;
    pZone->aux[2].output = out_fx2;
    pZone->aux[2].outmode = 0;
    pZone->playmode = pm_forward;
    pZone->sample_start = 0;
    pZone->reverse = 0;

    pZone->loop_crossfade_length = 1024;
    pZone->lag_generator[0] = 0.f;
    pZone->lag_generator[1] = 0.f;
    pZone->n_hitpoints = 1;
    pZone->hp[0].start_sample = 0;
    pZone->hp[0].end_sample = 0;

    pZone->hp[0].env = 0;
    pZone->mute_group = 0;
    /*	pZone->polymode = 0;
            pZone->portamento = -10;
            pZone->portamento_mode = 0;*/
    int mm;
    for (mm = 0; mm < mm_entries; mm++)
    {
        pZone->mm[mm].source = 0;
        pZone->mm[mm].source2 = 0;
        pZone->mm[mm].strength = 0.0f;
        pZone->mm[mm].destination = 0;
        pZone->mm[mm].curve = 0;
        pZone->mm[mm].active = 1;
    }
    for (mm = 0; mm < nc_entries; mm++)
    {
        pZone->nc[mm].source = 0;
        pZone->nc[mm].low = 0;
        pZone->nc[mm].high = 127;
    }

    pZone->LFO[0].repeat = 16;
    pZone->LFO[1].repeat = 16;
    pZone->LFO[2].repeat = 16;

    vtCopyString(pZone->name, "empty", 31);
}

//-------------------------------------------------------------------------------------------------

bool sampler::add_zone(const fs::path &filename, int *new_z, char part, bool use_root_key)
{
    // find free zone and create zone object
    int i = GetFreeZoneId();
    if (i < 0)
        return false;

    // check if sample is loaded already
    int32_t s = 0;
    if (!filename.empty())
    {
        // AS this guy seems to want to parse out the first few chars of filename which seems fishy... leave as is for now.
        bool is_loaded = get_sample_id(path_to_string(filename).c_str(), &s);

        if (is_loaded)
        {
            samples[s]->remember(); // increase refcount
        }
        else
        {
            // if it's not loaded,
            // find a free sample slot and load it
            s = this->GetFreeSampleId();
            if (s < 0)
                return false;

            samples[s] = new sample(conf);

            if (!(samples[s]->load(filename)))
            {
                delete samples[s];
                samples[s] = 0;
                // return false;
                s = -1; // failed to load
            }
        }
    }
    else
        s = -1;


    std::lock_guard lockUntilEnd( cs_patch );
    InitZone(i);

    zones[i].part = part;
    zones[i].layer = editorlayer;
    zones[i].sample_id = s;
    zones[i].key_root = find_next_free_key(zones[i].part);
    zones[i].key_low = zones[i].key_root;
    zones[i].key_high = zones[i].key_root;

    if (s >= 0)
    {
        zones[i].sample_stop = samples[s]->sample_length;
        zones[i].loop_end = samples[s]->sample_length;

        zones[i].hp[0].end_sample = samples[s]->sample_length;
        zones[i].pitchcorrection = samples[s]->meta.detune;

        /*if(samples[s]->inst_present)
        {
                zones[i].key_high = samples[s]->inst_tag.bHighNote;
                zones[i].key_low = samples[s]->inst_tag.bLowNote;
                zones[i].key_root = samples[s]->inst_tag.bUnshiftedNote;
                zones[i].velocity_high = samples[s]->inst_tag.bHighVelocity;
                zones[i].velocity_low = samples[s]->inst_tag.bLowVelocity;
                //zones[i].amplitude = powf(10,samples[s]->inst_tag.chGain/20.0f);
                zones[i].finetune = samples[s]->inst_tag.chFineTune/128.0f;
        }*/

        // move loop & slices transfer from the sample to a separate function when it is
        // updates of it so that it only exists in one place
        if (use_root_key && samples[s]->meta.rootkey_present)
        {
            zones[i].key_root = samples[s]->meta.key_root;
            if (samples[s]->meta.key_present)
            {
                zones[i].key_low = samples[s]->meta.key_low;
                zones[i].key_high = samples[s]->meta.key_high;
            }
            if (samples[s]->meta.vel_present)
            {
                zones[i].velocity_low = samples[s]->meta.vel_low;
                zones[i].velocity_high = samples[s]->meta.vel_high;
            }
        }
        if (samples[s]->meta.loop_present)
        {
            zones[i].loop_start = samples[s]->meta.loop_start;
            zones[i].loop_end = samples[s]->meta.loop_end;
            zones[i].playmode = pm_forward_loop;
        }
        if (samples[s]->meta.playmode_present)
            zones[i].playmode = samples[s]->meta.playmode;

        if (samples[s]->meta.n_slices > 1)
        {
            zones[i].n_hitpoints = min(max_hitpoints - 1, samples[s]->meta.n_slices);
            zones[i].key_high = min(127, zones[i].key_low + samples[s]->meta.n_slices - 1);
            int j;

            for (j = 0; j < zones[i].n_hitpoints; j++)
            {
                zones[i].hp[j].start_sample = samples[s]->meta.slice_start[j];
                zones[i].hp[j].end_sample = samples[s]->meta.slice_end[j];
            }
        }

        sprintf(zones[i].name, samples[s]->GetName());
    }
    else
    {
        sprintf(zones[i].name, "empty");
    }
    if (new_z)
        *new_z = i;
    zone_exists[i] = true;
    update_zone_switches(i);
    return true;
}

//-------------------------------------------------------------------------------------------------

bool sampler::replace_zone(int z, const fs::path &fileName)
{
    // ATTENTION !!! if sample refcount> 1 then the sampling should only be changed for the current zone !!
    // kill all notes for the given zone
    kill_notes(z);
    int s_old = zones[z].sample_id;

    int s = 0;
    {
        std::lock_guard g(cs_patch);

        s = GetFreeSampleId();
        if (s < 0)
            return false;
        samples[s] = new sample(conf);
    }

    if (!(samples[s]->load(fileName)))
    {
        delete samples[s];
        samples[s] = 0;
        return false;
    }


    std::lock_guard lockUntilEnd( cs_patch );
    if ((s_old >= 0) && samples[s_old]->forget())
    {
        delete samples[s_old];
        samples[s_old] = 0;
    }

    zones[z].sample_id = s;
    zones[z].sample_start = 0;
    zones[z].sample_stop = samples[s]->sample_length;
    zones[z].hp[0].start_sample = 0;

    if (samples[s]->meta.n_slices > 1)
    {
        zones[z].n_hitpoints = min(max_hitpoints - 1, samples[s]->meta.n_slices);
        // zones[z].key_high = min(127,zones[z].key_low + samples[s]->meta.n_slices - 1);

        for (unsigned int j = 0; j < zones[z].n_hitpoints; j++)
        {
            zones[z].hp[j].start_sample = samples[s]->meta.slice_start[j];
            zones[z].hp[j].end_sample = samples[s]->meta.slice_end[j];
        }
    }

    // if (samples[s]->smpl_chunk.dwMIDIUnityNote) zones[z].key_root = (char)
    // samples[s]->smpl_chunk.dwMIDIUnityNote;
    if (samples[s]->meta.loop_present)
    {
        zones[z].loop_start = samples[s]->meta.loop_start;
        zones[z].loop_end = samples[s]->meta.loop_end;
        zones[z].pitchcorrection = samples[s]->meta.detune;
        // if (samples[s]->smpl_loop.dwType == 0)
        if (s_old < 0) // if no sample was loaded prior
        {
            zones[z].playmode = pm_forward_loop;
        }
        else
        {
            switch (zones[z].playmode) // if loop-points are present, keep the playmode
            {
            case pm_forward_loop:
            case pm_forward_loop_bidirectional:
                //		case pm_forward_loop_crossfade:
            case pm_forward_loop_until_release:
                break;
            default:
                zones[z].playmode = pm_forward;
                break;
            }
        }
    }
    else
    {
        switch (zones[z].playmode) // if loop-points aren't present, but the playmode want to use
                                   // it, change to pm_forward
        {
        case pm_forward_loop:
        case pm_forward_loop_bidirectional:
        // case pm_forward_loop_crossfade:
        case pm_forward_loop_until_release:
            zones[z].playmode = pm_forward;
            break;
        }
    }

    std::string nameOnly;
    decode_path(fileName, nullptr, nullptr, &nameOnly);
    vtCopyString(zones[z].name, nameOnly.c_str(), 32);
    update_zone_switches(z);
    return true;
}

bool sampler::free_zone(uint32_t zoneid)
{
    if (!zone_exists[zoneid])
        return false;
    std::lock_guard g(cs_patch);
    kill_notes(zoneid);
    zone_exists[zoneid] = false;
    if ((zones[zoneid].sample_id >= 0) && samples[zones[zoneid].sample_id]->forget())
    {
        delete samples[zones[zoneid].sample_id];
        samples[zones[zoneid].sample_id] = 0;
    }
    return true;
}

//-------------------------------------------------------------------------------------------------

void sampler::free_all()
{
    int i;
    for (i = 0; i < max_zones; i++)
    {
        if (zone_exists[i])
        {
            free_zone(i);
        }
    }

    int c;
    for (c = 0; c < 16; c++)
    {
        hold[c] = false;
        part_init(c, false);
        parts[c].MIDIchannel = c;
    }

    for (int i = 0; i < 16; i++)
    {
        customcontrollers[i] = "-";
        customcontrollers_bp[i] = false;
    }

    update_highest_voice_id();

    holdbuffer.clear();
}

//-------------------------------------------------------------------------------------------------

int sampler::GetFreeSampleId()
{
    int i;
    for (i = 0; i < max_samples; i++)
    {
        if (!samples[i])
        {
            return i;
        }
    }
    return -1;
}

//-------------------------------------------------------------------------------------------------

int sampler::GetFreeZoneId()
{
    int i;
    for (i = 0; i < max_zones; i++)
    {
        if (!zone_exists[i])
        {
            return i;
        }
    }

#if WINDOWS
    MessageBox(::GetActiveWindow(),
               "Zone limit reached.\n\nPlease nag at developer to increase the limit.",
               "Too many zones", MB_OK | MB_ICONERROR);
#else
#warning Implement user feedback
    SC3::Log::logos() << "Zone limit reached" << std::endl;
#endif

    return -1;
}

//-------------------------------------------------------------------------------------------------

int sampler::find_next_free_key(int part)
{
    int i, key = 35;
    // find highest key
    for (i = 0; i < max_zones; i++)
    {
        if (zone_exists[i] && (zones[i].part == part))
        {
            key = max(key, zones[i].key_high);
        }
    }
    return key + 1;
}

//-------------------------------------------------------------------------------------------------

void sampler::idle()
{
    if (sample_replace_filename[0])
    {
        if (!get_zone_poly(selected->get_active_id()))
        {
            if (zone_exist(selected->get_active_id()) && (selected->get_active_type() == 1))
            {
                replace_zone(selected->get_active_id(), string_to_path(sample_replace_filename));
            }
            sample_replace_filename[0] = 0;
        }
    }
}

//-------------------------------------------------------------------------------------------------

void sampler::part_clear_zones(int p)
{
    std::lock_guard g(cs_patch);
    int i;
    for (i = 0; i < max_zones; i++)
    {
        if ((zone_exists[i]) && (zones[i].part == p))
        {
            free_zone(i);
        }
    }
}

//-------------------------------------------------------------------------------------------------

void sampler::part_init(int p, bool clear_zones, bool leave_outputs)
{
    // only set state (parts) and let the "part voice" (partv) be updated elsewhere
    int oldmc = parts[p].MIDIchannel;
    aux_buss tempaux[3];

    if (leave_outputs)
    {
        memcpy(tempaux, parts[p].aux, sizeof(aux_buss) * 3);
        memset(&parts[p], 0, sizeof(sample_part));
        memcpy(parts[p].aux, tempaux, sizeof(aux_buss) * 3);
    }
    else
        memset(&parts[p], 0, sizeof(sample_part));

    parts[p].MIDIchannel = oldmc;
    parts[p].database_id = -1;
    if (!leave_outputs)
    {
        parts[p].aux[0].level = 0;
        parts[p].aux[1].level = 0;
        parts[p].aux[2].level = 0;
        parts[p].aux[0].balance = 0;
        parts[p].aux[1].balance = 0;
        parts[p].aux[2].balance = 0;
        parts[p].aux[0].output = out_output1;
        parts[p].aux[1].output = out_fx1;
        parts[p].aux[2].output = out_fx2;
        parts[p].aux[1].outmode = 0;
        parts[p].aux[2].outmode = 0;
    }

    for (int mm = 0; mm < mm_part_entries; mm++)
    {
        parts[p].mm[mm].active = 1;
    }

    for (int mm = 0; mm < num_part_ncs; mm++)
    {
        parts[p].nc[mm].high = 127;
    }

    parts[p].Filter[0].mix = 1.f;
    parts[p].Filter[1].mix = 1.f;
    parts[p].polylimit = 32;
    parts[p].portamento = -10;
    parts[p].polymode = polymode_poly;
    parts[p].vs_xf_equality = 1;
    strcpy(parts[p].name, "init");

    for (int i = 0; i < 16; i++)
    {
        strcpy(parts[p].userparametername[i], "");
    }

    if (clear_zones)
        part_clear_zones(p);
}

//=========================================================================================
//
// Browser Sample-Preview Class
//
//=========================================================================================

sampler::Preview::Preview(timedata *pTD, sampler *pParent)
{
    // Init Preview voice/zone/sample
    mpParent = pParent;
    mpVoice = (sampler_voice *)_mm_malloc(sizeof(sampler_voice), 16);
    new (mpVoice) sampler_voice(0, pTD);
    mpSample = new sample(mpParent->conf);
    mActive = false;
    mAutoPreview = mpParent->conf->mAutoPreview;
    mFilename[0] = 0;

    memset(&mPart, 0, sizeof(sample_part));
    mPart.portamento_mode = 0;
    mPart.portamento = -10.f;
    SInitZone(&mZone);
}

//-----------------------------------------------------------------------------------------

sampler::Preview::~Preview()
{
    mpVoice->~sampler_voice();
    _mm_free(mpVoice);
    delete mpSample;
}

//-----------------------------------------------------------------------------------------

void sampler::Preview::Start(const wchar_t *Filename)
{
    mActive = false;

    
    // wchar_t conversion to path not working. figure out later or change to take path above

    //auto ppath = string_to_path(Filename);

    char fnu8[2048];
    vtWStringToString(fnu8, Filename, 2048);
    auto ppath = string_to_path(fnu8);

    if (mpSample->load(ppath))
    {
        mZone.sample_start = 0;
        mZone.sample_stop = mpSample->sample_length;
        mZone.key_root = 60;
        mZone.playmode = pm_forward_shot;
        mZone.aux[0].level = mpParent->conf->mPreviewLevel;
        mpVoice->play(mpSample, &mZone, &mPart, 60, 127, 0, mpParent->controllers,
                      mpParent->automation, 1.f);

        SetPlayingState(true);
    }
}

//-----------------------------------------------------------------------------------------

void sampler::Preview::Stop() { mpVoice->uberrelease(); }

//-----------------------------------------------------------------------------------------

void sampler::Preview::SetPlayingState(bool State)
{
    mActive = State;

    actiondata ad;
    ad.id = ip_browser_previewbutton;
    ad.subid = -1; // send to all
    ad.actiontype = vga_intval;
    ad.data.i[0] = State ? 1 : 0;
    mpParent->postEventsToWrapper(ad, false);
}

//-----------------------------------------------------------------------------------------
