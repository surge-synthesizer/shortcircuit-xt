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

#ifndef SCXT_SRC_VOICE_VOICE_H
#define SCXT_SRC_VOICE_VOICE_H

#include "engine/zone.h"
#include "engine/engine.h"
#include "dsp/data_tables.h"
#include "dsp/generator.h"
#include "dsp/processor/processor.h"

#include "modulation/voice_matrix.h"
#include "modulation/has_modulators.h"

#include "configuration.h"

#include "sst/filters/HalfRateFilter.h"
#include "sst/basic-blocks/dsp/BlockInterpolators.h"

namespace scxt::voice
{
struct alignas(16) Voice : MoveableOnly<Voice>,
                           SampleRateSupport,
                           scxt::modulation::shared::HasModulators<Voice>
{
    float output alignas(16)[2][blockSize << 2];
    // I do *not* own these. The engine guarantees it outlives the voice
    engine::Zone *zone{nullptr};
    engine::Engine *engine{nullptr};
    engine::Engine::pathToZone_t zonePath{};
    int8_t sampleIndex{0}; // int since - == no sample

    bool forceOversample{true};

    std::array<dsp::GeneratorState, maxGeneratorsPerVoice> GD;
    std::array<dsp::GeneratorIO, maxGeneratorsPerVoice> GDIO;
    std::array<dsp::GeneratorFPtr, maxGeneratorsPerVoice> Generator;
    std::array<bool, maxGeneratorsPerVoice> monoGenerator{};
    bool allGeneratorsMono{};
    int16_t numGeneratorsActive{0};

    std::pair<int16_t, int16_t> sampleIndexRange() const;

    sst::filters::HalfRate::HalfRateFilter halfRate;

    int16_t channel{0};
    uint8_t key{60};
    int32_t noteId{-1};
    uint8_t originalMidiKey{
        60}; // the actual physical key pressed not the one I resolved to after tuning
    float velocity{1.f};
    float velKeyFade{1.f};
    float keytrackPerOct{0.f}; // resolvee key - pitch cnter / 12
    float polyAT{0.f};
    static constexpr size_t noteExpressionCount{7};
    float noteExpressions[noteExpressionCount]{};
    // These are the same as teh CLAP expression IDs but I dont want to include
    // clap.h here
    enum struct ExpressionIDs
    {
        VOLUME = 0,
        PAN = 1,
        TUNING = 2,
        VIBRATO = 3,
        EXPRESSION = 4,
        BRIGHTNESS = 5,
        PRESSURE = 6
    };

    scxt::voice::modulation::Matrix modMatrix;
    std::unique_ptr<modulation::MatrixEndpoints> endpoints;

    ahdsrenv_t &aeg{eg[0]}, &eg2{eg[1]};
    ahdsrenvOS_t &aegOS{egOS[0]};

    inline float envelope_rate_linear_nowrap(float f)
    {
        // Super sneaky: this works for Oversample also since blockSize += and samplerate *2
        return blockSize * sampleRateInv * dsp::twoToTheXTable.twoToThe(-f);
    }

    inline const sst::basic_blocks::tables::TwoToTheXProvider &twoToTheXProvider()
    {
        return dsp::twoToTheXTable;
    }

    template <typename ET, int EB, typename ER>
    friend struct sst::basic_blocks::modulators::ADSREnvelope;

    float transportPhasors[scxt::numTransportPhasors];
    double startBeat{0.f};
    void updateTransportPhasors();

    Voice(engine::Engine *e, engine::Zone *z);

    ~Voice();

    /**
     * Generate the next blocksize of data into output
     * @return false if you cant
     */
    bool process();
    template <bool OS> bool processWithOS();

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
    void calculateGeneratorRatio(float pitch, int cSampleIndex, int generatorIndex);

    /**
     * Processors: Storage, memory blocks, types, and more
     */
    dsp::processor::Processor *processors[engine::processorCount]{};
    dsp::processor::ProcessorType processorType[engine::processorCount]{};
    uint8_t processorPlacementStorage alignas(
        16)[engine::processorCount][dsp::processor::processorMemoryBufferSize];
    int32_t processorIntParams alignas(
        16)[engine::processorCount][dsp::processor::maxProcessorIntParams];
    bool processorIsActive[engine::processorCount]{false, false, false, false};
    bool processorConsumesMono[engine::processorCount]{false, false, false, false};

    void initializeProcessors();

    using lipol = sst::basic_blocks::dsp::lipol_sse<blockSize, false>;
    using lipolOS = sst::basic_blocks::dsp::lipol_sse<blockSize << 1, false>;

    lipol samplePan, sampleAmp, outputPan, outputAmp;
    lipolOS samplePanOS, sampleAmpOS, outputPanOS, outputAmpOS;

    void panOutputsBy(bool inputIsMono, const lipol &pv);

    lipol processorMix[engine::processorCount];
    lipolOS processorMixOS[engine::processorCount];

    lipol processorLevel[engine::processorCount];
    lipolOS processorLevelOS[engine::processorCount];

    /*
     * Voice State on Creation
     */
    bool useOversampling{false};

    /*
     * Voice Playback State Model.
     */
    bool isGated{false};
    std::array<bool, maxGeneratorsPerVoice> isGeneratorRunning{};
    bool isAnyGeneratorRunning{};
    bool isAEGRunning{false};

    bool isVoicePlaying{false};
    bool isVoiceAssigned{false};

    int16_t terminationSequence{-1};
    // how many blocks is the early-terminate/steal fade
    static constexpr int blocksToTerminate{8};

    void attack()
    {
        isGated = true;
        isAEGRunning = true;

        isVoicePlaying = true;
        isVoiceAssigned = true;

        voiceStarted();
    }
    void release() { isGated = false; }
    void beginTerminationSequence() { terminationSequence = blocksToTerminate; }
    void cleanupVoice();

    void onSampleRateChanged() override;
};
} // namespace scxt::voice

#endif // __SCXT_VOICE_VOICE_H
