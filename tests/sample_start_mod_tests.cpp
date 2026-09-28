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

#include "configuration.h"
#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "messaging/messaging.h"
#include "modulation/voice_matrix.h"
#include "voice/voice.h"

#include "test_utils.h"

/*
 * Start Pos modulation moves where a voice begins in the sample. A negative amount reaches
 * back before the start marker (or past the end marker when reversed), whether or not the
 * zone loops. GH #2270.
 */

// named rather than anonymous so the unity build cannot collide with other files' helpers
namespace sample_start_mod_test
{
namespace vm = scxt::voice::modulation;

constexpr int64_t START_MARKER{20000};
constexpr int64_t END_MARKER{200000};

// a few blocks of play either side of where the voice should have begun
constexpr int64_t SLOP{8 * (int64_t)scxt::blockSize};

struct StartModFixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Part *part{nullptr};
    scxt::engine::Zone *zone{nullptr};

    StartModFixture()
    {
        eng.reset(makeEngine());
        part = eng->getPatch()->getPart(0).get();
        part->addGroup();

        auto z = std::make_unique<scxt::engine::Zone>();
        z->mapping.keyboardRange = {48, 84};
        z->mapping.velocityRange = {0, 127};
        z->mapping.rootKey = 60;
        z->initialize();
        part->getGroup(0)->addZone(z);
        zone = part->getGroup(0)->getZone(0).get();

        auto p = samplePath("WavStereo48k.wav");
        REQUIRE(std::filesystem::exists(p));

        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        auto sid = eng->getSampleManager()->loadSampleByPath(p);
        REQUIRE(sid.has_value());
        auto &v = zone->variantData.variants[0];
        v.sampleID = *sid;
        v.active = true;
        REQUIRE(zone->attachToSample(*eng->getSampleManager(), 0,
                                     scxt::engine::Zone::SampleInformationRead::ENDPOINTS));

        v.startSample = START_MARKER;
        v.endSample = END_MARKER;
        v.startLoop = 50000;
        v.endLoop = 100000;

        // a constant full scale source
        part->macros[0].value = 1.f;
    }

    scxt::engine::Zone::SingleVariant &variant() { return zone->variantData.variants[0]; }

    int64_t sampleLength() const { return zone->samplePointers[0]->sampleLengthPerChannel; }

    void routeStartPos(float depth)
    {
        auto &row = zone->routingTable.routes[0];
        row.active = true;
        row.source = vm::sourcesForScanning().macroSources.macros[0];
        row.target = vm::MatrixConfig::TargetIdentifier{'samp', 'spos', 0};
        row.depth = depth;
        zone->onRoutingChanged();
    }

    // a bipolar target spans -1..1, so depth is scaled by two
    int64_t offsetFor(float depth) const { return (int64_t)(2 * depth * sampleLength()); }

    // position after the first block of a note on the root key
    int64_t firstBlockPosition()
    {
        eng->processNoteOnEvent(0, 0, 60, -1, 1.f, 0.f);
        eng->processAudio();

        scxt::voice::Voice *voice{nullptr};
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned)
                voice = v;
        }
        REQUIRE(voice);
        REQUIRE(voice->numGeneratorsActive == 1);
        REQUIRE_FALSE(voice->GD[0].isFinished);
        return voice->GD[0].samplePos;
    }
};

// forward play only moves up, so the voice began at or just below where it now is
void requireStartedNear(int64_t pos, int64_t expected, bool reverse)
{
    INFO("pos " << pos << " expected " << expected);
    if (reverse)
    {
        REQUIRE(pos <= expected);
        REQUIRE(pos > expected - SLOP);
    }
    else
    {
        REQUIRE(pos >= expected);
        REQUIRE(pos < expected + SLOP);
    }
}
} // namespace sample_start_mod_test

using namespace sample_start_mod_test;

TEST_CASE("Start Pos modulation moves a forward voice", "[modulation]")
{
    StartModFixture f;
    auto depth = GENERATE(0.01f, -0.01f);
    auto loop = GENERATE(false, true);
    INFO("depth " << depth << " loop " << loop);

    f.variant().loopActive = loop;
    f.routeStartPos(depth);

    requireStartedNear(f.firstBlockPosition(), START_MARKER + f.offsetFor(depth), false);
}

TEST_CASE("Start Pos modulation moves a reversed voice", "[modulation]")
{
    StartModFixture f;
    auto depth = GENERATE(0.01f, -0.01f);
    auto loop = GENERATE(false, true);
    INFO("depth " << depth << " loop " << loop);

    f.variant().playReverse = true;
    f.variant().loopActive = loop;
    f.routeStartPos(depth);

    requireStartedNear(f.firstBlockPosition(), END_MARKER - f.offsetFor(depth), true);
}

TEST_CASE("Start Pos modulation stops at the ends of the sample", "[modulation]")
{
    StartModFixture f;
    auto reverse = GENERATE(false, true);
    INFO("reverse " << reverse);

    f.variant().playReverse = reverse;
    f.routeStartPos(-1.f);

    auto pos = f.firstBlockPosition();
    REQUIRE(pos >= 0);
    REQUIRE(pos < f.sampleLength());
}
