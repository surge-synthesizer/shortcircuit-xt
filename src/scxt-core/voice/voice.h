/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_SCXT_CORE_VOICE_VOICE_H
#define SCXT_SRC_SCXT_CORE_VOICE_VOICE_H

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
#include "sst/basic-blocks/dsp/LagCollection.h"

namespace scxt::voice
{
struct alignas(16) Voice : MoveableOnly<Voice>,
                           SampleRateSupport,
                           scxt::modulation::shared::HasModulators<Voice, egsPerZone>
{
    float output alignas(16)[2][blockSize << 2];
    // I do *not* own these. The engine guarantees it outlives the voice
    engine::Zone *zone{nullptr};
    engine::Engine *engine{nullptr};
    engine::Engine::pathToZone_t zonePath{};
    int8_t sampleIndex{0}; // int since - == no sample
    float sampleIndexF{0.f};
    float sampleIndexFraction{0};

    float inLoopF{0.f}, currentLoopPercentageF{0.f}, currentSamplePercentageF{0.f}, loopCountF{0.0};

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
    float releaseVelocity{0.f};
    float velKeyFade{1.f};
    float keytrackPerOct{0.f}; // resolve key - pitch center / 12
    float polyAT{0.f};
    float mpePitchBend{0.f}; // in semis
    float mpeTimbre{0.f};    // 0..1 normalized
    float mpePressure{0.f};  // 0..1 normalized

    float keyChangedInLegatoModeTrigger{0.f};

    float glideSemitones{0.f};
    float glideProgress{0.f};
    bool inGlide{false};

    void initiateGlide(uint8_t newKey)
    {
        auto glideTime = zone->parentGroup->outputInfo.glideTime;
        if (glideTime <= 0.f)
        {
            inGlide = false;
            glideSemitones = 0.f;
            glideProgress = 0.f;
            return;
        }
        // If already gliding, start from the current sounding pitch, not the target key
        float currentOffset = inGlide ? glideSemitones * (1.f - glideProgress) : 0.f;
        glideSemitones = (float)key + currentOffset - (float)newKey;
        glideProgress = 0.f;
        inGlide = true;
    }

    void startGlideFrom(float pitch)
    {
        inGlide = false;
        auto p = calculateVoicePitch();
        auto diff = pitch - p;
        glideSemitones = diff;
        glideProgress = 0.f;
        inGlide = true;
    }

    // Advance glide state and return the pitch offset to apply
    float updateGlide()
    {
        if (!inGlide)
            return 0.f;

        auto glideTime = zone->parentGroup->outputInfo.glideTime;
        if (glideTime > 0.f)
        {
            auto rate = dsp::twentyFiveSecondExpTable.lookupRate(glideTime, dsp::twoToTheXTable);

            // In constant rate mode, scale rate by interval so larger intervals take longer
            if (zone->parentGroup->outputInfo.glideRateMode == engine::Group::CONSTANT_RATE)
            {
                auto semis = std::fabs(glideSemitones);
                if (semis > 0.f)
                    rate /= (semis / 12.f);
            }

            float glideIncrement = blockSize * sampleRateInv * rate;
            glideProgress += glideIncrement;
            if (glideProgress >= 1.f)
            {
                glideProgress = 1.f;
                inGlide = false;
            }
            return glideSemitones * (1.f - glideProgress);
        }
        else
        {
            inGlide = false;
            return 0.f;
        }
    }

    float retuningForKeyAtAttack{0.f};
    bool retuneContinuous{true};

    float normalizationAmplitudeLinear; // in multiples not db

    static constexpr size_t noteExpressionCount{7};
    float noteExpressions[noteExpressionCount]{};
    sst::basic_blocks::dsp::LagCollection<noteExpressionCount> noteExpressionLags;

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

    ahdsrenv_t &aeg{eg[0]};
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
    float isGatedF{0.f}, isReleasedF{0.f};
    void setIsGated(bool g)
    {
        isGated = g;
        isGatedF = g ? 1.f : 0.f;
        isReleasedF = g ? 0.f : 1.f;
    };

    std::array<bool, maxGeneratorsPerVoice> isGeneratorRunning{};
    bool isAnyGeneratorRunning{};
    bool isAEGRunning{false};

    bool isVoicePlaying{false};
    bool isVoiceAssigned{false};

    int16_t terminationSequence{-1};
    // how many blocks is the early-terminate/steal fade
    static constexpr int blocksToTerminateAt48k{8};
    int blocksToTerminate{blocksToTerminateAt48k};

    void attack()
    {
        setIsGated(true);
        isAEGRunning = true;

        isVoicePlaying = true;
        isVoiceAssigned = true;

        releaseVelocity = 0.f;

        voiceStarted();
        firstSamplePlayback = true;
    }
    bool firstSamplePlayback{false};

    void release() { setIsGated(false); }
    void beginTerminationSequence()
    {
        // 8 block fade at 48k
        blocksToTerminate =
            (int)std::ceil(std::max(sampleRate, 44100.) * blocksToTerminateAt48k / 48000);
        if (forceOversample)
            blocksToTerminate *= 2;
        terminationSequence = blocksToTerminate;
    }
    void cleanupVoice();

    void onSampleRateChanged() override;
};
} // namespace scxt::voice

#endif // __SCXT_VOICE_VOICE_H
