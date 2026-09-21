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

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "configuration.h"
#include "dsp/data_tables.h"
#include "dsp/generator.h"
#include "dsp/resampling.h"

#include <algorithm>
#include <filesystem>
#include <memory>

#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "messaging/messaging.h"
#include "voice/voice.h"

#include "test_utils.h"

/*
 * Golden tests for the sample generator.
 *
 * The generator is a pure function of (GeneratorState, GeneratorIO), so these drive
 * it directly - no engine, no voice, no zone. See the block comment at the top of
 * dsp/generator.cpp for the memory layout contract these fixtures have to honour.
 *
 * The important property of every capture here is that it SPANS AT LEAST TWO LOOP
 * SEAMS. A golden that sits in the middle of a loop agrees with itself forever while
 * the seam behaviour rots, which is exactly the bug class this file exists to catch.
 * checkSpannedSeams() enforces it rather than trusting the constants to stay put.
 *
 * To regenerate after an intentional change:
 *     SCXT_PRINT_GOLDEN=1 ./build/tests/scxt-test "[generator][golden]"
 * and paste the printed arrays back over the expected{} initializers.
 */

namespace
{

constexpr double goldenSampleRate{48000.0};
constexpr double goldenPi{3.14159265358979323846};

// A short sample with a short loop, so seams arrive every 64 samples rather than
// every few seconds. blockSize is 16, so a 64 sample loop also puts seams at four
// different offsets within the block as the capture proceeds.
constexpr int gWaveSize{2048};
constexpr int gStartSample{0};
constexpr int gEndSample{512};
constexpr int gStartLoop{256};
constexpr int gEndLoop{320};
constexpr int gLoopLen{gEndLoop - gStartLoop}; // 64
constexpr int gFade{32};                       // 2 blocks worth, spans a block boundary

constexpr float goldenTolerance{1e-5f};

constexpr int32_t unityRatio{1 << 24};

bool goldenPrintMode() { return std::getenv("SCXT_PRINT_GOLDEN") != nullptr; }

/*
 * Two tones, switching at startLoop, so the crossfade has distinguishable material on
 * each side of the seam.
 *
 * Both frequencies are chosen so that startLoop is a whole number of cycles, which
 * makes the signal continuous at the switch: the only discontinuity anywhere in the
 * buffer is then the loop seam itself, which is what lets a continuity assertion mean
 * something. Neither period divides the 64 sample loop length (85.33 and 23.27
 * samples), so the loop is not accidentally seamless either - uncrossfaded, the wrap
 * from endLoop back to startLoop steps by most of the signal amplitude.
 */
float testSignal(int n)
{
    auto f = (n < gStartLoop) ? 562.5 : 2062.5;
    return (float)(0.7 * std::sin(2.0 * goldenPi * f * n / goldenSampleRate));
}

struct GenConfig
{
    std::string name;
    bool stereo{false};
    bool isFloat{true};
    bool loopActive{true};
    bool loopForward{true}; // FORWARD_ONLY as opposed to ALTERNATE_DIRECTIONS
    bool loopWhileGated{false};
    bool playReverse{false};
    int32_t loopFade{0};
    int32_t ratio{unityRatio};
    scxt::dsp::InterpolationTypes interp{scxt::dsp::InterpolationTypes::Sinc};
    int warmupBlocks{32};
    int recordBlocks{10};
    int ungateAtBlock{-1};    // relative to the start of the recorded region
    int startPos{-1};         // -1 means the natural start for the direction
    int blockSizeOverride{0}; // 0 means the engine block size
};

struct GenHarness
{
    std::vector<float> bufFL, bufFR;
    std::vector<int16_t> bufIL, bufIR;

    scxt::dsp::GeneratorState gs;
    scxt::dsp::GeneratorIO io;
    scxt::dsp::GeneratorFPtr fn{nullptr};

    float outL alignas(16)[scxt::blockSize * 2];
    float outR alignas(16)[scxt::blockSize * 2];

    std::vector<float> recL, recR;
    // only meaningful with blockSizeOverride == 1, where one call is one sample
    std::vector<int> recPos;

    // seam bookkeeping, filled in by run()
    int seamsInRecord{0};

    explicit GenHarness(const GenConfig &c)
    {
        // The sinc kernels read dsp::sincTable, which is otherwise only initialized by
        // the Engine constructor. Without this the Sinc paths silently produce silence.
        static bool tablesReady = []() {
            scxt::dsp::sincTable.init();
            return true;
        }();
        (void)tablesReady;

        const auto pad = scxt::dsp::FIRoffset;

        bufFL.assign(gWaveSize + scxt::dsp::FIRipol_N, 0.f);
        bufFR.assign(gWaveSize + scxt::dsp::FIRipol_N, 0.f);
        bufIL.assign(gWaveSize + scxt::dsp::FIRipol_N, 0);
        bufIR.assign(gWaveSize + scxt::dsp::FIRipol_N, 0);
        for (int i = 0; i < gWaveSize; ++i)
        {
            auto v = testSignal(i);
            bufFL[i + pad] = v;
            bufFR[i + pad] = -v; // inverted so a channel mixup is visible
            bufIL[i + pad] = (int16_t)std::lround(v * 32767.f);
            bufIR[i + pad] = (int16_t)-std::lround(v * 32767.f);
        }

        if (c.isFloat)
        {
            io.sampleDataL = (void *)(bufFL.data() + pad);
            io.sampleDataR = (void *)(bufFR.data() + pad);
        }
        else
        {
            io.sampleDataL = (void *)(bufIL.data() + pad);
            io.sampleDataR = (void *)(bufIR.data() + pad);
        }
        io.outputL = outL;
        io.outputR = outR;
        io.waveSize = gWaveSize;

        gs = scxt::dsp::GeneratorState{};
        gs.playbackLowerBound = gStartSample;
        gs.playbackUpperBound = gEndSample;
        gs.playbackInvertedBounds = 1.f / std::max(1, gEndSample - gStartSample);
        gs.loopLowerBound = c.loopActive ? gStartLoop : gStartSample;
        gs.loopUpperBound = c.loopActive ? gEndLoop : gEndSample;
        gs.loopInvertedBounds = 1.f / std::max(1, gs.loopUpperBound - gs.loopLowerBound);
        gs.loopFade = c.loopFade;
        gs.ratio = c.ratio;
        gs.blockSize = c.blockSizeOverride > 0 ? c.blockSizeOverride : scxt::blockSize;
        gs.isFinished = false;
        gs.gated = true;
        gs.loopCount = -1;
        gs.interpolationType = c.interp;
        gs.samplePos = c.playReverse ? gs.playbackUpperBound : gs.playbackLowerBound;
        if (c.startPos >= 0)
            gs.samplePos = c.startPos;
        gs.sampleSubPos = 0;
        gs.loopDirection = c.playReverse ? -1 : 1;
        gs.directionAtOutset = gs.loopDirection;
        // the generator derives this per block; seed it so a capture with no warmup
        // does not read a zero and count a spurious turnaround on its first block
        gs.travelDirection = gs.loopDirection * (c.ratio < 0 ? -1 : 1);

        fn = scxt::dsp::GetFPtrGeneratorSample(c.stereo, c.isFloat, c.loopActive, c.loopForward,
                                               c.loopWhileGated);
    }

    void run(const GenConfig &c)
    {
        for (int b = 0; b < c.warmupBlocks; ++b)
            fn(&gs, &io);

        recL.clear();
        recR.clear();
        recPos.clear();
        seamsInRecord = 0;

        // A seam is either a wrap (position jumps by much more than one block of
        // travel) or a turnaround (direction flips). Detecting it from the state
        // rather than from loopCount keeps this honest across all five loop modes.
        auto travelPerBlock = std::abs((double)c.ratio / (double)unityRatio) * gs.blockSize;
        auto prevPos = gs.samplePos;
        auto prevDir = gs.travelDirection;

        for (int b = 0; b < c.recordBlocks; ++b)
        {
            if (b == c.ungateAtBlock)
                gs.gated = false;

            const auto posBefore = gs.samplePos;
            fn(&gs, &io);
            if (gs.blockSize == 1)
                recPos.push_back(posBefore);

            for (int i = 0; i < gs.blockSize; ++i)
            {
                recL.push_back(outL[i]);
                recR.push_back(outR[i]);
            }

            if (std::abs(gs.samplePos - prevPos) > travelPerBlock + 2 ||
                gs.travelDirection != prevDir)
                seamsInRecord++;
            prevPos = gs.samplePos;
            prevDir = gs.travelDirection;
        }
    }
};

/*
 * The whole point: assert the capture actually crossed loop seams. If a later edit
 * to the loop bounds, the ratio or the record length quietly turns one of these into
 * a mid-loop capture, this fires instead of the golden silently going soft.
 */
void checkSpannedSeams(const GenConfig &c, const GenHarness &h)
{
    if (!c.loopActive)
        return;
    INFO("config " << c.name << " crossed " << h.seamsInRecord << " seams in "
                   << c.recordBlocks * scxt::blockSize << " recorded samples");
    REQUIRE(h.seamsInRecord >= 2);
}

void goldenCheckOrPrint(const std::string &label, const std::vector<float> &got,
                        const std::vector<float> &expected)
{
    if (goldenPrintMode())
    {
        std::printf("// %s\n        {", label.c_str());
        for (size_t i = 0; i < got.size(); ++i)
        {
            if (i > 0 && i % 5 == 0)
                std::printf("\n         ");
            std::printf("%.9ff%s", got[i], i + 1 < got.size() ? ", " : "");
        }
        std::printf("};\n");
        return;
    }

    REQUIRE(got.size() == expected.size());
    for (size_t i = 0; i < got.size(); ++i)
    {
        INFO(label << " sample[" << i << "]: got=" << got[i] << " expected=" << expected[i]);
        REQUIRE(got[i] == Approx(expected[i]).margin(goldenTolerance));
    }
}

// Drive a config and hand back the left channel, having asserted the seam coverage.
std::vector<float> capture(const GenConfig &c, GenHarness &h)
{
    h.run(c);
    checkSpannedSeams(c, h);
    return h.recL;
}

} // namespace

TEST_CASE("Generator harness honours the layout contract", "[generator]")
{
    // A no-loop forward pass should reproduce the sample data at unity ratio. This is
    // the sanity check that the FIRoffset padding, the read pointer setup and the
    // output indexing in the fixture all line up; if this is wrong every golden below
    // is measuring the fixture rather than the generator.
    //
    // Note the generator's indexing convention: the read pointer is
    // SampleData + samplePos - FIRoffset and the kernels take their interpolation
    // point at FIRoffset - 1, so samplePos p at zero subposition reads data[p - 1].
    // Output n therefore carries sample n - 1, and output 0 comes from the leading
    // zero pad.
    GenConfig c;
    c.name = "contract";
    c.loopActive = false;
    c.warmupBlocks = 0;
    c.recordBlocks = 8;
    c.interp = scxt::dsp::InterpolationTypes::ZeroOrderHold;

    GenHarness h(c);
    h.run(c);

    REQUIRE(h.recL.size() == 8 * scxt::blockSize);
    REQUIRE(h.recL[0] == Approx(0.f).margin(1e-9));
    for (int i = 1; i < 8 * scxt::blockSize; ++i)
    {
        INFO("sample " << i);
        REQUIRE(h.recL[i] == Approx(testSignal(i - 1)).margin(1e-6));
    }
}

TEST_CASE("Generator no-loop playback terminates at the bounds", "[generator]")
{
    SECTION("forward")
    {
        GenConfig c;
        c.name = "noloop-fwd-terminate";
        c.loopActive = false;
        c.warmupBlocks = 0;
        c.recordBlocks = 40; // 640 samples over a 512 sample playback region

        GenHarness h(c);
        h.run(c);

        REQUIRE(h.gs.isFinished);
        REQUIRE(h.gs.samplePos == gEndSample);
        // everything past the end of the region is silence
        for (int i = gEndSample + 1; i < (int)h.recL.size(); ++i)
        {
            INFO("sample " << i);
            REQUIRE(h.recL[i] == Approx(0.f).margin(1e-9));
        }
    }

    SECTION("reverse")
    {
        GenConfig c;
        c.name = "noloop-rev-terminate";
        c.loopActive = false;
        c.playReverse = true;
        c.warmupBlocks = 0;
        c.recordBlocks = 40;

        GenHarness h(c);
        h.run(c);

        REQUIRE(h.gs.isFinished);
        REQUIRE(h.gs.samplePos == gStartSample);
        for (int i = gEndSample + 1; i < (int)h.recL.size(); ++i)
        {
            INFO("sample " << i);
            REQUIRE(h.recL[i] == Approx(0.f).margin(1e-9));
        }
    }
}

/*
 * The output at a given playhead position must not depend on where the block
 * boundaries happen to fall. The loop is 64 samples and blockSize is 16, so starting
 * the voice s samples earlier moves every seam to a different offset within the
 * block while leaving the position-to-output mapping alone; the two captures then
 * have to agree once shifted back by s.
 *
 * This is the sharpest statement of the crossfade engagement bug: while fadeActive
 * could only be set at block entry, the fade started up to a block late and the
 * output depended on block phase.
 */
void checkBlockPhaseInvariance(GenConfig base)
{
    base.warmupBlocks = 32; // 512 samples: a whole number of loops either way
    base.recordBlocks = 12;

    GenHarness bh(base);
    bh.run(base);

    const int natural = base.playReverse ? gEndSample : gStartLoop;
    for (int s = 1; s < scxt::blockSize; ++s)
    {
        auto shifted = base;
        shifted.startPos = base.playReverse ? natural - s : natural + s;

        GenHarness sh(shifted);
        sh.run(shifted);

        for (size_t i = 0; i + s < bh.recL.size(); ++i)
        {
            INFO(base.name << " shift " << s << " sample " << i);
            REQUIRE(sh.recL[i] == Approx(bh.recL[i + s]).margin(1e-5));
        }
    }
}

TEST_CASE("Generator output does not depend on block phase", "[generator]")
{
    SECTION("forward loop with fade")
    {
        GenConfig c;
        c.name = "phase-fwd-fade";
        c.loopFade = gFade;
        checkBlockPhaseInvariance(c);
    }

    SECTION("reverse loop with fade")
    {
        GenConfig c;
        c.name = "phase-rev-fade";
        c.playReverse = true;
        c.loopFade = gFade;
        checkBlockPhaseInvariance(c);
    }

    SECTION("alternate loop with fade")
    {
        GenConfig c;
        c.name = "phase-alt-fade";
        c.loopForward = false;
        c.loopFade = gFade;
        checkBlockPhaseInvariance(c);
    }

    SECTION("alternate loop with fade, started in reverse")
    {
        GenConfig c;
        c.name = "phase-alt-rev-fade";
        c.loopForward = false;
        c.playReverse = true;
        c.loopFade = gFade;
        checkBlockPhaseInvariance(c);
    }
}

TEST_CASE("Generator crossfade keeps the loop seam continuous", "[generator]")
{
    /*
     * The point of issue #2149. The test signal is a 1900Hz sine at 0.7, so the
     * steepest legitimate step between adjacent output samples is
     * 0.7 * 2pi * 1900 / 48000 = 0.174; a crossfade blends two such streams and adds
     * the gain ramp, so allow a generous multiple of that. A seam that is not being
     * smoothed jumps by order the full signal amplitude and blows straight through.
     */
    constexpr float maxStep{0.35f};

    auto worstStep = [](const std::vector<float> &v) {
        float w = 0.f;
        for (size_t i = 1; i < v.size(); ++i)
            w = std::max(w, std::abs(v[i] - v[i - 1]));
        return w;
    };

    SECTION("forward")
    {
        GenConfig c;
        c.name = "continuity-fwd";
        c.loopFade = gFade;
        c.recordBlocks = 24;

        GenHarness h(c);
        auto got = capture(c, h);
        INFO("worst step " << worstStep(got));
        REQUIRE(worstStep(got) < maxStep);
    }

    SECTION("reverse")
    {
        GenConfig c;
        c.name = "continuity-rev";
        c.playReverse = true;
        c.loopFade = gFade;
        c.recordBlocks = 24;

        GenHarness h(c);
        auto got = capture(c, h);
        INFO("worst step " << worstStep(got));
        REQUIRE(worstStep(got) < maxStep);
    }

    /*
     * The alternate window is entered twice per turnaround, once on the way in and once
     * on the way out, and the playhead and its reflection change places in between. Gain
     * that follows which side of the bound we are on therefore leaves the mix sitting on
     * the reflection as the window ends - a whole fade away from where playback carries
     * on - and steps back to the playhead. gFade is 32 against a 64 sample loop, so the
     * window edges at +/-16 from each bound fall inside the capture with room either
     * side, which is where that step used to land.
     */
    auto alternate = GENERATE(false, true);
    DYNAMIC_SECTION("alternate, reverse=" << alternate)
    {
        GenConfig c;
        c.name = "continuity-alt";
        c.loopForward = false;
        c.playReverse = alternate;
        c.loopFade = gFade;
        c.recordBlocks = 24;

        GenHarness h(c);
        auto got = capture(c, h);
        INFO("worst step " << worstStep(got));
        REQUIRE(worstStep(got) < maxStep);
    }
}

TEST_CASE("Generator alternate loop crossfade mirrors about the turnaround", "[generator]")
{
    /*
     * A ping-pong turn has no discontinuity to hide - the playhead reverses, so the
     * value is continuous - but the waveform audibly mirrors. The crossfade blends the
     * playhead against its reflection in the turn, which sits half a fade below the
     * bound so that the window is the loopFade samples running up to that bound.
     *
     * Three things follow, and all are checked here. Exactly at the turn the playhead
     * and its reflection are the same sample, so the crossfade must be a no-op there
     * however its gain is shaped. Away from the turn but inside the window it has to
     * actually change the output, or it is not running at all. And at the far edge of
     * the window it has to be a no-op again, because a sample later the window is over
     * and the fade is simply off - anything left there is a step.
     *
     * An odd fade is run as well as an even one: the window is +/- loopFade/2 and that
     * division truncates, so a gain scaled by the full loopFade reaches the edge still
     * slightly open, which the even case hides exactly.
     *
     * Driven a sample at a time so the output can be lined up with the playhead.
     */
    auto render = [](bool reverse, int32_t fade, std::vector<int> *positions) {
        GenConfig c;
        c.name = "alt-mirror";
        c.loopForward = false;
        c.playReverse = reverse;
        c.loopFade = fade;
        c.blockSizeOverride = 1;
        c.warmupBlocks = 512;
        c.recordBlocks = 512;

        GenHarness h(c);
        h.run(c);
        if (positions)
            *positions = h.recPos;
        return h.recL;
    };

    for (bool reverse : {false, true})
    {
        for (int32_t fade : {gFade, 21})
        {
            DYNAMIC_SECTION("reverse=" << reverse << " fade=" << fade)
            {
                std::vector<int> pos, plainPos;
                auto faded = render(reverse, fade, &pos);
                auto plain = render(reverse, 0, &plainPos);

                REQUIRE(faded.size() == plain.size());
                REQUIRE(pos.size() == faded.size());

                // keyed by position, not time: the fade moves the turn, so the two runs
                // no longer walk the same trajectory. At unity ratio every position is
                // read at subPos 0, so the unfaded run gives one value per position.
                std::map<int, float> plainAt;
                for (size_t i = 0; i < plain.size(); ++i)
                    plainAt[plainPos[i]] = plain[i];

                int atBound{0}, atEdge{0}, changedInWindow{0};
                const int half = fade / 2;
                for (size_t i = 0; i < faded.size(); ++i)
                {
                    const auto fromLo = std::abs(pos[i] - (gStartLoop - half));
                    const auto fromHi = std::abs(pos[i] - (gEndLoop - half));
                    const auto fromTurn = std::min(fromLo, fromHi);

                    // the low turn's window reaches below startLoop, where the unfaded
                    // run never goes, so there is nothing to compare against there
                    auto pa = plainAt.find(pos[i]);
                    if (pa == plainAt.end())
                        continue;
                    const auto plainHere = pa->second;

                    if (fromTurn == 0)
                    {
                        INFO("turn sample " << i << " at position " << pos[i]);
                        REQUIRE(faded[i] == Approx(plainHere).margin(1e-6));
                        atBound++;
                    }
                    else if (fromTurn == half)
                    {
                        INFO("window edge sample " << i << " at position " << pos[i]);
                        REQUIRE(faded[i] == Approx(plainHere).margin(1e-6));
                        atEdge++;
                    }
                    else if (fromTurn < half && std::abs(faded[i] - plainHere) > 1e-4)
                    {
                        changedInWindow++;
                    }
                }

                INFO("turn " << atBound << ", edge " << atEdge << ", changed " << changedInWindow);
                REQUIRE(atBound >= 2);         // the capture really did turn around
                REQUIRE(atEdge >= 2);          // and really did leave the window
                REQUIRE(changedInWindow > 60); // and the crossfade really is doing something
            }
        }
    }
}

TEST_CASE("Generator alternate loop turns half a fade inside the loop", "[generator]")
{
    /*
     * The fade fills the loopFade samples up to each marker rather than straddling it, so
     * the playhead and its reflection mirror through that window and the turn lands half
     * a fade inside the loop.
     *
     * Both HALion reference sets agree: turnarounds at 66500 and 199500 against bounds of
     * 133000 and 266000, and the two legs crossing at 132500 against a loop end of 165000.
     */
    auto turns = [](int32_t fade) {
        GenConfig c;
        c.name = "alt-turn";
        c.loopForward = false;
        c.loopFade = fade;
        c.blockSizeOverride = 1;
        c.warmupBlocks = 512;
        c.recordBlocks = 512;

        GenHarness h(c);
        h.run(c);
        auto [lo, hi] = std::minmax_element(h.recPos.begin(), h.recPos.end());
        return std::make_pair(*lo, *hi);
    };

    SECTION("no fade turns at the bounds")
    {
        auto [lo, hi] = turns(0);
        REQUIRE(lo == gStartLoop);
        REQUIRE(hi == gEndLoop);
    }

    for (int32_t fade : {gFade, 21})
    {
        DYNAMIC_SECTION("fade=" << fade)
        {
            const int half = fade / 2;
            auto [lo, hi] = turns(fade);
            REQUIRE(lo == gStartLoop - half);
            REQUIRE(hi == gEndLoop - half);
        }
    }
}

TEST_CASE("Generator reports looping only once inside the loop", "[generator]")
{
    /*
     * isInLoop and loopCount feed the Is Looping and Loop Count modulation sources and
     * the loop-count play mode. A reverse voice starts at playbackUpperBound, above the
     * loop, so testing only against the lower bound made it "in loop" on its first
     * block and bumped loopCount from -1 to 0 before any loop had been entered.
     */
    SECTION("reverse voice is not looping before it reaches the loop")
    {
        GenConfig c;
        c.name = "inloop-rev";
        c.playReverse = true;
        c.warmupBlocks = 0;
        c.recordBlocks = 1; // one block: still far above endLoop

        GenHarness h(c);
        h.run(c);

        REQUIRE(h.gs.samplePos > gEndLoop);
        REQUIRE_FALSE(h.gs.isInLoop);
        REQUIRE(h.gs.loopCount == -1);
    }

    SECTION("reverse voice is looping once it is inside")
    {
        GenConfig c;
        c.name = "inloop-rev-inside";
        c.playReverse = true;
        c.warmupBlocks = 32;
        c.recordBlocks = 1;

        GenHarness h(c);
        h.run(c);

        REQUIRE(h.gs.samplePos <= gEndLoop);
        REQUIRE(h.gs.isInLoop);
        REQUIRE(h.gs.loopCount >= 0);
    }

    SECTION("forward voice is not looping before it reaches the loop")
    {
        GenConfig c;
        c.name = "inloop-fwd";
        c.warmupBlocks = 0;
        c.recordBlocks = 1; // starts at startSample, well below startLoop

        GenHarness h(c);
        h.run(c);

        REQUIRE(h.gs.samplePos < gStartLoop);
        REQUIRE_FALSE(h.gs.isInLoop);
        REQUIRE(h.gs.loopCount == -1);
    }
}

TEST_CASE("Generator terminates under a negative playback ratio", "[generator]")
{
    /*
     * A negative ratio means the playhead travels opposite to loopDirection. The
     * end-of-playback tests have to ask which way we are actually moving; asking
     * loopDirection instead clamps the position at the bound and never finishes, so
     * the voice hangs. Both sections run to a bound loopDirection alone would not
     * predict.
     */
    SECTION("forward loop direction, negative ratio, travelling down")
    {
        GenConfig c;
        c.name = "negratio-down";
        c.loopActive = false;
        c.ratio = -unityRatio;
        c.startPos = 256;
        c.warmupBlocks = 0;
        c.recordBlocks = 24; // 384 samples for a 256 sample descent

        GenHarness h(c);
        h.run(c);

        REQUIRE(h.gs.travelDirection == -1);
        REQUIRE(h.gs.loopDirection == 1); // unchanged: the ratio sign is not the loop's business
        REQUIRE(h.gs.samplePos == gStartSample);
        REQUIRE(h.gs.isFinished);
    }

    SECTION("reverse loop direction, negative ratio, travelling up")
    {
        GenConfig c;
        c.name = "negratio-up";
        c.loopActive = false;
        c.playReverse = true;
        c.ratio = -unityRatio;
        c.startPos = 256;
        c.warmupBlocks = 0;
        c.recordBlocks = 24;

        GenHarness h(c);
        h.run(c);

        REQUIRE(h.gs.travelDirection == 1);
        REQUIRE(h.gs.loopDirection == -1);
        REQUIRE(h.gs.samplePos == gEndSample);
        REQUIRE(h.gs.isFinished);
    }
}

TEST_CASE("Generator stereo tracks the mono path per channel", "[generator]")
{
    // The right buffer is the left negated, so a correct stereo instantiation gives
    // outR == -outL exactly. Cheap way to cover the stereo kernels without doubling
    // every golden array.
    GenConfig c;
    c.name = "stereo-mirror";
    c.stereo = true;
    c.loopFade = gFade;

    GenHarness h(c);
    h.run(c);
    checkSpannedSeams(c, h);

    REQUIRE(h.recL.size() == h.recR.size());
    REQUIRE(h.recL.size() > 0);
    for (size_t i = 0; i < h.recL.size(); ++i)
    {
        INFO("sample " << i);
        REQUIRE(h.recR[i] == Approx(-h.recL[i]).margin(1e-7));
    }
}

/*
 * The goldens.
 *
 * Only configurations believed correct today are captured here. Reverse-with-fade
 * and alternate-with-fade are deliberately absent: they are broken (issue #2149) and
 * capturing them now would enshrine the bug. They get goldens once fixed.
 */

TEST_CASE("Generator golden - no loop", "[generator][golden]")
{
    struct Case
    {
        const char *name;
        bool reverse;
    };
    // 128 warmup samples, then 512 recorded over a 512 sample playback region: the
    // capture brackets the moment the playhead reaches the far bound and the voice
    // finishes, so the trailing silence is part of the golden.
    static const std::array<Case, 2> cases{{{"noloop-fwd", false}, {"noloop-rev", true}}};

    static const std::array<std::vector<float>, 2> expected{{
        // noloop-fwd
        {0.051495217f,  0.000000022f,  -0.051495172f, -0.102711312f, -0.153370857f, -0.203199238f,
         -0.251926512f, -0.299288541f, -0.345028728f, -0.388899177f, -0.430662155f, -0.470091283f,
         -0.506972969f, -0.541107297f, -0.572309315f, -0.600409985f, -0.625257015f, -0.646715701f,
         -0.664669693f, -0.679021835f, -0.689694345f, -0.696629286f, -0.699789107f, -0.699156821f,
         -0.694735646f, -0.686549783f, -0.674643219f, -0.659080923f, -0.639946878f, -0.617344856f,
         -0.591397583f, -0.562245250f, -0.530046225f, -0.494974762f, -0.457221031f, -0.416989535f,
         -0.374498367f, -0.329977721f, -0.283668905f, -0.235822916f, -0.186698973f, -0.136563241f,
         -0.085687503f, -0.034347393f, 0.017178837f,  0.068611994f,  0.119673304f,  0.170086116f,
         0.219577208f,  0.267878383f,  0.314727932f,  0.359871924f,  0.403065771f,  0.444075316f,
         0.482678413f,  0.518665791f,  0.551842511f,  0.582028687f,  0.609060824f,  0.632792532f,
         0.653094947f,  0.669858158f,  0.682991505f,  0.692423642f,  0.698103249f,  0.699999928f,
         0.698103249f,  0.692423642f,  0.682991564f,  0.669858277f,  0.653095007f,  0.632792592f,
         0.609060943f,  0.582028747f,  0.551842570f,  0.518665850f,  0.482678413f,  0.444075316f,
         0.403065801f,  0.359871954f,  0.314727962f,  0.267878443f,  0.219577238f,  0.170086160f,
         0.119673342f,  0.068612024f,  0.017178880f,  -0.034347348f, -0.085687466f, -0.136563212f,
         -0.186698914f, -0.235822871f, -0.283668905f, -0.329977721f, -0.374498338f, -0.416989505f,
         -0.457221001f, -0.494974732f, -0.530046105f, -0.562245190f, -0.591397524f, -0.617344797f,
         -0.639946878f, -0.659080803f, -0.674643159f, -0.686549723f, -0.694735527f, -0.699156821f,
         -0.699789107f, -0.696629286f, -0.689694405f, -0.679021895f, -0.664669752f, -0.646715760f,
         -0.625257015f, -0.600410044f, -0.572309375f, -0.541107357f, -0.506972969f, -0.470091313f,
         -0.430662155f, -0.388899207f, -0.345028758f, -0.299286216f, -0.251957119f, -0.203039721f,
         -0.153866366f, -0.101662315f, -0.053108696f, 0.001856980f,  0.185085431f,  0.360920966f,
         0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,
         0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f,
         -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f,
         -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,
         0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,  0.609060943f,  0.494974732f,
         0.345028788f,  0.170086160f,  -0.017178826f, -0.203199223f, -0.374498308f, -0.518665791f,
         -0.625257015f, -0.686549723f, -0.698103189f, -0.659080923f, -0.572309494f, -0.444075286f,
         -0.283668905f, -0.102711365f, 0.085687436f,  0.267878324f,  0.430662096f,  0.562245190f,
         0.653094888f,  0.696629286f,  0.689694345f,  0.632792532f,  0.530046225f,  0.388899148f,
         0.219577268f,  0.034347400f,  -0.153370842f, -0.329977691f, -0.482678384f, -0.600409985f,
         -0.674643159f, -0.699999869f, -0.674643159f, -0.600410044f, -0.482678413f, -0.329977751f,
         -0.153370902f, 0.034347337f,  0.219577193f,  0.388899118f,  0.530046165f,  0.632792532f,
         0.689694345f,  0.696629286f,  0.653094947f,  0.562245250f,  0.430662155f,  0.267878413f,
         0.085687503f,  -0.102711305f, -0.283668876f, -0.444075257f, -0.572309375f, -0.659080863f,
         -0.698103249f, -0.686549723f, -0.625257075f, -0.518665791f, -0.374498367f, -0.203199312f,
         -0.017178891f, 0.170086086f,  0.345028698f,  0.494974703f,  0.609060884f,  0.679021835f,
         0.699789107f,  0.669858217f,  0.591397583f,  0.470091283f,  0.314727932f,  0.136563256f,
         -0.051495165f, -0.235822856f, -0.403065771f, -0.541107297f, -0.639946818f, -0.692423522f,
         -0.694735646f, -0.646715701f, -0.551842570f, -0.416989535f, -0.251926571f, -0.068612039f,
         0.119673289f,  0.299288541f,  0.457220972f,  0.582028806f,  0.664669752f,  0.699156761f,
         0.682991505f,  0.617344856f,  0.506972969f,  0.359871954f,  0.186698973f,  0.000000033f,
         -0.186698884f, -0.359871894f, -0.506972969f, -0.617344737f, -0.682991505f, -0.699156821f,
         -0.664669752f, -0.582028806f, -0.457221001f, -0.299288601f, -0.119673356f, 0.068611972f,
         0.251926482f,  0.416989505f,  0.551842451f,  0.646715641f,  0.694735646f,  0.692423582f,
         0.639946938f,  0.541107416f,  0.403065801f,  0.235822931f,  0.051495228f,  -0.136563197f,
         -0.314727873f, -0.470091254f, -0.591397583f, -0.669858217f, -0.699789107f, -0.679021835f,
         -0.609060943f, -0.494974732f, -0.345028788f, -0.170086160f, 0.017178826f,  0.203199223f,
         0.374498308f,  0.518665791f,  0.625257015f,  0.686549723f,  0.698103189f,  0.659080923f,
         0.572309494f,  0.444075286f,  0.283668905f,  0.102711365f,  -0.085687436f, -0.267878324f,
         -0.430662096f, -0.562245190f, -0.653094888f, -0.696629286f, -0.689694345f, -0.632792532f,
         -0.530046225f, -0.388899148f, -0.219577268f, -0.034347400f, 0.153370842f,  0.329977691f,
         0.482678384f,  0.600409985f,  0.674643159f,  0.699999869f,  0.674643159f,  0.600410044f,
         0.482678413f,  0.329977751f,  0.153370902f,  -0.034347337f, -0.219577193f, -0.388899118f,
         -0.530046165f, -0.632792532f, -0.689694345f, -0.696629286f, -0.653094947f, -0.562245250f,
         -0.430662155f, -0.267878413f, -0.085687503f, 0.102711305f,  0.283668876f,  0.444075257f,
         0.572309375f,  0.659080863f,  0.698103249f,  0.686549723f,  0.625257075f,  0.518665791f,
         0.374498367f,  0.203199312f,  0.017178891f,  -0.170086086f, -0.345028698f, -0.494974703f,
         -0.609060884f, -0.679021835f, -0.699789107f, -0.669858217f, -0.591397583f, -0.470091283f,
         -0.314727932f, -0.136563256f, 0.051495165f,  0.235822856f,  0.403065771f,  0.541107297f,
         0.639946818f,  0.692423522f,  0.694735646f,  0.646715701f,  0.551842570f,  0.416989535f,
         0.251926571f,  0.068612039f,  -0.119673289f, -0.299288541f, -0.457220972f, -0.582028806f,
         -0.664669752f, -0.699156761f, -0.682991505f, -0.617344856f, -0.506972969f, -0.359871954f,
         -0.186698973f, 0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f},
        // noloop-rev
        {0.186698973f,  0.359871954f,  0.506972969f,  0.617344856f,  0.682991505f,  0.699156761f,
         0.664669752f,  0.582028806f,  0.457220972f,  0.299288541f,  0.119673289f,  -0.068612039f,
         -0.251926571f, -0.416989535f, -0.551842570f, -0.646715701f, -0.694735646f, -0.692423522f,
         -0.639946818f, -0.541107297f, -0.403065771f, -0.235822856f, -0.051495165f, 0.136563256f,
         0.314727932f,  0.470091283f,  0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,
         0.609060884f,  0.494974703f,  0.345028698f,  0.170086086f,  -0.017178891f, -0.203199312f,
         -0.374498367f, -0.518665791f, -0.625257075f, -0.686549723f, -0.698103249f, -0.659080863f,
         -0.572309375f, -0.444075257f, -0.283668876f, -0.102711305f, 0.085687503f,  0.267878413f,
         0.430662155f,  0.562245250f,  0.653094947f,  0.696629286f,  0.689694345f,  0.632792532f,
         0.530046165f,  0.388899118f,  0.219577193f,  0.034347337f,  -0.153370902f, -0.329977751f,
         -0.482678413f, -0.600410044f, -0.674643159f, -0.699999869f, -0.674643159f, -0.600409985f,
         -0.482678384f, -0.329977691f, -0.153370842f, 0.034347400f,  0.219577268f,  0.388899148f,
         0.530046225f,  0.632792532f,  0.689694345f,  0.696629286f,  0.653094888f,  0.562245190f,
         0.430662096f,  0.267878324f,  0.085687436f,  -0.102711365f, -0.283668905f, -0.444075286f,
         -0.572309494f, -0.659080923f, -0.698103189f, -0.686549723f, -0.625257015f, -0.518665791f,
         -0.374498308f, -0.203199223f, -0.017178826f, 0.170086160f,  0.345028788f,  0.494974732f,
         0.609060943f,  0.679021835f,  0.699789107f,  0.669858217f,  0.591397583f,  0.470091254f,
         0.314727873f,  0.136563197f,  -0.051495228f, -0.235822931f, -0.403065801f, -0.541107416f,
         -0.639946938f, -0.692423582f, -0.694735646f, -0.646715641f, -0.551842451f, -0.416989505f,
         -0.251926482f, -0.068611972f, 0.119673356f,  0.299288601f,  0.457221001f,  0.582028806f,
         0.664669752f,  0.699159205f,  0.682960927f,  0.617504358f,  0.506477535f,  0.360920966f,
         0.185085431f,  0.001856980f,  -0.053108696f, -0.101662315f, -0.153866366f, -0.203039721f,
         -0.251957119f, -0.299286216f, -0.345028758f, -0.388899207f, -0.430662155f, -0.470091313f,
         -0.506972969f, -0.541107357f, -0.572309375f, -0.600410044f, -0.625257015f, -0.646715760f,
         -0.664669752f, -0.679021895f, -0.689694405f, -0.696629286f, -0.699789107f, -0.699156821f,
         -0.694735527f, -0.686549723f, -0.674643159f, -0.659080803f, -0.639946878f, -0.617344797f,
         -0.591397524f, -0.562245190f, -0.530046105f, -0.494974732f, -0.457221001f, -0.416989505f,
         -0.374498338f, -0.329977721f, -0.283668905f, -0.235822871f, -0.186698914f, -0.136563212f,
         -0.085687466f, -0.034347348f, 0.017178880f,  0.068612024f,  0.119673342f,  0.170086160f,
         0.219577238f,  0.267878443f,  0.314727962f,  0.359871954f,  0.403065801f,  0.444075316f,
         0.482678413f,  0.518665850f,  0.551842570f,  0.582028747f,  0.609060943f,  0.632792592f,
         0.653095007f,  0.669858277f,  0.682991564f,  0.692423642f,  0.698103249f,  0.699999928f,
         0.698103249f,  0.692423642f,  0.682991505f,  0.669858158f,  0.653094947f,  0.632792532f,
         0.609060824f,  0.582028687f,  0.551842511f,  0.518665791f,  0.482678413f,  0.444075316f,
         0.403065771f,  0.359871924f,  0.314727932f,  0.267878383f,  0.219577208f,  0.170086116f,
         0.119673304f,  0.068611994f,  0.017178837f,  -0.034347393f, -0.085687503f, -0.136563241f,
         -0.186698973f, -0.235822916f, -0.283668905f, -0.329977721f, -0.374498367f, -0.416989535f,
         -0.457221031f, -0.494974762f, -0.530046225f, -0.562245250f, -0.591397583f, -0.617344856f,
         -0.639946878f, -0.659080923f, -0.674643219f, -0.686549783f, -0.694735646f, -0.699156821f,
         -0.699789107f, -0.696629286f, -0.689694345f, -0.679021835f, -0.664669693f, -0.646715701f,
         -0.625257015f, -0.600409985f, -0.572309315f, -0.541107297f, -0.506972969f, -0.470091283f,
         -0.430662155f, -0.388899177f, -0.345028728f, -0.299288541f, -0.251926512f, -0.203199238f,
         -0.153370857f, -0.102711312f, -0.051495172f, 0.000000022f,  0.051495217f,  0.102711357f,
         0.153370887f,  0.203199282f,  0.251926571f,  0.299288571f,  0.345028758f,  0.388899207f,
         0.430662155f,  0.470091313f,  0.506972969f,  0.541107357f,  0.572309375f,  0.600410044f,
         0.625257015f,  0.646715760f,  0.664669752f,  0.679021895f,  0.689694405f,  0.696629286f,
         0.699789107f,  0.699156821f,  0.694735527f,  0.686549723f,  0.674643159f,  0.659080803f,
         0.639946878f,  0.617344797f,  0.591397524f,  0.562245190f,  0.530046105f,  0.494974732f,
         0.457221001f,  0.416989505f,  0.374498338f,  0.329977721f,  0.283668905f,  0.235822871f,
         0.186698914f,  0.136563212f,  0.085687466f,  0.034347348f,  -0.017178880f, -0.068612024f,
         -0.119673342f, -0.170086160f, -0.219577238f, -0.267878443f, -0.314727962f, -0.359871954f,
         -0.403065801f, -0.444075316f, -0.482678413f, -0.518665850f, -0.551842570f, -0.582028747f,
         -0.609060943f, -0.632792592f, -0.653095007f, -0.669858277f, -0.682991564f, -0.692423642f,
         -0.698103249f, -0.699999928f, -0.698103249f, -0.692423642f, -0.682991505f, -0.669858158f,
         -0.653094947f, -0.632792532f, -0.609060824f, -0.582028687f, -0.551842511f, -0.518665791f,
         -0.482678413f, -0.444075316f, -0.403065771f, -0.359871924f, -0.314727932f, -0.267878383f,
         -0.219577208f, -0.170086116f, -0.119673304f, -0.068611994f, -0.017178837f, 0.034347393f,
         0.085687503f,  0.136563241f,  0.186698973f,  0.235822916f,  0.283668905f,  0.329977721f,
         0.374498367f,  0.416989535f,  0.457221031f,  0.494974762f,  0.530046225f,  0.562245250f,
         0.591397583f,  0.617344856f,  0.639946878f,  0.659080923f,  0.674643219f,  0.686549783f,
         0.694735646f,  0.699156821f,  0.699789107f,  0.696629286f,  0.689694345f,  0.679021835f,
         0.664669693f,  0.646715701f,  0.625257015f,  0.600409985f,  0.572309315f,  0.541107297f,
         0.506972969f,  0.470091283f,  0.430662155f,  0.388899177f,  0.345028728f,  0.299289465f,
         0.251914948f,  0.203259110f,  0.153185874f,  0.103101805f,  0.050895613f,  0.000689670f,
         -0.000599579f, 0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,  0.000000000f,
         0.000000000f,  0.000000000f},
    }};

    for (size_t ci = 0; ci < cases.size(); ++ci)
    {
        GenConfig c;
        c.name = cases[ci].name;
        c.loopActive = false;
        c.playReverse = cases[ci].reverse;
        c.warmupBlocks = 8;
        c.recordBlocks = 32;

        GenHarness h(c);
        goldenCheckOrPrint(c.name, capture(c, h), expected[ci]);
    }
}

TEST_CASE("Generator golden - looping without fade", "[generator][golden]")
{
    struct Case
    {
        const char *name;
        bool loopForward;
        bool reverse;
        int recordBlocks;
    };
    static const std::array<Case, 4> cases{{
        {"loop-fwd-nofade", true, false, 10},
        {"loop-rev-nofade", true, true, 10},
        {"loop-alt-nofade", false, false, 20},
        {"loop-alt-rev-nofade", false, true, 20},
    }};

    static const std::array<std::vector<float>, 4> expected{{
        // loop-fwd-nofade
        {-0.674643159f, 0.001856980f,  0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,
         0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,
         0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f,
         -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f,
         -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,
         0.699789107f,  0.679021835f,  0.609060943f,  0.494974732f,  0.345028788f,  0.170086160f,
         -0.017178826f, -0.203199223f, -0.374498308f, -0.518665791f, -0.625257015f, -0.686549723f,
         -0.698103189f, -0.659080923f, -0.572309494f, -0.444075286f, -0.283668905f, -0.102711365f,
         0.085687436f,  0.267878324f,  0.430662096f,  0.562245190f,  0.653094888f,  0.696629286f,
         0.689694345f,  0.632792532f,  0.530046225f,  0.388899148f,  0.219577268f,  0.034347400f,
         -0.153370842f, -0.329977691f, -0.482678384f, -0.600409985f, -0.674643159f, 0.001856980f,
         0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,
         0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f,
         -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f,
         -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,
         0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,
         0.609060943f,  0.494974732f,  0.345028788f,  0.170086160f,  -0.017178826f, -0.203199223f,
         -0.374498308f, -0.518665791f, -0.625257015f, -0.686549723f, -0.698103189f, -0.659080923f,
         -0.572309494f, -0.444075286f, -0.283668905f, -0.102711365f, 0.085687436f,  0.267878324f,
         0.430662096f,  0.562245190f,  0.653094888f,  0.696629286f,  0.689694345f,  0.632792532f,
         0.530046225f,  0.388899148f,  0.219577268f,  0.034347400f,  -0.153370842f, -0.329977691f,
         -0.482678384f, -0.600409985f, -0.674643159f, 0.001856980f,  0.185085431f,  0.360920966f,
         0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,
         0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f,
         -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f,
         -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,
         0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f},
        // loop-rev-nofade
        {-0.053108696f, -0.600409985f, -0.482678384f, -0.329977691f, -0.153370842f, 0.034347400f,
         0.219577268f,  0.388899148f,  0.530046225f,  0.632792532f,  0.689694345f,  0.696629286f,
         0.653094888f,  0.562245190f,  0.430662096f,  0.267878324f,  0.085687436f,  -0.102711365f,
         -0.283668905f, -0.444075286f, -0.572309494f, -0.659080923f, -0.698103189f, -0.686549723f,
         -0.625257015f, -0.518665791f, -0.374498308f, -0.203199223f, -0.017178826f, 0.170086160f,
         0.345028788f,  0.494974732f,  0.609060943f,  0.679021835f,  0.699789107f,  0.669858217f,
         0.591397583f,  0.470091254f,  0.314727873f,  0.136563197f,  -0.051495228f, -0.235822931f,
         -0.403065801f, -0.541107416f, -0.639946938f, -0.692423582f, -0.694735646f, -0.646715641f,
         -0.551842451f, -0.416989505f, -0.251926482f, -0.068611972f, 0.119673356f,  0.299288601f,
         0.457221001f,  0.582028806f,  0.664669752f,  0.699159205f,  0.682960927f,  0.617504358f,
         0.506477535f,  0.360920966f,  0.185085431f,  0.001856980f,  -0.053108696f, -0.600409985f,
         -0.482678384f, -0.329977691f, -0.153370842f, 0.034347400f,  0.219577268f,  0.388899148f,
         0.530046225f,  0.632792532f,  0.689694345f,  0.696629286f,  0.653094888f,  0.562245190f,
         0.430662096f,  0.267878324f,  0.085687436f,  -0.102711365f, -0.283668905f, -0.444075286f,
         -0.572309494f, -0.659080923f, -0.698103189f, -0.686549723f, -0.625257015f, -0.518665791f,
         -0.374498308f, -0.203199223f, -0.017178826f, 0.170086160f,  0.345028788f,  0.494974732f,
         0.609060943f,  0.679021835f,  0.699789107f,  0.669858217f,  0.591397583f,  0.470091254f,
         0.314727873f,  0.136563197f,  -0.051495228f, -0.235822931f, -0.403065801f, -0.541107416f,
         -0.639946938f, -0.692423582f, -0.694735646f, -0.646715641f, -0.551842451f, -0.416989505f,
         -0.251926482f, -0.068611972f, 0.119673356f,  0.299288601f,  0.457221001f,  0.582028806f,
         0.664669752f,  0.699159205f,  0.682960927f,  0.617504358f,  0.506477535f,  0.360920966f,
         0.185085431f,  0.001856980f,  -0.053108696f, -0.600409985f, -0.482678384f, -0.329977691f,
         -0.153370842f, 0.034347400f,  0.219577268f,  0.388899148f,  0.530046225f,  0.632792532f,
         0.689694345f,  0.696629286f,  0.653094888f,  0.562245190f,  0.430662096f,  0.267878324f,
         0.085687436f,  -0.102711365f, -0.283668905f, -0.444075286f, -0.572309494f, -0.659080923f,
         -0.698103189f, -0.686549723f, -0.625257015f, -0.518665791f, -0.374498308f, -0.203199223f,
         -0.017178826f, 0.170086160f,  0.345028788f,  0.494974732f},
        // loop-alt-nofade
        {-0.053108696f, 0.001856980f,  0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,
         0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,
         0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f,
         -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f,
         -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,
         0.699789107f,  0.679021835f,  0.609060943f,  0.494974732f,  0.345028788f,  0.170086160f,
         -0.017178826f, -0.203199223f, -0.374498308f, -0.518665791f, -0.625257015f, -0.686549723f,
         -0.698103189f, -0.659080923f, -0.572309494f, -0.444075286f, -0.283668905f, -0.102711365f,
         0.085687436f,  0.267878324f,  0.430662096f,  0.562245190f,  0.653094888f,  0.696629286f,
         0.689694345f,  0.632792532f,  0.530046225f,  0.388899148f,  0.219577268f,  0.034347400f,
         -0.153370842f, -0.329977691f, -0.482678384f, -0.600409985f, -0.674643159f, -0.600409985f,
         -0.482678384f, -0.329977691f, -0.153370842f, 0.034347400f,  0.219577268f,  0.388899148f,
         0.530046225f,  0.632792532f,  0.689694345f,  0.696629286f,  0.653094888f,  0.562245190f,
         0.430662096f,  0.267878324f,  0.085687436f,  -0.102711365f, -0.283668905f, -0.444075286f,
         -0.572309494f, -0.659080923f, -0.698103189f, -0.686549723f, -0.625257015f, -0.518665791f,
         -0.374498308f, -0.203199223f, -0.017178826f, 0.170086160f,  0.345028788f,  0.494974732f,
         0.609060943f,  0.679021835f,  0.699789107f,  0.669858217f,  0.591397583f,  0.470091254f,
         0.314727873f,  0.136563197f,  -0.051495228f, -0.235822931f, -0.403065801f, -0.541107416f,
         -0.639946938f, -0.692423582f, -0.694735646f, -0.646715641f, -0.551842451f, -0.416989505f,
         -0.251926482f, -0.068611972f, 0.119673356f,  0.299288601f,  0.457221001f,  0.582028806f,
         0.664669752f,  0.699159205f,  0.682960927f,  0.617504358f,  0.506477535f,  0.360920966f,
         0.185085431f,  0.001856980f,  -0.053108696f, 0.001856980f,  0.185085431f,  0.360920966f,
         0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,
         0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f,
         -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f,
         -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,
         0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,  0.609060943f,  0.494974732f,
         0.345028788f,  0.170086160f,  -0.017178826f, -0.203199223f, -0.374498308f, -0.518665791f,
         -0.625257015f, -0.686549723f, -0.698103189f, -0.659080923f, -0.572309494f, -0.444075286f,
         -0.283668905f, -0.102711365f, 0.085687436f,  0.267878324f,  0.430662096f,  0.562245190f,
         0.653094888f,  0.696629286f,  0.689694345f,  0.632792532f,  0.530046225f,  0.388899148f,
         0.219577268f,  0.034347400f,  -0.153370842f, -0.329977691f, -0.482678384f, -0.600409985f,
         -0.674643159f, -0.600409985f, -0.482678384f, -0.329977691f, -0.153370842f, 0.034347400f,
         0.219577268f,  0.388899148f,  0.530046225f,  0.632792532f,  0.689694345f,  0.696629286f,
         0.653094888f,  0.562245190f,  0.430662096f,  0.267878324f,  0.085687436f,  -0.102711365f,
         -0.283668905f, -0.444075286f, -0.572309494f, -0.659080923f, -0.698103189f, -0.686549723f,
         -0.625257015f, -0.518665791f, -0.374498308f, -0.203199223f, -0.017178826f, 0.170086160f,
         0.345028788f,  0.494974732f,  0.609060943f,  0.679021835f,  0.699789107f,  0.669858217f,
         0.591397583f,  0.470091254f,  0.314727873f,  0.136563197f,  -0.051495228f, -0.235822931f,
         -0.403065801f, -0.541107416f, -0.639946938f, -0.692423582f, -0.694735646f, -0.646715641f,
         -0.551842451f, -0.416989505f, -0.251926482f, -0.068611972f, 0.119673356f,  0.299288601f,
         0.457221001f,  0.582028806f,  0.664669752f,  0.699159205f,  0.682960927f,  0.617504358f,
         0.506477535f,  0.360920966f,  0.185085431f,  0.001856980f,  -0.053108696f, 0.001856980f,
         0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,
         0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f,
         -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f,
         -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,
         0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,
         0.609060943f,  0.494974732f,  0.345028788f,  0.170086160f,  -0.017178826f, -0.203199223f,
         -0.374498308f, -0.518665791f, -0.625257015f, -0.686549723f, -0.698103189f, -0.659080923f,
         -0.572309494f, -0.444075286f, -0.283668905f, -0.102711365f, 0.085687436f,  0.267878324f,
         0.430662096f,  0.562245190f,  0.653094888f,  0.696629286f,  0.689694345f,  0.632792532f,
         0.530046225f,  0.388899148f,  0.219577268f,  0.034347400f,  -0.153370842f, -0.329977691f,
         -0.482678384f, -0.600409985f},
        // loop-alt-rev-nofade
        {-0.053108696f, 0.001856980f,  0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,
         0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,
         0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f,
         -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f,
         -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,
         0.699789107f,  0.679021835f,  0.609060943f,  0.494974732f,  0.345028788f,  0.170086160f,
         -0.017178826f, -0.203199223f, -0.374498308f, -0.518665791f, -0.625257015f, -0.686549723f,
         -0.698103189f, -0.659080923f, -0.572309494f, -0.444075286f, -0.283668905f, -0.102711365f,
         0.085687436f,  0.267878324f,  0.430662096f,  0.562245190f,  0.653094888f,  0.696629286f,
         0.689694345f,  0.632792532f,  0.530046225f,  0.388899148f,  0.219577268f,  0.034347400f,
         -0.153370842f, -0.329977691f, -0.482678384f, -0.600409985f, -0.674643159f, -0.600409985f,
         -0.482678384f, -0.329977691f, -0.153370842f, 0.034347400f,  0.219577268f,  0.388899148f,
         0.530046225f,  0.632792532f,  0.689694345f,  0.696629286f,  0.653094888f,  0.562245190f,
         0.430662096f,  0.267878324f,  0.085687436f,  -0.102711365f, -0.283668905f, -0.444075286f,
         -0.572309494f, -0.659080923f, -0.698103189f, -0.686549723f, -0.625257015f, -0.518665791f,
         -0.374498308f, -0.203199223f, -0.017178826f, 0.170086160f,  0.345028788f,  0.494974732f,
         0.609060943f,  0.679021835f,  0.699789107f,  0.669858217f,  0.591397583f,  0.470091254f,
         0.314727873f,  0.136563197f,  -0.051495228f, -0.235822931f, -0.403065801f, -0.541107416f,
         -0.639946938f, -0.692423582f, -0.694735646f, -0.646715641f, -0.551842451f, -0.416989505f,
         -0.251926482f, -0.068611972f, 0.119673356f,  0.299288601f,  0.457221001f,  0.582028806f,
         0.664669752f,  0.699159205f,  0.682960927f,  0.617504358f,  0.506477535f,  0.360920966f,
         0.185085431f,  0.001856980f,  -0.053108696f, 0.001856980f,  0.185085431f,  0.360920966f,
         0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,
         0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f,
         -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f,
         -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,
         0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,  0.609060943f,  0.494974732f,
         0.345028788f,  0.170086160f,  -0.017178826f, -0.203199223f, -0.374498308f, -0.518665791f,
         -0.625257015f, -0.686549723f, -0.698103189f, -0.659080923f, -0.572309494f, -0.444075286f,
         -0.283668905f, -0.102711365f, 0.085687436f,  0.267878324f,  0.430662096f,  0.562245190f,
         0.653094888f,  0.696629286f,  0.689694345f,  0.632792532f,  0.530046225f,  0.388899148f,
         0.219577268f,  0.034347400f,  -0.153370842f, -0.329977691f, -0.482678384f, -0.600409985f,
         -0.674643159f, -0.600409985f, -0.482678384f, -0.329977691f, -0.153370842f, 0.034347400f,
         0.219577268f,  0.388899148f,  0.530046225f,  0.632792532f,  0.689694345f,  0.696629286f,
         0.653094888f,  0.562245190f,  0.430662096f,  0.267878324f,  0.085687436f,  -0.102711365f,
         -0.283668905f, -0.444075286f, -0.572309494f, -0.659080923f, -0.698103189f, -0.686549723f,
         -0.625257015f, -0.518665791f, -0.374498308f, -0.203199223f, -0.017178826f, 0.170086160f,
         0.345028788f,  0.494974732f,  0.609060943f,  0.679021835f,  0.699789107f,  0.669858217f,
         0.591397583f,  0.470091254f,  0.314727873f,  0.136563197f,  -0.051495228f, -0.235822931f,
         -0.403065801f, -0.541107416f, -0.639946938f, -0.692423582f, -0.694735646f, -0.646715641f,
         -0.551842451f, -0.416989505f, -0.251926482f, -0.068611972f, 0.119673356f,  0.299288601f,
         0.457221001f,  0.582028806f,  0.664669752f,  0.699159205f,  0.682960927f,  0.617504358f,
         0.506477535f,  0.360920966f,  0.185085431f,  0.001856980f,  -0.053108696f, 0.001856980f,
         0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,
         0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f,
         -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f,
         -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,
         0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,
         0.609060943f,  0.494974732f,  0.345028788f,  0.170086160f,  -0.017178826f, -0.203199223f,
         -0.374498308f, -0.518665791f, -0.625257015f, -0.686549723f, -0.698103189f, -0.659080923f,
         -0.572309494f, -0.444075286f, -0.283668905f, -0.102711365f, 0.085687436f,  0.267878324f,
         0.430662096f,  0.562245190f,  0.653094888f,  0.696629286f,  0.689694345f,  0.632792532f,
         0.530046225f,  0.388899148f,  0.219577268f,  0.034347400f,  -0.153370842f, -0.329977691f,
         -0.482678384f, -0.600409985f},
    }};

    for (size_t ci = 0; ci < cases.size(); ++ci)
    {
        GenConfig c;
        c.name = cases[ci].name;
        c.loopForward = cases[ci].loopForward;
        c.playReverse = cases[ci].reverse;
        c.recordBlocks = cases[ci].recordBlocks;

        GenHarness h(c);
        goldenCheckOrPrint(c.name, capture(c, h), expected[ci]);
    }
}

TEST_CASE("Generator golden - gated loop and release", "[generator][golden]")
{
    // The fade case matters for the release path: once ungated the playhead runs on
    // past endLoop into the tail, so there is no seam left and the crossfade has to
    // stop rather than blend in pre-loop material on the way out.
    struct Case
    {
        const char *name;
        int ungateAtBlock;
        int32_t loopFade;
    };
    static const std::array<Case, 3> cases{{{"gated-loop-held", -1, 0},
                                            {"gated-loop-release", 4, 0},
                                            {"gated-loop-fade-release", 4, gFade}}};

    static const std::array<std::vector<float>, 3> expected{{
        // gated-loop-held
        {-0.674643159f, 0.001856980f,  0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,
         0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,
         0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f,
         -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f,
         -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,
         0.699789107f,  0.679021835f,  0.609060943f,  0.494974732f,  0.345028788f,  0.170086160f,
         -0.017178826f, -0.203199223f, -0.374498308f, -0.518665791f, -0.625257015f, -0.686549723f,
         -0.698103189f, -0.659080923f, -0.572309494f, -0.444075286f, -0.283668905f, -0.102711365f,
         0.085687436f,  0.267878324f,  0.430662096f,  0.562245190f,  0.653094888f,  0.696629286f,
         0.689694345f,  0.632792532f,  0.530046225f,  0.388899148f,  0.219577268f,  0.034347400f,
         -0.153370842f, -0.329977691f, -0.482678384f, -0.600409985f, -0.674643159f, 0.001856980f,
         0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,
         0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f,
         -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f,
         -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,
         0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,
         0.609060943f,  0.494974732f,  0.345028788f,  0.170086160f,  -0.017178826f, -0.203199223f,
         -0.374498308f, -0.518665791f, -0.625257015f, -0.686549723f, -0.698103189f, -0.659080923f,
         -0.572309494f, -0.444075286f, -0.283668905f, -0.102711365f, 0.085687436f,  0.267878324f,
         0.430662096f,  0.562245190f,  0.653094888f,  0.696629286f,  0.689694345f,  0.632792532f,
         0.530046225f,  0.388899148f,  0.219577268f,  0.034347400f,  -0.153370842f, -0.329977691f,
         -0.482678384f, -0.600409985f, -0.674643159f, 0.001856980f,  0.185085431f,  0.360920966f,
         0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,
         0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f,
         -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f,
         -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,
         0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,  0.609060943f,  0.494974732f,
         0.345028788f,  0.170086160f,  -0.017178826f, -0.203199223f, -0.374498308f, -0.518665791f,
         -0.625257015f, -0.686549723f, -0.698103189f, -0.659080923f, -0.572309494f, -0.444075286f,
         -0.283668905f, -0.102711365f, 0.085687436f,  0.267878324f,  0.430662096f,  0.562245190f,
         0.653094888f,  0.696629286f,  0.689694345f,  0.632792532f,  0.530046225f,  0.388899148f,
         0.219577268f,  0.034347400f,  -0.153370842f, -0.329977691f, -0.482678384f, -0.600409985f},
        // gated-loop-release
        {-0.674643159f, 0.001856980f,  0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,
         0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,
         0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f,
         -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f,
         -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,
         0.699789107f,  0.679021835f,  0.609060943f,  0.494974732f,  0.345028788f,  0.170086160f,
         -0.017178826f, -0.203199223f, -0.374498308f, -0.518665791f, -0.625257015f, -0.686549723f,
         -0.698103189f, -0.659080923f, -0.572309494f, -0.444075286f, -0.283668905f, -0.102711365f,
         0.085687436f,  0.267878324f,  0.430662096f,  0.562245190f,  0.653094888f,  0.696629286f,
         0.689694345f,  0.632792532f,  0.530046225f,  0.388899148f,  0.219577268f,  0.034347400f,
         -0.153370842f, -0.329977691f, -0.482678384f, -0.600409985f, -0.674643159f, -0.699999869f,
         -0.674643159f, -0.600410044f, -0.482678413f, -0.329977751f, -0.153370902f, 0.034347337f,
         0.219577193f,  0.388899118f,  0.530046165f,  0.632792532f,  0.689694345f,  0.696629286f,
         0.653094947f,  0.562245250f,  0.430662155f,  0.267878413f,  0.085687503f,  -0.102711305f,
         -0.283668876f, -0.444075257f, -0.572309375f, -0.659080863f, -0.698103249f, -0.686549723f,
         -0.625257075f, -0.518665791f, -0.374498367f, -0.203199312f, -0.017178891f, 0.170086086f,
         0.345028698f,  0.494974703f,  0.609060884f,  0.679021835f,  0.699789107f,  0.669858217f,
         0.591397583f,  0.470091283f,  0.314727932f,  0.136563256f,  -0.051495165f, -0.235822856f,
         -0.403065771f, -0.541107297f, -0.639946818f, -0.692423522f, -0.694735646f, -0.646715701f,
         -0.551842570f, -0.416989535f, -0.251926571f, -0.068612039f, 0.119673289f,  0.299288541f,
         0.457220972f,  0.582028806f,  0.664669752f,  0.699156761f,  0.682991505f,  0.617344856f,
         0.506972969f,  0.359871954f,  0.186698973f,  0.000000033f,  -0.186698884f, -0.359871894f,
         -0.506972969f, -0.617344737f, -0.682991505f, -0.699156821f, -0.664669752f, -0.582028806f,
         -0.457221001f, -0.299288601f, -0.119673356f, 0.068611972f,  0.251926482f,  0.416989505f,
         0.551842451f,  0.646715641f,  0.694735646f,  0.692423582f,  0.639946938f,  0.541107416f,
         0.403065801f,  0.235822931f,  0.051495228f,  -0.136563197f, -0.314727873f, -0.470091254f,
         -0.591397583f, -0.669858217f, -0.699789107f, -0.679021835f, -0.609060943f, -0.494974732f,
         -0.345028788f, -0.170086160f, 0.017178826f,  0.203199223f,  0.374498308f,  0.518665791f,
         0.625257015f,  0.686549723f,  0.698103189f,  0.659080923f,  0.572309494f,  0.444075286f,
         0.283668905f,  0.102711365f,  -0.085687436f, -0.267878324f, -0.430662096f, -0.562245190f,
         -0.653094888f, -0.696629286f, -0.689694345f, -0.632792532f, -0.530046225f, -0.388899148f,
         -0.219577268f, -0.034347400f, 0.153370842f,  0.329977691f,  0.482678384f,  0.600409985f},
        // gated-loop-fade-release
        {-0.053108696f, 0.001856980f,  0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,
         0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,
         0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f,
         -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f,
         -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,
         0.699789107f,  0.679021835f,  0.609060943f,  0.457119524f,  0.271540463f,  0.065336391f,
         -0.147455245f, -0.352828890f, -0.537845552f, -0.691563070f, -0.805791795f, -0.875617325f,
         -0.899651229f, -0.879998684f, -0.821941137f, -0.733376503f, -0.624055564f, -0.504687369f,
         -0.385988206f, -0.277755380f, -0.188042924f, -0.122511573f, -0.084004425f, -0.072387055f,
         -0.084663205f, -0.115355894f, -0.157120481f, -0.201534837f, -0.239995942f, -0.264639914f,
         -0.269242376f, -0.249621764f, -0.205688626f, -0.136437073f, -0.674643159f, -0.699999869f,
         -0.674643159f, -0.600410044f, -0.482678413f, -0.329977751f, -0.153370902f, 0.034347337f,
         0.219577193f,  0.388899118f,  0.530046165f,  0.632792532f,  0.689694345f,  0.696629286f,
         0.653094947f,  0.562245250f,  0.430662155f,  0.267878413f,  0.085687503f,  -0.102711305f,
         -0.283668876f, -0.444075257f, -0.572309375f, -0.659080863f, -0.698103249f, -0.686549723f,
         -0.625257075f, -0.518665791f, -0.374498367f, -0.203199312f, -0.017178891f, 0.170086086f,
         0.345028698f,  0.494974703f,  0.609060884f,  0.679021835f,  0.699789107f,  0.669858217f,
         0.591397583f,  0.470091283f,  0.314727932f,  0.136563256f,  -0.051495165f, -0.235822856f,
         -0.403065771f, -0.541107297f, -0.639946818f, -0.692423522f, -0.694735646f, -0.646715701f,
         -0.551842570f, -0.416989535f, -0.251926571f, -0.068612039f, 0.119673289f,  0.299288541f,
         0.457220972f,  0.582028806f,  0.664669752f,  0.699156761f,  0.682991505f,  0.617344856f,
         0.506972969f,  0.359871954f,  0.186698973f,  0.000000033f,  -0.186698884f, -0.359871894f,
         -0.506972969f, -0.617344737f, -0.682991505f, -0.699156821f, -0.664669752f, -0.582028806f,
         -0.457221001f, -0.299288601f, -0.119673356f, 0.068611972f,  0.251926482f,  0.416989505f,
         0.551842451f,  0.646715641f,  0.694735646f,  0.692423582f,  0.639946938f,  0.541107416f,
         0.403065801f,  0.235822931f,  0.051495228f,  -0.136563197f, -0.314727873f, -0.470091254f,
         -0.591397583f, -0.669858217f, -0.699789107f, -0.679021835f, -0.609060943f, -0.494974732f,
         -0.345028788f, -0.170086160f, 0.017178826f,  0.203199223f,  0.374498308f,  0.518665791f,
         0.625257015f,  0.686549723f,  0.698103189f,  0.659080923f,  0.572309494f,  0.444075286f,
         0.283668905f,  0.102711365f,  -0.085687436f, -0.267878324f, -0.430662096f, -0.562245190f,
         -0.653094888f, -0.696629286f, -0.689694345f, -0.632792532f, -0.530046225f, -0.388899148f,
         -0.219577268f, -0.034347400f, 0.153370842f,  0.329977691f,  0.482678384f,  0.600409985f},
    }};

    for (size_t ci = 0; ci < cases.size(); ++ci)
    {
        GenConfig c;
        c.name = cases[ci].name;
        c.loopWhileGated = true;
        c.ungateAtBlock = cases[ci].ungateAtBlock;
        c.loopFade = cases[ci].loopFade;
        c.recordBlocks = 12;

        GenHarness h(c);
        h.run(c);
        // the release case leaves the loop, so it only has to span seams while gated
        if (cases[ci].ungateAtBlock < 0)
            checkSpannedSeams(c, h);
        goldenCheckOrPrint(c.name, h.recL, expected[ci]);
    }
}

TEST_CASE("Generator golden - forward loop with fade", "[generator][golden]")
{
    // The one fade path believed correct today. Captured at three ratios so the seam
    // lands at different sub-positions and different offsets within the 16 sample
    // block - at unity with a block aligned seam, engagement bugs can hide.
    struct Case
    {
        const char *name;
        int32_t ratio;
        int recordBlocks;
    };
    static const std::array<Case, 3> cases{{
        {"loop-fwd-fade-r10", unityRatio, 10},
        {"loop-fwd-fade-r13", (int32_t)(unityRatio * 1.3), 10},
        {"loop-fwd-fade-r05", (int32_t)(unityRatio * 0.5), 24},
    }};

    static const std::array<std::vector<float>, 3> expected{{
        // loop-fwd-fade-r10
        {-0.053108696f, 0.001856980f,  0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,
         0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,
         0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f,
         -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f,
         -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,
         0.699789107f,  0.679021835f,  0.609060943f,  0.457119524f,  0.271540463f,  0.065336391f,
         -0.147455245f, -0.352828890f, -0.537845552f, -0.691563070f, -0.805791795f, -0.875617325f,
         -0.899651229f, -0.879998684f, -0.821941137f, -0.733376503f, -0.624055564f, -0.504687369f,
         -0.385988206f, -0.277755380f, -0.188042924f, -0.122511573f, -0.084004425f, -0.072387055f,
         -0.084663205f, -0.115355894f, -0.157120481f, -0.201534837f, -0.239995942f, -0.264639914f,
         -0.269242376f, -0.249621764f, -0.205688626f, -0.136437073f, -0.053108696f, 0.001856980f,
         0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,
         0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f,
         -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f,
         -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,
         0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,
         0.609060943f,  0.457119524f,  0.271540463f,  0.065336391f,  -0.147455245f, -0.352828890f,
         -0.537845552f, -0.691563070f, -0.805791795f, -0.875617325f, -0.899651229f, -0.879998684f,
         -0.821941137f, -0.733376503f, -0.624055564f, -0.504687369f, -0.385988206f, -0.277755380f,
         -0.188042924f, -0.122511573f, -0.084004425f, -0.072387055f, -0.084663205f, -0.115355894f,
         -0.157120481f, -0.201534837f, -0.239995942f, -0.264639914f, -0.269242376f, -0.249621764f,
         -0.205688626f, -0.136437073f, -0.053108696f, 0.001856980f,  0.185085431f,  0.360920966f,
         0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,
         0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f,
         -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f,
         -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,
         0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f},
        // loop-fwd-fade-r13
        {0.245498404f,  0.455915123f,  0.610744953f,  0.691110253f,  0.687212706f,  0.599527299f,
         0.400912225f,  0.152182013f,  -0.117711887f, -0.403746694f, -0.622719824f, -0.782495975f,
         -0.881794214f, -0.890108168f, -0.828229964f, -0.722072244f, -0.570879936f, -0.406992853f,
         -0.252561420f, -0.156613365f, -0.075630456f, -0.039910443f, -0.082381442f, -0.119333021f,
         -0.172753990f, -0.241657823f, -0.267882228f, -0.265424460f, -0.229044527f, -0.120552756f,
         -0.032540865f, 0.164449379f,  0.391290039f,  0.567707002f,  0.673659384f,  0.697976470f,
         0.637133121f,  0.498609245f,  0.299293011f,  0.063485771f,  -0.180061951f, -0.401655734f,
         -0.574278176f, -0.676882267f, -0.696958184f, -0.632058144f, -0.490095079f, -0.288377732f,
         -0.051500197f, 0.191656411f,  0.411445498f,  0.581069589f,  0.679847240f,  0.695735455f,
         0.626796663f,  0.443575948f,  0.204514459f,  -0.062382679f, -0.326835662f, -0.581568003f,
         -0.755617917f, -0.859305859f, -0.897123277f, -0.850503266f, -0.744055212f, -0.610998750f,
         -0.449106157f, -0.291868597f, -0.158964127f, -0.098054618f, -0.052384242f, -0.050166894f,
         -0.116421446f, -0.166001722f, -0.218905807f, -0.265470952f, -0.267390370f, -0.235383540f,
         -0.169020608f, -0.044751085f, 0.102330081f,  0.345460564f,  0.532302916f,  0.656066418f,
         0.699996591f,  0.658501863f,  0.536725044f,  0.349508584f,  0.119678609f,  -0.124742970f,
         -0.353955448f, -0.540012300f, -0.660228550f, -0.699947476f, -0.654325604f, -0.528925955f,
         -0.339037538f, -0.107812293f, 0.136557847f,  0.364278257f,  0.547584474f,  0.664126992f,
         0.699696541f,  0.649956346f,  0.520970762f,  0.255118132f,  -0.007250945f, -0.273977995f,
         -0.515715301f, -0.725242794f, -0.845317125f, -0.889538825f, -0.869373202f, -0.775608718f,
         -0.636256039f, -0.490884513f, -0.331901819f, -0.193081632f, -0.090061605f, -0.067100510f,
         -0.055431940f, -0.081071354f, -0.160294086f, -0.211899430f, -0.253905267f, -0.268845558f,
         -0.241630852f, -0.182100937f, -0.093153849f, 0.045765512f,  0.297123551f,  0.492961496f,
         0.634276748f,  0.697424591f,  0.675553024f,  0.571321726f,  0.397432506f,  0.175086841f,
         -0.068606205f, -0.303934395f, -0.502205729f, -0.639246285f, -0.698347270f, -0.672303081f,
         -0.564288855f, -0.387474418f, -0.163417518f, 0.080563888f,  0.314722627f,  0.510509074f,
         0.644052207f,  0.699069977f,  0.668854475f,  0.557089448f,  0.339698970f,  0.047330864f,
         -0.220146462f, -0.469062924f, -0.673137784f, -0.827442050f},
        // loop-fwd-fade-r05
        {-0.053108696f, -0.037501011f, 0.001856980f,  0.082470551f,  0.185085431f,  0.280057371f,
         0.360920966f,  0.435939908f,  0.506477535f,  0.567709923f,  0.617504358f,  0.656068206f,
         0.682960927f,  0.697425067f,  0.699159205f,  0.688173711f,  0.664669752f,  0.629072189f,
         0.582028806f,  0.524395525f,  0.457221001f,  0.381727517f,  0.299288601f,  0.211404160f,
         0.119673356f,  0.025765073f,  -0.068611972f, -0.161740690f, -0.251926482f, -0.337528646f,
         -0.416989505f, -0.488863409f, -0.551842451f, -0.604781032f, -0.646715641f, -0.676883578f,
         -0.694735646f, -0.699947357f, -0.692423582f, -0.672301412f, -0.639946938f, -0.595948696f,
         -0.541107416f, -0.476420701f, -0.403065801f, -0.322377086f, -0.235822931f, -0.144977987f,
         -0.051495228f, 0.042924516f,  0.136563197f,  0.227717221f,  0.314727873f,  0.396012276f,
         0.470091254f,  0.535617113f,  0.591397583f,  0.636417627f,  0.669858217f,  0.691110969f,
         0.699789107f,  0.695734978f,  0.679021835f,  0.649954259f,  0.609060943f,  0.557085812f,
         0.457119524f,  0.386047542f,  0.271540463f,  0.187241450f,  0.065336391f,  -0.025589691f,
         -0.147455245f, -0.238183096f, -0.352828890f, -0.436816692f, -0.537845552f, -0.609308541f,
         -0.691563070f, -0.745875418f, -0.805791795f, -0.839787602f, -0.875617325f, -0.887767434f,
         -0.899651229f, -0.890107453f, -0.879998684f, -0.850501418f, -0.821941137f, -0.775605738f,
         -0.733376503f, -0.674374521f, -0.624055564f, -0.557222188f, -0.504687369f, -0.435089052f,
         -0.385988206f, -0.318487585f, -0.277755380f, -0.216610640f, -0.188042924f, -0.136575773f,
         -0.122511573f, -0.082870066f, -0.084004425f, -0.057041384f, -0.072387055f, -0.057661787f,
         -0.084663205f, -0.080564439f, -0.115355894f, -0.119333342f, -0.157120481f, -0.166002288f,
         -0.201534837f, -0.211900070f, -0.239995942f, -0.248566449f, -0.264639914f, -0.268648237f,
         -0.269242376f, -0.266798943f, -0.249621764f, -0.239556998f, -0.205688626f, -0.189705431f,
         -0.136437073f, -0.110728249f, -0.053108696f, -0.037501011f, 0.001856980f,  0.082470551f,
         0.185085431f,  0.280057371f,  0.360920966f,  0.435939908f,  0.506477535f,  0.567709923f,
         0.617504358f,  0.656068206f,  0.682960927f,  0.697425067f,  0.699159205f,  0.688173711f,
         0.664669752f,  0.629072189f,  0.582028806f,  0.524395525f,  0.457221001f,  0.381727517f,
         0.299288601f,  0.211404160f,  0.119673356f,  0.025765073f,  -0.068611972f, -0.161740690f,
         -0.251926482f, -0.337528646f, -0.416989505f, -0.488863409f, -0.551842451f, -0.604781032f,
         -0.646715641f, -0.676883578f, -0.694735646f, -0.699947357f, -0.692423582f, -0.672301412f,
         -0.639946938f, -0.595948696f, -0.541107416f, -0.476420701f, -0.403065801f, -0.322377086f,
         -0.235822931f, -0.144977987f, -0.051495228f, 0.042924516f,  0.136563197f,  0.227717221f,
         0.314727873f,  0.396012276f,  0.470091254f,  0.535617113f,  0.591397583f,  0.636417627f,
         0.669858217f,  0.691110969f,  0.699789107f,  0.695734978f,  0.679021835f,  0.649954259f,
         0.609060943f,  0.557085812f,  0.457119524f,  0.386047542f,  0.271540463f,  0.187241450f,
         0.065336391f,  -0.025589691f, -0.147455245f, -0.238183096f, -0.352828890f, -0.436816692f,
         -0.537845552f, -0.609308541f, -0.691563070f, -0.745875418f, -0.805791795f, -0.839787602f,
         -0.875617325f, -0.887767434f, -0.899651229f, -0.890107453f, -0.879998684f, -0.850501418f,
         -0.821941137f, -0.775605738f, -0.733376503f, -0.674374521f, -0.624055564f, -0.557222188f,
         -0.504687369f, -0.435089052f, -0.385988206f, -0.318487585f, -0.277755380f, -0.216610640f,
         -0.188042924f, -0.136575773f, -0.122511573f, -0.082870066f, -0.084004425f, -0.057041384f,
         -0.072387055f, -0.057661787f, -0.084663205f, -0.080564439f, -0.115355894f, -0.119333342f,
         -0.157120481f, -0.166002288f, -0.201534837f, -0.211900070f, -0.239995942f, -0.248566449f,
         -0.264639914f, -0.268648237f, -0.269242376f, -0.266798943f, -0.249621764f, -0.239556998f,
         -0.205688626f, -0.189705431f, -0.136437073f, -0.110728249f, -0.053108696f, -0.037501011f,
         0.001856980f,  0.082470551f,  0.185085431f,  0.280057371f,  0.360920966f,  0.435939908f,
         0.506477535f,  0.567709923f,  0.617504358f,  0.656068206f,  0.682960927f,  0.697425067f,
         0.699159205f,  0.688173711f,  0.664669752f,  0.629072189f,  0.582028806f,  0.524395525f,
         0.457221001f,  0.381727517f,  0.299288601f,  0.211404160f,  0.119673356f,  0.025765073f,
         -0.068611972f, -0.161740690f, -0.251926482f, -0.337528646f, -0.416989505f, -0.488863409f,
         -0.551842451f, -0.604781032f, -0.646715641f, -0.676883578f, -0.694735646f, -0.699947357f,
         -0.692423582f, -0.672301412f, -0.639946938f, -0.595948696f, -0.541107416f, -0.476420701f,
         -0.403065801f, -0.322377086f, -0.235822931f, -0.144977987f, -0.051495228f, 0.042924516f,
         0.136563197f,  0.227717221f,  0.314727873f,  0.396012276f,  0.470091254f,  0.535617113f,
         0.591397583f,  0.636417627f,  0.669858217f,  0.691110969f,  0.699789107f,  0.695734978f,
         0.679021835f,  0.649954259f,  0.609060943f,  0.557085812f,  0.457119524f,  0.386047542f,
         0.271540463f,  0.187241450f,  0.065336391f,  -0.025589691f, -0.147455245f, -0.238183096f,
         -0.352828890f, -0.436816692f, -0.537845552f, -0.609308541f, -0.691563070f, -0.745875418f,
         -0.805791795f, -0.839787602f, -0.875617325f, -0.887767434f, -0.899651229f, -0.890107453f,
         -0.879998684f, -0.850501418f, -0.821941137f, -0.775605738f, -0.733376503f, -0.674374521f,
         -0.624055564f, -0.557222188f, -0.504687369f, -0.435089052f, -0.385988206f, -0.318487585f,
         -0.277755380f, -0.216610640f, -0.188042924f, -0.136575773f, -0.122511573f, -0.082870066f,
         -0.084004425f, -0.057041384f, -0.072387055f, -0.057661787f, -0.084663205f, -0.080564439f,
         -0.115355894f, -0.119333342f, -0.157120481f, -0.166002288f, -0.201534837f, -0.211900070f,
         -0.239995942f, -0.248566449f, -0.264639914f, -0.268648237f, -0.269242376f, -0.266798943f,
         -0.249621764f, -0.239556998f, -0.205688626f, -0.189705431f, -0.136437073f, -0.110728249f},
    }};

    for (size_t ci = 0; ci < cases.size(); ++ci)
    {
        GenConfig c;
        c.name = cases[ci].name;
        c.loopFade = gFade;
        c.ratio = cases[ci].ratio;
        c.recordBlocks = cases[ci].recordBlocks;

        GenHarness h(c);
        goldenCheckOrPrint(c.name, capture(c, h), expected[ci]);
    }
}

TEST_CASE("Generator golden - alternate loop with fade", "[generator][golden]")
{
    // The mirror crossfade at each ping-pong turnaround. Previously these got the wrap
    // crossfade, which blended in pre-loop material at a bound that has no seam.
    struct Case
    {
        const char *name;
        bool reverse;
    };
    static const std::array<Case, 2> cases{{{"loop-alt-fade", false}, {"loop-alt-rev-fade", true}}};

    static const std::array<std::vector<float>, 2> expected{{
        // loop-alt-fade
        {-0.053108696f, 0.001856980f,  0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,
         0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,
         0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f,
         -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f,
         -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,
         0.699789107f,  0.679021835f,  0.609060943f,  0.460743964f,  0.293297082f,  0.123205177f,
         -0.034202829f, -0.166082561f, -0.263109148f, -0.320135951f, -0.336431205f, -0.315484703f,
         -0.264416456f, -0.193055540f, -0.112782851f, -0.035257593f, 0.028850904f,  0.071002543f,
         0.085687436f,  0.071002543f,  0.028850904f,  -0.035257593f, -0.112782851f, -0.193055540f,
         -0.264416456f, -0.315484703f, -0.336431205f, -0.320135951f, -0.263109148f, -0.166082561f,
         -0.034202829f, 0.123205177f,  0.293297082f,  0.460743964f,  0.609060943f,  0.679021835f,
         0.699789107f,  0.669858217f,  0.591397583f,  0.470091254f,  0.314727873f,  0.136563197f,
         -0.051495228f, -0.235822931f, -0.403065801f, -0.541107416f, -0.639946938f, -0.692423582f,
         -0.694735646f, -0.646715641f, -0.551842451f, -0.416989505f, -0.251926482f, -0.068611972f,
         0.119673356f,  0.299288601f,  0.457221001f,  0.582028806f,  0.664669752f,  0.699159205f,
         0.682960927f,  0.617504358f,  0.506477535f,  0.360920966f,  0.185085431f,  0.001856980f,
         -0.053108696f, -0.113953330f, -0.177377596f, -0.236715242f, -0.294387162f, -0.348982871f,
         -0.400325894f, -0.448001415f, -0.491657406f, -0.530970216f, -0.565648794f, -0.595436871f,
         -0.620114267f, -0.639499128f, -0.653448343f, -0.661859274f, -0.664669752f, -0.661859274f,
         -0.653448343f, -0.639499128f, -0.620114267f, -0.595436871f, -0.565648794f, -0.530970216f,
         -0.491657406f, -0.448001415f, -0.400325894f, -0.348982871f, -0.294387162f, -0.236715242f,
         -0.177377596f, -0.113953330f, -0.053108696f, 0.001856980f,  0.185085431f,  0.360920966f,
         0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,
         0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f,
         -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f,
         -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,
         0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,  0.609060943f,  0.460743964f,
         0.293297082f,  0.123205177f,  -0.034202829f, -0.166082561f, -0.263109148f, -0.320135951f,
         -0.336431205f, -0.315484703f, -0.264416456f, -0.193055540f, -0.112782851f, -0.035257593f,
         0.028850904f,  0.071002543f,  0.085687436f,  0.071002543f,  0.028850904f,  -0.035257593f,
         -0.112782851f, -0.193055540f, -0.264416456f, -0.315484703f, -0.336431205f, -0.320135951f,
         -0.263109148f, -0.166082561f, -0.034202829f, 0.123205177f,  0.293297082f,  0.460743964f,
         0.609060943f,  0.679021835f,  0.699789107f,  0.669858217f,  0.591397583f,  0.470091254f,
         0.314727873f,  0.136563197f,  -0.051495228f, -0.235822931f, -0.403065801f, -0.541107416f,
         -0.639946938f, -0.692423582f, -0.694735646f, -0.646715641f, -0.551842451f, -0.416989505f,
         -0.251926482f, -0.068611972f, 0.119673356f,  0.299288601f,  0.457221001f,  0.582028806f,
         0.664669752f,  0.699159205f,  0.682960927f,  0.617504358f,  0.506477535f,  0.360920966f,
         0.185085431f,  0.001856980f,  -0.053108696f, -0.113953330f, -0.177377596f, -0.236715242f,
         -0.294387162f, -0.348982871f, -0.400325894f, -0.448001415f, -0.491657406f, -0.530970216f,
         -0.565648794f, -0.595436871f, -0.620114267f, -0.639499128f, -0.653448343f, -0.661859274f,
         -0.664669752f, -0.661859274f, -0.653448343f, -0.639499128f, -0.620114267f, -0.595436871f,
         -0.565648794f, -0.530970216f, -0.491657406f, -0.448001415f, -0.400325894f, -0.348982871f,
         -0.294387162f, -0.236715242f, -0.177377596f, -0.113953330f, -0.053108696f, 0.001856980f,
         0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,
         0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f,
         -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f,
         -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,
         0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,
         0.609060943f,  0.460743964f,  0.293297082f,  0.123205177f,  -0.034202829f, -0.166082561f,
         -0.263109148f, -0.320135951f, -0.336431205f, -0.315484703f, -0.264416456f, -0.193055540f,
         -0.112782851f, -0.035257593f, 0.028850904f,  0.071002543f,  0.085687436f,  0.071002543f,
         0.028850904f,  -0.035257593f, -0.112782851f, -0.193055540f, -0.264416456f, -0.315484703f,
         -0.336431205f, -0.320135951f, -0.263109148f, -0.166082561f, -0.034202829f, 0.123205177f,
         0.293297082f,  0.460743964f},
        // loop-alt-rev-fade
        {-0.053108696f, -0.113953330f, -0.177377596f, -0.236715242f, -0.294387162f, -0.348982871f,
         -0.400325894f, -0.448001415f, -0.491657406f, -0.530970216f, -0.565648794f, -0.595436871f,
         -0.620114267f, -0.639499128f, -0.653448343f, -0.661859274f, -0.664669752f, -0.661859274f,
         -0.653448343f, -0.639499128f, -0.620114267f, -0.595436871f, -0.565648794f, -0.530970216f,
         -0.491657406f, -0.448001415f, -0.400325894f, -0.348982871f, -0.294387162f, -0.236715242f,
         -0.177377596f, -0.113953330f, -0.053108696f, 0.001856980f,  0.185085431f,  0.360920966f,
         0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,
         0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f,
         -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f,
         -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,
         0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,  0.609060943f,  0.460743964f,
         0.293297082f,  0.123205177f,  -0.034202829f, -0.166082561f, -0.263109148f, -0.320135951f,
         -0.336431205f, -0.315484703f, -0.264416456f, -0.193055540f, -0.112782851f, -0.035257593f,
         0.028850904f,  0.071002543f,  0.085687436f,  0.071002543f,  0.028850904f,  -0.035257593f,
         -0.112782851f, -0.193055540f, -0.264416456f, -0.315484703f, -0.336431205f, -0.320135951f,
         -0.263109148f, -0.166082561f, -0.034202829f, 0.123205177f,  0.293297082f,  0.460743964f,
         0.609060943f,  0.679021835f,  0.699789107f,  0.669858217f,  0.591397583f,  0.470091254f,
         0.314727873f,  0.136563197f,  -0.051495228f, -0.235822931f, -0.403065801f, -0.541107416f,
         -0.639946938f, -0.692423582f, -0.694735646f, -0.646715641f, -0.551842451f, -0.416989505f,
         -0.251926482f, -0.068611972f, 0.119673356f,  0.299288601f,  0.457221001f,  0.582028806f,
         0.664669752f,  0.699159205f,  0.682960927f,  0.617504358f,  0.506477535f,  0.360920966f,
         0.185085431f,  0.001856980f,  -0.053108696f, -0.113953330f, -0.177377596f, -0.236715242f,
         -0.294387162f, -0.348982871f, -0.400325894f, -0.448001415f, -0.491657406f, -0.530970216f,
         -0.565648794f, -0.595436871f, -0.620114267f, -0.639499128f, -0.653448343f, -0.661859274f,
         -0.664669752f, -0.661859274f, -0.653448343f, -0.639499128f, -0.620114267f, -0.595436871f,
         -0.565648794f, -0.530970216f, -0.491657406f, -0.448001415f, -0.400325894f, -0.348982871f,
         -0.294387162f, -0.236715242f, -0.177377596f, -0.113953330f, -0.053108696f, 0.001856980f,
         0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,  0.682960927f,  0.699159205f,
         0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,  0.119673356f,  -0.068611972f,
         -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f, -0.694735646f, -0.692423582f,
         -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f, -0.051495228f, 0.136563197f,
         0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,  0.699789107f,  0.679021835f,
         0.609060943f,  0.460743964f,  0.293297082f,  0.123205177f,  -0.034202829f, -0.166082561f,
         -0.263109148f, -0.320135951f, -0.336431205f, -0.315484703f, -0.264416456f, -0.193055540f,
         -0.112782851f, -0.035257593f, 0.028850904f,  0.071002543f,  0.085687436f,  0.071002543f,
         0.028850904f,  -0.035257593f, -0.112782851f, -0.193055540f, -0.264416456f, -0.315484703f,
         -0.336431205f, -0.320135951f, -0.263109148f, -0.166082561f, -0.034202829f, 0.123205177f,
         0.293297082f,  0.460743964f,  0.609060943f,  0.679021835f,  0.699789107f,  0.669858217f,
         0.591397583f,  0.470091254f,  0.314727873f,  0.136563197f,  -0.051495228f, -0.235822931f,
         -0.403065801f, -0.541107416f, -0.639946938f, -0.692423582f, -0.694735646f, -0.646715641f,
         -0.551842451f, -0.416989505f, -0.251926482f, -0.068611972f, 0.119673356f,  0.299288601f,
         0.457221001f,  0.582028806f,  0.664669752f,  0.699159205f,  0.682960927f,  0.617504358f,
         0.506477535f,  0.360920966f,  0.185085431f,  0.001856980f,  -0.053108696f, -0.113953330f,
         -0.177377596f, -0.236715242f, -0.294387162f, -0.348982871f, -0.400325894f, -0.448001415f,
         -0.491657406f, -0.530970216f, -0.565648794f, -0.595436871f, -0.620114267f, -0.639499128f,
         -0.653448343f, -0.661859274f, -0.664669752f, -0.661859274f, -0.653448343f, -0.639499128f,
         -0.620114267f, -0.595436871f, -0.565648794f, -0.530970216f, -0.491657406f, -0.448001415f,
         -0.400325894f, -0.348982871f, -0.294387162f, -0.236715242f, -0.177377596f, -0.113953330f,
         -0.053108696f, 0.001856980f,  0.185085431f,  0.360920966f,  0.506477535f,  0.617504358f,
         0.682960927f,  0.699159205f,  0.664669752f,  0.582028806f,  0.457221001f,  0.299288601f,
         0.119673356f,  -0.068611972f, -0.251926482f, -0.416989505f, -0.551842451f, -0.646715641f,
         -0.694735646f, -0.692423582f, -0.639946938f, -0.541107416f, -0.403065801f, -0.235822931f,
         -0.051495228f, 0.136563197f,  0.314727873f,  0.470091254f,  0.591397583f,  0.669858217f,
         0.699789107f,  0.679021835f},
    }};

    for (size_t ci = 0; ci < cases.size(); ++ci)
    {
        GenConfig c;
        c.name = cases[ci].name;
        c.loopForward = false;
        c.playReverse = cases[ci].reverse;
        c.loopFade = gFade;
        c.recordBlocks = 20;

        GenHarness h(c);
        goldenCheckOrPrint(c.name, capture(c, h), expected[ci]);
    }
}

TEST_CASE("Generator golden - interpolation types", "[generator][golden]")
{
    struct Case
    {
        const char *name;
        scxt::dsp::InterpolationTypes interp;
    };
    static const std::array<Case, 3> cases{{
        {"loop-fwd-fade-linear", scxt::dsp::InterpolationTypes::Linear},
        {"loop-fwd-fade-zoh", scxt::dsp::InterpolationTypes::ZeroOrderHold},
        {"loop-fwd-fade-zohaa", scxt::dsp::InterpolationTypes::ZOHAA},
    }};

    static const std::array<std::vector<float>, 3> expected{{
        // loop-fwd-fade-linear
        {0.243457705f,  0.454551131f,  0.607087731f,  0.684822917f,  0.683175862f,  0.597655058f,
         0.397246629f,  0.150383502f,  -0.117711894f, -0.401692122f, -0.619019747f, -0.780739427f,
         -0.878268361f, -0.884806752f, -0.825249314f, -0.721027076f, -0.569344699f, -0.406770229f,
         -0.252561539f, -0.158650249f, -0.078456163f, -0.041032746f, -0.084072173f, -0.121421076f,
         -0.173649549f, -0.241743997f, -0.267596394f, -0.264851004f, -0.228573456f, -0.123699367f,
         -0.020599408f, 0.168024197f,  0.389288306f,  0.562156081f,  0.669860423f,  0.695708990f,
         0.631615520f,  0.494666576f,  0.299292713f,  0.063192666f,  -0.178595886f, -0.400478840f,
         -0.570814669f, -0.670724392f, -0.692886055f, -0.630065501f, -0.485894352f, -0.286000222f,
         -0.051500116f, 0.190007865f,  0.407941759f,  0.579263628f,  0.675843596f,  0.689406097f,
         0.623054981f,  0.442154437f,  0.202310115f,  -0.062795617f, -0.326835483f, -0.578430653f,
         -0.751012981f, -0.857359529f, -0.893714011f, -0.845781088f, -0.741651714f, -0.610380769f,
         -0.448736548f, -0.292590857f, -0.158964366f, -0.100484975f, -0.055357136f, -0.051236112f,
         -0.117781743f, -0.167450890f, -0.219373703f, -0.265380472f, -0.266720891f, -0.234933674f,
         -0.170035526f, -0.036048088f, 0.112014085f,  0.342549741f,  0.529044211f,  0.650166333f,
         0.695923328f,  0.656408012f,  0.532109201f,  0.346672833f,  0.119678453f,  -0.123601101f,
         -0.350959569f, -0.538353324f, -0.656318307f, -0.693579674f, -0.650443733f, -0.527307153f,
         -0.336173445f, -0.106798857f, 0.136557743f,  0.361332417f,  0.542871475f,  0.662009835f,
         0.695636332f,  0.644043446f,  0.517795324f,  0.254236758f,  -0.007879358f, -0.273019552f,
         -0.515714943f, -0.721286416f, -0.840169668f, -0.887548089f, -0.866312921f, -0.771761537f,
         -0.634559929f, -0.490702838f, -0.332620144f, -0.194602907f, -0.090061940f, -0.069681555f,
         -0.058286697f, -0.082003191f, -0.161252275f, -0.212683842f, -0.253983587f, -0.268590719f,
         -0.241293907f, -0.180587977f, -0.091566637f, 0.056003973f,  0.290597409f,  0.492258340f,
         0.630472183f,  0.691073656f,  0.671568215f,  0.569551826f,  0.394052923f,  0.173563451f,
         -0.068606168f, -0.301440299f, -0.497897118f, -0.637225389f, -0.694273293f, -0.666186810f,
         -0.560878277f, -0.386346698f, -0.162097603f, 0.080139786f,  0.314722329f,  0.506479323f,
         0.638471484f,  0.696795106f,  0.665031910f,  0.552021444f,  0.337371141f,  0.047036905f,
         -0.219203591f, -0.466847301f, -0.673137188f, -0.822979748f},
        // loop-fwd-fade-zoh
        {0.136563227f,  0.314727932f,  0.591397524f,  0.669858217f,  0.699789166f,  0.609060884f,
         0.457119524f,  0.271540403f,  0.065336354f,  -0.352828920f, -0.537845612f, -0.691563129f,
         -0.875617325f, -0.899651408f, -0.879998624f, -0.733376503f, -0.624055564f, -0.504687369f,
         -0.385988176f, -0.188042909f, -0.122511536f, -0.084004387f, -0.084663205f, -0.115355864f,
         -0.157120496f, -0.239995927f, -0.264642060f, -0.269213825f, -0.249773487f, -0.137469441f,
         -0.051495194f, 0.000000000f,  0.359871924f,  0.506972969f,  0.617344856f,  0.699156821f,
         0.664669752f,  0.582028747f,  0.457221001f,  0.119673319f,  -0.068612002f, -0.251926512f,
         -0.551842511f, -0.646715701f, -0.694735646f, -0.639946818f, -0.541107297f, -0.403065741f,
         -0.235822901f, 0.136563227f,  0.314727932f,  0.470091254f,  0.669858217f,  0.699789166f,
         0.679021895f,  0.457119524f,  0.271540403f,  0.065336354f,  -0.147455275f, -0.537845612f,
         -0.691563129f, -0.805791795f, -0.899651408f, -0.879998624f, -0.821941078f, -0.624055564f,
         -0.504687369f, -0.385988176f, -0.277755290f, -0.122511536f, -0.084004387f, -0.072387025f,
         -0.115355864f, -0.157120496f, -0.201534793f, -0.264642060f, -0.269213825f, -0.249773487f,
         -0.205209121f, -0.051495194f, 0.000000000f,  0.186698928f,  0.506972969f,  0.617344856f,
         0.682991505f,  0.664669752f,  0.582028747f,  0.457221001f,  0.299288571f,  -0.068612002f,
         -0.251926512f, -0.416989505f, -0.646715701f, -0.694735646f, -0.692423582f, -0.541107297f,
         -0.403065741f, -0.235822901f, -0.051495194f, 0.314727932f,  0.470091254f,  0.591397524f,
         0.699789166f,  0.679021895f,  0.609060884f,  0.271540403f,  0.065336354f,  -0.147455275f,
         -0.352828920f, -0.691563129f, -0.805791795f, -0.875617325f, -0.879998624f, -0.821941078f,
         -0.733376503f, -0.504687369f, -0.385988176f, -0.277755290f, -0.188042909f, -0.084004387f,
         -0.072387025f, -0.084663205f, -0.157120496f, -0.201534793f, -0.239995927f, -0.269213825f,
         -0.249773487f, -0.205209121f, -0.137469441f, 0.000000000f,  0.186698928f,  0.359871924f,
         0.617344856f,  0.682991505f,  0.699156821f,  0.582028747f,  0.457221001f,  0.299288571f,
         0.119673319f,  -0.251926512f, -0.416989505f, -0.551842511f, -0.694735646f, -0.692423582f,
         -0.639946818f, -0.403065741f, -0.235822901f, -0.051495194f, 0.136563227f,  0.470091254f,
         0.591397524f,  0.669858217f,  0.679021895f,  0.609060884f,  0.457119524f,  0.065336354f,
         -0.147455275f, -0.352828920f, -0.537845612f, -0.805791795f},
        // loop-fwd-fade-zohaa
        {0.243701771f,  0.455565512f,  0.609605432f,  0.690663159f,  0.687355042f,  0.600345731f,
         0.402792543f,  0.153508812f,  -0.117711775f, -0.401506603f, -0.621342957f, -0.782283306f,
         -0.881450415f, -0.890391231f, -0.828602374f, -0.723035395f, -0.572643161f, -0.408071041f,
         -0.252561569f, -0.157980070f, -0.076394692f, -0.040027563f, -0.082477421f, -0.119217388f,
         -0.172682121f, -0.241485357f, -0.267758727f, -0.265495479f, -0.228573561f, -0.124589317f,
         -0.030798040f, 0.167327195f,  0.389847994f,  0.565923810f,  0.673425436f,  0.698085666f,
         0.638086677f,  0.499570340f,  0.299293101f,  0.065984592f,  -0.178216726f, -0.401279896f,
         -0.572951913f, -0.676220655f, -0.697008312f, -0.632737279f, -0.491810471f, -0.289658487f,
         -0.051500335f, 0.189220503f,  0.409869611f,  0.580808461f,  0.679272056f,  0.695894301f,
         0.627182186f,  0.444731385f,  0.206737280f,  -0.060982451f, -0.326835573f, -0.579655766f,
         -0.754599631f, -0.859196961f, -0.897300124f, -0.851258099f, -0.744584084f, -0.612107098f,
         -0.450937122f, -0.292897850f, -0.158964187f, -0.099105448f, -0.052893177f, -0.050229490f,
         -0.116315082f, -0.165774539f, -0.218832523f, -0.265371233f, -0.267362326f, -0.235785678f,
         -0.170035660f, -0.040685296f, 0.104299039f,  0.343129724f,  0.530780554f,  0.655265689f,
         0.699949205f,  0.659035265f,  0.538255990f,  0.350719422f,  0.119678810f,  -0.122257218f,
         -0.352284968f, -0.539716423f, -0.659441113f, -0.699884832f, -0.654624999f, -0.529968202f,
         -0.341170162f, -0.109219946f, 0.136557713f,  0.362098187f,  0.546343446f,  0.663973212f,
         0.699727178f,  0.650709450f,  0.521571755f,  0.256514341f,  -0.004859393f, -0.272608638f,
         -0.515715122f, -0.723782659f, -0.844715357f, -0.889537096f, -0.870032549f, -0.776757896f,
         -0.636896729f, -0.492052287f, -0.333672673f, -0.193999991f, -0.090061739f, -0.067811564f,
         -0.055698540f, -0.081089303f, -0.160060421f, -0.211642712f, -0.253862888f, -0.268805325f,
         -0.242244631f, -0.181469291f, -0.091566727f, 0.044248462f,  0.291307658f,  0.493426502f,
         0.633259296f,  0.697135091f,  0.675763547f,  0.572239399f,  0.399429947f,  0.176459819f,
         -0.068606041f, -0.301641762f, -0.500831127f, -0.639051676f, -0.698155403f, -0.672844112f,
         -0.564815760f, -0.388804227f, -0.165812910f, 0.079131290f,  0.314722508f,  0.508742750f,
         0.643235981f,  0.699034810f,  0.669488728f,  0.558382332f,  0.340472221f,  0.048858002f,
         -0.217765599f, -0.467822224f, -0.673137665f, -0.826519489f},
    }};

    for (size_t ci = 0; ci < cases.size(); ++ci)
    {
        GenConfig c;
        c.name = cases[ci].name;
        c.loopFade = gFade;
        c.ratio = (int32_t)(unityRatio * 1.3); // non integer so the kernels actually interpolate
        c.interp = cases[ci].interp;

        GenHarness h(c);
        goldenCheckOrPrint(c.name, capture(c, h), expected[ci]);
    }
}

TEST_CASE("Generator golden - int16 sample data", "[generator][golden]")
{
    // loop-fwd-fade-i16
    static const std::vector<float> expected{
        0.244809553f,  0.455757529f,  0.610600889f,  0.690840721f,  0.687198400f,  0.599847138f,
        0.400819868f,  0.152186096f,  -0.116970979f, -0.402961791f, -0.622119904f, -0.782237947f,
        -0.881562769f, -0.890019059f, -0.828306973f, -0.722441852f, -0.570699394f, -0.406885028f,
        -0.252952397f, -0.156919971f, -0.075859733f, -0.039860442f, -0.082320303f, -0.119234227f,
        -0.172570676f, -0.241583213f, -0.267814815f, -0.265332043f, -0.229074642f, -0.120737746f,
        -0.032754876f, 0.164379448f,  0.391186774f,  0.567143261f,  0.673273861f,  0.697955549f,
        0.636979043f,  0.498516977f,  0.299874783f,  0.064190485f,  -0.179348215f, -0.401514143f,
        -0.574147999f, -0.676541865f, -0.696856022f, -0.632308662f, -0.489958286f, -0.288337171f,
        -0.052197210f, 0.190906584f,  0.410813391f,  0.580876291f,  0.679690123f,  0.695685029f,
        0.626987815f,  0.444075465f,  0.204455927f,  -0.062327188f, -0.326072335f, -0.580822945f,
        -0.755151808f, -0.859013200f, -0.896890700f, -0.850577176f, -0.744270861f, -0.611455441f,
        -0.448947906f, -0.291775346f, -0.159312665f, -0.098296881f, -0.052505929f, -0.050120965f,
        -0.116345458f, -0.165872484f, -0.218716905f, -0.265430480f, -0.267315805f, -0.235327676f,
        -0.169174775f, -0.044879861f, 0.101526625f,  0.345335186f,  0.532172918f,  0.655679941f,
        0.699799120f,  0.658680677f,  0.536588132f,  0.349467605f,  0.120349780f,  -0.123983204f,
        -0.353290796f, -0.539833546f, -0.660081089f, -0.699810088f, -0.654438615f, -0.529344022f,
        -0.338932842f, -0.107813060f, 0.135822430f,  0.363550663f,  0.547061086f,  0.663914144f,
        0.699548244f,  0.650108576f,  0.521366835f,  0.255750686f,  -0.007276929f, -0.273862213f,
        -0.514983773f, -0.724637449f, -0.844992220f, -0.889241219f, -0.869140744f, -0.775826275f,
        -0.636580944f, -0.491396755f, -0.331729621f, -0.193013579f, -0.090351462f, -0.067246869f,
        -0.055470664f, -0.080998361f, -0.160234824f, -0.211748466f, -0.253743768f, -0.268848896f,
        -0.241552308f, -0.182106540f, -0.093230799f, 0.045047052f,  0.296435684f,  0.492793173f,
        0.634145200f,  0.697210252f,  0.675576508f,  0.571683586f,  0.397316486f,  0.175090209f,
        -0.067865387f, -0.303203076f, -0.501635015f, -0.639050364f, -0.698185563f, -0.672381639f,
        -0.564597309f, -0.388043970f, -0.163339019f, 0.080512121f,  0.314005703f,  0.509873271f,
        0.643691480f,  0.698854268f,  0.668721259f,  0.557426572f,  0.340296686f,  0.048024435f,
        -0.220095530f, -0.468920946f, -0.672490060f, -0.826970518f};

    GenConfig c;
    c.name = "loop-fwd-fade-i16";
    c.isFloat = false;
    c.loopFade = gFade;
    c.ratio = (int32_t)(unityRatio * 1.3);

    GenHarness h(c);
    goldenCheckOrPrint(c.name, capture(c, h), expected);
}

// named rather than anonymous so the unity build cannot collide with other files' helpers
namespace generator_test
{
namespace dsp = scxt::dsp;

// one mono float generator looping over a buffer with the zero pads a real sample has
struct LoopedGenerator
{
    static constexpr int pad{dsp::FIRoffset};
    std::vector<float> buffer;
    dsp::GeneratorState GD;
    dsp::GeneratorIO IO;
    float outL[2 * scxt::blockSize]{}, outR[2 * scxt::blockSize]{};
    bool forward{true};
    bool whileGated{false};

    LoopedGenerator(int fileLength, int loopStart, int loopLength, bool fwd = true)
        : buffer(pad + fileLength + pad, 0.f), forward(fwd)
    {
        dsp::sincTable.init();

        IO.sampleDataL = sample();
        IO.sampleDataR = sample();
        IO.outputL = outL;
        IO.outputR = outR;
        IO.waveSize = fileLength;

        GD.samplePos = loopStart;
        GD.playbackLowerBound = 0;
        GD.playbackUpperBound = fileLength;
        GD.loopLowerBound = loopStart;
        GD.loopUpperBound = loopStart + loopLength;
        GD.loopInvertedBounds = 1.f / loopLength;
        GD.loopDirection = 1;
        GD.directionAtOutset = 1;
        GD.isFinished = false;
        GD.gated = true;
    }

    float *sample() { return buffer.data() + pad; }

    void fillWithSine(int cycleLength)
    {
        for (int i = 0; i < IO.waveSize; ++i)
            sample()[i] = std::sin(2.0 * M_PI * i / cycleLength);
    }

    // the output then traces the playhead
    void fillWithRamp()
    {
        for (int i = 0; i < IO.waveSize; ++i)
            sample()[i] = i * 1e-3f;
    }

    void setRatio(double r) { GD.ratio = (int64_t)std::llround(r * (1 << 24)); }

    std::vector<float> render(int samples)
    {
        auto gen = dsp::GetFPtrGeneratorSample(false, true, true, forward, whileGated);
        std::vector<float> res;
        while ((int)res.size() < samples)
        {
            gen(&GD, &IO);
            res.insert(res.end(), outL, outL + GD.blockSize);
        }
        res.resize(samples);
        return res;
    }
};

// upward crossings of the mean, which is the fundamental for the simple shapes used here
double cyclesPerSample(const std::vector<float> &v, int skip)
{
    double mean{0};
    for (int i = skip; i < (int)v.size(); ++i)
        mean += v[i];
    mean /= (v.size() - skip);

    int crossings{0};
    for (int i = skip + 1; i < (int)v.size(); ++i)
        if (v[i - 1] <= mean && v[i] > mean)
            crossings++;
    return (double)crossings / (v.size() - skip);
}

struct RootZeroZone
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Zone *zone{nullptr};

    RootZeroZone()
    {
        eng.reset(makeEngine());
        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();

        auto z = std::make_unique<scxt::engine::Zone>();
        z->mapping.keyboardRange = {0, 127};
        z->mapping.velocityRange = {0, 127};
        z->mapping.rootKey = 0;
        z->initialize();
        part.getGroup(0)->addZone(z);
        zone = part.getGroup(0)->getZone(0).get();

        auto p = samplePath("WavStereo48k.wav");
        REQUIRE(std::filesystem::exists(p));

        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        auto sid = eng->getSampleManager()->loadSampleByPath(p);
        REQUIRE(sid.has_value());
        zone->variantData.variants[0].sampleID = *sid;
        zone->variantData.variants[0].active = true;
        REQUIRE(zone->attachToSample(*eng->getSampleManager(), 0,
                                     scxt::engine::Zone::SampleInformationRead::ENDPOINTS));
    }

    double ratioForKey(int key)
    {
        eng->processNoteOnEvent(0, 0, key, -1, 1.f, 0.f);
        eng->processAudio();

        scxt::voice::Voice *newest{nullptr};
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned &&
                (!newest || v->voiceCreationId > newest->voiceCreationId))
                newest = v;
        }
        REQUIRE(newest);
        REQUIRE(newest->numGeneratorsActive == 1);
        auto res = (double)newest->GD[0].ratio;

        eng->processNoteOffEvent(0, 0, key, -1, 0.f);
        for (int i = 0; i < 200; ++i)
            eng->processAudio();
        return res;
    }
};

TEST_CASE("A single cycle keeps rising past seven octaves up", "[generator]")
{
    static constexpr int cycle{2048};

    SECTION("Zero order hold runs at the output rate")
    {
        LoopedGenerator g(cycle, 0, cycle);
        g.fillWithSine(cycle);
        g.GD.interpolationType = dsp::InterpolationTypes::ZeroOrderHold;
        g.setRatio(200);
        REQUIRE(cyclesPerSample(g.render(1 << 16), 512) == Approx(200.0 / cycle).epsilon(0.005));
    }

    SECTION("Sinc with the doubled block of an oversampled voice")
    {
        LoopedGenerator g(cycle, 0, cycle);
        g.fillWithSine(cycle);
        g.GD.blockSize = 2 * scxt::blockSize;
        g.setRatio(150);
        REQUIRE(cyclesPerSample(g.render(1 << 16), 512) == Approx(150.0 / cycle).epsilon(0.005));
    }
}

TEST_CASE("A single cycle shorter than the interpolator loops at pitch", "[generator]")
{
    auto cycle = GENERATE(8, 16, 23);
    INFO("cycle " << cycle);

    LoopedGenerator g(cycle, 0, cycle);
    g.fillWithSine(cycle);
    g.setRatio(0.1 * cycle);
    auto out = g.render(1 << 14);

    REQUIRE(cyclesPerSample(out, 256) == Approx(0.1).epsilon(0.005));

    float peak{0};
    for (auto s : out)
        peak = std::max(peak, std::fabs(s));
    REQUIRE(peak < 2.f);
}

TEST_CASE("A ping-pong loop plays at its true pitch", "[generator]")
{
    static constexpr int loopLength{64};

    // neither ratio divides the loop, so a stepped period shows
    auto ratio = GENERATE(5.5, 28.8);
    auto gated = GENERATE(false, true);
    INFO("ratio " << ratio << " gated " << gated);

    LoopedGenerator g(4096, 1024, loopLength, false);
    g.fillWithRamp();
    g.whileGated = gated;
    g.setRatio(ratio);
    REQUIRE(cyclesPerSample(g.render(1 << 16), 512) ==
            Approx(ratio / (2 * loopLength)).epsilon(0.005));
}

TEST_CASE("A ping-pong loop turns around at its bounds", "[generator]")
{
    LoopedGenerator g(4096, 1024, 64, false);
    g.fillWithRamp();
    g.setRatio(28.8);
    auto out = g.render(1 << 12);

    // a sample or two of slack for the interpolator reading behind the playhead
    auto [lo, hi] = std::minmax_element(out.begin() + 64, out.end());
    REQUIRE(*lo > 1021e-3f);
    REQUIRE(*hi < 1090e-3f);
}

TEST_CASE("A ping-pong loop counts each return to where it set out", "[generator]")
{
    LoopedGenerator g(4096, 1024, 64, false);
    g.fillWithRamp();

    // back at the start every 128 samples
    g.render(5 * 128 + 8);
    REQUIRE(g.GD.loopCount == 5);
}

TEST_CASE("A ping-pong loop counts every return when one step crosses it several times",
          "[generator]")
{
    static constexpr int loopLength{4};
    auto outset = GENERATE(1, -1);
    INFO("outset " << outset);

    LoopedGenerator g(4096, 1024, loopLength, false);
    g.fillWithRamp();
    g.setRatio(10.3);
    g.GD.loopDirection = outset;
    g.GD.directionAtOutset = outset;
    g.GD.samplePos = outset > 0 ? g.GD.loopLowerBound : g.GD.loopUpperBound;
    // already in the loop, so skip the not yet looped -1
    g.GD.loopCount = 0;

    static constexpr int steps{256};
    g.render(steps);

    // back at the outset bound after every whole round trip travelled
    auto travelled = (int64_t)steps * g.GD.ratio;
    REQUIRE(g.GD.loopCount == travelled / ((int64_t)2 * loopLength << 24));
}

TEST_CASE("A ping-pong playhead outside its loop heads back rather than jumping in", "[generator]")
{
    LoopedGenerator g(4096, 1024, 64, false);
    g.fillWithRamp();
    g.GD.samplePos = 2000;
    auto out = g.render(2048);

    REQUIRE(out[16] > 1.9f);
    REQUIRE(out[256] < out[16]);

    auto [lo, hi] = std::minmax_element(out.begin() + 1024, out.end());
    REQUIRE(*lo > 1021e-3f);
    REQUIRE(*hi < 1090e-3f);
}

TEST_CASE("A key eight octaves above the root keeps its full ratio", "[generator]")
{
    RootZeroZone f;
    auto twoOctaves = f.ratioForKey(24);
    REQUIRE(twoOctaves > 0);
    REQUIRE(f.ratioForKey(96) == Approx(64.0 * twoOctaves).epsilon(1e-5));
}
} // namespace generator_test
