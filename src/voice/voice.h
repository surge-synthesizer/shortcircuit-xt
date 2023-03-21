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

#ifndef SCXT_SRC_VOICE_VOICE_H
#define SCXT_SRC_VOICE_VOICE_H

#include "engine/zone.h"
#include "dsp/data_tables.h"
#include "dsp/generator.h"
#include "dsp/processor/processor.h"

#include "modulation/voice_matrix.h"

#include "vembertech/vt_dsp/lipol.h"
#include "modulation/modulators/steplfo.h"

#include "configuration.h"

#include "sst/basic-blocks/modulators/ADSREnvelope.h"
#include "sst/basic-blocks/modulators/AHDSRShapedSC.h"
#include "sst/filters/HalfRateFilter.h"

namespace scxt::voice
{
struct alignas(16) Voice : MoveableOnly<Voice>, SampleRateSupport
{
    float output alignas(16)[2][blockSize << 1];
    engine::Zone *zone{nullptr}; // I do *not* own this. The engine guarantees it outlives the voice

    dsp::GeneratorState GD;
    dsp::GeneratorIO GDIO;
    dsp::GeneratorFPtr Generator;
    bool monoGenerator{false};

    sst::filters::HalfRate::HalfRateFilter halfRate;

    int16_t channel{0};
    uint8_t key{60};
    int32_t noteId{-1};
    uint8_t originalMidiKey{
        60}; // the actual physical key pressed not the one I resolved to after tuning

    modulation::VoiceModMatrix modMatrix;
    modulation::modulators::StepLFO lfos[engine::lfosPerZone];

    typedef sst::basic_blocks::modulators::AHDSRShapedSC<
        Voice, blockSize, sst::basic_blocks::modulators::ThirtyTwoSecondRange>
        ahdsrenv_t;
    ahdsrenv_t aeg, eg2;

    // TODO obviously this sucks move to a table
    inline float envelope_rate_linear_nowrap(float f)
    {
        return blockSize * sampleRateInv * pow(2.f, -f);
    }

    template <typename ET, int EB, typename ER>
    friend struct sst::basic_blocks::modulators::ADSREnvelope;

    Voice(engine::Zone *z);

    ~Voice();

    /**
     * Generate the next blocksize of data into output
     * @return false if you cant
     */
    bool process();

    /**
     * Voice Setup
     */
    void voiceStarted();

    /**
     * Initialize the dsp generator state
     */
    void initializeGenerator();

    /**
     * Calculates the pitch of this voice with modulation, MPE, tuning etc in
     */
    float calculateVoicePitch();

    /**
     * Calculates and updates the generator playback speed.
     */
    void calculateGeneratorRatio(float pitch);

    /**
     * Processors: Storage, memory blocks, types, and more
     */
    dsp::processor::Processor *processors[engine::processorsPerZone]{nullptr, nullptr};
    dsp::processor::ProcessorType processorType[engine::processorsPerZone]{
        dsp::processor::proct_none, dsp::processor::proct_none};
    uint8_t processorPlacementStorage alignas(
        16)[engine::processorsPerZone][dsp::processor::processorMemoryBufferSize];
    int32_t processorIntParams alignas(
        16)[engine::processorsPerZone][dsp::processor::maxProcessorIntParams];
    bool processorConsumesMono[engine::processorsPerZone]{false, false, false, false};
    bool processorProducesStereo[engine::processorsPerZone]{false, false, false, false};

    void initializeProcessors();

    void panOutputsBy(bool inputIsMono, float pv);

    // TODO - this should be more carefully structured for modulation onto the entire filter
    lipol_ps processorMix[engine::processorsPerZone];

    /*
     * Voice State on Creation
     */
    bool useOversampling{false};

    /*
     * Voice Playback State Model.
     */
    bool isGated{false};
    bool isGeneratorRunning{false};
    bool isAEGRunning{false};

    bool isVoicePlaying{false};
    bool isVoiceAssigned{false};

    void attack()
    {
        assert(zone->samplePointers[0]);
        isGated = true;
        isGeneratorRunning = true;
        isAEGRunning = true;

        isVoicePlaying = true;
        isVoiceAssigned = true;
        voiceStarted();
    }
    void release() { isGated = false; }
    void cleanupVoice()
    {
        zone->removeVoice(this);
        zone = nullptr;
        isVoiceAssigned = false;
    }
};
} // namespace scxt::voice

#endif // __SCXT_VOICE_VOICE_H
