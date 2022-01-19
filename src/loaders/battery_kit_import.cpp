//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
#include "globals.h"
#include "infrastructure/logfile.h"
#include "sampler.h"
#include "configuration.h"
#include "util/unitconversion.h"
#include <float.h>
#include <stdio.h>
#include <tinyxml.h>
#include <vt_util/vt_string.h>

float battery_envtime2sc(float t)
{
    if (t == 0)
        return -10.f;
    return log2(powf(t / 127.0f, 3));
}

bool sampler::load_battery_kit(const fs::path &fileName, char channel, bool replace)
{

    fs::path path;
    std::string fn_only;
    decode_path(fileName, 0, 0, &fn_only, &path);
    path = path / "samples";

    TiXmlDocument doc(path_to_string(fileName)); // wants utf8
    doc.LoadFile();

    TiXmlElement *patch = doc.FirstChildElement("BatteryPatch");
    if (!patch)
        return false;

    // so far so good.. time to start creating the samplegroup
    const char *groupname = patch->Attribute("name");
    double oi;
    patch->Attribute("volume", &oi);

    if (replace)
        part_init(channel, true, true);
    vtCopyString(parts[channel].name, groupname, 32);

    // load samples
    TiXmlElement *slot = (TiXmlElement *)patch->FirstChild("SampleSlot");
    while (slot)
    {
        TiXmlElement *samplefile = (TiXmlElement *)slot->FirstChild("Sample");
        while (samplefile)
        {
            fs::path fn = build_path(path, samplefile->Attribute("file"));
            int newzone;

            if (add_zone(fn, &newzone, channel))
            {
                double dval;
                int ival;
                samplefile->Attribute("lowVelo", &ival);
                zones[newzone].velocity_low = ival;
                samplefile->Attribute("highVelo", &ival);
                zones[newzone].velocity_high = ival;

                slot->Attribute("rootKey", &ival);
                zones[newzone].key_root = ival;
                slot->Attribute("lowKey", &ival);
                zones[newzone].key_low = ival;
                slot->Attribute("highKey", &ival);
                zones[newzone].key_high = ival;

                slot->Attribute("pan", &dval);
                zones[newzone].aux[0].balance = (float)dval * 2 - 1;
                slot->Attribute("volume", &dval);
                zones[newzone].aux[0].level = linear_to_dB((float)dval);
                samplefile->Attribute("layerVolume", &dval);
                zones[newzone].pre_filter_gain = linear_to_dB((float)dval);

                slot->Attribute("vattack", &dval);
                zones[newzone].AEG.attack = battery_envtime2sc((float)dval);
                slot->Attribute("vhold", &dval);
                zones[newzone].AEG.hold = battery_envtime2sc((float)dval);
                slot->Attribute("vdecay", &dval);
                zones[newzone].AEG.decay = battery_envtime2sc((float)dval);
                slot->Attribute("vsustain", &dval);
                zones[newzone].AEG.sustain =
                    powf((float)dval / 127, 1 / 3); // battery is 3rd order -> 2nd order
                slot->Attribute("vrelease", &dval);
                zones[newzone].AEG.release = battery_envtime2sc((float)dval);

                int status;
                slot->Attribute("status", &status);
                zones[newzone].AEG.attack = -10;
                zones[newzone].AEG.hold = -10;
                zones[newzone].AEG.decay = -10;
                zones[newzone].AEG.sustain = 1;
                zones[newzone].AEG.release = -5;

                if (!(status & 32))
                { // env off
                    zones[newzone].playmode = pm_forward_shot;
                }
                else if (status & 128)
                { // AHDSR mode
                    // do nothing
                    if (status & 64)
                        zones[newzone].playmode = pm_forward_loop;
                }
                else
                { // AHD mode
                    zones[newzone].AEG.release = zones[newzone].AEG.decay;
                    zones[newzone].playmode = pm_forward_shot;
                }

                if (status & 8)
                    zones[newzone].keytrack = 1;
                else
                    zones[newzone].keytrack = 0;

                slot->Attribute("sstart", &ival);
                zones[newzone].sample_start = ival;

                // tuning
                double tune, fine;
                slot->Attribute("pitch", &dval);
                fine = 12 * log(dval) / log(2.0);
                tune = ceil(fine - 0.5);
                fine -= tune;
                zones[newzone].finetune = (float)fine;
                zones[newzone].transpose = (int)tune;

                int modsrc, moddst, modamt;
                slot->Attribute("modSrc0", &modsrc);
                slot->Attribute("modDest0", &moddst);
                slot->Attribute("modAmt0", &modamt);
                if ((modsrc == 1) && (moddst == 1))
                {
                    zones[newzone].mm[0].strength = modamt * 0.01f;
                }
                else
                {
                    zones[newzone].mm[0].source = 0;
                    zones[newzone].mm[0].destination = 0;
                    zones[newzone].mm[0].strength = 0;
                }
            }
            samplefile = (TiXmlElement *)samplefile->NextSibling("Sample");
        }
        slot = (TiXmlElement *)slot->NextSibling("SampleSlot");
    }

    return true;
}