/*
 * ShortCircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortCircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortCircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortCircuitXT is inspired by, and shares some code with, the
 * commercial product ShortCircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for ShortCircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortCircuitXT is available at
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

namespace scxt::voice
{
struct Voice;
}

namespace scxt::engine
{
struct Group;

constexpr int processorsPerZone{2};
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

    // TODO: This becomes 'meta' eventually probably
    KeyboardRange keyboardRange;
    uint8_t rootKey{60};

    std::array<dsp::processor::ProcessorStorage, processorsPerZone> processorStorage;

    Group *parentGroup{nullptr};

    bool isActive() { return activeVoices != 0; }
    uint32_t activeVoices{0};
    std::array<voice::Voice *, maxVoices> voiceWeakPointers;
    void initialize()
    {
        for (auto &v : voiceWeakPointers)
            v = nullptr;

        for (auto &l : lfoStorage)
        {
            modulation::modulators::load_lfo_preset(modulation::modulators::lp_clear, &l);
        }
    }
    // Just a weak ref - don't take ownership. engine manages lifetime
    void addVoice(voice::Voice *);
    void removeVoice(voice::Voice *);

    std::array<modulation::VoiceModMatrix::Routing, modulation::numVoiceRoutingSlots> routingTable;
    std::array<modulation::modulators::StepLFOStorage, lfosPerZone> lfoStorage;

    datamodel::AdsrStorage aegStorage, eg2Storage;

    bool operator==(const Zone &other) const
    {
        auto res = sampleID.id == other.sampleID.id;
        res = res && rootKey == other.rootKey && keyboardRange == other.keyboardRange;
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