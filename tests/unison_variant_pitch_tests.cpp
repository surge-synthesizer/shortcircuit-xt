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

/*
 * Per-variant pitch offset, as it reaches the generator rate in
 * Voice::calculateGeneratorRatio.
 *
 * A UNISON zone stacks every active variant into one voice with a generator each, so each
 * generator has to take its offset from the variant it is actually playing. Every other
 * playback mode picks one variant per note and runs a single generator, which makes them the
 * control: the variant the tactic chose is the variant whose offset must show up in the rate,
 * and that has to stay true.
 *
 * The rate also carries the sample's own rate factor, so every variant here holds the same
 * file - then the only thing that can move one generator away from another is its offset.
 */

#include "catch2/catch2.hpp"

#include <cmath>
#include <filesystem>
#include <memory>
#include <vector>

#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "messaging/messaging.h"
#include "voice/voice.h"

#include "test_utils.h"

namespace fs = std::filesystem;

using Zone = scxt::engine::Zone;

namespace
{
struct VariantPitchFixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Group *group{nullptr};
    Zone *zone{nullptr};
    std::vector<float> offsets;

    VariantPitchFixture(Zone::VariantPlaybackMode mode, const std::vector<float> &pitchOffsets)
        : offsets(pitchOffsets)
    {
        eng.reset(makeEngine());

        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();
        group = part.getGroup(0).get();
        // pinned rather than assumed: oversampling halves every ratio, and the cases below
        // compare ratios taken from two separate fixtures
        group->outputInfo.oversample = true;

        auto z = std::make_unique<Zone>();
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

        for (size_t i = 0; i < offsets.size(); ++i)
        {
            zone->variantData.variants[i].sampleID = *sid;
            zone->variantData.variants[i].active = true;
            // ENDPOINTS only - MAPPING would let the wav's chunks overwrite our key range
            REQUIRE(zone->attachToSample(*eng->getSampleManager(), (int)i,
                                         Zone::SampleInformationRead::ENDPOINTS));
            // after the attach, which rewrites the variant out of the file
            zone->variantData.variants[i].pitchOffset = offsets[i];
        }
        REQUIRE(zone->getNumSampleLoaded() == (int)offsets.size());

        zone->variantData.variantPlaybackMode = mode;
    }

    void noteOn(int key) { eng->processNoteOnEvent(0, 0, key, -1, 1.f, 0.f); }
    void noteOff(int key) { eng->processNoteOffEvent(0, 0, key, -1, 0.f); }
    void runBlocks(int n)
    {
        for (int i = 0; i < n; ++i)
            eng->processAudio();
    }

    // The voice started most recently. A released voice still rings, so pressing the same key
    // repeatedly leaves older voices assigned alongside the one we just made.
    scxt::voice::Voice *newestVoice() const
    {
        scxt::voice::Voice *res{nullptr};
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned && (!res || v->voiceCreationId > res->voiceCreationId))
                res = v;
        }
        return res;
    }
};

/*
 * Press the root key once and report every generator's rate. The rate is computed inside
 * Voice::process, so a block has to run before there is anything to read.
 */
std::vector<double> ratiosForOneNote(VariantPitchFixture &f, int expectedGenerators)
{
    f.noteOn(60);
    f.runBlocks(1);

    auto *v = f.newestVoice();
    REQUIRE(v);
    REQUIRE(v->numGeneratorsActive == expectedGenerators);

    std::vector<double> out;
    for (int i = 0; i < expectedGenerators; ++i)
        out.push_back((double)v->GD[i].ratio);

    f.noteOff(60);
    return out;
}

/*
 * Press the root key n times and record the one generator's rate under whichever variant that
 * press chose. A mode which does not reach every variant in n presses leaves a zero behind.
 */
std::vector<double> ratioPerVariant(VariantPitchFixture &f, int nPresses)
{
    std::vector<double> out(f.offsets.size(), 0.0);

    for (int i = 0; i < nPresses; ++i)
    {
        f.noteOn(60);
        f.runBlocks(1);

        auto *v = f.newestVoice();
        REQUIRE(v);
        // one variant per note in every mode but unison
        REQUIRE(v->numGeneratorsActive == 1);
        REQUIRE(v->sampleIndex >= 0);
        REQUIRE(v->sampleIndex < (int)f.offsets.size());

        auto r = (double)v->GD[0].ratio;
        INFO("press " << i << " chose variant " << (int)v->sampleIndex);
        if (out[v->sampleIndex] != 0.0)
            REQUIRE(r == Approx(out[v->sampleIndex]).epsilon(1e-9));
        out[v->sampleIndex] = r;

        f.noteOff(60);
        f.runBlocks(1);
    }
    return out;
}

double semitonesUp(double base, float semis) { return base * std::pow(2.0, semis / 12.0); }
} // namespace

/*
 * The headline case. Three variants an octave apart in each direction, so a generator reading
 * the wrong variant's offset is out by a factor of two rather than by a rounding error.
 */
TEST_CASE("A unison stack detunes each variant by its own pitch offset", "[variants]")
{
    VariantPitchFixture f(Zone::UNISON, {0.f, 12.f, -12.f});
    auto r = ratiosForOneNote(f, 3);

    INFO("ratios " << r[0] << " / " << r[1] << " / " << r[2]);
    REQUIRE(r[0] > 0);
    REQUIRE(r[1] / r[0] == Approx(2.0).epsilon(1e-5));
    REQUIRE(r[2] / r[0] == Approx(0.5).epsilon(1e-5));
}

/*
 * The same thing said without a closed form: unison generator i has to land on exactly the
 * rate variant i gets when the round robin hands it a note of its own. This is also the
 * control - the round robin half of it holds before and after the unison fix.
 */
TEST_CASE("A unison generator plays at the rate its variant gets alone", "[variants]")
{
    const std::vector<float> offsets{0.f, 7.f, -5.f};

    VariantPitchFixture rr(Zone::FORWARD_RR, offsets);
    // forward RR walks 1, 2, 0 from a fresh zone, so one press per variant plus one over
    auto alone = ratioPerVariant(rr, (int)offsets.size() + 1);

    // round robin is right on its own terms first, else the cross check below is circular
    REQUIRE(alone[0] > 0);
    REQUIRE(alone[1] == Approx(semitonesUp(alone[0], offsets[1])).epsilon(1e-5));
    REQUIRE(alone[2] == Approx(semitonesUp(alone[0], offsets[2])).epsilon(1e-5));

    VariantPitchFixture uni(Zone::UNISON, offsets);
    auto stacked = ratiosForOneNote(uni, (int)offsets.size());

    for (size_t i = 0; i < offsets.size(); ++i)
    {
        INFO("variant " << i << " offset " << offsets[i]);
        REQUIRE(stacked[i] == Approx(alone[i]).epsilon(1e-6));
    }
}

/*
 * The control proper. Every mode which picks one variant per note runs a single generator, and
 * that generator's rate has to follow the variant the tactic chose - which is what the unison
 * fix must not disturb.
 */
TEST_CASE("Single variant playback modes rate each variant by its own offset", "[variants]")
{
    const std::vector<float> offsets{0.f, 3.f, -2.f, 9.f};

    auto mode =
        GENERATE(Zone::FORWARD_RR, Zone::TRUE_RANDOM, Zone::RANDOM_NOREPEAT, Zone::RANDOM_CYCLE);

    VariantPitchFixture f(mode, offsets);
    INFO("playback mode " << (int)mode);

    // enough presses that even TRUE_RANDOM reaches all four with room to spare
    auto rates = ratioPerVariant(f, 60);

    for (size_t i = 0; i < offsets.size(); ++i)
    {
        INFO("variant " << i << " offset " << offsets[i]);
        REQUIRE(rates[i] > 0);
        REQUIRE(rates[i] == Approx(semitonesUp(rates[0], offsets[i])).epsilon(1e-5));
    }
}

/*
 * And the round robin order itself, so a change to the rate path that also perturbed variant
 * selection would not slip through the rate assertions above.
 */
TEST_CASE("Forward round robin still walks the variants in order under unison detune", "[variants]")
{
    VariantPitchFixture f(Zone::FORWARD_RR, {0.f, 12.f, -12.f});

    std::vector<int> seen;
    for (int i = 0; i < 9; ++i)
    {
        f.noteOn(60);
        f.runBlocks(1);
        auto *v = f.newestVoice();
        REQUIRE(v);
        seen.push_back(v->sampleIndex);
        f.noteOff(60);
        f.runBlocks(1);
    }

    for (size_t i = 1; i < seen.size(); ++i)
    {
        INFO("press " << i);
        REQUIRE(seen[i] == (seen[i - 1] + 1) % 3);
    }
}
