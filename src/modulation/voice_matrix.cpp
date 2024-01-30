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

#include "datamodel/parameter.h"
#include "voice_matrix.h"
#include "engine/zone.h"
#include "voice/voice.h"

namespace scxt::modulation
{

void VoiceModMatrix::snapRoutingFromZone(engine::Zone *z) { routingTable = z->routingTable; }

void VoiceModMatrix::snapDepthScalesFromZone(engine::Zone *z)
{
    // TODO - this is the wrong way to do this
    // The default is a depth of 1. Override other things
    for (auto &d : depthScales)
        d = 1.0;

    /*
     * LFOs
     */
    auto r = datamodel::lfoModulationRate();
    for (int idx = 0; idx < engine::lfosPerZone; ++idx)
    {
        depthScales[destIndex(vmd_LFO_Rate, idx)] = r.maxVal - r.minVal;
    }

    // Arbitrary
    depthScales[destIndex(vmd_Sample_Playback_Ratio, 0)] = 0.5;
    depthScales[destIndex(vmd_Sample_Pitch_Offset, 0)] = 60;

    // Processor Depth is by-processor
    for (int idx = 0; idx < engine::processorCount; ++idx)
    {
        for (int q = vmd_Processor_FP1; q <= vmd_Processor_FP9; ++q)
        {
            auto di = destIndex((VoiceModMatrixDestinationType)q, idx);
            auto pd = q - vmd_Processor_FP1;
            const auto &cd = z->processorDescription[idx].floatControlDescriptions[pd];
            depthScales[di] = cd.maxVal - cd.minVal;
        }
    }
}

void VoiceModMatrix::copyBaseValuesFromZone(engine::Zone *z)
{
    for (int i = 0; i < engine::processorCount; ++i)
    {
        // TODO - skip off
        baseValues[destIndex(vmd_Processor_Mix, i)] = z->processorStorage[i].mix;
        memcpy(&baseValues[destIndex(vmd_Processor_FP1, i)],
               z->processorStorage[i].floatParams.data(),
               sizeof(float) * dsp::processor::maxProcessorFloatParams);
    }

    for (int i = 0; i < engine::lfosPerZone; ++i)
    {
        baseValues[destIndex(vmd_LFO_Rate, i)] = z->modulatorStorage[i].rate;
    }

    // TODO : Shapes
    baseValues[destIndex(vmd_eg_A, 0)] = z->aegStorage.a;
    baseValues[destIndex(vmd_eg_H, 0)] = z->aegStorage.h;
    baseValues[destIndex(vmd_eg_D, 0)] = z->aegStorage.d;
    baseValues[destIndex(vmd_eg_S, 0)] = z->aegStorage.s;
    baseValues[destIndex(vmd_eg_R, 0)] = z->aegStorage.r;

    baseValues[destIndex(vmd_eg_AShape, 0)] = z->aegStorage.aShape;
    baseValues[destIndex(vmd_eg_DShape, 0)] = z->aegStorage.dShape;
    baseValues[destIndex(vmd_eg_RShape, 0)] = z->aegStorage.rShape;

    baseValues[destIndex(vmd_eg_A, 1)] = z->eg2Storage.a;
    baseValues[destIndex(vmd_eg_H, 1)] = z->eg2Storage.h;
    baseValues[destIndex(vmd_eg_D, 1)] = z->eg2Storage.d;
    baseValues[destIndex(vmd_eg_S, 1)] = z->eg2Storage.s;
    baseValues[destIndex(vmd_eg_R, 1)] = z->eg2Storage.r;

    baseValues[destIndex(vmd_eg_AShape, 1)] = z->eg2Storage.aShape;
    baseValues[destIndex(vmd_eg_DShape, 1)] = z->eg2Storage.dShape;
    baseValues[destIndex(vmd_eg_RShape, 1)] = z->eg2Storage.rShape;

    baseValues[destIndex(vmd_Sample_Playback_Ratio, 0)] = 0;
    baseValues[destIndex(vmd_Sample_Pitch_Offset, 0)] = z->mapping.pitchOffset;
    baseValues[destIndex(vmd_Zone_Sample_Pan, 0)] = z->mapping.pan;
    baseValues[destIndex(vmd_Zone_Sample_Amplitude, 0)] = z->mapping.amplitude;

    // TODO: FixMe when we do output section
    baseValues[destIndex(vmd_Zone_Output_Pan, 0)] = z->outputInfo.pan;
    baseValues[destIndex(vmd_Zone_Output_Amplitude, 0)] = z->outputInfo.amplitude;
}

void VoiceModMatrix::attachSourcesFromVoice(voice::Voice *v)
{
    sourcePointers[vms_none] = nullptr;
    for (int i = 0; i < engine::lfosPerZone; ++i)
    {
        sourcePointers[vms_LFO1 + i] = &(v->lfos[i].output);
    }
    sourcePointers[vms_AEG] = &(v->aeg.outBlock0);
    sourcePointers[vms_EG2] = &(v->eg2.outBlock0);
    sourcePointers[vms_ModWheel] = &(v->zone->parentGroup->parentPart->midiCCSmoothers[1].output);
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

    case vmd_Sample_Playback_Ratio:
        return "vmd_sample_playback_ratio";
    case vmd_Sample_Pitch_Offset:
        return "vmd_sample_pitch_offset";
    case vmd_Zone_Sample_Pan:
        return "vmd_pan";
    case vmd_Zone_Output_Pan:
        return "vmd_output_pan";

    case vmd_Zone_Sample_Amplitude:
        return "vmd_amp";
    case vmd_Zone_Output_Amplitude:
        return "vmd_output_amp";

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
    if (dest < numVoiceMatrixSources)
    {
        return voiceModMatrixSources[(int)dest].streamingName;
    }
    throw std::logic_error("Mysterious unhandled condition");
}

std::optional<VoiceModMatrixSource> fromVoiceModMatrixSourceStreamingName(const std::string &s)
{
    for (int i = vmd_none + 1; i < numVoiceMatrixSources; i++)
    {
        if (getVoiceModMatrixSourceStreamingName((VoiceModMatrixSource)i) == s)
        {
            return ((VoiceModMatrixSource)i);
        }
    }
    return vms_none;
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
        return engine::processorCount;
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

std::optional<std::string> getMenuDisplayName(const VoiceModMatrixDestinationAddress &dest,
                                              const engine::Zone &z)
{
    const auto &[vmd, idx] = dest;
    switch (vmd)
    {
    case vmd_none:
        return std::nullopt;
    case vmd_LFO_Rate:
        return "LFO " + std::to_string(idx + 1);

    case vmd_eg_A:
    case vmd_eg_H:
    case vmd_eg_D:
    case vmd_eg_S:
    case vmd_eg_R:
    case vmd_eg_AShape:
    case vmd_eg_DShape:
    case vmd_eg_RShape:
        return idx == 0 ? std::string("AEG") : std::string("EG2");

    case vmd_Zone_Sample_Pan:
    case vmd_Zone_Sample_Amplitude:
    case vmd_Sample_Playback_Ratio:
    case vmd_Sample_Pitch_Offset:
        return "Sample";

    case vmd_Zone_Output_Pan:
    case vmd_Zone_Output_Amplitude:
        return "Output";

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
    {
        auto pfx = std::string("P") + std::to_string(idx + 1) + ": ";
        if (z.processorStorage[idx].type == dsp::processor::proct_none)
        {
            // this should in theory get filtered out of the user choices
            return std::nullopt;
        }
        return z.processorDescription[idx].typeDisplayName;
    }
    default:
        throw std::logic_error("Unexpected destination");
    }
}

std::optional<std::string> getSubMenuDisplayName(const VoiceModMatrixDestinationAddress &dest,
                                                 const engine::Zone &z)
{
    const auto &[vmd, idx] = dest;
    switch (vmd)
    {
    case vmd_none:
        return std::nullopt;
    case vmd_LFO_Rate:
        return "Rate";
    case vmd_eg_A:
        return "Attack";
    case vmd_eg_H:
        return "Hold";
    case vmd_eg_D:
        return "Decay";
    case vmd_eg_S:
        return "Sustain";
    case vmd_eg_R:
        return "Release";
    case vmd_eg_AShape:
        return "Attack Shape";
    case vmd_eg_DShape:
        return "Decay Shape";
    case vmd_eg_RShape:
        return "Release Shape";
    case vmd_Zone_Sample_Pan:
    case vmd_Zone_Output_Pan:
        return "Pan";
    case vmd_Zone_Sample_Amplitude:
    case vmd_Zone_Output_Amplitude:
        return "Amplitude";
    case vmd_Sample_Playback_Ratio:
        return "Playback Ratio";
    case vmd_Sample_Pitch_Offset:
        return "Pitch Offset";
    case vmd_Processor_Mix:
        return "mix";
    case vmd_Processor_FP1:
    case vmd_Processor_FP2:
    case vmd_Processor_FP3:
    case vmd_Processor_FP4:
    case vmd_Processor_FP5:
    case vmd_Processor_FP6:
    case vmd_Processor_FP7:
    case vmd_Processor_FP8:
    case vmd_Processor_FP9:
    {
        auto ct = (int)(vmd - vmd_Processor_FP1);

        if (z.processorDescription[idx].floatControlDescriptions[ct].type == datamodel::pmd::NONE)
            return std::nullopt;

        return z.processorDescription[idx].floatControlDescriptions[ct].name;
    }
    default:
        throw std::logic_error("Unexpected destination");
    }
}

std::optional<std::string>
getVoiceModMatrixDestDisplayName(const VoiceModMatrixDestinationAddress &dest,
                                 const engine::Zone &z)
{
    auto menuName = getMenuDisplayName(dest, z);
    auto subMenuName = getSubMenuDisplayName(dest, z);
    if (menuName && subMenuName)
    {
        return *menuName + "/" + *subMenuName;
    }
    return std::nullopt;
}

voiceModMatrixDestinationNames_t getVoiceModulationDestinationNames(const engine::Zone &z)
{
    voiceModMatrixDestinationNames_t res;
    // TODO code this way better - index on the 'outside' sorts but is inefficient
    int maxIndex = std::max({2, engine::processorCount, engine::lfosPerZone});
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
    for (int s = (int)vms_none; s < numVoiceMatrixSources; s++)
    {
        res.emplace_back((VoiceModMatrixSource)s, voiceModMatrixSources[s].displayName);
    }
    return res;
}

} // namespace scxt::modulation
