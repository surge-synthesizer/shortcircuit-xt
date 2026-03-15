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
#include "engine/zone.h"
#include "modulation/modulator_storage.h"
#include "modulation/has_modulators.h"

// Use the Zone instantiation of HasModulators to access evaluateGate and the Stage type.
// evaluateGate is static so no Zone object is needed.
using ZoneMods = scxt::modulation::shared::HasModulators<scxt::engine::Zone, scxt::egsPerZone>;
using AS = scxt::modulation::modulators::AdsrStorage;
using GM = AS::GateMode;
using Stage = ZoneMods::ahdsrenv_t::Stage;

static constexpr Stage stg_attack{Stage::s_attack};
static constexpr Stage stg_hold{Stage::s_hold};
static constexpr Stage stg_decay{Stage::s_decay};
static constexpr Stage stg_sustain{Stage::s_sustain};
static constexpr Stage stg_release{Stage::s_release};

TEST_CASE("GateMode::GATED follows key gate regardless of sample or stage")
{
    // Key held: always true
    REQUIRE(ZoneMods::evaluateGate(true, GM::GATED, stg_attack));
    REQUIRE(ZoneMods::evaluateGate(true, GM::GATED, stg_sustain));
    REQUIRE(ZoneMods::evaluateGate(true, GM::GATED, stg_release));

    // Key released: always false
    REQUIRE_FALSE(ZoneMods::evaluateGate(false, GM::GATED, stg_attack));
    REQUIRE_FALSE(ZoneMods::evaluateGate(false, GM::GATED, stg_sustain));

    // samplePlaying has no effect on GATED
    REQUIRE(ZoneMods::evaluateGate(true, GM::GATED, stg_sustain, false));
    REQUIRE_FALSE(ZoneMods::evaluateGate(false, GM::GATED, stg_sustain, true));
}

TEST_CASE("GateMode::SEMI_GATED gates on key before sustain, open after")
{
    // Key held: true before sustain, false at/after sustain
    REQUIRE(ZoneMods::evaluateGate(true, GM::SEMI_GATED, stg_attack));
    REQUIRE(ZoneMods::evaluateGate(true, GM::SEMI_GATED, stg_hold));
    REQUIRE(ZoneMods::evaluateGate(true, GM::SEMI_GATED, stg_decay));
    REQUIRE_FALSE(ZoneMods::evaluateGate(true, GM::SEMI_GATED, stg_sustain));
    REQUIRE_FALSE(ZoneMods::evaluateGate(true, GM::SEMI_GATED, stg_release));

    // Key released: always false (regardless of stage)
    REQUIRE_FALSE(ZoneMods::evaluateGate(false, GM::SEMI_GATED, stg_attack));
    REQUIRE_FALSE(ZoneMods::evaluateGate(false, GM::SEMI_GATED, stg_decay));
}

TEST_CASE("GateMode::ONESHOT ignores key gate, open before sustain")
{
    // Key state irrelevant: true before sustain
    REQUIRE(ZoneMods::evaluateGate(true, GM::ONESHOT, stg_attack));
    REQUIRE(ZoneMods::evaluateGate(false, GM::ONESHOT, stg_attack));
    REQUIRE(ZoneMods::evaluateGate(false, GM::ONESHOT, stg_decay));

    // False at/after sustain regardless of key
    REQUIRE_FALSE(ZoneMods::evaluateGate(true, GM::ONESHOT, stg_sustain));
    REQUIRE_FALSE(ZoneMods::evaluateGate(false, GM::ONESHOT, stg_sustain));
    REQUIRE_FALSE(ZoneMods::evaluateGate(true, GM::ONESHOT, stg_release));
}

TEST_CASE("GateMode::SAMPLE_GATED follows sample playback, ignores key gate and stage")
{
    // Sample playing: gate open regardless of key state or stage
    REQUIRE(ZoneMods::evaluateGate(true, GM::SAMPLE_GATED, stg_attack, true));
    REQUIRE(ZoneMods::evaluateGate(false, GM::SAMPLE_GATED, stg_attack, true));
    REQUIRE(ZoneMods::evaluateGate(true, GM::SAMPLE_GATED, stg_sustain, true));
    REQUIRE(ZoneMods::evaluateGate(false, GM::SAMPLE_GATED, stg_sustain, true));
    REQUIRE(ZoneMods::evaluateGate(true, GM::SAMPLE_GATED, stg_release, true));

    // Sample stopped: gate closed regardless of key state or stage
    REQUIRE_FALSE(ZoneMods::evaluateGate(true, GM::SAMPLE_GATED, stg_attack, false));
    REQUIRE_FALSE(ZoneMods::evaluateGate(false, GM::SAMPLE_GATED, stg_attack, false));
    REQUIRE_FALSE(ZoneMods::evaluateGate(true, GM::SAMPLE_GATED, stg_sustain, false));
    REQUIRE_FALSE(ZoneMods::evaluateGate(false, GM::SAMPLE_GATED, stg_release, false));
}

TEST_CASE("GateMode serialization round-trips including SAMPLE_GATED")
{
    REQUIRE(AS::toStringGateMode(GM::GATED) == "g");
    REQUIRE(AS::toStringGateMode(GM::SEMI_GATED) == "s");
    REQUIRE(AS::toStringGateMode(GM::ONESHOT) == "o");
    REQUIRE(AS::toStringGateMode(GM::SAMPLE_GATED) == "q");

    REQUIRE(AS::fromStringGateMode("g") == GM::GATED);
    REQUIRE(AS::fromStringGateMode("s") == GM::SEMI_GATED);
    REQUIRE(AS::fromStringGateMode("o") == GM::ONESHOT);
    REQUIRE(AS::fromStringGateMode("q") == GM::SAMPLE_GATED);

    // Unknown string falls back to GATED
    REQUIRE(AS::fromStringGateMode("?") == GM::GATED);
}
