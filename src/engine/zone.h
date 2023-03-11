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
#ifndef SCXT_SRC_ENGINE_ZONE_H
#define SCXT_SRC_ENGINE_ZONE_H

#include <array>

#include "configuration.h"
#include "utils.h"

#include "keyboard.h"

#include "sample/sample_manager.h"
#include "dsp/processor/processor.h"
#include "modulation/voice_matrix.h"
#include "modulation/modulators/steplfo.h"

#include "datamodel/adsr_storage.h"
#include <fmt/core.h>

namespace scxt::voice
{
struct Voice;
}

namespace scxt::engine
{
struct Group;
struct Engine;

constexpr int processorsPerZone{4};
constexpr int lfosPerZone{3};

struct Zone : MoveableOnly<Zone>
{
    Zone() : id(ZoneID::next()) { initialize(); }
    Zone(SampleID sid) : id(ZoneID::next()), sampleID(sid) { initialize(); }
    Zone(Zone &&) = default;

    ZoneID id;
    SampleID sampleID;
    std::shared_ptr<sample::Sample> sample{nullptr};

    float output alignas(16)[maxOutputs][2][blockSize];
    void process();

    // TODO: editable name
    std::string getName() const
    {
        if (sample)
            return sample->getDisplayName();
        return id.to_string();
    }

    // TODO: Multi-output
    size_t getNumOutputs() const { return 1; }

    bool attachToSample(const sample::SampleManager &manager)
    {
        if (sampleID.isValid())
        {
            sample = manager.getSample(sampleID);
        }
        else
        {
            sample.reset();
        }
        return sample != nullptr;
    }

    struct ZoneMappingData
    {
        int16_t rootKey{60};
        KeyboardRange keyboardRange;
        VelocityRange velocityRange;

        int16_t pbDown{2}, pbUp{2};

        int16_t exclusiveGroup;

        float velocitySens{1.0};
        float amplitude{1.0};   // linear
        float pan{0.0};         // -1..1
        float pitchOffset{0.0}; // semitones/keys

        auto asTuple() const
        {
            return std::tie(rootKey, keyboardRange, velocityRange, pbDown, pbUp, amplitude, pan,
                            pitchOffset, exclusiveGroup, velocitySens);
        }
        bool operator==(const ZoneMappingData &other) const { return asTuple() == other.asTuple(); }
    } mapping;

    std::array<dsp::processor::ProcessorStorage, processorsPerZone> processorStorage;
    std::array<dsp::processor::ProcessorControlDescription, processorsPerZone> processorDescription;

    Group *parentGroup{nullptr};

    bool isActive() { return activeVoices != 0; }
    uint32_t activeVoices{0};
    std::array<voice::Voice *, maxVoices> voiceWeakPointers;
    void initialize();
    // Just a weak ref - don't take ownership. engine manages lifetime
    void addVoice(voice::Voice *);
    void removeVoice(voice::Voice *);

    void setProcessorType(int whichProcessor, dsp::processor::ProcessorType type);
    void setupProcessorControlDescriptions(int whichProcessor, dsp::processor::ProcessorType type,
                                           dsp::processor::Processor *tmpProcessor = nullptr);

    modulation::VoiceModMatrix::routingTable_t routingTable;
    std::array<modulation::modulators::StepLFOStorage, lfosPerZone> lfoStorage;

    datamodel::AdsrStorage aegStorage, eg2Storage;

    void setupOnUnstream(const engine::Engine &e);
    engine::Engine *getEngine();

    bool operator==(const Zone &other) const
    {
        auto res = sampleID.id == other.sampleID.id;
        res = res && mapping == other.mapping;
        // Bail out before the expensive checks
        if (!res)
            return false;
        res = res && (processorStorage == other.processorStorage) &&
              (routingTable == other.routingTable) && (lfoStorage == other.lfoStorage);
        return res;
    }
    bool operator!=(const Zone &other) const { return !(*this == other); }
};
} // namespace scxt::engine

#endif