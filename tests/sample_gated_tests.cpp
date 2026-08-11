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

#include "catch2/catch2.hpp"

#include <filesystem>
#include <memory>

#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "messaging/messaging.h"
#include "voice/voice.h"

#include "test_utils.h"

/*
 * A sample gated AEG is what "one shot" means now that the per-variant play mode is gone:
 * the voice lasts exactly as long as the sample plays. A loop with no end would hold it
 * open forever, so the generator ignores those; a counted loop does end, so it still runs.
 */

namespace fs = std::filesystem;

namespace
{
namespace mm = scxt::modulation::modulators;
using zone_t = scxt::engine::Zone;

struct SampleGatedFixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Group *group{nullptr};
    zone_t *zone{nullptr};

    SampleGatedFixture()
    {
        eng = std::make_unique<scxt::engine::Engine>();
        eng->prepareToPlay(TEST_SAMPLE_RATE);
        eng->midikeyRetuner.setTuningMode(scxt::tuning::MidikeyRetuner::TWELVE_TET);

        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();
        group = part.getGroup(0).get();

        auto z = std::make_unique<zone_t>();
        z->mapping.keyboardRange = {48, 84};
        z->mapping.velocityRange = {0, 127};
        z->mapping.rootKey = 60;
        z->initialize();
        group->addZone(z);
        zone = group->getZone(0).get();

        auto p = samplePath("WavStereo48k.wav");
        REQUIRE(fs::exists(p));

        // loadSampleByPath asserts it is on the serial thread; we are the only thread.
        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        auto sid = eng->getSampleManager()->loadSampleByPath(p);
        REQUIRE(sid.has_value());

        zone->variantData.variants[0].sampleID = *sid;
        zone->variantData.variants[0].active = true;
        // ENDPOINTS only — MAPPING would let the wav's chunks overwrite our key range.
        REQUIRE(zone->attachToSample(*eng->getSampleManager(), 0,
                                     zone_t::SampleInformationRead::ENDPOINTS));
    }

    // a loop well inside the sample, so the generator's bounds say which rule won
    void setLoop(zone_t::LoopMode mode)
    {
        auto &v = zone->variantData.variants[0];
        v.loopActive = true;
        v.loopMode = mode;
        v.startLoop = v.startSample + 100;
        v.endLoop = v.endSample - 100;
        v.loopCountWhenCounted = 2;
    }

    void setAegGateMode(mm::AdsrStorage::GateMode gm) { zone->egStorage[0].gateMode = gm; }

    // the generator bounds are set once, when the voice starts
    scxt::voice::Voice *playAndFindVoice(int key = 60)
    {
        eng->processNoteOnEvent(0, 0, key, -1, 1.f, 0.f);
        eng->processAudio();

        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned && v->isVoicePlaying)
                return v;
        }
        return nullptr;
    }
};
} // namespace

TEST_CASE("A sample gated AEG only allows a loop that ends", "[dsp]")
{
    SECTION("A gated AEG loops as asked")
    {
        SampleGatedFixture f;
        f.setLoop(zone_t::LoopMode::LOOP_DURING_VOICE);

        auto *v = f.playAndFindVoice();
        REQUIRE(v != nullptr);
        REQUIRE(v->GD[0].loopUpperBound == f.zone->variantData.variants[0].endLoop);
    }

    SECTION("A sample gated AEG ignores an open ended loop")
    {
        SampleGatedFixture f;
        f.setLoop(zone_t::LoopMode::LOOP_DURING_VOICE);
        f.setAegGateMode(mm::AdsrStorage::GateMode::SAMPLE_GATED);

        auto *v = f.playAndFindVoice();
        REQUIRE(v != nullptr);
        // playing straight through to the end, not around the loop
        REQUIRE(v->GD[0].loopUpperBound == f.zone->variantData.variants[0].endSample);
    }

    SECTION("A sample gated AEG keeps a counted loop")
    {
        SampleGatedFixture f;
        f.setLoop(zone_t::LoopMode::LOOP_COUNT);
        f.setAegGateMode(mm::AdsrStorage::GateMode::SAMPLE_GATED);

        auto *v = f.playAndFindVoice();
        REQUIRE(v != nullptr);
        REQUIRE(v->GD[0].loopUpperBound == f.zone->variantData.variants[0].endLoop);
    }
}
