/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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
#include <vector>
#include "utils.h"
#include "dsp/processor/processor.h"
#include "base_matrix.h"

namespace scxt::engine
{
struct Zone;
} // namespace scxt::engine
namespace scxt::voice
{
struct Voice;
}

namespace scxt::modulation
{

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

    static constexpr inline size_t destIndex(VoiceModMatrixDestinationType t, size_t idx)
    {
        return (size_t)t + numVoiceMatrixDestinations * idx;
    }

    bool operator==(const VoiceModMatrixDestinationAddress &other) const
    {
        return other.type == type && other.index == index;
    }
};

std::string getVoiceModMatrixDestStreamingName(const VoiceModMatrixDestinationType &dest);
std::optional<VoiceModMatrixDestinationType>
fromVoiceModMatrixDestStreamingName(const std::string &s);
int getVoiceModMatrixDestIndexCount(const VoiceModMatrixDestinationType &);

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
struct VoiceModMatrixSourceMetadata
{
    VoiceModMatrixSource id;
    std::string streamingName;
    std::string displayName;
};

const std::vector<VoiceModMatrixSourceMetadata> voiceModMatrixSources = {
    {VoiceModMatrixSource::vms_none, "vms_none", ""},
    {VoiceModMatrixSource::vms_LFO1, "vms_lfo1", "LFO1"},
    {VoiceModMatrixSource::vms_LFO2, "vms_lfo2", "LFO2"},
    {VoiceModMatrixSource::vms_LFO3, "vms_lfo3", "LFO3"},
    {VoiceModMatrixSource::vms_AEG, "vms_aeg", "AEG"},
    {VoiceModMatrixSource::vms_EG2, "vms_eg2", "EG2"},
    {VoiceModMatrixSource::vms_ModWheel, "vms_modwheel", "ModWheel"}};

std::string getVoiceModMatrixSourceStreamingName(const VoiceModMatrixSource &dest);
std::optional<VoiceModMatrixSource> fromVoiceModMatrixSourceStreamingName(const std::string &s);

std::optional<std::string> getMenuDisplayName(const VoiceModMatrixDestinationAddress &dest,
                                              const engine::Zone &z);
std::optional<std::string> getSubMenuDisplayName(const VoiceModMatrixDestinationAddress &dest,
                                                 const engine::Zone &z);

// Destinations require a zone since we need to interrogate the processors
std::optional<std::string>
getVoiceModMatrixDestDisplayName(const VoiceModMatrixDestinationAddress &dest,
                                 const engine::Zone &z);

typedef std::vector<std::pair<VoiceModMatrixDestinationAddress, std::string>>
    voiceModMatrixDestinationNames_t;
voiceModMatrixDestinationNames_t getVoiceModulationDestinationNames(const engine::Zone &);
typedef std::vector<std::pair<VoiceModMatrixSource, std::string>> voiceModMatrixSourceNames_t;
voiceModMatrixSourceNames_t getVoiceModMatrixSourceNames(const engine::Zone &z);

typedef std::tuple<bool, voiceModMatrixSourceNames_t, voiceModMatrixDestinationNames_t,
                   modMatrixCurveNames_t>
    voiceModMatrixMetadata_t;
inline voiceModMatrixMetadata_t getVoiceModMatrixMetadata(const engine::Zone &z)
{
    return {true, getVoiceModMatrixSourceNames(z), getVoiceModulationDestinationNames(z),
            getModMatrixCurveNames()};
}

struct VoiceModMatrixTraits
{
    static constexpr int numModMatrixSlots{12};
    typedef VoiceModMatrixSource SourceEnum;
    static constexpr SourceEnum SourceEnumNoneValue{vms_none};
    typedef VoiceModMatrixDestinationAddress DestAddress;
    typedef VoiceModMatrixDestinationType DestEnum;
    static constexpr DestEnum DestEnumNoneValue{vmd_none};
};

struct VoiceModMatrix : public MoveableOnly<VoiceModMatrix>, ModMatrix<VoiceModMatrixTraits>
{
    VoiceModMatrix() { clear(); }

    void snapRoutingFromZone(engine::Zone *z);
    void snapDepthScalesFromZone(engine::Zone *z);
    void copyBaseValuesFromZone(engine::Zone *z);
    void attachSourcesFromVoice(voice::Voice *v);
};
} // namespace scxt::modulation

#endif // __SCXT_VOICE_MATRIX_H
