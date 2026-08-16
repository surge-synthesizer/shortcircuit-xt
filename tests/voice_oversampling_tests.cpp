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
 * The voice runs its generators at twice the block rate in two quite different situations. The
 * group can ask for it, which oversamples the whole voice and is carried by the OS template
 * argument on processWithOS. Or a single voice can reach for it on its own when a note is
 * pitched far enough up that the generator would alias, in which case the generator alone runs
 * long and halfRate decimates it back at the bottom of the generator loop.
 *
 * Only the first of those is visible to the OS template argument, so everything between the
 * generator and that decimation - the variant's normalization, amplitude and pan, and the fold
 * of a unison stack down into the voice's output - has to be sized by whether this voice is
 * oversampling, not by whether its group is.
 *
 * These cases press an octave above the root key, which is well past the rate threshold, with
 * group oversampling off: the one arrangement where the two ideas of oversampling disagree.
 * The controls hold the same variant settings under group oversampling and at the root key,
 * where they agree.
 */

#include "catch2/catch2.hpp"

#include <cmath>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
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
// long enough that the sample is well past whatever its file opens with
constexpr int renderedBlocks{64};

struct Render
{
    std::vector<float> l, r;
    int blockLen{scxt::blockSize};
};

struct OversampleFixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Group *group{nullptr};
    Zone *zone{nullptr};
    bool groupOversample;

    OversampleFixture(bool oversample, int nVariants) : groupOversample(oversample)
    {
        eng.reset(makeEngine());

        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();
        group = part.getGroup(0).get();
        group->outputInfo.oversample = oversample;

        auto z = std::make_unique<Zone>();
        z->mapping.keyboardRange = {0, 127};
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

        for (int i = 0; i < nVariants; ++i)
        {
            zone->variantData.variants[i].sampleID = *sid;
            zone->variantData.variants[i].active = true;
            // ENDPOINTS only - MAPPING would let the wav's chunks overwrite our key range
            REQUIRE(zone->attachToSample(*eng->getSampleManager(), i,
                                         Zone::SampleInformationRead::ENDPOINTS));
            // the attach rewrites the variant out of the file, so pin the gains afterwards
            // and let each case move exactly the one it is about
            zone->variantData.variants[i].normalizationAmplitude = 1.f;
            zone->variantData.variants[i].amplitude = 1.f;
            zone->variantData.variants[i].pan = 0.f;
        }
        REQUIRE(zone->getNumSampleLoaded() == nVariants);

        // unison throughout, which makes the generator count the variant count - so a one
        // variant fixture and a stacked one differ in the stack and nothing else
        zone->variantData.variantPlaybackMode = Zone::UNISON;
    }

    // exactly one note is pressed per fixture, so there is only ever one
    scxt::voice::Voice *onlyVoice() const
    {
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned)
                return v;
        }
        return nullptr;
    }

    /*
     * Hold a key down and collect the voice's own output block by block. The voice buffer is
     * the last thing written before the group reads it, so it is the narrowest place to look:
     * everything downstream is a scalar gain and would hide nothing but would add its own.
     */
    Render render(int key, int nBlocks)
    {
        Render out;
        out.blockLen = scxt::blockSize << (groupOversample ? 1 : 0);

        eng->processNoteOnEvent(0, 0, key, -1, 1.f, 0.f);
        for (int b = 0; b < nBlocks; ++b)
        {
            eng->processAudio();
            auto *v = onlyVoice();
            REQUIRE(v);
            for (int i = 0; i < out.blockLen; ++i)
            {
                out.l.push_back(v->output[0][i]);
                out.r.push_back(v->output[1][i]);
            }
        }
        eng->processNoteOffEvent(0, 0, key, -1, 0.f);
        return out;
    }
};

float peakOf(const Render &r)
{
    float p{0.f};
    for (size_t i = 0; i < r.l.size(); ++i)
        p = std::max({p, std::fabs(r.l[i]), std::fabs(r.r[i])});
    return p;
}

/*
 * A comparison across silence proves nothing, so insist the reference sounds - and in the top
 * half of each block specifically, since that is the half a mis-sized loop leaves alone.
 */
void requireAudible(const Render &r)
{
    float lo{0.f}, hi{0.f};
    for (size_t i = 0; i < r.l.size(); ++i)
    {
        auto a = std::max(std::fabs(r.l[i]), std::fabs(r.r[i]));
        if ((int)(i % r.blockLen) < r.blockLen / 2)
            lo = std::max(lo, a);
        else
            hi = std::max(hi, a);
    }
    INFO("peak " << lo << " in the bottom half of the block, " << hi << " in the top half");
    REQUIRE(lo > 1e-3f);
    REQUIRE(hi > 1e-3f);
}

struct Divergence
{
    int index{-1};
    std::string what;
};

/*
 * Everything the voice does after the generator is a scalar gain, so a change made to every
 * sample of the generator's block reaches the output as a plain scale. Anything that does not
 * is a change which reached only part of the block, and the index it first shows up at says
 * which part.
 */
Divergence divergenceFrom(const Render &cut, const Render &ref, float scale, float tol = 1e-4f)
{
    Divergence d;
    REQUIRE(cut.l.size() == ref.l.size());

    auto bound = tol * peakOf(ref);
    for (size_t i = 0; i < ref.l.size(); ++i)
    {
        auto el = std::fabs(cut.l[i] - scale * ref.l[i]);
        auto er = std::fabs(cut.r[i] - scale * ref.r[i]);
        if (el > bound || er > bound)
        {
            std::ostringstream oss;
            oss << "diverges at sample " << i << ", which is index " << (i % ref.blockLen)
                << " of a " << ref.blockLen << " sample block: got L " << cut.l[i] << " R "
                << cut.r[i] << ", wanted " << scale << " x (L " << ref.l[i] << " R " << ref.r[i]
                << ")";
            d.index = (int)i;
            d.what = oss.str();
            return d;
        }
    }
    return d;
}

// As above for a channel which should hold nothing at all.
Divergence firstSoundIn(const std::vector<float> &ch, int blockLen, float bound)
{
    Divergence d;
    for (size_t i = 0; i < ch.size(); ++i)
    {
        if (std::fabs(ch[i]) > bound)
        {
            std::ostringstream oss;
            oss << "sounds at sample " << i << ", which is index " << (i % blockLen) << " of a "
                << blockLen << " sample block: " << ch[i] << " against a bound of " << bound;
            d.index = (int)i;
            d.what = oss.str();
            return d;
        }
    }
    return d;
}
} // namespace

TEST_CASE("Variant amplitude scales the whole block when a pitched up voice oversamples",
          "[oversample]")
{
    OversampleFixture ref(false, 1);
    auto flat = ref.render(72, renderedBlocks);
    requireAudible(flat);

    OversampleFixture cut(false, 1);
    cut.zone->variantData.variants[0].amplitude = 0.5f;
    auto scaled = cut.render(72, renderedBlocks);

    // the variant amplitude is cubed on its way to the generator output
    auto d = divergenceFrom(scaled, flat, 0.125f);
    INFO(d.what);
    REQUIRE(d.index == -1);
}

TEST_CASE("A hard panned variant empties the other channel when a pitched up voice oversamples",
          "[oversample]")
{
    OversampleFixture f(false, 1);
    f.zone->variantData.variants[0].pan = 1.f;
    auto r = f.render(72, renderedBlocks);

    // equal power panned hard right leaves no path from either input to the left output
    auto peak = peakOf(r);
    REQUIRE(peak > 1e-3f);

    auto d = firstSoundIn(r.l, r.blockLen, 1e-4f * peak);
    INFO(d.what);
    REQUIRE(d.index == -1);
}

TEST_CASE("A unison stack accumulates every generator across the whole block", "[oversample]")
{
    OversampleFixture one(false, 1);
    auto single = one.render(72, renderedBlocks);
    requireAudible(single);

    OversampleFixture two(false, 2);
    auto stacked = two.render(72, renderedBlocks);

    // two copies of one file at one pitch, so the stack is the single voice twice over
    auto d = divergenceFrom(stacked, single, 2.f);
    INFO(d.what);
    REQUIRE(d.index == -1);
}

TEST_CASE("Every generator in a voice runs at one block length", "[oversample]")
{
    OversampleFixture f(false, 2);
    // straddle the rate threshold: at the root key the first variant is under it and the
    // second, an octave up, is well over
    f.zone->variantData.variants[1].pitchOffset = 12.f;

    f.eng->processNoteOnEvent(0, 0, 60, -1, 1.f, 0.f);
    f.eng->processAudio();

    auto *v = f.onlyVoice();
    REQUIRE(v);
    REQUIRE(v->numGeneratorsActive == 2);

    /*
     * The generators share one output buffer and one decimation, so they cannot disagree about
     * how long a block is. Whichever way the voice decides, it decides once.
     */
    INFO("generator blocks " << v->GD[0].blockSize << " and " << v->GD[1].blockSize
                             << ", voice oversampling " << v->useOversampling);
    REQUIRE(v->GD[0].blockSize == v->GD[1].blockSize);
    REQUIRE(v->GD[0].blockSize == scxt::blockSize * (v->useOversampling ? 2 : 1));
}

/*
 * The controls. Group oversampling and the root key are the two arrangements where the voice's
 * block length and its group's already agree, so nothing above should have moved them.
 */
TEST_CASE("Variant gain and pan are unchanged when the group oversamples", "[oversample]")
{
    SECTION("amplitude")
    {
        OversampleFixture ref(true, 1);
        auto flat = ref.render(72, renderedBlocks);
        requireAudible(flat);

        OversampleFixture cut(true, 1);
        cut.zone->variantData.variants[0].amplitude = 0.5f;
        auto scaled = cut.render(72, renderedBlocks);

        auto d = divergenceFrom(scaled, flat, 0.125f);
        INFO(d.what);
        REQUIRE(d.index == -1);
    }

    SECTION("pan")
    {
        OversampleFixture f(true, 1);
        f.zone->variantData.variants[0].pan = 1.f;
        auto r = f.render(72, renderedBlocks);

        auto peak = peakOf(r);
        REQUIRE(peak > 1e-3f);

        auto d = firstSoundIn(r.l, r.blockLen, 1e-4f * peak);
        INFO(d.what);
        REQUIRE(d.index == -1);
    }

    SECTION("unison stack")
    {
        OversampleFixture one(true, 1);
        auto single = one.render(72, renderedBlocks);
        requireAudible(single);

        OversampleFixture two(true, 2);
        auto stacked = two.render(72, renderedBlocks);

        auto d = divergenceFrom(stacked, single, 2.f);
        INFO(d.what);
        REQUIRE(d.index == -1);
    }
}

TEST_CASE("Variant gain and pan are unchanged below the oversampling threshold", "[oversample]")
{
    SECTION("amplitude")
    {
        OversampleFixture ref(false, 1);
        auto flat = ref.render(60, renderedBlocks);
        requireAudible(flat);

        OversampleFixture cut(false, 1);
        cut.zone->variantData.variants[0].amplitude = 0.5f;
        auto scaled = cut.render(60, renderedBlocks);

        auto d = divergenceFrom(scaled, flat, 0.125f);
        INFO(d.what);
        REQUIRE(d.index == -1);
    }

    SECTION("pan")
    {
        OversampleFixture f(false, 1);
        f.zone->variantData.variants[0].pan = 1.f;
        auto r = f.render(60, renderedBlocks);

        auto peak = peakOf(r);
        REQUIRE(peak > 1e-3f);

        auto d = firstSoundIn(r.l, r.blockLen, 1e-4f * peak);
        INFO(d.what);
        REQUIRE(d.index == -1);
    }

    SECTION("unison stack")
    {
        OversampleFixture one(false, 1);
        auto single = one.render(60, renderedBlocks);
        requireAudible(single);

        OversampleFixture two(false, 2);
        auto stacked = two.render(60, renderedBlocks);

        auto d = divergenceFrom(stacked, single, 2.f);
        INFO(d.what);
        REQUIRE(d.index == -1);
    }
}

TEST_CASE("A voice at the root key does not reach for oversampling", "[oversample]")
{
    // the premise the controls above rest on - if this ever stopped holding they would stop
    // being controls without saying so
    OversampleFixture f(false, 1);
    f.eng->processNoteOnEvent(0, 0, 60, -1, 1.f, 0.f);
    f.eng->processAudio();

    auto *v = f.onlyVoice();
    REQUIRE(v);
    REQUIRE(!v->useOversampling);
    REQUIRE(v->GD[0].blockSize == scxt::blockSize);
}

TEST_CASE("A voice an octave up reaches for oversampling on its own", "[oversample]")
{
    // and the premise the failing cases rest on
    OversampleFixture f(false, 1);
    f.eng->processNoteOnEvent(0, 0, 72, -1, 1.f, 0.f);
    f.eng->processAudio();

    auto *v = f.onlyVoice();
    REQUIRE(v);
    REQUIRE(!v->forceOversample);
    REQUIRE(v->useOversampling);
    REQUIRE(v->GD[0].blockSize == scxt::blockSize * 2);
}
