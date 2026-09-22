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
#include <filesystem>
#include <functional>
#include <memory>

#include "engine/engine.h"
#include "engine/part.h"
#include "engine/group.h"
#include "engine/zone.h"
#include "engine/patch.h"
#include "messaging/messaging.h"

#include "test_utils.h"

namespace
{
constexpr float upperBoundDB{12.f};

// the top of a asCubicDecibelAttenuationWithUpperDBBound control, as ParamMetaData computes it
float topOfRange(float db) { return std::cbrt(std::pow(10.f, db / 20.f)); }

float amplitudeRatio(float db) { return std::pow(10.f, db / 20.f); }

struct GainFixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Zone *zone{nullptr};

    GainFixture()
    {
        eng.reset(makeEngine());
        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();

        auto z = std::make_unique<scxt::engine::Zone>();
        z->mapping.keyboardRange = {0, 127};
        z->mapping.velocityRange = {0, 127};
        z->mapping.rootKey = 60;
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

    scxt::engine::Group &group() { return *eng->getPatch()->getPart(0)->getGroup(0); }

    // main bus rms, skipping the head of the run while the gain smoothers are still moving
    double runRMS(int blocks = 160, int skip = 24)
    {
        eng->processNoteOnEvent(0, 0, 60, -1, 1.f, 0.f);
        double acc{0};
        int n{0};
        for (int b = 0; b < blocks; ++b)
        {
            eng->processAudio();
            if (b < skip)
                continue;
            const auto &o = eng->getPatch()->busses.mainBus.output;
            for (int i = 0; i < (int)scxt::blockSize; ++i)
            {
                acc += (double)o[0][i] * o[0][i] + (double)o[1][i] * o[1][i];
                n += 2;
            }
        }
        return std::sqrt(acc / n);
    }
};

// the gain a control reaches at the top of its range, relative to its value at unity
double gainAtTop(const std::function<void(GainFixture &, float)> &setAmp)
{
    GainFixture unity;
    setAmp(unity, 1.f);
    auto atUnity = unity.runRMS();
    REQUIRE(atUnity > 0.0);

    GainFixture boosted;
    setAmp(boosted, topOfRange(upperBoundDB));
    auto atTop = boosted.runRMS();

    return atTop / atUnity;
}
} // namespace

TEST_CASE("Cubic decibel volumes reach the top of their declared range", "[gain]")
{
    SECTION("group volume")
    {
        auto r = gainAtTop([](auto &f, float v) { f.group().outputInfo.amplitude = v; });
        REQUIRE(r == Approx(amplitudeRatio(upperBoundDB)).epsilon(0.02));
    }

    SECTION("zone volume")
    {
        auto r = gainAtTop([](auto &f, float v) { f.zone->outputInfo.amplitude = v; });
        REQUIRE(r == Approx(amplitudeRatio(upperBoundDB)).epsilon(0.02));
    }

    SECTION("variant amplitude")
    {
        auto r = gainAtTop([](auto &f, float v) { f.zone->variantData.variants[0].amplitude = v; });
        REQUIRE(r == Approx(amplitudeRatio(upperBoundDB)).epsilon(0.02));
    }
}
