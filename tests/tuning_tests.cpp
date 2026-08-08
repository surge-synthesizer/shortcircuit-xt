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

#include <cmath>

#include "engine/engine.h"
#include "tuning/midikey_retuner.h"
#include "tuning/scl_kbm.h"
#include "json/engine_traits.h"
#include "json/stream.h"
#include "messaging/client/enginestatus_messages.h"

#include "Tunings.h"

#include "console_harness.h"
#include "test_utils.h"

/*
 * SCL/KBM tuning. The retuner contract is "semitones to offset a 12-TET key by", so
 * every expectation here is expressed that way rather than in Hz.
 */

using RetuneTable = scxt::tuning::MidikeyRetuner::RetuneTable;

static float offsetAt(const RetuneTable &t, int midiNote)
{
    return t.semitones[midiNote + RetuneTable::midiOffset];
}

TEST_CASE("SCL/KBM - twelve tone equal temperament is a no-op", "[tuning]")
{
    RetuneTable table;
    std::string err;
    REQUIRE(scxt::tuning::buildRetuneTable(scxt::tuning::twelveTETSclText(), "", table, err));
    REQUIRE(err.empty());

    for (int k = 0; k < 128; ++k)
        REQUIRE(offsetAt(table, k) == Approx(0.f).margin(1e-4));

    REQUIRE(table.repetitionInterval == 12);
}

TEST_CASE("SCL/KBM - 19 EDO steps are 12/19 of a semitone", "[tuning]")
{
    RetuneTable table;
    std::string err;
    REQUIRE(scxt::tuning::buildRetuneTable(Tunings::evenDivisionOfSpanByM(2, 19).rawText, "", table,
                                           err));

    // The default mapping pins the scale to midi 60, so 60 is untouched
    REQUIRE(offsetAt(table, 60) == Approx(0.f).margin(1e-4));

    // and each key above it moves by one 19-EDO step rather than one semitone
    for (int i = 1; i < 12; ++i)
        REQUIRE(offsetAt(table, 60 + i) == Approx(i * 12.0 / 19.0 - i).margin(1e-4));

    REQUIRE(table.repetitionInterval == 19);
}

TEST_CASE("SCL/KBM - a KBM reference frequency shifts the whole keyboard", "[tuning]")
{
    RetuneTable table;
    std::string err;
    REQUIRE(scxt::tuning::buildRetuneTable(scxt::tuning::twelveTETSclText(),
                                           Tunings::tuneA69To(432.0).rawText, table, err));

    auto expected = 12.0 * std::log2(432.0 / 440.0);
    for (auto k : {24, 60, 69, 100})
        REQUIRE(offsetAt(table, k) == Approx(expected).margin(1e-4));

    // A KBM map size of zero is a linear mapping, so the keyboard still repeats on the scale
    REQUIRE(table.repetitionInterval == 12);
}

TEST_CASE("SCL/KBM - an explicit KBM size sets the repetition interval", "[tuning]")
{
    // Seven keys per repeat over a seven note scale, with one key deliberately unmapped.
    // Note the KBM grammar rejects trailing comments on value lines and spells an
    // unmapped key 'x'.
    std::string kbm = R"KBM(! Seven key mapping with a gap
7
0
127
60
60
261.6255653
7
0
1
2
x
3
4
5
)KBM";

    RetuneTable table;
    std::string err;
    REQUIRE(scxt::tuning::buildRetuneTable(Tunings::evenDivisionOfSpanByM(2, 7).rawText, kbm, table,
                                           err));

    REQUIRE(table.repetitionInterval == 7);

    // withSkippedNotesInterpolated means the unmapped key must still hold a sane
    // value; without it the retuner would smear nonsense across its neighbours.
    for (int k = 0; k < 128; ++k)
    {
        auto o = offsetAt(table, k);
        REQUIRE(std::isfinite(o));
        REQUIRE(std::fabs(o) < 128.f);
    }
}

TEST_CASE("SCL/KBM - a bad scale reports and changes nothing", "[tuning]")
{
    RetuneTable table;
    std::string err;
    REQUIRE(scxt::tuning::buildRetuneTable(Tunings::evenDivisionOfSpanByM(2, 6).rawText, "", table,
                                           err));
    auto before = table;

    REQUIRE(!scxt::tuning::buildRetuneTable("this is definitely not a scale", "", table, err));
    REQUIRE(!err.empty());
    REQUIRE(table.semitones == before.semitones);
    REQUIRE(table.repetitionInterval == before.repetitionInterval);

    // An empty scale is an error too, not a silent 12-TET
    err.clear();
    REQUIRE(!scxt::tuning::buildRetuneTable("", "", table, err));
    REQUIRE(!err.empty());
}

TEST_CASE("SCL/KBM - the retuner clamps out of range keys", "[tuning]")
{
    RetuneTable table;
    std::string err;
    REQUIRE(scxt::tuning::buildRetuneTable(Tunings::evenDivisionOfSpanByM(2, 6).rawText, "", table,
                                           err));

    scxt::tuning::MidikeyRetuner retuner;
    retuner.setSCLKBMTable(table);
    retuner.setTuningMode(scxt::tuning::MidikeyRetuner::SCL_KBM);

    // retuningForRemappedKeyWithInterpolation reaches for ik+1 with pitch bend applied,
    // so the table has to answer well outside 0..127
    for (auto k : {-4000, -300, -1, 0, 127, 128, 400, 4000})
        REQUIRE(std::isfinite(retuner.offsetKeyBy(0, k)));

    REQUIRE(retuner.getRepetitionInterval() == 6);
}

TEST_CASE("SCL/KBM - with no table loaded the retuner is transparent", "[tuning]")
{
    scxt::tuning::MidikeyRetuner retuner;
    retuner.setTuningMode(scxt::tuning::MidikeyRetuner::SCL_KBM);

    REQUIRE(!retuner.hasSCLKBM());
    REQUIRE(retuner.offsetKeyBy(0, 60) == Approx(0.f));
    REQUIRE(retuner.getRepetitionInterval() == 12);
}

TEST_CASE("SCL/KBM - a stretched scale remaps the key used for zone lookup", "[tuning]")
{
    // 6-EDO: every scale step is two semitones, so the sixth key above middle C
    // lands a full octave up.
    RetuneTable table;
    std::string err;
    REQUIRE(scxt::tuning::buildRetuneTable(Tunings::evenDivisionOfSpanByM(2, 6).rawText, "", table,
                                           err));

    scxt::tuning::MidikeyRetuner retuner;
    retuner.setSCLKBMTable(table);
    retuner.setTuningMode(scxt::tuning::MidikeyRetuner::SCL_KBM);

    REQUIRE(retuner.remapKeyTo(0, 60) == 60);
    REQUIRE(retuner.remapKeyTo(0, 61) == 62);
    REQUIRE(retuner.remapKeyTo(0, 66) == 72);

    // Clearing drops us straight back to an identity map
    retuner.clearSCLKBM();
    REQUIRE(retuner.remapKeyTo(0, 66) == 66);
}

TEST_CASE("SCL/KBM - the mode demotes to 12-TET with no scale behind it", "[tuning]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());

    eng->runtimeConfig.tuningMode = scxt::engine::Engine::TuningMode::SCL_KBM;
    eng->resetTuningFromRuntimeConfig();

    REQUIRE(eng->runtimeConfig.tuningMode == scxt::engine::Engine::TuningMode::TWELVE_TET);
    REQUIRE(eng->midikeyRetuner.tuningMode == scxt::tuning::MidikeyRetuner::TWELVE_TET);
}

TEST_CASE("SCL/KBM - a loaded scale keeps the mode", "[tuning]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto bg = eng->getMessageController()->threadingChecker.bypassChecksInScope();

    eng->dawExtraState.sclContents = Tunings::evenDivisionOfSpanByM(2, 6).rawText;
    std::string err;
    REQUIRE(eng->applySclKbmFromDawExtraState(err));

    eng->runtimeConfig.tuningMode = scxt::engine::Engine::TuningMode::SCL_KBM;
    eng->resetTuningFromRuntimeConfig();

    REQUIRE(eng->runtimeConfig.tuningMode == scxt::engine::Engine::TuningMode::SCL_KBM);
    REQUIRE(eng->midikeyRetuner.remapKeyTo(0, 66) == 72);

    // Clearing the scale text takes the mode with it
    eng->dawExtraState.sclContents = "";
    REQUIRE(eng->applySclKbmFromDawExtraState(err));
    eng->resetTuningFromRuntimeConfig();
    REQUIRE(eng->runtimeConfig.tuningMode == scxt::engine::Engine::TuningMode::TWELVE_TET);
}

TEST_CASE("SCL/KBM - the scale round-trips through daw extra state", "[tuning]")
{
    auto scl = Tunings::evenDivisionOfSpanByM(2, 6).rawText;
    auto kbm = Tunings::tuneA69To(432.0).rawText;

    std::string saved;
    {
        std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
        auto bg = eng->getMessageController()->threadingChecker.bypassChecksInScope();

        eng->dawExtraState.sclContents = scl;
        eng->dawExtraState.kbmContents = kbm;
        std::string err;
        REQUIRE(eng->applySclKbmFromDawExtraState(err));
        eng->runtimeConfig.tuningMode = scxt::engine::Engine::TuningMode::SCL_KBM;
        eng->resetTuningFromRuntimeConfig();

        auto sg = scxt::engine::Engine::StreamGuard(scxt::engine::Engine::FOR_DAW);
        saved = scxt::json::streamEngineState(*eng);
    }

    std::unique_ptr<scxt::engine::Engine> reloaded(makeEngine());
    {
        auto bg = reloaded->getMessageController()->threadingChecker.bypassChecksInScope();
        scxt::json::unstreamEngineState(*reloaded, saved);
    }

    REQUIRE(reloaded->dawExtraState.sclContents == scl);
    REQUIRE(reloaded->dawExtraState.kbmContents == kbm);
    REQUIRE(reloaded->runtimeConfig.tuningMode == scxt::engine::Engine::TuningMode::SCL_KBM);
    REQUIRE(reloaded->midikeyRetuner.hasSCLKBM());

    // and it retunes identically to a table built straight from the same text
    RetuneTable expected;
    std::string err;
    REQUIRE(scxt::tuning::buildRetuneTable(scl, kbm, expected, err));
    for (int k = 0; k < 128; ++k)
        REQUIRE(reloaded->midikeyRetuner.offsetKeyBy(0, k) == Approx(offsetAt(expected, k)));
}

TEST_CASE("SCL/KBM - a multi without the scale falls back rather than sounding untuned", "[tuning]")
{
    // The mode streams with a multi but the scale does not, so a multi saved in
    // SCL/KBM has to land on 12-TET rather than in a mode with nothing behind it.
    std::string saved;
    {
        std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
        auto bg = eng->getMessageController()->threadingChecker.bypassChecksInScope();

        eng->dawExtraState.sclContents = Tunings::evenDivisionOfSpanByM(2, 6).rawText;
        std::string err;
        REQUIRE(eng->applySclKbmFromDawExtraState(err));
        eng->runtimeConfig.tuningMode = scxt::engine::Engine::TuningMode::SCL_KBM;
        eng->resetTuningFromRuntimeConfig();

        auto sg = scxt::engine::Engine::StreamGuard(scxt::engine::Engine::FOR_MULTI);
        saved = scxt::json::streamEngineState(*eng);
    }

    std::unique_ptr<scxt::engine::Engine> reloaded(makeEngine());
    {
        auto bg = reloaded->getMessageController()->threadingChecker.bypassChecksInScope();
        scxt::json::unstreamEngineState(*reloaded, saved);
    }

    REQUIRE(reloaded->dawExtraState.sclContents.empty());
    REQUIRE(!reloaded->midikeyRetuner.hasSCLKBM());
    REQUIRE(reloaded->runtimeConfig.tuningMode == scxt::engine::Engine::TuningMode::TWELVE_TET);
}

TEST_CASE("SCL/KBM - SetSclKbm applies through the real message pipe", "[tuning]")
{
    // The engine-level tests poke the retuner directly; this one goes over the wire so
    // the payload serialization and the audio-thread handoff are covered too.
    scxt::clients::console_ui::ConsoleHarness th;
    REQUIRE(th.start());
    REQUIRE(th.engine);

    auto scl = Tunings::evenDivisionOfSpanByM(2, 6).rawText;
    th.sendToSerialization(scxt::messaging::client::SetSclKbm({scl, ""}));
    th.stepUI(20);

    REQUIRE(th.engine->dawExtraState.sclContents == scl);
    REQUIRE(th.engine->midikeyRetuner.hasSCLKBM());
    REQUIRE(th.engine->runtimeConfig.tuningMode == scxt::engine::Engine::TuningMode::SCL_KBM);
    REQUIRE(th.engine->midikeyRetuner.remapKeyTo(0, 66) == 72);

    // A scale which does not parse leaves the live tuning exactly as it was
    th.sendToSerialization(scxt::messaging::client::SetSclKbm({"not a scale at all", ""}));
    th.stepUI(20);

    REQUIRE(th.engine->dawExtraState.sclContents == scl);
    REQUIRE(th.engine->runtimeConfig.tuningMode == scxt::engine::Engine::TuningMode::SCL_KBM);
    REQUIRE(th.engine->midikeyRetuner.remapKeyTo(0, 66) == 72);

    // and clearing it takes the mode back to 12-TET
    th.sendToSerialization(scxt::messaging::client::SetSclKbm({"", ""}));
    th.stepUI(20);

    REQUIRE(th.engine->dawExtraState.sclContents.empty());
    REQUIRE(!th.engine->midikeyRetuner.hasSCLKBM());
    REQUIRE(th.engine->runtimeConfig.tuningMode == scxt::engine::Engine::TuningMode::TWELVE_TET);
    REQUIRE(th.engine->midikeyRetuner.remapKeyTo(0, 66) == 66);
}

TEST_CASE("SCL/KBM - a mapping with no scale means 12-TET, not no tuning", "[tuning]")
{
    // "Put A4 at 432" needs a KBM but no custom scale, so the scale gets filled in
    scxt::clients::console_ui::ConsoleHarness th;
    REQUIRE(th.start());
    REQUIRE(th.engine);

    th.sendToSerialization(
        scxt::messaging::client::SetSclKbm({"", Tunings::tuneA69To(432.0).rawText}));
    th.stepUI(20);

    REQUIRE(th.engine->dawExtraState.sclContents == scxt::tuning::twelveTETSclText());
    REQUIRE(th.engine->midikeyRetuner.hasSCLKBM());
    REQUIRE(th.engine->runtimeConfig.tuningMode == scxt::engine::Engine::TuningMode::SCL_KBM);

    auto expected = 12.0 * std::log2(432.0 / 440.0);
    REQUIRE(th.engine->midikeyRetuner.offsetKeyBy(0, 69) == Approx(expected).margin(1e-4));
}

TEST_CASE("SCL/KBM - the editor's seed mapping never moves the pitch", "[tuning]")
{
    /*
     * The editor fills an empty KBM box so there is something to edit. Applying an
     * untouched seed must be inaudible against ANY scale, or opening the editor and
     * pressing Apply becomes destructive. A 69-at-440 seed would fail this for
     * anything but 12-TET.
     */
    auto seed = scxt::tuning::defaultKbmText();

    for (const auto &scl :
         {scxt::tuning::twelveTETSclText(), Tunings::evenDivisionOfSpanByM(2, 19).rawText,
          Tunings::evenDivisionOfSpanByM(3, 13).rawText})
    {
        RetuneTable seeded, bare;
        std::string err;
        REQUIRE(scxt::tuning::buildRetuneTable(scl, seed, seeded, err));
        REQUIRE(scxt::tuning::buildRetuneTable(scl, "", bare, err));
        for (int k = 0; k < 128; ++k)
            REQUIRE(offsetAt(seeded, k) == Approx(offsetAt(bare, k)).margin(1e-4));
    }
}

TEST_CASE("SCL/KBM - resetting to no scale clears the mapping too", "[tuning]")
{
    // "Reset to No SCL/KBM" has to leave the engine as if the session never had one
    scxt::clients::console_ui::ConsoleHarness th;
    REQUIRE(th.start());
    REQUIRE(th.engine);

    th.sendToSerialization(scxt::messaging::client::SetSclKbm(
        {Tunings::evenDivisionOfSpanByM(2, 6).rawText, Tunings::tuneA69To(432.0).rawText}));
    th.stepUI(20);
    REQUIRE(!th.engine->dawExtraState.sclContents.empty());
    REQUIRE(!th.engine->dawExtraState.kbmContents.empty());

    th.sendToSerialization(scxt::messaging::client::SetSclKbm({"", ""}));
    th.stepUI(20);

    REQUIRE(th.engine->dawExtraState.sclContents.empty());
    REQUIRE(th.engine->dawExtraState.kbmContents.empty());
    REQUIRE(!th.engine->midikeyRetuner.hasSCLKBM());
    REQUIRE(th.engine->runtimeConfig.tuningMode == scxt::engine::Engine::TuningMode::TWELVE_TET);
    REQUIRE(th.engine->midikeyRetuner.offsetKeyBy(0, 60) == Approx(0.f));
}

TEST_CASE("SCL/KBM - the tuning mode round-trips as a string", "[tuning]")
{
    auto s = scxt::engine::Engine::toStringTuningMode(scxt::engine::Engine::TuningMode::SCL_KBM);
    REQUIRE(s != "err");
    REQUIRE(scxt::engine::Engine::fromStringTuningMode(s) ==
            scxt::engine::Engine::TuningMode::SCL_KBM);
}
