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
#include "engine/engine.h"
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

using MS = scxt::modulation::ModulatorStorage;
using TM = MS::TriggerMode;
using CurveLFO = scxt::modulation::modulators::CurveLFO;
using Transport = sst::basic_blocks::modulators::Transport;

TEST_CASE("TriggerMode serialization round-trips")
{
    REQUIRE(MS::toStringTriggerMode(TM::KEYTRIGGER) == "kt");
    REQUIRE(MS::toStringTriggerMode(TM::SONGPOS) == "song");
    REQUIRE(MS::toStringTriggerMode(TM::RANDOM) == "rnd");
    REQUIRE(MS::toStringTriggerMode(TM::RELEASE) == "rel");
    REQUIRE(MS::toStringTriggerMode(TM::ONESHOT) == "ones");

    REQUIRE(MS::fromStringTriggerMode("kt") == TM::KEYTRIGGER);
    REQUIRE(MS::fromStringTriggerMode("song") == TM::SONGPOS);
    REQUIRE(MS::fromStringTriggerMode("rnd") == TM::RANDOM);
    REQUIRE(MS::fromStringTriggerMode("rel") == TM::RELEASE);
    REQUIRE(MS::fromStringTriggerMode("ones") == TM::ONESHOT);

    // Unknown falls back to KEYTRIGGER - including the retired "free" token, which
    // never did anything but key trigger anyway
    REQUIRE(MS::fromStringTriggerMode("free") == TM::KEYTRIGGER);
    REQUIRE(MS::fromStringTriggerMode("?") == TM::KEYTRIGGER);
}

TEST_CASE("Non-songpos trigger modes start at the phase param")
{
    sst::basic_blocks::dsp::RNG rng{8675309};
    Transport tr;
    tr.timeInSeconds = 3.7;

    MS ms;
    ms.start_phase = 0.3f;
    ms.rate = 0.f;

    for (auto tm : {TM::KEYTRIGGER, TM::RELEASE, TM::ONESHOT})
    {
        ms.triggerMode = tm;
        REQUIRE(ZoneMods::initialLFOPhase(ms, tr, ms.rate, rng) == Approx(0.3f));
    }
}

TEST_CASE("RANDOM trigger starts at a random phase, ignoring the phase param")
{
    sst::basic_blocks::dsp::RNG rng{8675309};
    Transport tr;

    MS ms;
    ms.triggerMode = TM::RANDOM;
    ms.start_phase = 0.3f;

    auto a = ZoneMods::initialLFOPhase(ms, tr, ms.rate, rng);
    auto b = ZoneMods::initialLFOPhase(ms, tr, ms.rate, rng);

    REQUIRE(a >= 0.f);
    REQUIRE(a < 1.f);
    REQUIRE(b >= 0.f);
    REQUIRE(b < 1.f);
    REQUIRE(a != b);
}

TEST_CASE("SONGPOS trigger locks phase to the seconds timeline")
{
    sst::basic_blocks::dsp::RNG rng{8675309};
    Transport tr;

    MS ms;
    ms.triggerMode = TM::SONGPOS;
    ms.start_phase = 0.f;
    ms.temposync = false;

    // rate is log2 Hz, so rate 0 is a one second cycle
    ms.rate = 0.f;
    tr.timeInSeconds = 0.0;
    REQUIRE(ZoneMods::initialLFOPhase(ms, tr, ms.rate, rng) == Approx(0.f));

    tr.timeInSeconds = 0.25;
    REQUIRE(ZoneMods::initialLFOPhase(ms, tr, ms.rate, rng) == Approx(0.25f));

    // A whole number of cycles later we are back where we started
    tr.timeInSeconds = 7.0;
    REQUIRE(ZoneMods::initialLFOPhase(ms, tr, ms.rate, rng) == Approx(0.f));

    // rate 1 is 2Hz, so a quarter second is half a cycle
    ms.rate = 1.f;
    tr.timeInSeconds = 0.25;
    REQUIRE(ZoneMods::initialLFOPhase(ms, tr, ms.rate, rng) == Approx(0.5f));

    // The phase param offsets the song lock
    ms.rate = 0.f;
    ms.start_phase = 0.1f;
    tr.timeInSeconds = 0.25;
    REQUIRE(ZoneMods::initialLFOPhase(ms, tr, ms.rate, rng) == Approx(0.35f));

    // ...and wraps
    tr.timeInSeconds = 0.95;
    REQUIRE(ZoneMods::initialLFOPhase(ms, tr, ms.rate, rng) == Approx(0.05f));
}

TEST_CASE("Temposynced SONGPOS lands on the same phase at the same beat, any tempo")
{
    sst::basic_blocks::dsp::RNG rng{8675309};

    MS ms;
    ms.triggerMode = TM::SONGPOS;
    ms.start_phase = 0.f;
    ms.temposync = true;
    ms.rate = 1.f;

    // Same musical position, three tempos: a temposynced LFO is a function of beats
    auto phaseAtBeat = [&](double beats, double tempo) {
        Transport tr;
        tr.tempo = tempo;
        tr.timeInSeconds = beats * 60.0 / tempo;
        return ZoneMods::initialLFOPhase(ms, tr, ms.rate, rng);
    };

    for (auto beats : {0.0, 1.0, 2.5, 9.75})
    {
        REQUIRE(phaseAtBeat(beats, 90.0) == Approx(phaseAtBeat(beats, 120.0)).margin(1e-5));
        REQUIRE(phaseAtBeat(beats, 140.0) == Approx(phaseAtBeat(beats, 120.0)).margin(1e-5));
    }

    // And it does actually move
    REQUIRE(phaseAtBeat(0.0, 120.0) != Approx(phaseAtBeat(0.7, 120.0)));
}

TEST_CASE("lfoEnvelopeGate maps trigger mode onto the sub-envelope gate")
{
    // Key trigger, songpos and random just follow the key
    for (auto tm : {TM::KEYTRIGGER, TM::SONGPOS, TM::RANDOM})
    {
        REQUIRE(ZoneMods::lfoEnvelopeGate(tm, true, stg_attack, stg_sustain));
        REQUIRE(ZoneMods::lfoEnvelopeGate(tm, true, stg_sustain, stg_sustain));
        REQUIRE_FALSE(ZoneMods::lfoEnvelopeGate(tm, false, stg_attack, stg_sustain));
    }

    // RELEASE fires with the key already up, so it holds its own gate open
    REQUIRE(ZoneMods::lfoEnvelopeGate(TM::RELEASE, false, stg_attack, stg_sustain));
    REQUIRE(ZoneMods::lfoEnvelopeGate(TM::RELEASE, false, stg_sustain, stg_sustain));

    // ONESHOT ignores the key and drops the gate once the envelope is past its peak
    REQUIRE(ZoneMods::lfoEnvelopeGate(TM::ONESHOT, false, stg_attack, stg_sustain));
    REQUIRE(ZoneMods::lfoEnvelopeGate(TM::ONESHOT, false, stg_decay, stg_sustain));
    REQUIRE_FALSE(ZoneMods::lfoEnvelopeGate(TM::ONESHOT, true, stg_sustain, stg_sustain));
    REQUIRE_FALSE(ZoneMods::lfoEnvelopeGate(TM::ONESHOT, true, stg_release, stg_sustain));

    // The curve LFO's DAR envelope hands in s_hold as its cutoff instead
    REQUIRE(ZoneMods::lfoEnvelopeGate(TM::ONESHOT, true, stg_attack, stg_hold));
    REQUIRE_FALSE(ZoneMods::lfoEnvelopeGate(TM::ONESHOT, true, stg_hold, stg_hold));
}

/*
 * Drive a real Group's ENV-shaped LFO through the shared processLFOBlock with a held
 * gate and watch its output: a looping env should cycle (climb to its peak more than
 * once), a non-looping one should climb once and hold. Runs on the test thread with no
 * voice, so the group stays put for as long as we keep calling it.
 */
namespace
{
constexpr double LOOP_TEST_SR = 48000.0;

scxt::engine::Group *setupEnvLFOGroup(scxt::engine::Engine *e, bool loop)
{
    e->prepareToPlay(LOOP_TEST_SR);

    auto &part = *e->getPatch()->getPart(0);
    part.addGroup();
    auto *g = part.getGroup(0).get();
    g->setSampleRate(LOOP_TEST_SR);

    auto z = std::make_unique<scxt::engine::Zone>();
    z->mapping.keyboardRange = {60, 60};
    z->mapping.velocityRange = {0, 127};
    z->initialize();
    g->addZone(z);

    auto &ms = g->modulatorStorage[0];
    ms.modulatorShape = MS::LFO_ENV;
    ms.triggerMode = MS::KEYTRIGGER;
    ms.envLfoStorage.loop = loop;
    ms.envLfoStorage.delay = 0.f;
    ms.envLfoStorage.attack = 0.1f;
    ms.envLfoStorage.hold = 0.f;
    ms.envLfoStorage.decay = 0.1f;
    ms.envLfoStorage.sustain = 1.f;
    ms.envLfoStorage.release = 0.1f;

    // attack() binds the endpoints, sets lfoEvaluator[0]=ENV and starts the env LFO
    g->warmup();
    g->attack();
    return g;
}

// How many times the env output climbs from near-zero to near-peak.
int countEnvPeaks(scxt::engine::Engine *e, scxt::engine::Group *g, int maxBlocks)
{
    int peaks = 0;
    bool armed = true; // ready to count the next rise
    for (int b = 0; b < maxBlocks; ++b)
    {
        g->processLFOBlock(0, g->modulatorStorage[0], /*gate=*/true, e->transport, e->rng,
                           g->endpoints.lfo[0]);
        auto o = g->envLfos[0].output;
        if (armed && o > 0.9f)
        {
            peaks++;
            armed = false;
        }
        if (o < 0.1f)
            armed = true;
    }
    return peaks;
}
} // namespace

TEST_CASE("Group looping envelope LFO cycles while held")
{
    auto *e = new scxt::engine::Engine();
    auto *g = setupEnvLFOGroup(e, /*loop=*/true);

    // ~6.7s of a 0.1/0.1/0.1 env is many cycles; require it came back at least twice
    REQUIRE(countEnvPeaks(e, g, 20000) >= 2);

    delete e;
}

TEST_CASE("Group non-looping envelope LFO rises once and holds while held")
{
    auto *e = new scxt::engine::Engine();
    auto *g = setupEnvLFOGroup(e, /*loop=*/false);

    // Held gated with sustain 1.0, it climbs to the peak once and stays there
    REQUIRE(countEnvPeaks(e, g, 20000) == 1);

    delete e;
}
