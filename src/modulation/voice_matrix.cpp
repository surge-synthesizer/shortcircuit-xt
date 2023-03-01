/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include <optional>

#include "voice_matrix.h"
#include "engine/zone.h"
#include "voice/voice.h"

namespace scxt::modulation
{
void VoiceModMatrix::clear()
{
    memset(sourcePointers, 0, sizeof(sourcePointers));
    memset(baseValues, 0, sizeof(baseValues));
    memset(modulatedValues, 0, sizeof(modulatedValues));
}

void VoiceModMatrix::snapRoutingFromZone(engine::Zone *z) { routingTable = z->routingTable; }

void VoiceModMatrix::copyBaseValuesFromZone(engine::Zone *z)
{
    for (int i = 0; i < engine::processorsPerZone; ++i)
    {
        // TODO - skip off
        baseValues[destIndex(vmd_Processor_Mix, i)] = z->processorStorage[i].mix;
        memcpy(&baseValues[destIndex(vmd_Processor_FP1, i)],
               z->processorStorage[i].floatParams.data(),
               sizeof(float) * dsp::processor::maxProcessorFloatParams);
    }

    for (int i=0; i<engine::lfosPerZone; ++i)
    {
        baseValues[destIndex(vmd_LFO_Rate, i)] = z->lfoStorage[i].rate;
    }

    // TODO : Shapes
    baseValues[destIndex(vmd_eg_A, 0)] = z->aegStorage.a;
    baseValues[destIndex(vmd_eg_H, 0)] = z->aegStorage.h;
    baseValues[destIndex(vmd_eg_D, 0)] = z->aegStorage.d;
    baseValues[destIndex(vmd_eg_S, 0)] = z->aegStorage.s;
    baseValues[destIndex(vmd_eg_R, 0)] = z->aegStorage.r;

    baseValues[destIndex(vmd_eg_A, 1)] = z->eg2Storage.a;
    baseValues[destIndex(vmd_eg_H, 1)] = z->eg2Storage.h;
    baseValues[destIndex(vmd_eg_D, 1)] = z->eg2Storage.d;
    baseValues[destIndex(vmd_eg_S, 1)] = z->eg2Storage.s;
    baseValues[destIndex(vmd_eg_R, 1)] = z->eg2Storage.r;
}

void VoiceModMatrix::attachSourcesFromVoice(voice::Voice *v)
{
    sourcePointers[vms_none] = nullptr;
    for (int i = 0; i < engine::lfosPerZone; ++i)
    {
        sourcePointers[vms_LFO1 + i] = &(v->lfos[i].output);
    }
    sourcePointers[vms_AEG] = &(v->aeg.output);
    sourcePointers[vms_EG2] = &(v->eg2.output);
}

void VoiceModMatrix::initializeModulationValues()
{
    memcpy(modulatedValues, baseValues, sizeof(modulatedValues));
}

void VoiceModMatrix::process()
{
    memcpy(modulatedValues, baseValues, sizeof(modulatedValues));
    for (const auto &r : routingTable)
    {
        if (!r.active)
            continue;
        if (r.dst.type == vmd_none || r.src == vms_none)
            continue;
        if (!sourcePointers[r.src])
            continue;
        modulatedValues[r.dst] += (*(sourcePointers[r.src])) * r.depth;
    }
}

std::string getVoiceModMatrixDestStreamingName(const VoiceModMatrixDestinationType &dest)
{
    switch (dest)
    {
    case vmd_none:
        return "vmd_none";

    case vmd_LFO_Rate:
        return "vmd_lfo_rate";

    case vmd_Processor_Mix:
        return "vmd_processor_mix";
    case vmd_Processor_FP1:
        return "vmd_processor_fp1";
    case vmd_Processor_FP2:
        return "vmd_processor_fp2";
    case vmd_Processor_FP3:
        return "vmd_processor_fp3";
    case vmd_Processor_FP4:
        return "vmd_processor_fp4";
    case vmd_Processor_FP5:
        return "vmd_processor_fp5";
    case vmd_Processor_FP6:
        return "vmd_processor_fp6";
    case vmd_Processor_FP7:
        return "vmd_processor_fp7";
    case vmd_Processor_FP8:
        return "vmd_processor_fp8";
    case vmd_Processor_FP9:
        return "vmd_processor_fp9";

    case vmd_eg_A:
        return "vmd_eg_a";
    case vmd_eg_H:
        return "vmd_eg_h";
    case vmd_eg_D:
        return "vmd_eg_d";
    case vmd_eg_S:
        return "vmd_eg_s";
    case vmd_eg_R:
        return "vmd_eg_r";

    case vmd_eg_AShape:
        return "vmd_eg_ashape";
    case vmd_eg_DShape:
        return "vmd_eg_dshape";
    case vmd_eg_RShape:
        return "vmd_eg_rshape";

    case numVoiceMatrixDestinations:
        throw std::logic_error("Can't convert numVoiceMatrixDestinations to string");
    }

    throw std::logic_error("Invalid enum");
}
std::optional<VoiceModMatrixDestinationType>
fromVoiceModMatrixDestStreamingName(const std::string &s)
{
    for (int i = vmd_none; i < numVoiceMatrixDestinations; ++i)
    {
        if (getVoiceModMatrixDestStreamingName((VoiceModMatrixDestinationType)i) == s)
        {
            return ((VoiceModMatrixDestinationType)i);
        }
    }
    return vmd_none;
}

std::string getVoiceModMatrixSourceStreamingName(const VoiceModMatrixSource &dest)
{
    switch (dest)
    {
    case vms_none:
        return "vms_none";
    case vms_LFO1:
        return "vms_lfo1";
    case vms_LFO2:
        return "vms_lfo2";
    case vms_LFO3:
        return "vms_lfo3";

    case vms_AEG:
        return "vms_aeg";
    case vms_EG2:
        return "vms_eg2";

    case numVoiceMatrixSources:
        throw std::logic_error("Don't call with numVoiceMatrixSources");
    }

    throw std::logic_error("Mysterious unhandled condition");
}
std::optional<VoiceModMatrixSource> fromVoiceModMatrixSourceStreamingName(const std::string &s)
{
    for (int i = vmd_none; i < numVoiceMatrixSources; ++i)
    {
        if (getVoiceModMatrixSourceStreamingName((VoiceModMatrixSource)i) == s)
        {
            return ((VoiceModMatrixSource)i);
        }
    }
    return vms_none;
}

std::string getVoiceModMatrixCurveStreamingName(const VoiceModMatrixCurve &dest)
{
    switch (dest)
    {
    case vmc_none:
        return "vmc_none";
    case vmc_cube:
        return "vmc_cube";

    case numVoiceMatrixCurves:
        throw std::logic_error("Don't call with numVoiceMatrixCurves");
    }

    throw std::logic_error("Mysterious unhandled condition");
}
std::optional<VoiceModMatrixCurve> fromVoiceModMatrixCurveStreamingName(const std::string &s)
{
    for (int i = vmc_none; i < numVoiceMatrixCurves; ++i)
    {
        if (getVoiceModMatrixCurveStreamingName((VoiceModMatrixCurve)i) == s)
        {
            return ((VoiceModMatrixCurve)i);
        }
    }
    return vmc_none;
}

std::string getVoiceModMatrixSourceDisplayName(const VoiceModMatrixSource &dest)
{
    switch (dest)
    {
    case vms_none:
        return "None";
    case vms_LFO1:
        return "LFO1";
    case vms_LFO2:
        return "LFO2";
    case vms_LFO3:
        return "LFO3";

    case vms_AEG:
        return "AEG";
    case vms_EG2:
        return "EG2";

    case numVoiceMatrixSources:
        throw std::logic_error("Don't call with numVoiceMatrixSources");
    }

    throw std::logic_error("Mysterious unhandled condition");
}

std::string getVoiceModMatrixCurveDisplayName(const VoiceModMatrixCurve &dest)
{
    switch (dest)
    {
    case vmc_none:
        return "None";
    case vmc_cube:
        return "Cube";

    case numVoiceMatrixCurves:
        throw std::logic_error("Don't call with numVoiceMatrixCurves");
    }

    throw std::logic_error("Mysterious unhandled condition");
}

int getVoiceModMatrixDestIndexCount(const VoiceModMatrixDestinationType &t)
{
    switch (t)
    {
    case vmd_LFO_Rate:
        return engine::lfosPerZone;
    case vmd_Processor_Mix:
    case vmd_Processor_FP1:
    case vmd_Processor_FP2:
    case vmd_Processor_FP3:
    case vmd_Processor_FP4:
    case vmd_Processor_FP5:
    case vmd_Processor_FP6:
    case vmd_Processor_FP7:
    case vmd_Processor_FP8:
    case vmd_Processor_FP9:
        return engine::processorsPerZone;
    case vmd_eg_A:
    case vmd_eg_H:
    case vmd_eg_D:
    case vmd_eg_S:
    case vmd_eg_R:
    case vmd_eg_AShape:
    case vmd_eg_DShape:
    case vmd_eg_RShape:
        return 2; // aeg eg2

    default:
        return 1;
    }
}
std::optional<std::string>
getVoiceModMatrixDestDisplayName(const VoiceModMatrixDestinationAddress &dest,
                                 const engine::Zone &z)
{
    // TODO: This is obviously .... wrong
    /*
     * These are a bit trickier since they are indexed so
     */
    const auto &[vmd, idx] = dest;

    if (vmd == vmd_none)
    {
        return "Off";
    }
    if (vmd >= vmd_LFO_Rate && vmd <= vmd_LFO_Rate)
    {
        return "LFO " + std::to_string(idx + 1) + " Rate";
    }

    if (vmd >= vmd_Processor_Mix && vmd <= vmd_Processor_FP9)
    {
        auto pfx = std::string("P") + std::to_string(idx + 1) + ": ";
        if (z.processorStorage[idx].type == dsp::processor::proct_none)
        {
            // this should in theory get filtered out of the user choices
            return {};
        }
        pfx += z.processorDescription[idx].typeDisplayName + " ";
        if (vmd == vmd_Processor_Mix)
            return pfx + "mix";
        else
        {
            auto ct = (int)(vmd - vmd_Processor_FP1);
            if (z.processorDescription[idx].floatControlDescriptions[ct].type ==
                datamodel::ControlDescription::NONE)
                return {};

            return pfx +
                   z.processorDescription[idx].floatControlNames[(int)(vmd - vmd_Processor_FP1)];
        }
    }

    if (vmd >= vmd_eg_A && vmd <= vmd_eg_RShape)
    {
        auto pfx = (idx == 0 ? std::string("AEG") : std::string("EG2"));

        switch (vmd)
        {
        case vmd_eg_A:
            pfx += " Attack";
            break;
        case vmd_eg_H:
            pfx += " Hold";
            break;
        case vmd_eg_D:
            pfx += " Decay";
            break;
        case vmd_eg_S:
            pfx += " Sustain";
            break;
        case vmd_eg_R:
            pfx += " Release";
            break;
        case vmd_eg_AShape:
            pfx += " Attack Shape";
            break;
        case vmd_eg_DShape:
            pfx += " Decay Shape";
            break;
        case vmd_eg_RShape:
            pfx += " Release Shape";
            break;
        default:
            assert(false);
        }
        return pfx;
    }
    assert(false);
    return fmt::format("ERR {} {}", vmd, idx);
}

voiceModMatrixDestinationNames_t getVoiceModulationDestinationNames(const engine::Zone &z)
{
    voiceModMatrixDestinationNames_t res;
    // TODO code this way better - index on the 'outside' sorts but is inefficient
    int maxIndex = std::max({2, engine::processorsPerZone, engine::lfosPerZone});
    for (int idx = 0; idx < maxIndex; ++idx)
    {
        for (int i = vmd_none; i < numVoiceMatrixDestinations; ++i)
        {
            auto vd = (VoiceModMatrixDestinationType)i;
            auto ic = getVoiceModMatrixDestIndexCount(vd);
            if (idx < ic)
            {
                auto addr = VoiceModMatrixDestinationAddress{vd, (size_t)idx};
                auto dn = getVoiceModMatrixDestDisplayName(addr, z);
                if (dn.has_value())
                    res.emplace_back(addr, *dn);
            }
        }
    }
    return res;
}
voiceModMatrixSourceNames_t getVoiceModMatrixSourceNames(const engine::Zone &z)
{
    voiceModMatrixSourceNames_t res;
    for (int s = (int)vms_none; s < numVoiceMatrixSources; ++s)
    {
        auto vms = (VoiceModMatrixSource)s;
        res.emplace_back(vms, getVoiceModMatrixSourceDisplayName(vms));
    }
    return res;
}

voiceModMatrixCurveNames_t getVoiceModMatrixCurveNames(const engine::Zone &)
{
    voiceModMatrixCurveNames_t res;
    for (int s = (int)vmc_none; s < numVoiceMatrixCurves; ++s)
    {
        auto vms = (VoiceModMatrixCurve)s;
        res.emplace_back(vms, getVoiceModMatrixCurveDisplayName(vms));
    }
    return res;
}
} // namespace scxt::modulation
