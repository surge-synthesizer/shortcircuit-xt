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
#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>

#include "engine/engine.h"
#include "engine/part.h"
#include "json/engine_traits.h"

/*
 * Pins down the routing/dialect contract introduced by the MPE channel
 * refactor: global OmniFlavor decides MIDI semantics; per-part channel
 * value lives in {-1, 0..15} and is only consulted in OMNI mode.
 *
 * Legacy-stream tests round-trip an engine through JSON, then mutate the
 * resulting string (legacy stream version + injected -2/-3 sentinels) to
 * verify the migration in Engine::SC_TO.
 */

using namespace scxt;

namespace
{

scxt::engine::Engine *makeMidiEngine()
{
    auto *e = new scxt::engine::Engine();
    e->prepareToPlay(48000.0);
    return e;
}

// Streaming asserts the caller is the serialization thread. These tests
// drive the engine from the test thread, so bypass that check for the
// duration of the JSON conversion.
std::string streamEngine(scxt::engine::Engine &e)
{
    auto guard = e.getMessageController()->threadingChecker.bypassChecksInScope();
    scxt::engine::Engine::StreamGuard sg(scxt::engine::Engine::StreamReason::FOR_MULTI);
    return tao::json::to_string(scxt::json::scxt_value(e));
}

void unstreamEngine(const std::string &s, scxt::engine::Engine &into)
{
    auto guard = into.getMessageController()->threadingChecker.bypassChecksInScope();
    tao::json::events::transformer<tao::json::events::to_basic_value<scxt::json::scxt_traits>>
        consumer;
    tao::json::events::from_string(consumer, s);
    auto val = std::move(consumer.value);
    val.to(into);
}

// Lower the streamingVersion in the JSON to the prior value so
// SC_UNSTREAMING_FROM_PRIOR_TO(0x2026'05'21) fires on unstream.
std::string downgradeStreamingVersion(std::string s)
{
    auto pos = s.find("\"streamingVersion\":");
    REQUIRE(pos != std::string::npos);
    auto vstart = s.find(':', pos) + 1;
    auto vend = s.find_first_of(",}", vstart);
    s.replace(vstart, vend - vstart, std::to_string(0x2026'03'08));
    return s;
}

// Replace the FIRST occurrence of "c":<old> with "c":<new> inside the
// JSON. Tests construct their fixture so only part 0 needs touching.
std::string rewriteFirstPartChannel(std::string s, int newVal)
{
    auto pos = s.find("\"c\":");
    REQUIRE(pos != std::string::npos);
    auto vstart = pos + 4;
    auto vend = s.find_first_of(",}", vstart);
    s.replace(vstart, vend - vstart, std::to_string(newVal));
    return s;
}

} // namespace

TEST_CASE("OMNI mode routes per-part channel filter", "[midi_channel]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeMidiEngine());
    eng->setOmniFlavor(scxt::engine::Engine::OmniFlavor::OMNI);

    auto &p0 = *eng->getPatch()->getPart(0);
    auto &p1 = *eng->getPatch()->getPart(1);

    p0.configuration.channel = -1; // omni
    p1.configuration.channel = 3;  // ch 4

    for (int ch = 0; ch < 16; ++ch)
    {
        CHECK(p0.respondsToMIDIChannel(ch));
    }
    for (int ch = 0; ch < 16; ++ch)
    {
        if (ch == 3)
            CHECK(p1.respondsToMIDIChannel(ch));
        else
            CHECK_FALSE(p1.respondsToMIDIChannel(ch));
    }
}

TEST_CASE("MPE mode routes all channels regardless of per-part channel", "[midi_channel]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeMidiEngine());
    auto &p = *eng->getPatch()->getPart(0);
    p.configuration.channel = 5; // a stored value that MPE should ignore

    eng->setOmniFlavor(scxt::engine::Engine::OmniFlavor::MPE);

    for (int ch = 0; ch < 16; ++ch)
        CHECK(p.respondsToMIDIChannel(ch));

    CHECK(p.configuration.channel == 5); // unchanged
}

TEST_CASE("MPE mode flips voice manager dialect", "[midi_channel]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeMidiEngine());

    eng->setOmniFlavor(scxt::engine::Engine::OmniFlavor::OMNI);
    CHECK(eng->voiceManager.dialect == scxt::engine::Engine::voiceManager_t::MIDI1Dialect::MIDI1);

    eng->setOmniFlavor(scxt::engine::Engine::OmniFlavor::MPE);
    CHECK(eng->voiceManager.dialect ==
          scxt::engine::Engine::voiceManager_t::MIDI1Dialect::MIDI1_MPE);

    eng->setOmniFlavor(scxt::engine::Engine::OmniFlavor::CHOCT);
    CHECK(eng->voiceManager.dialect == scxt::engine::Engine::voiceManager_t::MIDI1Dialect::MIDI1);
}

TEST_CASE("CHOCT mode routes all channels and applies transposition", "[midi_channel]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeMidiEngine());
    auto &p = *eng->getPatch()->getPart(0);
    p.configuration.channel = 9;
    eng->setOmniFlavor(scxt::engine::Engine::OmniFlavor::CHOCT);

    for (int ch = 0; ch < 16; ++ch)
        CHECK(p.respondsToMIDIChannel(ch));

    // CHOCT transposes by repetition interval scaled by channel index.
    // Channels 0..7 → 0..7 octaves up, channels 8..15 → -8..-1 octaves.
    auto ri = eng->midikeyRetuner.getRepetitionInterval();
    CHECK(p.getChannelBasedTransposition(0) == 0);
    CHECK(p.getChannelBasedTransposition(3) == 3 * ri);
    CHECK(p.getChannelBasedTransposition(8) == -8 * ri);

    // Back in OMNI: transposition reverts to 0.
    eng->setOmniFlavor(scxt::engine::Engine::OmniFlavor::OMNI);
    CHECK(p.getChannelBasedTransposition(3) == 0);
}

TEST_CASE("Flavor flip preserves stored per-part channel", "[midi_channel]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeMidiEngine());
    auto &p0 = *eng->getPatch()->getPart(0);
    auto &p1 = *eng->getPatch()->getPart(1);
    auto &p2 = *eng->getPatch()->getPart(2);
    p0.configuration.channel = -1;
    p1.configuration.channel = 3;
    p2.configuration.channel = 7;

    eng->setOmniFlavor(scxt::engine::Engine::OmniFlavor::MPE);
    CHECK(p0.configuration.channel == -1);
    CHECK(p1.configuration.channel == 3);
    CHECK(p2.configuration.channel == 7);

    eng->setOmniFlavor(scxt::engine::Engine::OmniFlavor::CHOCT);
    CHECK(p0.configuration.channel == -1);
    CHECK(p1.configuration.channel == 3);
    CHECK(p2.configuration.channel == 7);

    eng->setOmniFlavor(scxt::engine::Engine::OmniFlavor::OMNI);
    CHECK(p0.configuration.channel == -1);
    CHECK(p1.configuration.channel == 3);
    CHECK(p2.configuration.channel == 7);
}

TEST_CASE("Legacy unstream: -2 sentinel recovers MPE flavor and clamps channel",
          "[midi_channel][streaming]")
{
    std::string s;
    {
        std::unique_ptr<scxt::engine::Engine> src(makeMidiEngine());
        src->setOmniFlavor(scxt::engine::Engine::OmniFlavor::OMNI);
        src->getPatch()->getPart(0)->configuration.channel = -1;
        s = streamEngine(*src);
    }
    s = downgradeStreamingVersion(s);
    s = rewriteFirstPartChannel(s, -2); // legacy MPE sentinel on part 0

    std::unique_ptr<scxt::engine::Engine> dst(makeMidiEngine());
    unstreamEngine(s, *dst);

    CHECK(dst->runtimeConfig.omniFlavor == scxt::engine::Engine::OmniFlavor::MPE);
    CHECK(dst->getPatch()->getPart(0)->configuration.channel == -1);
    CHECK(dst->voiceManager.dialect ==
          scxt::engine::Engine::voiceManager_t::MIDI1Dialect::MIDI1_MPE);
}

TEST_CASE("Legacy unstream: -3 sentinel recovers CHOCT flavor and clamps channel",
          "[midi_channel][streaming]")
{
    std::string s;
    {
        std::unique_ptr<scxt::engine::Engine> src(makeMidiEngine());
        src->setOmniFlavor(scxt::engine::Engine::OmniFlavor::OMNI);
        src->getPatch()->getPart(0)->configuration.channel = -1;
        s = streamEngine(*src);
    }
    s = downgradeStreamingVersion(s);
    s = rewriteFirstPartChannel(s, -3);

    std::unique_ptr<scxt::engine::Engine> dst(makeMidiEngine());
    unstreamEngine(s, *dst);

    CHECK(dst->runtimeConfig.omniFlavor == scxt::engine::Engine::OmniFlavor::CHOCT);
    CHECK(dst->getPatch()->getPart(0)->configuration.channel == -1);
}

TEST_CASE("Legacy unstream: -2 sentinel with matching MPE flavor still clamps",
          "[midi_channel][streaming]")
{
    std::string s;
    {
        std::unique_ptr<scxt::engine::Engine> src(makeMidiEngine());
        src->setOmniFlavor(scxt::engine::Engine::OmniFlavor::MPE);
        src->getPatch()->getPart(0)->configuration.channel = -1;
        s = streamEngine(*src);
    }
    s = downgradeStreamingVersion(s);
    s = rewriteFirstPartChannel(s, -2);

    std::unique_ptr<scxt::engine::Engine> dst(makeMidiEngine());
    unstreamEngine(s, *dst);

    CHECK(dst->runtimeConfig.omniFlavor == scxt::engine::Engine::OmniFlavor::MPE);
    CHECK(dst->getPatch()->getPart(0)->configuration.channel == -1);
}

TEST_CASE("Current-version unstream: legacy sentinel still clamps, flavor unchanged",
          "[midi_channel][streaming]")
{
    // Regression guard: when we next bump the streaming version, the
    // legacy-recovery branch will no longer fire — but the clamp inside
    // that branch goes with it. This test pins the current-version path:
    // no flavor recovery, but the channel still ends up valid.
    //
    // Today both branches sit under the same SC_UNSTREAMING_FROM_PRIOR_TO
    // guard, so a synthetic -2 in a *current-version* stream would simply
    // round-trip unchanged. Confirm that, so a future split between
    // "always clamp" and "conditional recovery" stays honest.
    std::string s;
    {
        std::unique_ptr<scxt::engine::Engine> src(makeMidiEngine());
        src->setOmniFlavor(scxt::engine::Engine::OmniFlavor::OMNI);
        src->getPatch()->getPart(0)->configuration.channel = -1;
        s = streamEngine(*src);
    }
    s = rewriteFirstPartChannel(s, -2);

    std::unique_ptr<scxt::engine::Engine> dst(makeMidiEngine());
    unstreamEngine(s, *dst);

    CHECK(dst->runtimeConfig.omniFlavor == scxt::engine::Engine::OmniFlavor::OMNI);
    // No clamp on current-version path today — the raw sentinel survives.
    CHECK(dst->getPatch()->getPart(0)->configuration.channel == -2);
}

TEST_CASE("Multi load round-trips per-part channels in valid range", "[midi_channel][streaming]")
{
    std::string s;
    {
        std::unique_ptr<scxt::engine::Engine> src(makeMidiEngine());
        src->setOmniFlavor(scxt::engine::Engine::OmniFlavor::OMNI);
        src->getPatch()->getPart(0)->configuration.channel = -1;
        src->getPatch()->getPart(1)->configuration.channel = 3;
        src->getPatch()->getPart(2)->configuration.channel = 7;
        s = streamEngine(*src);
    }

    std::unique_ptr<scxt::engine::Engine> dst(makeMidiEngine());
    unstreamEngine(s, *dst);
    CHECK(dst->getPatch()->getPart(0)->configuration.channel == -1);
    CHECK(dst->getPatch()->getPart(1)->configuration.channel == 3);
    CHECK(dst->getPatch()->getPart(2)->configuration.channel == 7);
}
