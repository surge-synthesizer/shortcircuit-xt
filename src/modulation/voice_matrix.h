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

// These values are streamed so order matters. Basically "always add at the end" is the answer
enum VoiceModMatrixDestination
{
    vmd_none,

    vmd_LFO1_Rate,
    vmd_LFO2_Rate,
    vmd_LFO3_Rate, // We assume these are contiguous in voice.cpp (that is LVO3 = LVO1 + 2)

    vmd_Processor1_Mix,
    vmd_Processor2_Mix,

    vmd_aeg_A,
    vmd_aeg_D,
    vmd_aeg_S,
    vmd_aeg_R,

    vmd_aeg_AShape,
    vmd_aeg_DShape,
    vmd_aeg_RShape,

    vmd_eg2_A,
    vmd_eg2_D,
    vmd_eg2_S,
    vmd_eg2_R,

    vmd_eg2_AShape,
    vmd_eg2_DShape,
    vmd_eg2_RShape,

    numVoiceMatrixDestinations
};

std::string getVoiceModMatrixDestStreamingName(const VoiceModMatrixDestination &dest);
std::optional<VoiceModMatrixDestination> fromVoiceModMatrixDestStreamingName(const std::string &s);

// These values are streamed so order matters. Basically "always add at the end" is the answer
enum VoiceModMatrixSource
{
    vms_none,

    vms_LFO1,
    vms_LFO2,
    vms_LFO3,

    numVoiceMatrixSources,
};

std::string getVoiceModMatrixSourceStreamingName(const VoiceModMatrixSource &dest);
std::optional<VoiceModMatrixSource> fromVoiceModMatrixSourceStreamingName(const std::string &s);

struct VoiceModMatrix : public MoveableOnly<VoiceModMatrix>
{
    VoiceModMatrix() { clear(); }
    struct Routing
    {
        VoiceModMatrixSource src{vms_none};
        VoiceModMatrixDestination dst{vmd_none};
        float depth{0};

        bool operator==(const Routing &other) const
        {
            return src == other.src && dst == other.dst && depth == other.depth;
        }
        bool operator!=(const Routing &other) const { return !(*this == other); }
    };

    std::array<Routing, numVoiceRoutingSlots> routingTable;

    const float *getValuePtr(VoiceModMatrixDestination dest) const
    {
        return &modulatedValues[dest];
    }

    float getValue(VoiceModMatrixDestination dest) const { return modulatedValues[dest]; }

    void clear();
    void snapRoutingFromZone(engine::Zone *z);
    void copyBaseValuesFromZone(engine::Zone *z);
    void attachSourcesFromVoice(voice::Voice *v);
    void initializeModulationValues();
    void process();

  protected:
    float *sourcePointers[numVoiceMatrixSources];
    float baseValues[numVoiceMatrixDestinations];
    float modulatedValues[numVoiceMatrixDestinations];
};
} // namespace scxt::modulation

#endif // __SCXT_VOICE_MATRIX_H
