//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004-2006 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "configuration.h"
#include "sample.h"
#include "sampler.h"
#include "sampler_state.h"
#include "synthesis/modmatrix.h"
#include <assert.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <tinyxml/tinyxml.h>

#include "globals.h"
#include <string.h>
//#include <wx/wfstream.h>
//#include <wx/zipstrm.h>

#if WINDOWS
#include <shlobj.h>
#include <windows.h>
#else
#include "windows_compat.h"
#endif

#include <string>
using std::max;
using std::min;
using std::string;

#if !TARGET_HEADLESS
#include "shortcircuit_editor2.h"
#endif

#include "infrastructure/logfile.h"
#include "infrastructure/file_map_view.h"
#include "filesystem/import.h"

#include <vt_util/vt_string.h>

const int ff_revision = 10;

// rev 2 replaced filter numerical id's with identifier strings (beta 2)
// rev 3 is intermediary format where none of the specs are really known
// rev 4 will have (amplitude^2 replaced as dB's as well as exponential lfo & envelope times) LOTS
// of changes rev 5: slices have endpoints as well.. the version field stores the actual version rev
// 6: sawtooth osc amplitude scales better with unison count.. rev 7: ringmod & freqshift have
// amount controls rev 8: c0-c15 are now stored WITHOUT the name of the controller (as originally
// intended), sf2 relative addressing are now delimited with a '|' sign instead of a comma rev 8:
// was also saving effect-id's incorrectly rev 9: SF2 now actually(!) stores files as |,

// rev 10: SC V2.. HUGE amount of changes

/*bool sampler::load_file(const WCHAR *filename, char part, int *new_z)
{
        // syntax:
        // "path\file.ext>patchid"	- for program id
        // "path\file.ext|sampleid"	- for addressing within multi-sample containers

        assert(0);

        return false;
}*/

bool sampler::load_file(const fs::path &file_name, int *new_g, int *new_z, bool *is_group, char channel,
                        int add_zones_to_groupid, bool replace)
{
    // AS TODO any fn taking a filename should be fixed to propagate this path object downward
    fs::path validFileName;
    int programid = 0;
    int sampleid = 0;
    std::string extension, nameOnly;    
    fs::path pathOnly;

    decode_path(file_name, &validFileName, &extension, &nameOnly, &pathOnly, &programid, &sampleid );

    if ((!extension.compare("wav")) || (!extension.compare("aif")) ||
        (!extension.compare("aiff")) || (!extension.compare("rcy")) ||
        (!extension.compare("rex")) || (!extension.compare("rx2")))
    {
        if (is_group)
            *is_group = false;
        return add_zone(path_to_string(validFileName).c_str(), new_z, channel, add_zones_to_groupid != 0);
    }
    else if ((!extension.compare("gig")) || (!extension.compare("dls")) ||
             (!extension.compare("sc2p")) || (!extension.compare("sc2m")) ||
             (!extension.compare("sfz")))
    {
        // memory mapped file reads go here
        // TODO move all file formats here as they manage memmapping

        // assign path to configuration
        conf->set_relative_path(pathOnly);

        auto mapper = std::make_unique<SC3::FileMapView>(validFileName);
        if (!mapper->isMapped())
            return false;

        auto data = mapper->data();
        auto datasize = mapper->dataSize();

        bool result = false;

        if ((!extension.compare("sc2p")) || (!extension.compare("sc2m")))
        {
            if (is_group)
                *is_group = true;
            result = LoadAllFromRIFF(data, datasize, replace, channel);
        }
        else if ((!extension.compare("gig")) || (!extension.compare("dls")))
        {
            if (is_group)
                *is_group = true;
            result = parse_dls_preset(data, datasize, channel, programid,
                path_to_string(validFileName).c_str());
        }
        else if (!extension.compare("sfz"))
        {
            if (is_group)
                *is_group = true;
            result = load_sfz((char *)data, datasize, new_g, channel);
            vtCopyString(parts[channel].name, nameOnly.c_str(), 32);
            // TODO add name from last part of filename
        }

        return result;
    }
    else if (!extension.compare("akp"))
    {
        if (is_group)
            *is_group = true;
        return load_akai_s6k_program(path_to_string(validFileName).c_str(), channel, true);
    }
    else if (!extension.compare("kit"))
    {
        if (is_group)
            *is_group = true;
        return load_battery_kit(path_to_string(validFileName).c_str(), channel, true);
    }
    else if (!extension.compare("scg"))
    {
        if (is_group)
            *is_group = true;
        if (new_g)
            *new_g = 0;
        return load_all_from_sc1_xml(0, 0, validFileName, false, channel);
    }
    else if (!extension.compare("scm"))
    {
        if (is_group)
            *is_group = true;
        if (new_g)
            *new_g = 0;
        return load_all_from_sc1_xml(0, 0, validFileName, replace);
    }
    else if (!extension.compare("sf2"))
    {
        if (is_group)
            *is_group = true;
        return load_sf2_preset(path_to_string(validFileName).c_str(), new_g, channel, programid);
    }
    return false;
}

char *float_to_str(float value, char *str)
{
    if (!str)
        return 0;
    sprintf(str, "%f", value);
    return str;
}

char *yes_no(int value, char *str)
{
    if (!str)
        return 0;
    if (value)
        sprintf(str, "yes");
    else
        sprintf(str, "no");
    return str;
}

bool is_yes(const char *str)
{
    if (!str)
        return 0;
    if (stricmp(str, "yes") == 0)
        return 1;
    return 0;
}

void sampler::recall_zone_from_element(TiXmlElement &element, sample_zone *zone, int revision,
                                       configuration *conf)
{
    assert(conf != 0);
    double d;
    int i;
    const char *tstring;
    vtCopyString(zone->name, element.Attribute("name"), 32);

    element.Attribute("layer", &i);
    zone->layer = i;
    element.Attribute("key_root", &i);
    zone->key_root = i;
    element.Attribute("key_low", &i);
    zone->key_low = i;
    element.Attribute("key_high", &i);
    zone->key_high = i;
    element.Attribute("lo_vel", &i);
    zone->velocity_low = i;
    element.Attribute("hi_vel", &i);
    zone->velocity_high = i;

    element.Attribute("key_low_fade", &i);
    zone->key_low_fade = i;
    element.Attribute("key_high_fade", &i);
    zone->key_high_fade = i;
    element.Attribute("lo_vel_fade", &i);
    zone->velocity_low_fade = i;
    element.Attribute("hi_vel_fade", &i);
    zone->velocity_high_fade = i;

    // element.Attribute("part",&i);				zone->part = i;		// don't store (stored in the parent
    // instead)
    element.Attribute("output", &i);
    zone->aux[0].output = i;
    if (element.QueryIntAttribute("aux_output", &i) == TIXML_SUCCESS)
        zone->aux[1].output = i;
    else
        zone->aux[1].output = -1;

    element.Attribute("transpose", &i);
    zone->transpose = i;
    element.Attribute("finetune", &d);
    zone->finetune = (float)d;
    element.Attribute("pitchcorrect", &d);
    zone->pitchcorrection = (float)d;

    tstring = element.Attribute("playmode");
    zone->playmode = pm_forward;
    for (i = 0; i < n_playmodes; i++)
        if (!strcmp(tstring, playmode_abberations[i]))
            zone->playmode = i;

    element.Attribute("velsense", &d);
    zone->velsense = (float)d;

    if (element.QueryDoubleAttribute("keytrack", &d) == TIXML_SUCCESS)
        zone->keytrack = (float)d;
    else
        zone->keytrack = 1;

    element.Attribute("sample_start", &i);
    zone->sample_start = i;
    element.Attribute("sample_end", &i);
    zone->sample_stop = i;
    element.Attribute("loop_start", &i);
    zone->loop_start = i;
    element.Attribute("loop_end", &i);
    zone->loop_end = i;
    element.Attribute("loop_crossfade_length", &i);
    zone->loop_crossfade_length = i;

    element.Attribute("amplitude", &d);
    zone->aux[0].level = (float)d;
    if (element.QueryDoubleAttribute("aux_level", &d) == TIXML_SUCCESS)
        zone->aux[1].level = (float)d;
    element.Attribute("pan", &d);
    zone->aux[0].balance = (float)d;
    element.Attribute("prefilter_gain", &d);
    zone->pre_filter_gain = (float)d;
    element.Attribute("PB_depth", &i);
    zone->pitch_bend_depth = i;

    element.Attribute("mute_group", &i);
    zone->mute_group = i;
    element.Attribute("ignore_polymode", &i);
    zone->ignore_part_polymode = i;
    //	element.Attribute("polymode",&i);		zone->polymode = i;
    //	element.Attribute("portamode",&i);		zone->portamento_mode = i;
    //	if (element.QueryDoubleAttribute("portamento",&d) == TIXML_SUCCESS) zone->portamento =
    //(float)d; 	else zone->portamento = -10.f;

    zone->mute = is_yes(element.Attribute("mute"));

    //	element.Attribute("ef_attack",&d);			zone->ef_attack = (float)d;
    //	element.Attribute("ef_release",&d);			zone->ef_release = (float)d;
    element.Attribute("lag0", &d);
    zone->lag_generator[0] = (float)d;
    element.Attribute("lag1", &d);
    zone->lag_generator[1] = (float)d;

    // filters
    TiXmlElement *sub;
    int j;
    sub = element.FirstChild("output")->ToElement();
    while (sub)
    {
        sub->Attribute("i", &j);
        if (j < 3)
        {
            if (sub->QueryDoubleAttribute("level", &d) == TIXML_SUCCESS)
                zone->aux[j].level = d;
            if (sub->QueryDoubleAttribute("balance", &d) == TIXML_SUCCESS)
                zone->aux[j].balance = d;
            if (sub->QueryIntAttribute("output", &i) == TIXML_SUCCESS)
                zone->aux[j].output = i;
            if (j && (sub->QueryIntAttribute("outmode", &i) == TIXML_SUCCESS))
                zone->aux[j].outmode = i;
        }
        sub = sub->NextSibling("output")->ToElement();
    }

    sub = element.FirstChild("filter")->ToElement();
    while (sub)
    {
        sub->Attribute("i", &j);
        if (j < 2)
        {
            char abb[16];
            if (revision < 2)
            {
                // convert old (beta1) filter numbers to abberation
                sub->Attribute("type", &i);
                if ((i >= 0) && (i < 14))
                    vtCopyString(abb, filter_abberations_beta1[i], 16);
                else
                    strcpy(abb, "NONE");
            }
            else
            {
                vtCopyString(abb, sub->Attribute("type"), 16);
            }
            // find filter abberation in list, if not found, set to NONE
            int l, ft = 0;
            for (l = 0; l < ft_num_types; l++)
            {
                if (strcmp(filter_abberations[l], abb) == 0)
                    ft = l;
            }
            zone->Filter[j].type = ft;

            if (sub->QueryDoubleAttribute("mix", &d) == TIXML_SUCCESS)
                zone->Filter[j].mix = d;
            zone->Filter[j].bypass = is_yes(sub->Attribute("bypass"));

            int k;
            for (k = 0; k < n_filter_parameters; k++)
            {
                char label[9];
                sprintf(label, "p%i", k);

                // TODO restore morphEQ import
                /*				if((ft == ft_morpheq)&&(k<2)&&(revision<10))
                {
                        int bank, patch;
                        sub->Attribute(label,&patch);
                        sprintf(label,"pb%i",k);
                        sub->Attribute(label,&bank);
                        zone->Filter[j].p[k] = meq_loader.get_id(bank,patch);
                }
                else */
                {
                    if (sub->QueryDoubleAttribute(label, &d) == TIXML_SUCCESS)
                        zone->Filter[j].p[k] = (float)d;
                }
            }
            for (k = 0; k < n_filter_iparameters; k++)
            {
                char label[9];
                sprintf(label, "ip%i", k);
                if (sub->QueryIntAttribute(label, &i) == TIXML_SUCCESS)
                    zone->Filter[j].ip[k] = i;
            }
            // rev 6: sawtooth osc amplitude scales better with unison count..
            // kompensera gain f�r gamla filer
            if ((revision < 6) && (ft == ft_osc_saw))
            {
                float d = max(1.f, (float)sqrt(zone->Filter[j].p[4]));
                zone->Filter[j].p[2] *= 1 / d;
            }

            // rev 7: ringmod & freqshift have amount controls
            if (((ft == ft_fx_freqshift) || (ft == ft_fx_ringmod)) && (revision < 7))
            {
                zone->Filter[j].p[1] = 1.f;
                if (ft == ft_fx_ringmod)
                    zone->Filter[j].p[2] = 1.f;
            }
        }
        sub = sub->NextSibling("filter")->ToElement();
    }

    // envelopes
    envelope_AHDSR *env[2];
    env[0] = &zone->AEG;
    env[1] = &zone->EG2;

    sub = element.FirstChild("envelope")->ToElement();
    while (sub)
    {
        sub->Attribute("i", &j);
        if (j < 2)
        {
            sub->Attribute("attack", &d);
            env[j]->attack = (float)d;
            sub->Attribute("attack_shape", &d);
            env[j]->shape[0] = (float)d;
            sub->Attribute("hold", &d);
            env[j]->hold = (float)d;
            sub->Attribute("decay", &d);
            env[j]->decay = (float)d;
            sub->Attribute("decay_shape", &d);
            env[j]->shape[1] = (float)d;
            sub->Attribute("sustain", &d);
            env[j]->sustain = (float)d;
            sub->Attribute("release", &d);
            env[j]->release = (float)d;
            sub->Attribute("release_shape", &d);
            env[j]->shape[2] = (float)d;
        }
        sub = sub->NextSibling("envelope")->ToElement();
    }

    sub = element.FirstChild("lfo")->ToElement();
    while (sub)
    {
        sub->Attribute("i", &j);
        if (j < 3)
        {
            sub->Attribute("rate", &d);
            zone->LFO[j].rate = (float)d;
            sub->Attribute("smooth", &d);
            zone->LFO[j].smooth = (float)d;
            sub->Attribute("cyclemode", &i);
            zone->LFO[j].cyclemode = i;
            sub->Attribute("triggermode", &i);
            zone->LFO[j].triggermode = i;
            sub->Attribute("temposync", &i);
            zone->LFO[j].temposync = i;
            sub->Attribute("repeat", &i);
            zone->LFO[j].repeat = i;
            sub->Attribute("once", &i);
            zone->LFO[j].onlyonce = i;
            int k;
            for (k = 0; k < 32; k++)
            {
                char label[9];
                sprintf(label, "step%i", k);
                if (sub->QueryDoubleAttribute(label, &d) == TIXML_SUCCESS)
                    zone->LFO[j].data[k] = (float)d;
            }
        }

        sub = sub->NextSibling("lfo")->ToElement();
    }

    modmatrix t_mm;
    t_mm.assign(conf, zone, 0);

    sub = element.FirstChild("modulation")->ToElement();
    while (sub)
    {
        int id;
        sub->Attribute("i", &id);
        if (id < mm_entries)
        {
            int x;
            const char *ts;
            // ts = sub->Attribute("src");
            // rev8 undvik att namnet p� controllern lagras
            char msrc[256];
            vtCopyString(msrc, sub->Attribute("src"), 256);
            char *colon = strrchr(msrc, ':');
            if (colon && (msrc[0] == 'c' && (revision < 8)))
            {
                *colon = 0;
            }
            // zone->mm[id].source = 0;
            for (x = 0; x < t_mm.get_n_sources(); x++)
            {
                if (stricmp(t_mm.get_source_idname(x), msrc) == 0)
                    zone->mm[id].source = x;
            }
            ts = sub->Attribute("dest");
            // zone->mm[id].destination = 0;
            for (x = 0; x < t_mm.get_n_destinations(); x++)
            {
                if (stricmp(t_mm.get_destination_idname(x), ts) == 0)
                    zone->mm[id].destination = x;
            }

            ts = sub->Attribute("src2");
            // zone->mm[id].destination = 0;
            for (x = 0; x < t_mm.get_n_sources(); x++)
            {
                if (stricmp(t_mm.get_source_idname(x), ts) == 0)
                    zone->mm[id].source2 = x;
            }

            sub->Attribute("amount", &d);
            zone->mm[id].strength = (float)d;
            sub->Attribute("curve", &zone->mm[id].curve);
            sub->Attribute("active", &zone->mm[id].active);
        }
        sub = sub->NextSibling("modulation")->ToElement();
    }

    sub = element.FirstChild("nc")->ToElement();
    while (sub)
    {
        int id;
        sub->Attribute("i", &id);
        if (id < nc_entries)
        {
            int x;
            char msrc[256];
            vtCopyString(msrc, sub->Attribute("src"), 256);

            for (x = 0; x < t_mm.get_n_sources(); x++)
            {
                if (stricmp(t_mm.get_source_idname(x), msrc) == 0)
                    zone->nc[id].source = x;
            }

            sub->Attribute("low", &zone->nc[id].low);
            sub->Attribute("high", &zone->nc[id].high);
        }
        sub = sub->NextSibling("nc")->ToElement();
    }

    sub = element.FirstChild("slice")->ToElement();
    while (sub)
    {
        int id;
        sub->Attribute("i", &id);
        if (id < max_hitpoints)
        {
            zone->n_hitpoints = max(zone->n_hitpoints, id + 1);
            sub->Attribute("samplepos", &i);
            zone->hp[id].start_sample = i;
            sub->Attribute("env", &d);
            zone->hp[id].env = (float)d;
            zone->hp[id].muted = is_yes(sub->Attribute("mute"));

            if (sub->QueryIntAttribute("end", &i) == TIXML_SUCCESS)
            {
                zone->hp[id].end_sample = i;
                sub = sub->NextSibling("slice")->ToElement(); // do after
            }
            else
            {
                sub = sub->NextSibling("slice")->ToElement(); // do before, since end wasn't found
                if (sub)
                {
                    sub->Attribute("samplepos", &i);
                    zone->hp[id].end_sample = i;
                }
                else
                    zone->hp[id].end_sample = zone->sample_stop;
            }
        }
    }
}

void sampler::store_zone_as_element(TiXmlElement &element, sample_zone *zone, configuration *conf)
{
    char tempstr[64];
    element.SetAttribute("name", zone->name);
    element.SetAttribute("layer", zone->layer);
    element.SetAttribute("key_root", zone->key_root);
    element.SetAttribute("key_low", zone->key_low);
    element.SetAttribute("key_high", zone->key_high);
    element.SetAttribute("lo_vel", zone->velocity_low);
    element.SetAttribute("hi_vel", zone->velocity_high);

    element.SetAttribute("key_low_fade", zone->key_low_fade);
    element.SetAttribute("key_high_fade", zone->key_high_fade);
    element.SetAttribute("lo_vel_fade", zone->velocity_low_fade);
    element.SetAttribute("hi_vel_fade", zone->velocity_high_fade);

    // element.SetAttribute("channel",zone->channel);

    for (int i = 0; i < 3; i++)
    {
        TiXmlElement output("output");
        output.Clear();
        output.SetAttribute("i", i);
        output.SetAttribute("level", float_to_str(zone->aux[i].level, tempstr));
        output.SetAttribute("balance", float_to_str(zone->aux[i].balance, tempstr));
        output.SetAttribute("output", zone->aux[i].output);
        if (i)
            output.SetAttribute("outmode", zone->aux[i].outmode);
        element.InsertEndChild(output);
    }

    element.SetAttribute("transpose", zone->transpose);
    element.SetAttribute("finetune", float_to_str(zone->finetune, tempstr));
    element.SetAttribute("pitchcorrect", float_to_str(zone->pitchcorrection, tempstr));
    element.SetAttribute("playmode", (char *)playmode_abberations[zone->playmode]);
    element.SetAttribute("sample_start", zone->sample_start);
    element.SetAttribute("sample_end", zone->sample_stop);
    element.SetAttribute("loop_start", zone->loop_start);
    element.SetAttribute("loop_end", zone->loop_end);
    element.SetAttribute("loop_crossfade_length", zone->loop_crossfade_length);
    element.SetAttribute("velsense", float_to_str(zone->velsense, tempstr));
    element.SetAttribute("keytrack", float_to_str(zone->keytrack, tempstr));
    element.SetAttribute("amplitude", float_to_str(zone->aux[0].level, tempstr));

    element.SetAttribute("prefilter_gain", float_to_str(zone->pre_filter_gain, tempstr));
    element.SetAttribute("PB_depth", zone->pitch_bend_depth);
    element.SetAttribute("mute_group", zone->mute_group);
    element.SetAttribute("ignore_polymode", zone->ignore_part_polymode);
    // element.SetAttribute("polymode",zone->polymode);
    // element.SetAttribute("portamode",zone->portamento_mode);
    // element.SetAttribute("portamento",float_to_str(zone->portamento,tempstr));
    element.SetAttribute("mute", yes_no(zone->mute != 0, tempstr));
    //	element.SetAttribute("ef_attack",float_to_str(zone->ef_attack,tempstr));
    //	element.SetAttribute("ef_release",float_to_str(zone->ef_release,tempstr));
    element.SetAttribute("lag0", float_to_str(zone->lag_generator[0], tempstr));
    element.SetAttribute("lag1", float_to_str(zone->lag_generator[1], tempstr));

    // filters
    int i;
    for (i = 0; i < 2; i++)
    {
        TiXmlElement filter("filter");
        filter.Clear();
        filter.SetAttribute("i", i);
        filter.SetAttribute("type", filter_abberations[zone->Filter[i].type]);
        filter.SetAttribute("bypass", yes_no(zone->Filter[i].bypass, tempstr));
        filter.SetAttribute("mix", float_to_str(zone->Filter[i].mix, tempstr));

        // filter.SetAttribute("p0",float_to_str(zone->Filter[i].p[0],tempstr));
        int k;
        for (k = 0; k < n_filter_parameters; k++)
        {
            /*if ((zone->Filter[i].type == ft_morpheq)&&(k<2))	// evil hardcoding
            {
                    char label[16];
                    sprintf(label,"p%i",k);
                    int bank,patch;
                    meq_loader.set_id(&bank,&patch,zone->Filter[i].p[k]);
                    filter.SetAttribute(label,patch);
                    sprintf(label,"pb%i",k);
                    filter.SetAttribute(label,bank);
            }
            else */
            {
                char label[9];
                sprintf(label, "p%i", k);
                if (zone->Filter[i].p[k] != 0.0f)
                    filter.SetAttribute(label, float_to_str(zone->Filter[i].p[k], tempstr));
            }
        }
        for (k = 0; k < n_filter_iparameters; k++)
        {
            char label[9];
            sprintf(label, "ip%i", k);
            if (zone->Filter[i].ip[k])
                filter.SetAttribute(label, zone->Filter[i].ip[k]);
        }
        if (zone->Filter[i].type)
            element.InsertEndChild(filter);
    }

    envelope_AHDSR *env[2];
    env[0] = &zone->AEG;
    env[1] = &zone->EG2;

    for (i = 0; i < 2; i++)
    {
        TiXmlElement envelope("envelope");
        envelope.Clear();
        envelope.SetAttribute("i", i);
        envelope.SetAttribute("attack", float_to_str(env[i]->attack, tempstr));
        envelope.SetAttribute("attack_shape", float_to_str(env[i]->shape[0], tempstr));
        envelope.SetAttribute("hold", float_to_str(env[i]->hold, tempstr));
        envelope.SetAttribute("decay", float_to_str(env[i]->decay, tempstr));
        envelope.SetAttribute("decay_shape", float_to_str(env[i]->shape[1], tempstr));
        envelope.SetAttribute("sustain", float_to_str(env[i]->sustain, tempstr));
        envelope.SetAttribute("release", float_to_str(env[i]->release, tempstr));
        envelope.SetAttribute("release_shape", float_to_str(env[i]->shape[2], tempstr));
        element.InsertEndChild(envelope);
    }

    for (i = 0; i < 3; i++)
    {
        TiXmlElement lfo("lfo");
        lfo.Clear();
        lfo.SetAttribute("i", i);
        lfo.SetAttribute("rate", float_to_str(zone->LFO[i].rate, tempstr));
        lfo.SetAttribute("smooth", float_to_str(zone->LFO[i].smooth, tempstr));
        lfo.SetAttribute("cyclemode", zone->LFO[i].cyclemode);
        lfo.SetAttribute("triggermode", zone->LFO[i].triggermode);
        lfo.SetAttribute("temposync", zone->LFO[i].temposync);
        lfo.SetAttribute("repeat", zone->LFO[i].repeat);
        lfo.SetAttribute("once", zone->LFO[i].onlyonce);
        int k;
        for (k = 0; k < 32; k++)
        {
            char label[9];
            sprintf(label, "step%i", k);
            if (zone->LFO[i].data[k] != 0)
                lfo.SetAttribute(label, float_to_str(zone->LFO[i].data[k], tempstr));
        }
        element.InsertEndChild(lfo);
    }

    for (i = 0; i < mm_entries; i++)
    {
        if ((zone->mm[i].source > 0) || (zone->mm[i].destination > 0))
        {
            modmatrix t_mm;
            t_mm.assign(conf, zone, 0);

            TiXmlElement mm("modulation");
            mm.Clear();
            mm.SetAttribute("i", i);
            mm.SetAttribute("src", t_mm.get_source_idname(zone->mm[i].source));
            mm.SetAttribute("src2", t_mm.get_source_idname(zone->mm[i].source2));
            mm.SetAttribute("dest", t_mm.get_destination_idname(zone->mm[i].destination));
            mm.SetAttribute("amount", float_to_str(zone->mm[i].strength, tempstr));
            mm.SetAttribute("active", zone->mm[i].active);
            mm.SetAttribute("curve", zone->mm[i].curve);
            element.InsertEndChild(mm);
        }
    }

    for (i = 0; i < nc_entries; i++)
    {
        if (zone->nc[i].source > 0)
        {
            modmatrix t_mm;
            t_mm.assign(conf, zone, &parts[0]);

            TiXmlElement nc("nc");
            nc.Clear();
            nc.SetAttribute("i", i);
            nc.SetAttribute("src", t_mm.get_source_idname(zone->nc[i].source));
            nc.SetAttribute("low", zone->nc[i].low);
            nc.SetAttribute("high", zone->nc[i].high);
            element.InsertEndChild(nc);
        }
    }

    for (i = 0; i < zone->n_hitpoints; i++)
    {
        modmatrix t_mm;
        t_mm.assign(conf, zone, &parts[0]);

        TiXmlElement mm("slice");
        mm.Clear();
        mm.SetAttribute("i", i);
        mm.SetAttribute("samplepos", zone->hp[i].start_sample);
        mm.SetAttribute("end", zone->hp[i].end_sample);
        mm.SetAttribute("env", float_to_str(zone->hp[i].env, tempstr));
        mm.SetAttribute("mute", yes_no(zone->hp[i].muted, tempstr));
        element.InsertEndChild(mm);
    }
}

void store_part_as_element(TiXmlElement &element, sample_part *part, configuration *conf)
{
    char tempstr[64];
    element.SetAttribute("name", part->name);
    if (part->transpose)
        element.SetAttribute("transpose", part->transpose);
    if (part->formant)
        element.SetAttribute("formant", part->formant);
    element.SetAttribute("channel", part->MIDIchannel);
    element.SetAttribute("poly_limit", part->polylimit);
    element.SetAttribute("polymode", part->polymode);
    element.SetAttribute("pfg", float_to_str(part->pfg, tempstr));
    element.SetAttribute("portamento", float_to_str(part->portamento, tempstr));
    if (part->portamento_mode)
        element.SetAttribute("portamode", part->portamento_mode);
    if (part->vs_layercount)
        element.SetAttribute("vs_layers", part->vs_layercount);
    element.SetAttribute("xf_equality", part->vs_xf_equality);
    element.SetAttribute("vs_distribution", float_to_str(part->vs_distribution, tempstr));
    element.SetAttribute("vs_xfade", float_to_str(part->vs_xfade, tempstr));

    modmatrix t_mm;
    t_mm.assign(conf, 0, part);

    for (int i = 0; i < num_layers; i++)
    {
        bool used = false;
        TiXmlElement layer("layer");
        layer.Clear();
        layer.SetAttribute("i", i);
        for (int j = 0; j < num_layer_ncs; j++)
        {
            int ncid = i * num_layer_ncs + j;
            if (part->nc[ncid].source > 0)
            {
                TiXmlElement nc("nc");
                nc.Clear();
                nc.SetAttribute("i", j);
                // TODO, l�gg till samma rutiner som modmatrix src

                nc.SetAttribute("src", t_mm.get_source_idname(part->nc[ncid].source));
                nc.SetAttribute("low", part->nc[ncid].low);
                nc.SetAttribute("high", part->nc[ncid].high);
                layer.InsertEndChild(nc);
                used = true;
            }
        }
        if (used)
            element.InsertEndChild(layer);
    }

    for (int i = 0; i < 3; i++)
    {
        TiXmlElement output("output");
        output.Clear();
        output.SetAttribute("i", i);
        output.SetAttribute("level", float_to_str(part->aux[i].level, tempstr));
        output.SetAttribute("balance", float_to_str(part->aux[i].balance, tempstr));
        output.SetAttribute("output", part->aux[i].output);
        if (i)
            output.SetAttribute("outmode", part->aux[i].outmode);
        element.InsertEndChild(output);
    }

    for (int i = 0; i < 2; i++)
    {
        TiXmlElement filter("filter");
        filter.Clear();
        filter.SetAttribute("i", i);
        filter.SetAttribute("type", filter_abberations[part->Filter[i].type]);
        filter.SetAttribute("bypass", yes_no(part->Filter[i].bypass != 0, tempstr));
        filter.SetAttribute("mix", float_to_str(part->Filter[i].mix, tempstr));

        int k;
        for (k = 0; k < n_filter_parameters; k++)
        {
            char label[16];
            sprintf(label, "p%i", k);
            if (part->Filter[i].p[k] != 0.0f)
                filter.SetAttribute(label, float_to_str(part->Filter[i].p[k], tempstr));
        }
        for (k = 0; k < n_filter_iparameters; k++)
        {
            char label[16];
            sprintf(label, "ip%i", k);
            if (part->Filter[i].ip[k] != 0)
                filter.SetAttribute(label, part->Filter[i].ip[k]);
        }
        if (part->Filter[i].type)
            element.InsertEndChild(filter);
    }

    for (int i = 0; i < n_custom_controllers; i++)
    {
        TiXmlElement cctrl("userparam");
        cctrl.SetAttribute("i", i);
        cctrl.SetAttribute("name", part->userparametername[i]);
        cctrl.SetAttribute("bipolar", part->userparameterpolarity[i] ? 1 : 0);
        cctrl.SetAttribute("value", float_to_str(part->userparameter[i], tempstr));
        if (part->userparameter[i] || part->userparametername[i][0] ||
            part->userparameterpolarity[i]) // one of them has to differ from the default to be
                                            // stored
            element.InsertEndChild(cctrl);
    }

    for (int i = 0; i < mm_part_entries; i++)
    {
        if ((part->mm[i].source > 0) || (part->mm[i].destination > 0))
        {
            TiXmlElement mm("modulation");
            mm.Clear();
            mm.SetAttribute("i", i);
            mm.SetAttribute("src", t_mm.get_source_idname(part->mm[i].source));
            mm.SetAttribute("src2", t_mm.get_source_idname(part->mm[i].source2));
            mm.SetAttribute("dest", t_mm.get_destination_idname(part->mm[i].destination));
            mm.SetAttribute("amount", float_to_str(part->mm[i].strength, tempstr));
            mm.SetAttribute("active", part->mm[i].active);
            mm.SetAttribute("curve", part->mm[i].curve);
            element.InsertEndChild(mm);
        }
    }
}

void recall_part_from_element(TiXmlElement &element, sample_part *part, int revision,
                              configuration *conf, bool include_outputs,
                              bool include_userparam_state)
{
    assert(conf != 0);
    double d;
    int i;
    vtCopyString(part->name, element.Attribute("name"), 32);

    element.Attribute("transpose", &i);
    part->transpose = i;
    element.Attribute("formant", &i);
    part->formant = i;
    element.Attribute("channel", &i);
    part->MIDIchannel = i;
    element.Attribute("poly_limit", &i);
    part->polylimit = i;
    element.Attribute("polymode", &i);
    part->polymode = i;

    element.Attribute("pfg", &d);
    part->pfg = (float)d;
    element.Attribute("portamento", &d);
    part->portamento = (float)d;
    element.Attribute("portamode", &i);
    part->portamento_mode = i;

    element.Attribute("vs_layers", &i);
    part->vs_layercount = i;
    element.Attribute("xf_equality", &i);
    part->vs_xf_equality = i;
    element.Attribute("vs_distribution", &d);
    part->vs_distribution = (float)d;
    element.Attribute("vs_xfade", &d);
    part->vs_xfade = (float)d;

    TiXmlElement *sub;
    int j;
    if (include_outputs)
    {
        sub = element.FirstChild("output")->ToElement();
        while (sub)
        {
            sub->Attribute("i", &j);
            if (j < 3)
            {
                if (sub->QueryDoubleAttribute("level", &d) == TIXML_SUCCESS)
                    part->aux[j].level = d;
                if (sub->QueryDoubleAttribute("balance", &d) == TIXML_SUCCESS)
                    part->aux[j].balance = d;
                if (sub->QueryIntAttribute("output", &i) == TIXML_SUCCESS)
                    part->aux[j].output = i;
                if (j && (sub->QueryIntAttribute("outmode", &i) == TIXML_SUCCESS))
                    part->aux[j].outmode = i;
            }
            sub = sub->NextSibling("output")->ToElement();
        }
    }

    sub = element.FirstChild("filter")->ToElement();
    while (sub)
    {
        sub->Attribute("i", &j);
        if (j < 2)
        {
            char abb[16];
            vtCopyString(abb, sub->Attribute("type"), 16);

            // find filter abberation in list, if not found, set to NONE
            int l, ft = 0;
            for (l = 0; l < ft_num_types; l++)
            {
                if (strcmp(filter_abberations[l], abb) == 0)
                    ft = l;
            }
            part->Filter[j].type = ft;
            if (sub->QueryDoubleAttribute("mix", &d) == TIXML_SUCCESS)
                part->Filter[j].mix = d;
            part->Filter[j].bypass = is_yes(sub->Attribute("bypass"));

            int k;
            for (k = 0; k < n_filter_parameters; k++)
            {
                char label[9];
                sprintf(label, "p%i", k);
                if (sub->QueryDoubleAttribute(label, &d) == TIXML_SUCCESS)
                    part->Filter[j].p[k] = (float)d;
            }
            for (k = 0; k < n_filter_iparameters; k++)
            {
                char label[9];
                sprintf(label, "ip%i", k);
                if (sub->QueryIntAttribute(label, &i) == TIXML_SUCCESS)
                    part->Filter[j].ip[k] = i;
            }
        }
        sub = sub->NextSibling("filter")->ToElement();
    }

    sub = element.FirstChild("userparam")->ToElement();
    while (sub)
    {
        sub->Attribute("i", &j);
        if (j < n_custom_controllers)
        {
            if (sub->QueryDoubleAttribute("value", &d) == TIXML_SUCCESS)
                part->userparameter[j] = d;
            if (sub->QueryIntAttribute("bipolar", &i) == TIXML_SUCCESS)
                part->userparameterpolarity[j] = i;
            const char *pname = sub->Attribute("name");
            // if(pname) strncpy_s(part->userparametername[j],16,pname,_TRUNCATE);
            if (pname)
                strncpy(part->userparametername[j], pname, 16);
        }
        sub = sub->NextSibling("userparam")->ToElement();
    }

    modmatrix t_mm;
    t_mm.assign(conf, 0, part);
    sub = element.FirstChild("modulation")->ToElement();
    while (sub)
    {
        int id;
        sub->Attribute("i", &id);
        if (id < mm_part_entries)
        {
            int x;
            const char *ts;
            ts = sub->Attribute("src");
            for (x = 0; x < t_mm.get_n_sources(); x++)
            {
                if (stricmp(t_mm.get_source_idname(x), ts) == 0)
                    part->mm[id].source = x;
            }
            ts = sub->Attribute("dest");
            for (x = 0; x < t_mm.get_n_destinations(); x++)
            {
                if (stricmp(t_mm.get_destination_idname(x), ts) == 0)
                    part->mm[id].destination = x;
            }

            ts = sub->Attribute("src2");
            for (x = 0; x < t_mm.get_n_sources(); x++)
            {
                if (stricmp(t_mm.get_source_idname(x), ts) == 0)
                    part->mm[id].source2 = x;
            }
            sub->Attribute("amount", &d);
            part->mm[id].strength = (float)d;
            sub->Attribute("curve", &part->mm[id].curve);
            sub->Attribute("active", &part->mm[id].active);
        }
        sub = sub->NextSibling("modulation")->ToElement();
    }

    sub = element.FirstChild("layer")->ToElement();
    while (sub)
    {
        TiXmlElement *subsub;
        int lid;
        sub->Attribute("i", &lid);

        subsub = sub->FirstChild("nc")->ToElement();
        while (sub)
        {
            int id;
            sub->Attribute("i", &id);
            if ((id < num_layer_ncs) && (lid < num_layers))
            {
                int x;
                char msrc[256];
                vtCopyString(msrc, sub->Attribute("src"), 256);

                for (x = 0; x < t_mm.get_n_sources(); x++)
                {
                    if (stricmp(t_mm.get_source_idname(x), msrc) == 0)
                        part->nc[lid * num_layer_ncs + id].source = x;
                }

                sub->Attribute("low", &part->nc[lid * num_layer_ncs + id].low);
                sub->Attribute("high", &part->nc[lid * num_layer_ncs + id].high);
            }
            subsub = subsub->NextSibling("nc")->ToElement();
        }
        sub = sub->NextSibling("modulation")->ToElement();
    }
}

string recursive_search(string filename, string path)
{
    // returns the path of the filename i think. Since we haven't got enough running
    // to test this let me leave a log message in and a warning
#if ! WINDOWS
#warning Compiling untested rewrite of recursive_search
#endif

    SC3::Log::logos() << "Implement recursive_search " << filename << " " << path << std::endl;
    auto p = string_to_path(path);
    auto f = string_to_path(filename);

    for( auto & ent : fs::recursive_directory_iterator(p))
    {
        auto dp = fs::path(ent);
        auto fn = dp.filename();
        if( path_to_string(fn) == path_to_string(f) )
        {
            return path_to_string(dp);
        }
    }
    return "";
}

bool sampler::load_all_from_xml(void *data, int datasize, const fs::path &filename, bool replace,
                                int part_id)
{
    if (datasize && (*(int *)data == 'FFIR'))
    {
        return LoadAllFromRIFF(data, datasize, replace, part_id);
    }

    int i;
    int revision;
    //	double d;
    if (replace)
        free_all();
    TiXmlDocument doc;
    bool is_multi = (part_id == -1);
    if (datasize)
    {
        char *temp = (char *)malloc(datasize + 1);
        memcpy(temp, data, datasize);
        *(temp + datasize) = 0;
        doc.Parse(temp);
        free(temp);
    }
    else if (!filename.empty())
    {
        // TODO propagate path down
        if (!doc.LoadFile(path_to_string(filename).c_str()))
            return false;        
        // assign path to configuration
        conf->set_relative_path(filename);
    }
    else
        return false;

    TiXmlElement *sc = doc.FirstChild("shortcircuit")->ToElement();
    if (sc->QueryIntAttribute("revision", &revision) != TIXML_SUCCESS)
        revision = 1;

    TiXmlElement *global = sc->FirstChild("global")->ToElement();
#ifndef SCPB
    if (global)
    {
        /*global->Attribute("headroom",&i);
        this->set_headroom(i);		*/
        if (global->QueryIntAttribute("poly_cap", &i) == TIXML_SUCCESS)
            polyphony_cap = i;
    }
#endif

    string searchpath;
    bool dont_ask_path = false;

    TiXmlElement *part = sc->FirstChild("part")->ToElement();
    while (part)
    {
        int p_id;

        if (part_id != -1)
            p_id = part_id;
        else
            part->Attribute("i", &p_id);

        p_id &= 0xf;

        part_init(p_id, true, !is_multi);
        recall_part_from_element(*part, &parts[p_id], revision, conf, is_multi, is_multi);

        TiXmlElement *zone = part->FirstChild("zone")->ToElement();
        while (zone)
        {
            int zone_id = this->GetFreeZoneId();
            InitZone(zone_id);
            recall_zone_from_element(*zone, &zones[zone_id], revision, conf);

            zones[zone_id].part = p_id;

            int sample_id;

            bool is_loaded = false;

            if (zone->Attribute("filename"))
            {
                char samplefname[256];
                vtCopyString(samplefname, (char *)zone->Attribute("filename"), 256);
                if (revision < 9) // this wasn't correctly done in revision 8 either
                {
                    char *c = strstr(samplefname, ".sf2,");
                    if (c)
                        *(c + 4) = '|';
                }
                is_loaded = get_sample_id(
                    samplefname, &sample_id); // ACHTUNG!! blir fuck-up om filename inte finns (?)

                if (is_loaded)
                {
                    if (sample_id >= 0)
                        samples[sample_id]->remember();
                    zones[zone_id].sample_id = sample_id;
                    zone_exists[zone_id] = true;
                }
                else
                {
                    sample_id = this->GetFreeSampleId();
                    samples[sample_id] = new sample(conf);
                    if (samples[sample_id]->load(samplefname))
                    {
                        zones[zone_id].sample_id = sample_id;
                        zone_exists[zone_id] = true;
                    }
                    else
                    {
                        // not found use the search path
                        char filename[256], subsamplename[256];
                        string newpath;
                        newpath[0] = 0;
                        char *r = strrchr(samplefname, '\\');
                        if (r)
                            vtCopyString(filename, r + 1, 256);
                        else
                            vtCopyString(filename, samplefname, 256);

                        r = strrchr(filename, '|');
                        if (r)
                        {
                            vtCopyString(subsamplename, r, 256);
                            *r = 0;
                        }
                        else
                            subsamplename[0] = 0;
                        subsamplename[255] = 0;

                        int rlength = 0;
                        while (!dont_ask_path)
                        {
                            // if (!searchpath.empty()) rlength =
                            // SearchPath(searchpath.c_str(),filename,0,256,newpath,0); if(rlength)
                            // break;
#if WINDOWS

                            if (!searchpath.empty())
                            {
                                newpath = recursive_search(filename, searchpath);
                                if (!newpath.empty())
                                    break;
                            }

                            BROWSEINFO bi;
                            bi.ulFlags = BIF_NONEWFOLDERBUTTON | BIF_RETURNONLYFSDIRS;
                            char title[256];
                            sprintf(title,
                                    "The file [%s] could not be found. Select the directory "
                                    "wherein it is located. (will search subdirs)",
                                    filename);
                            char np[MAX_PATH];
                            bi.lpszTitle = title;
                            bi.pszDisplayName = np;
                            bi.lpfn = NULL;
                            bi.pidlRoot = NULL;
                            bi.hwndOwner = ::GetActiveWindow();
                            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

                            if (pidl != 0)
                            {
                                // get the name of the folder
                                TCHAR path[MAX_PATH];
                                if (SHGetPathFromIDList(pidl, path))
                                {
                                    searchpath = path;
                                    searchpath.append("\\");
                                }

                                // free memory used
                                IMalloc *imalloc = 0;
                                if (SUCCEEDED(SHGetMalloc(&imalloc)))
                                {
                                    imalloc->Free(pidl);
                                    imalloc->Release();
                                }
                            }
                            else
                            {
                                dont_ask_path = true;
                                break;
                            }
#else
#warning Substantial part of unimplemented UI code to prompt being skipped
#endif

                        }

                        if (newpath.size() && subsamplename[0])
                            newpath.append(subsamplename);

                        if (newpath.size() && samples[sample_id]->load(newpath.c_str()))
                        {
                            zones[zone_id].sample_id = sample_id;
                            zone_exists[zone_id] = true;
                        }
                        else
                        {
                            delete samples[sample_id];
                            samples[sample_id] = 0;
                            zones[zone_id].sample_id = -1;
                            zone_exists[zone_id] = true;
                        }
                    }
                }
                update_zone_switches(zone_id);
            }

            zone = zone->NextSibling("zone")->ToElement();
        }
        part = part->NextSibling("part")->ToElement();
    }
    return true;
}

bool sampler::load_all_from_sc1_xml(void *data, int datasize, const fs::path &filename,
                                    bool replace,
                                    int part_id)
{
    int revision;
    //	double d;
    if (replace)
        free_all();
    TiXmlDocument doc;
    bool is_multi = (part_id == -1);
    if (datasize)
    {
        char *temp = (char *)malloc(datasize + 1);
        memcpy(temp, data, datasize);
        *(temp + datasize) = 0;
        doc.Parse(temp);
        free(temp);
    }
    else if (!filename.empty())
    {
        // TODO propagate path down to Tixml
        if (!doc.LoadFile(path_to_string(filename).c_str()))
            return false;

        // assign path to configuration
        conf->set_relative_path(filename);
    }
    else
        return false;

    TiXmlElement *sc = doc.FirstChild("shortcircuit")->ToElement();
    if (sc->QueryIntAttribute("revision", &revision) != TIXML_SUCCESS)
        revision = 1;

    TiXmlElement *global = sc->FirstChild("global")->ToElement();

    string searchpath;
    bool dont_ask_path = false;

    TiXmlElement *part = sc->FirstChild("part")->ToElement();
    while (part)
    {
        int p_id;

        if (part_id != -1)
            p_id = part_id;
        else
            part->Attribute("i", &p_id);

        p_id &= 0xf;

        part_init(p_id, true, !is_multi);
        recall_part_from_element(*part, &parts[p_id], revision, conf, is_multi, is_multi);

        TiXmlElement *zone = part->FirstChild("zone")->ToElement();
        while (zone)
        {
            int zone_id = this->GetFreeZoneId();
            InitZone(zone_id);
            recall_zone_from_element(*zone, &zones[zone_id], revision, conf);

            zones[zone_id].part = p_id;

            int sample_id;

            bool is_loaded = false;

            if (zone->Attribute("filename"))
            {
                char samplefname[256];
                vtCopyString(samplefname, (char *)zone->Attribute("filename"), 256);
                if (revision < 9) // this wasn't correctly done in revision 8 either
                {
                    char *c = strstr(samplefname, ".sf2,");
                    if (c)
                        *(c + 4) = '|';
                }
                is_loaded = get_sample_id(
                    samplefname, &sample_id); // ACHTUNG!! blir fuck-up om filename inte finns (?)

                if (is_loaded)
                {
                    if (sample_id >= 0)
                        samples[sample_id]->remember();
                    zones[zone_id].sample_id = sample_id;
                    zone_exists[zone_id] = true;
                }
                else
                {
                    sample_id = this->GetFreeSampleId();
                    samples[sample_id] = new sample(conf);
                    if (samples[sample_id]->load(samplefname))
                    {
                        zones[zone_id].sample_id = sample_id;
                        zone_exists[zone_id] = true;
                    }
                    else
                    {
                        // not found use the search path
                        char filename[256], subsamplename[256];
                        string newpath;
                        newpath[0] = 0;
                        char *r = strrchr(samplefname, '\\');
                        if (r)
                            vtCopyString(filename, r + 1, 256);
                        else
                            vtCopyString(filename, samplefname, 256);

                        r = strrchr(filename, '|');
                        if (r)
                        {
                            vtCopyString(subsamplename, r, 256);
                            *r = 0;
                        }
                        else
                            subsamplename[0] = 0;
                        subsamplename[255] = 0;

                        int rlength = 0;
                        while (!dont_ask_path)
                        {
                            // if (!searchpath.empty()) rlength =
                            // SearchPath(searchpath.c_str(),filename,0,256,newpath,0); if(rlength)
                            // break;

                            if (!searchpath.empty())
                            {
                                newpath = recursive_search(filename, searchpath);
                                if (!newpath.empty())
                                    break;
                            }

#if WINDOWS
                            BROWSEINFO bi;
                            bi.ulFlags = BIF_NONEWFOLDERBUTTON | BIF_RETURNONLYFSDIRS;
                            char title[256];
                            sprintf(title,
                                    "The file [%s] could not be found. Select the directory "
                                    "wherein it is located. (will search subdirs)",
                                    filename);
                            char np[MAX_PATH];
                            bi.lpszTitle = title;
                            bi.pszDisplayName = np;
                            bi.lpfn = NULL;
                            bi.pidlRoot = NULL;
                            bi.hwndOwner = ::GetActiveWindow();
                            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

                            if (pidl != 0)
                            {
                                // get the name of the folder
                                TCHAR path[MAX_PATH];
                                if (SHGetPathFromIDList(pidl, path))
                                {
                                    searchpath = path;
                                    searchpath.append("\\");
                                }

                                // free memory used
                                IMalloc *imalloc = 0;
                                if (SUCCEEDED(SHGetMalloc(&imalloc)))
                                {
                                    imalloc->Free(pidl);
                                    imalloc->Release();
                                }
                            }
                            else
                            {
                                dont_ask_path = true;
                                break;
                            }
#else
#warning Skipping Shell Browse for Folder on Mac/Linux
#endif
                        }

                        if (newpath.size() && subsamplename[0])
                            newpath.append(subsamplename);

                        if (newpath.size() && samples[sample_id]->load(newpath.c_str()))
                        {
                            zones[zone_id].sample_id = sample_id;
                            zone_exists[zone_id] = true;
                        }
                        else
                        {
                            delete samples[sample_id];
                            samples[sample_id] = 0;
                            zones[zone_id].sample_id = -1;
                            zone_exists[zone_id] = true;
                        }
                    }
                }
                update_zone_switches(zone_id);
            }

            zone = zone->NextSibling("zone")->ToElement();
        }
        part = part->NextSibling("group")->ToElement();
    }
    return true;
}
