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

#include <memory>

#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "messaging/messaging.h"
#include "messaging/client/detail/message_helpers.h"

#include "test_utils.h"

/*
 * Every zone value edit lands through detail::pokeZoneMemberValue, and a float going onto a
 * zone which is currently sounding does not land on the field at all: it becomes a lag
 * destination, so the value walks to its target over a few blocks rather than stepping and
 * clicking. Nothing else exercised that branch - the tests which send value edits use quiet
 * zones, and the tests which sound voices never send a value edit - so it is fenced here.
 */

namespace
{
using Zone = scxt::engine::Zone;
namespace det = scxt::messaging::client::detail;

// mUILag runs at 120Hz, so at the test rate a ramp resolves in about this many blocks
constexpr int lagBlocks{(int)(TEST_SAMPLE_RATE / 120 / scxt::blockSize)};

/*
 * One part / one group / one zone driven straight from the test thread, the same shape
 * glide_tests uses. The zone has an oscillator rather than a sample so a held note keeps
 * sounding indefinitely: if the voice ended, removeVoice would snap the lag to its target
 * and a ramp assertion would pass for the wrong reason.
 */
struct LagFixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Zone *zone{nullptr};

    LagFixture()
    {
        eng = std::make_unique<scxt::engine::Engine>();
        eng->prepareToPlay(TEST_SAMPLE_RATE);

        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();
        auto *group = part.getGroup(0).get();

        auto z = std::make_unique<scxt::engine::Zone>();
        z->mapping.keyboardRange = {48, 84};
        z->mapping.velocityRange = {0, 127};
        z->mapping.rootKey = 60;
        z->initialize();
        group->addZone(z);
        zone = group->getZone(0).get();

        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        zone->setProcessorType(0, scxt::dsp::processor::proct_osc_sineplus);
    }

    void noteOn(int key) { eng->processNoteOnEvent(0, 0, key, -1, 1.f, 0.f); }
    void runBlocks(int n)
    {
        for (int i = 0; i < n; ++i)
            eng->processAudio();
    }

    // the edit a zone output float control sends, at the point it reaches the engine
    void pokeAmplitude(float to)
    {
        det::pokeZoneMemberValue<float>(zone, zone->outputInfo,
                                        offsetof(Zone::ZoneOutputInfo, amplitude), to);
    }
};
} // namespace

TEST_CASE("A float edit on a sounding zone ramps to its target", "[uilag]")
{
    LagFixture f;
    f.noteOn(60);
    f.runBlocks(4);
    REQUIRE(f.zone->isActive());

    auto orig = f.zone->outputInfo.amplitude;
    const auto target = orig + 0.4f;

    f.pokeAmplitude(target);

    // the write went to the lag, so the field itself has not moved yet
    REQUIRE(f.zone->outputInfo.amplitude == Approx(orig));

    f.runBlocks(1);
    auto afterOne = f.zone->outputInfo.amplitude;
    INFO("after one block " << afterOne << " from " << orig << " towards " << target);
    REQUIRE(afterOne > orig);
    REQUIRE(afterOne < target);

    f.runBlocks(lagBlocks + 5);
    // still sounding: had the voice ended, removeVoice would have snapped this to target
    REQUIRE(f.zone->isActive());
    REQUIRE(f.zone->outputInfo.amplitude == Approx(target));
}

TEST_CASE("A float edit on a quiet zone lands at once", "[uilag]")
{
    LagFixture f;
    REQUIRE(!f.zone->isActive());

    auto target = f.zone->outputInfo.amplitude + 0.4f;
    f.pokeAmplitude(target);
    REQUIRE(f.zone->outputInfo.amplitude == Approx(target));
}

TEST_CASE("A non-float edit lands at once even while sounding", "[uilag]")
{
    LagFixture f;
    f.noteOn(60);
    f.runBlocks(4);
    REQUIRE(f.zone->isActive());

    // only floats are lagged; an int16 has nothing to ramp through
    det::pokeZoneMemberValue<int16_t>(f.zone, f.zone->mapping,
                                      offsetof(Zone::ZoneMappingData, pbUp), (int16_t)9);
    REQUIRE(f.zone->mapping.pbUp == 9);
}

TEST_CASE("A second edit mid-ramp retargets rather than stalling", "[uilag]")
{
    LagFixture f;
    f.noteOn(60);
    f.runBlocks(4);
    REQUIRE(f.zone->isActive());

    auto orig = f.zone->outputInfo.amplitude;
    f.pokeAmplitude(orig + 0.4f);
    f.runBlocks(3);

    // what a drag does: a new target every few blocks, before the last one arrived
    const auto finalTarget = orig + 0.1f;
    f.pokeAmplitude(finalTarget);
    f.runBlocks(lagBlocks + 5);

    REQUIRE(f.zone->isActive());
    REQUIRE(f.zone->outputInfo.amplitude == Approx(finalTarget));
}
