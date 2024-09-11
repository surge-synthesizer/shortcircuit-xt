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
#ifndef SCXT_SRC_ENGINE_ZONE_H
#define SCXT_SRC_ENGINE_ZONE_H

#include <array>

#include "configuration.h"
#include "utils.h"

#include "keyboard.h"

#include "sst/basic-blocks/dsp/Lag.h"
#include "sample/sample_manager.h"
#include "dsp/processor/processor.h"
#include "modulation/voice_matrix.h"
#include "modulation/modulator_storage.h"

#include "group_and_zone.h"

#include <fmt/core.h>
#include "dsp/generator.h"
#include "bus.h"

namespace scxt::voice
{
struct Voice;
}

namespace scxt::engine
{
struct Group;
struct Engine;

constexpr int lfosPerZone{scxt::lfosPerZone};

struct Zone : MoveableOnly<Zone>, HasGroupZoneProcessors<Zone>, SampleRateSupport
{
    static constexpr int maxVariantsPerZone{scxt::maxVariantsPerZone};
    Zone() : id(ZoneID::next()) { initialize(); }
    Zone(SampleID sid) : id(ZoneID::next())
    {
        variantData.variants[0].sampleID = sid;
        variantData.variants[0].active = true;
        initialize();
    }
    Zone(Zone &&) = default;
    virtual ~Zone() { terminateAllVoices(); }

    ZoneID id;
    enum VariantPlaybackMode : uint16_t
    {
        FORWARD_RR,   // Cycle through variants in order
        TRUE_RANDOM,  // Pick next variant at random
        RANDOM_CYCLE, //  random without repeating the same variant twice
        UNISON
    };
    DECLARE_ENUM_STRING(VariantPlaybackMode);

    enum PlayMode
    {
        NORMAL,     // AEG gates; play on note on
        ONE_SHOT,   // SAMPLE playback gates; play on note on
        ON_RELEASE, // SAMPLE playback gates. play on note off
    };
    DECLARE_ENUM_STRING(PlayMode);

    enum LoopMode
    {
        LOOP_DURING_VOICE, // If a loop begins, stay in it for the life of teh voice
        LOOP_WHILE_GATED,  // If a loop begins, loop while gated
        LOOP_FOR_COUNT     // Loop exactly (n) times
    };
    DECLARE_ENUM_STRING(LoopMode);

    enum LoopDirection
    {
        FORWARD_ONLY,
        ALTERNATE_DIRECTIONS
    };
    DECLARE_ENUM_STRING(LoopDirection);

    struct SingleVariant
    {
        bool active{false};
        SampleID sampleID;
        int64_t startSample{-1}, endSample{-1}, startLoop{-1}, endLoop{-1};
        dsp::InterpolationTypes interpolationType{dsp::InterpolationTypes::Sinc};

        // VariantMode variantMode{RR};
        PlayMode playMode{NORMAL};
        bool loopActive{false};
        bool playReverse{false};
        LoopMode loopMode{LOOP_DURING_VOICE};
        LoopDirection loopDirection{FORWARD_ONLY};
        int loopCountWhenCounted{0};

        int64_t loopFade{0};
        float normalizationAmplitude{0.f}; // db
        // per-sample pitch and amplitude
        float pitchOffset{0.f}; // semitones
        float amplitude{0.f};   // db

        bool operator==(const SingleVariant &other) const
        {
            return active == other.active && sampleID == other.sampleID &&
                   startSample == other.startSample && endSample == other.endSample &&
                   startLoop == other.startLoop && endLoop == other.endLoop &&
                   normalizationAmplitude == other.normalizationAmplitude &&
                   pitchOffset == other.pitchOffset && amplitude == other.amplitude;
        }
    };

    struct Variants
    {
        std::array<SingleVariant, maxVariantsPerZone> variants;
        VariantPlaybackMode variantPlaybackMode{FORWARD_RR};
    } variantData;

    std::array<std::shared_ptr<sample::Sample>, maxVariantsPerZone> samplePointers;
    int8_t sampleIndex{-1};

    int numAvail{0};
    int setupFor{0};
    int lastPlayed{-1};
    std::array<int, maxVariantsPerZone> rrs{};

    auto getNumSampleLoaded() const
    {
        return std::distance(variantData.variants.begin(),
                             std::find_if(variantData.variants.begin(), variantData.variants.end(),
                                          [](const auto &s) { return s.active == false; }));
    }

    struct ZoneOutputInfo
    {
        float amplitude{1.f}, pan{0.f};
        bool muted{false};
        ProcRoutingPath procRouting{procRoute_linear};
        BusAddress routeTo{DEFAULT_BUS};
    } outputInfo;
    static_assert(std::is_standard_layout<ZoneOutputInfo>::value);

    float output alignas(16)[2][blockSize << 1];
    void process(Engine &onto);
    template <bool OS> void processWithOS(Engine &onto);

    std::string givenName{};
    std::string getName() const
    {
        if (!givenName.empty())
            return givenName;
        if (samplePointers[0])
            return samplePointers[0]->getDisplayName();
        return id.to_string();
    }

    // TODO: Multi-output
    size_t getNumOutputs() const { return 1; }

    enum SampleInformationRead
    {
        NONE = 0,
        MAPPING = 1 << 0,
        LOOP = 1 << 1,
        ENDPOINTS = 1 << 2,

        ALL = MAPPING | LOOP | ENDPOINTS
    };

    bool attachToSample(const sample::SampleManager &manager, int index = 0,
                        SampleInformationRead sir = ALL);
    bool attachToSampleAtVariation(const sample::SampleManager &manager, const SampleID &sid,
                                   int16_t variation, SampleInformationRead sir = ALL)
    {
        variantData.variants[variation].sampleID = sid;
        variantData.variants[variation].active = true;

        return attachToSample(manager, variation, sir);
    }

    void setNormalizedSampleLevel(bool usePeak = false, int associatedSampleID = -1);
    void clearNormalizedSampleLevel(int associatedSampleID = -1);

    struct ZoneMappingData
    {
        int16_t rootKey{60};
        KeyboardRange keyboardRange;
        VelocityRange velocityRange;

        int16_t pbDown{2}, pbUp{2};

        int16_t exclusiveGroup{0};

        float velocitySens{1.0};
        float amplitude{0.0};   // decibels
        float pan{0.0};         // -1..1
        float pitchOffset{0.0}; // semitones/keys

    } mapping;

    Group *parentGroup{nullptr};

    bool isActive() { return activeVoices != 0; }
    uint32_t activeVoices{0};
    std::array<voice::Voice *, maxVoices> voiceWeakPointers;
    int gatedVoiceCount{0};
    void terminateAllVoices();

    void initialize();
    // Just a weak ref - don't take ownership. engine manages lifetime
    void addVoice(voice::Voice *);
    void removeVoice(voice::Voice *);

    voice::modulation::Matrix::RoutingTable routingTable;
    void onRoutingChanged();

    std::array<modulation::ModulatorStorage, lfosPerZone> modulatorStorage;

    std::array<bool, lfosPerZone> lfosActive{};
    std::array<bool, egsPerZone> egsActive{};

    // 0 is the AEG, 1 is EG2
    std::array<modulation::modulators::AdsrStorage, 2> egStorage;

    void onProcessorTypeChanged(int, dsp::processor::ProcessorType) {}

    void setupOnUnstream(const engine::Engine &e);
    engine::Engine *getEngine();
    const engine::Engine *getEngine() const;

    sst::basic_blocks::dsp::UIComponentLagHandler mUILag;
    void onSampleRateChanged() override;
};
} // namespace scxt::engine

SC_DESCRIBE(scxt::engine::Zone::ZoneOutputInfo,
            SC_FIELD(amplitude, pmd().asCubicDecibelAttenuation().withName("Amplitude"));
            SC_FIELD(pan, pmd().asPercentBipolar().withName("Pan"));
            SC_FIELD(procRouting, pmd().asInt().withRange(0, 1));)

SC_DESCRIBE(
    scxt::engine::Zone::ZoneMappingData, SC_FIELD(rootKey, pmd().asMIDINote().withName("Root Key"));
    SC_FIELD(keyboardRange.keyStart, pmd().asMIDINote().withName("Key Start"));
    SC_FIELD(keyboardRange.keyEnd, pmd().asMIDINote().withName("Key End"));
    SC_FIELD(keyboardRange.fadeStart, pmd().asMIDIPitch().withUnit("").withName("Fade Start"));
    SC_FIELD(keyboardRange.fadeEnd, pmd().asMIDIPitch().withUnit("").withName("Fade End"));
    SC_FIELD(velocityRange.velStart, pmd().asMIDIPitch().withUnit("").withName("Velocity Start"));
    SC_FIELD(velocityRange.velEnd, pmd().asMIDIPitch().withUnit("").withName("Velocity End"));
    SC_FIELD(velocityRange.fadeStart,
             pmd().asMIDIPitch().withUnit("").withName("Velocity Fade Start"));
    SC_FIELD(velocityRange.fadeEnd, pmd().asMIDIPitch().withUnit("").withName("Velocity Fade End"));
    SC_FIELD(pbDown, pmd().asMIDIPitch().withUnit("").withDefault(2).withName("Pitch Bend Down"));
    SC_FIELD(pbUp, pmd().asMIDIPitch().withUnit("").withDefault(2).withName("Pitch Bend Up"));
    SC_FIELD(amplitude, pmd().asDecibelWithRange(-36, 36).withName("Amplitude").withDefault(0.f));
    SC_FIELD(pan, pmd().asPercentBipolar().withName("Pan").withDefault(0.0));
    SC_FIELD(pitchOffset, pmd().asSemitoneRange().withName("Pitch").withDefault(0.0)););

#endif
