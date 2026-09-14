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

#include "console_harness.h"
#include "dsp/processor/processor.h"
#include "dsp/processor/processor_impl.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "selection/selection_manager.h"
#include "sst/voice-effects/generator/SinePlus.h"
#include "sst/voice-effects/generator/TiltNoise.h"
#include "sst/voice-effects/dynamics/Compressor.h"

/*
 * An int param can change the range and even the units of a processor's float params. After
 * the int lands every float must sit inside its new range, and a processor which knows how to
 * carry a value across the change (sine plus: harmonic number <-> semitones) gets to first.
 */

namespace
{
namespace cmsg = scxt::messaging::client;
using PS = scxt::dsp::processor::ProcessorStorage;
using SinePlus = sst::voice_effects::generator::SinePlus<scxt::dsp::processor::SCXTVFXConfig<1>>;
using TiltNoise = sst::voice_effects::generator::TiltNoise<scxt::dsp::processor::SCXTVFXConfig<1>>;
using Compressor = sst::voice_effects::dynamics::Compressor<scxt::dsp::processor::SCXTVFXConfig<1>>;

ptrdiff_t floatAt(int i) { return offsetof(PS, floatParams) + i * sizeof(float); }
ptrdiff_t intAt(int i) { return offsetof(PS, intParams) + i * sizeof(int32_t); }

float semitonesOfHarmonic(float h) { return 12.f * std::log2(h); }

struct IntChangeFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    IntChangeFixture()
    {
        th.start();
        th.stepUI();
    }

    template <typename T> void send(const T &msg)
    {
        th.sendToSerialization(msg);
        th.stepUI(10);
    }

    scxt::engine::Zone &zone(int z)
    {
        return *th.engine->getPatch()->getPart(0)->getGroup(0)->getZone(z);
    }
    PS &storage(int z) { return zone(z).processorStorage[0]; }
    const auto &floatDescription(int z, int idx)
    {
        return zone(z).processorDescription[0].floatControlDescriptions[idx];
    }

    void selectOnly(int z) { send(cmsg::ApplySelectActions({{0, 0, z, true, true, true}})); }
    void setTypeOnSelection(scxt::dsp::processor::ProcessorType t)
    {
        send(cmsg::SetSelectedProcessorType({true, (int32_t)0, (int32_t)t}));
    }
    void setSinePlusOnSelection() { setTypeOnSelection(scxt::dsp::processor::proct_osc_sineplus); }

    void setFloat(int idx, float v)
    {
        send(cmsg::UpdateZoneOrGroupProcessorFloatValue({true, 0, floatAt(idx), v}));
    }
    void setInt(int idx, int32_t v)
    {
        send(cmsg::UpdateZoneOrGroupProcessorInt32TValue({true, 0, intAt(idx), v}));
    }
    void setKeytrack(bool v)
    {
        send(cmsg::UpdateZoneOrGroupProcessorBoolValue({true, 0, offsetof(PS, isKeytracked), v}));
    }
};
} // namespace

TEST_CASE("Turning harmonic quantize off keeps the overtone pitch", "[processor]")
{
    IntChangeFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.selectOnly(0);
    f.setSinePlusOnSelection();

    REQUIRE(f.storage(0).intParams[SinePlus::ipQuantA] == 1);
    f.setFloat(SinePlus::fpOffsetA, 3.f);

    // restating the value it already has is not a change
    f.setInt(SinePlus::ipQuantA, 1);
    REQUIRE(f.storage(0).floatParams[SinePlus::fpOffsetA] == Approx(3.f));

    f.setInt(SinePlus::ipQuantA, 0);
    REQUIRE(f.storage(0).floatParams[SinePlus::fpOffsetA] == Approx(semitonesOfHarmonic(3)));
    REQUIRE(f.floatDescription(0, SinePlus::fpOffsetA).minVal == Approx(12.f));

    f.setInt(SinePlus::ipQuantA, 1);
    REQUIRE(f.storage(0).floatParams[SinePlus::fpOffsetA] == Approx(3.f));
    REQUIRE(f.floatDescription(0, SinePlus::fpOffsetA).maxVal == Approx(24.f));

    // B is independent of A
    REQUIRE(f.storage(0).floatParams[SinePlus::fpOffsetB] == Approx(12.f));
}

TEST_CASE("Turning harmonic quantize on lands on the nearest harmonic", "[processor]")
{
    IntChangeFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.selectOnly(0);
    f.setSinePlusOnSelection();
    f.setInt(SinePlus::ipQuantB, 0);

    auto quantizeFrom = [&f](float semis) {
        f.setInt(SinePlus::ipQuantB, 0);
        f.setFloat(SinePlus::fpOffsetB, semis);
        f.setInt(SinePlus::ipQuantB, 1);
        return f.storage(0).floatParams[SinePlus::fpOffsetB];
    };

    REQUIRE(quantizeFrom(19.5f) == Approx(3.f));
    REQUIRE(quantizeFrom(12.f) == Approx(2.f));
    REQUIRE(quantizeFrom(55.02f) == Approx(24.f));
}

TEST_CASE("A multi selection only remaps the zones whose int changed", "[processor]")
{
    IntChangeFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 61, 72, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}, {0, 0, 1, true, false, false}}));
    f.setSinePlusOnSelection();

    // zone 1 is already unquantized, zone 0 is not, and both are the third harmonic
    f.selectOnly(1);
    f.setInt(SinePlus::ipQuantA, 0);
    f.setFloat(SinePlus::fpOffsetA, semitonesOfHarmonic(3));
    f.selectOnly(0);
    f.setFloat(SinePlus::fpOffsetA, 3.f);

    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}, {0, 0, 1, true, false, false}}));
    f.setInt(SinePlus::ipQuantA, 0);

    REQUIRE(f.storage(0).floatParams[SinePlus::fpOffsetA] == Approx(semitonesOfHarmonic(3)));
    REQUIRE(f.storage(1).floatParams[SinePlus::fpOffsetA] == Approx(semitonesOfHarmonic(3)));
}

TEST_CASE("Copying the lead processor to all carries its ranges too", "[processor]")
{
    IntChangeFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 61, 72, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}, {0, 0, 1, true, false, false}}));
    f.setSinePlusOnSelection();

    f.selectOnly(0);
    f.setInt(SinePlus::ipQuantA, 0);
    REQUIRE(f.floatDescription(1, SinePlus::fpOffsetA).maxVal == Approx(24.f));

    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}, {0, 0, 1, true, false, false}}));
    f.send(cmsg::CopyProcessorLeadToAll({true, 0}));

    REQUIRE(f.storage(1).intParams[SinePlus::ipQuantA] == 0);
    REQUIRE(f.floatDescription(1, SinePlus::fpOffsetA).minVal == Approx(12.f));
}

TEST_CASE("Undoing a quantize change restores the harmonic and its range", "[processor]")
{
    IntChangeFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.selectOnly(0);
    f.setSinePlusOnSelection();
    f.setFloat(SinePlus::fpOffsetA, 5.f);

    f.setInt(SinePlus::ipQuantA, 0);
    REQUIRE(f.storage(0).floatParams[SinePlus::fpOffsetA] == Approx(semitonesOfHarmonic(5)));

    f.send(cmsg::Undo(true));
    REQUIRE(f.storage(0).intParams[SinePlus::ipQuantA] == 1);
    REQUIRE(f.storage(0).floatParams[SinePlus::fpOffsetA] == Approx(5.f));
    REQUIRE(f.floatDescription(0, SinePlus::fpOffsetA).maxVal == Approx(24.f));

    f.send(cmsg::Redo(true));
    REQUIRE(f.storage(0).intParams[SinePlus::ipQuantA] == 0);
    REQUIRE(f.storage(0).floatParams[SinePlus::fpOffsetA] == Approx(semitonesOfHarmonic(5)));
    REQUIRE(f.floatDescription(0, SinePlus::fpOffsetA).minVal == Approx(12.f));
}

TEST_CASE("An int change clamps floats on a processor with no remap of its own", "[processor]")
{
    IntChangeFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.selectOnly(0);
    f.setTypeOnSelection(scxt::dsp::processor::proct_osc_tiltnoise);

    // tilt noise asks for a consistency check but has no remap, so it gets the plain clamp
    REQUIRE(f.floatDescription(0, TiltNoise::fpTilt).maxVal == Approx(6.f));
    auto stereo = f.storage(0).intParams[TiltNoise::ipStereo];
    f.setFloat(TiltNoise::fpTilt, 9.f);
    REQUIRE(f.storage(0).floatParams[TiltNoise::fpTilt] == Approx(9.f));

    f.setInt(TiltNoise::ipStereo, stereo ? 0 : 1);
    REQUIRE(f.storage(0).floatParams[TiltNoise::fpTilt] == Approx(6.f));
    REQUIRE(f.floatDescription(0, TiltNoise::fpStereoWidth).name.empty() == (bool)stereo);
}

TEST_CASE("An int change leaves a processor without a consistency check alone", "[processor]")
{
    IntChangeFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.selectOnly(0);
    f.setTypeOnSelection(scxt::dsp::processor::proct_Compressor);
    REQUIRE(!f.zone(0).processorDescription[0].requiresConsistencyCheck);

    // no ranges move with the detector, so no temp processor is spawned to check
    f.setFloat(Compressor::fpRatio, 1.0e6f);
    f.setInt(Compressor::ipDetector, f.storage(0).intParams[Compressor::ipDetector] ? 0 : 1);
    REQUIRE(f.storage(0).floatParams[Compressor::fpRatio] == Approx(1.0e6f));
}

TEST_CASE("Toggling keytrack clamps the base frequency into its new range", "[processor]")
{
    IntChangeFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.selectOnly(0);
    f.setSinePlusOnSelection();

    f.setKeytrack(false);
    REQUIRE(f.floatDescription(0, SinePlus::fpBaseFrequency).maxVal > 60.f);
    f.setFloat(SinePlus::fpBaseFrequency, 65.f);

    f.setKeytrack(true);
    REQUIRE(f.floatDescription(0, SinePlus::fpBaseFrequency).maxVal == Approx(48.f));
    REQUIRE(f.storage(0).floatParams[SinePlus::fpBaseFrequency] == Approx(48.f));
}

TEST_CASE("A full processor storage send lands inside its ranges", "[processor]")
{
    IntChangeFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.selectOnly(0);
    f.setSinePlusOnSelection();

    auto ps = f.storage(0);
    REQUIRE(ps.intParams[SinePlus::ipQuantA] == 1);
    ps.floatParams[SinePlus::fpOffsetA] = 40.f;
    f.send(cmsg::SendFullProcessorStorage({true, 0, ps}));

    REQUIRE(f.storage(0).floatParams[SinePlus::fpOffsetA] == Approx(24.f));
}
