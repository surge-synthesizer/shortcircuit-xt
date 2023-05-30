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

#ifndef SCXT_SRC_MODULATION_VOICE_MATRIX_H
#define SCXT_SRC_MODULATION_VOICE_MATRIX_H

#include <string>
#include <array>
#include "utils.h"
#include "dsp/processor/processor.h"

namespace scxt::engine
{
struct Zone;
}
namespace scxt::voice
{
struct Voice;
}

namespace scxt::modulation
{

static constexpr int numVoiceRoutingSlots{32};

/* These values are not streamed, but the blocks do matter since in the
 * display value conversion we assume that (say) all the EG are together.
 * So if you add stuff here think quickly about how you will make
 * getVoiceModMatrixDestDisplayName work and also adjust the switches
 * and the depth scales if appropriate.
 */
enum VoiceModMatrixDestinationType
{
    vmd_none,

    vmd_LFO_Rate,

    vmd_Processor_Mix,
    vmd_Processor_FP1,
    vmd_Processor_FP2,
    vmd_Processor_FP3,
    vmd_Processor_FP4,
    vmd_Processor_FP5,
    vmd_Processor_FP6,
    vmd_Processor_FP7,
    vmd_Processor_FP8,
    vmd_Processor_FP9, // These should be contiguous and match maxProcessorFloatParams

    vmd_eg_A,
    vmd_eg_H,
    vmd_eg_D,
    vmd_eg_S,
    vmd_eg_R,

    vmd_eg_AShape,
    vmd_eg_DShape,
    vmd_eg_RShape,

    vmd_Zone_Sample_Pan,
    vmd_Zone_Output_Pan,

    vmd_Zone_Sample_Amplitude,
    vmd_Zone_Output_Amplitude,

    vmd_Sample_Playback_Ratio,
    vmd_Sample_Pitch_Offset,

    numVoiceMatrixDestinations
};

struct VoiceModMatrixDestinationAddress
{
    static constexpr int maxIndex{4}; // 4 processors per zone
    static constexpr int maxDestinations{maxIndex * numVoiceMatrixDestinations};
    VoiceModMatrixDestinationType type{vmd_none};
    size_t index{0};

    // want in order, not index interleaved, so we can look at FP as a block etc...
    operator size_t() const { return (size_t)type + numVoiceMatrixDestinations * index; }

    bool operator==(const VoiceModMatrixDestinationAddress &other) const
    {
        return other.type == type && other.index == index;
    }
};

static inline size_t destIndex(VoiceModMatrixDestinationType type, size_t index)
{
    return (size_t)type + numVoiceMatrixDestinations * index;
}

std::string getVoiceModMatrixDestStreamingName(const VoiceModMatrixDestinationType &dest);
std::optional<VoiceModMatrixDestinationType>
fromVoiceModMatrixDestStreamingName(const std::string &s);
int getVoiceModMatrixDestIndexCount(const VoiceModMatrixDestinationType &);

// These values are streamed so order matters. Basically "always add at the end" is the answer
enum VoiceModMatrixSource
{
    vms_none,

    vms_LFO1,
    vms_LFO2,
    vms_LFO3,

    vms_AEG,
    vms_EG2,

    vms_ModWheel,

    numVoiceMatrixSources,
};

std::string getVoiceModMatrixSourceStreamingName(const VoiceModMatrixSource &dest);
std::optional<VoiceModMatrixSource> fromVoiceModMatrixSourceStreamingName(const std::string &s);

enum VoiceModMatrixCurve
{
    vmc_none,
    vmc_cube,

    numVoiceMatrixCurves
};

std::string getVoiceModMatrixCurveStreamingName(const VoiceModMatrixCurve &);
std::optional<VoiceModMatrixCurve> fromVoiceModMatrixCurveStreamingName(const std::string &);
std::string getVoiceModMatrixCurveDisplayName(const VoiceModMatrixCurve &dest);

std::string getVoiceModMatrixSourceDisplayName(const VoiceModMatrixSource &dest);
// Destinations require a zone since we need to interrogate the processors
std::optional<std::string>
getVoiceModMatrixDestDisplayName(const VoiceModMatrixDestinationAddress &dest,
                                 const engine::Zone &z);

typedef std::vector<std::pair<VoiceModMatrixDestinationAddress, std::string>>
    voiceModMatrixDestinationNames_t;
voiceModMatrixDestinationNames_t getVoiceModulationDestinationNames(const engine::Zone &);
typedef std::vector<std::pair<VoiceModMatrixSource, std::string>> voiceModMatrixSourceNames_t;
voiceModMatrixSourceNames_t getVoiceModMatrixSourceNames(const engine::Zone &z);
typedef std::vector<std::pair<VoiceModMatrixCurve, std::string>> voiceModMatrixCurveNames_t;
voiceModMatrixCurveNames_t getVoiceModMatrixCurveNames(const engine::Zone &z);

typedef std::tuple<bool, voiceModMatrixSourceNames_t, voiceModMatrixDestinationNames_t,
                   voiceModMatrixCurveNames_t>
    voiceModMatrixMetadata_t;
inline voiceModMatrixMetadata_t getVoiceModMatrixMetadata(const engine::Zone &z)
{
    return {true, getVoiceModMatrixSourceNames(z), getVoiceModulationDestinationNames(z),
            getVoiceModMatrixCurveNames(z)};
}

struct VoiceModMatrix : public MoveableOnly<VoiceModMatrix>
{
    VoiceModMatrix() { clear(); }
    struct Routing
    {
        bool active{true};
        VoiceModMatrixSource src{vms_none};
        VoiceModMatrixSource srcVia{vms_none};
        VoiceModMatrixDestinationAddress dst{vmd_none};
        VoiceModMatrixCurve curve{vmc_none};
        // TODO curves
        float depth{0};

        bool operator==(const Routing &other) const
        {
            return src == other.src && dst == other.dst && depth == other.depth;
        }
        bool operator!=(const Routing &other) const { return !(*this == other); }
    };

    typedef std::array<Routing, numVoiceRoutingSlots> routingTable_t;
    routingTable_t routingTable;

    float *getValuePtr(const VoiceModMatrixDestinationAddress &dest)
    {
        return &modulatedValues[dest];
    }

    float *getValuePtr(const VoiceModMatrixDestinationType &type, size_t index)
    {
        return &modulatedValues[destIndex(type, index)];
    }

    float getValue(const VoiceModMatrixDestinationAddress &dest) const
    {
        return modulatedValues[dest];
    }

    float getValue(const VoiceModMatrixDestinationType &type, size_t index) const
    {
        return modulatedValues[destIndex(type, index)];
    }

    void clear();
    void snapRoutingFromZone(engine::Zone *z);
    void snapDepthScalesFromZone(engine::Zone *z);
    void copyBaseValuesFromZone(engine::Zone *z);
    void attachSourcesFromVoice(voice::Voice *v);
    void initializeModulationValues();
    void process();

  protected:
    float *sourcePointers[VoiceModMatrixDestinationAddress::maxDestinations];
    float depthScales[VoiceModMatrixDestinationAddress::maxDestinations];
    float baseValues[VoiceModMatrixDestinationAddress::maxDestinations];
    float modulatedValues[VoiceModMatrixDestinationAddress::maxDestinations];
};
} // namespace scxt::modulation

#endif // __SCXT_VOICE_MATRIX_H
