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
 * Per-variant pan in a UNISON stack. The first generator writes the voice output directly and
 * the rest are mixed in from scratch, so a variant past the first takes a different path to
 * the output and has to land on the side it is panned to just the same.
 *
 * Each case silences every variant but the one it is about, so whatever reaches the output
 * came from that variant alone.
 */

#include "catch2/catch2.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
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
// no mono file ships with the test samples, so make one
fs::path writeMonoSine(const fs::path &dir)
{
    constexpr uint32_t rate{48000}, frames{48000};
    fs::create_directories(dir);
    auto path = dir / "MonoSine.wav";
    std::ofstream o(path, std::ios::binary);

    auto u32 = [&o](uint32_t v) { o.write((const char *)&v, 4); };
    auto u16 = [&o](uint16_t v) { o.write((const char *)&v, 2); };

    o.write("RIFF", 4);
    u32(4 + (8 + 16) + (8 + frames * 2));
    o.write("WAVE", 4);
    o.write("fmt ", 4);
    u32(16);
    u16(1);
    u16(1);
    u32(rate);
    u32(rate * 2);
    u16(2);
    u16(16);
    o.write("data", 4);
    u32(frames * 2);
    for (uint32_t i = 0; i < frames; ++i)
        u16((uint16_t)(int16_t)(16000 * std::sin(2.0 * M_PI * 440.0 * i / rate)));
    return path;
}

struct Energy
{
    double l{0}, r{0};
};

struct UnisonPanFixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    Zone *zone{nullptr};
    fs::path dir;

    // one entry per variant: true for the stereo file, false for the mono one
    explicit UnisonPanFixture(const std::vector<bool> &stereo)
    {
        dir = fs::temp_directory_path() / "scxt-unison-pan";
        fs::remove_all(dir);

        eng.reset(makeEngine());

        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();
        auto *group = part.getGroup(0).get();
        group->outputInfo.oversample = scxt::engine::Group::OS_OFF;

        auto z = std::make_unique<Zone>();
        z->mapping.keyboardRange = {0, 127};
        z->mapping.velocityRange = {0, 127};
        z->mapping.rootKey = 60;
        z->initialize();
        group->addZone(z);
        zone = group->getZone(0).get();

        // loadSampleByPath asserts it is on the serial thread; we are the only thread.
        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        auto stereoId = eng->getSampleManager()->loadSampleByPath(samplePath("WavStereo48k.wav"));
        auto monoId = eng->getSampleManager()->loadSampleByPath(writeMonoSine(dir));
        REQUIRE(stereoId.has_value());
        REQUIRE(monoId.has_value());

        for (size_t i = 0; i < stereo.size(); ++i)
        {
            zone->variantData.variants[i].sampleID = stereo[i] ? *stereoId : *monoId;
            zone->variantData.variants[i].active = true;
            // ENDPOINTS only - MAPPING would let the wav's chunks overwrite our key range
            REQUIRE(zone->attachToSample(*eng->getSampleManager(), (int)i,
                                         Zone::SampleInformationRead::ENDPOINTS));
            zone->variantData.variants[i].normalizationAmplitude = 1.f;
            zone->variantData.variants[i].amplitude = 1.f;
            zone->variantData.variants[i].pan = 0.f;
        }
        REQUIRE(zone->getNumSampleLoaded() == (int)stereo.size());
        zone->variantData.variantPlaybackMode = Zone::UNISON;
    }

    ~UnisonPanFixture()
    {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }

    // silence every variant but `only`, and pan that one
    void soloAndPan(int only, float pan)
    {
        for (int i = 0; i < zone->getNumSampleLoaded(); ++i)
            zone->variantData.variants[i].amplitude = (i == only) ? 1.f : 0.f;
        zone->variantData.variants[only].pan = pan;
    }

    Energy render()
    {
        Energy e;
        eng->processNoteOnEvent(0, 0, 60, -1, 1.f, 0.f);
        for (int b = 0; b < 64; ++b)
        {
            eng->processAudio();
            for (int i = 0; i < (int)scxt::maxVoices; ++i)
            {
                auto *v = zone->voiceWeakPointers[i];
                if (!v || !v->isVoiceAssigned)
                    continue;
                for (int k = 0; k < (int)scxt::blockSize; ++k)
                {
                    e.l += v->output[0][k] * v->output[0][k];
                    e.r += v->output[1][k] * v->output[1][k];
                }
            }
        }
        return e;
    }
};

void requireOnlyRight(const Energy &e)
{
    INFO("left " << e.l << " right " << e.r);
    REQUIRE(e.r > 1e-2);
    REQUIRE(e.l < 1e-6 * e.r);
}

void requireOnlyLeft(const Energy &e)
{
    INFO("left " << e.l << " right " << e.r);
    REQUIRE(e.l > 1e-2);
    REQUIRE(e.r < 1e-6 * e.l);
}
} // namespace

TEST_CASE("A unison variant past the first pans to the side it is set to", "[variants][pan]")
{
    for (auto stereo : {true, false})
    {
        DYNAMIC_SECTION((stereo ? "stereo" : "mono") << " second variant hard right")
        {
            UnisonPanFixture f({true, stereo});
            f.soloAndPan(1, 1.f);
            requireOnlyRight(f.render());
        }
        DYNAMIC_SECTION((stereo ? "stereo" : "mono") << " second variant hard left")
        {
            UnisonPanFixture f({true, stereo});
            f.soloAndPan(1, -1.f);
            requireOnlyLeft(f.render());
        }
    }

    SECTION("mono third variant hard right behind two stereo ones")
    {
        UnisonPanFixture f({true, true, false});
        f.soloAndPan(2, 1.f);
        requireOnlyRight(f.render());
    }

    SECTION("stereo second variant hard right behind a mono first")
    {
        UnisonPanFixture f({false, true});
        f.soloAndPan(1, 1.f);
        requireOnlyRight(f.render());
    }
}

TEST_CASE("The first unison variant pans to the side it is set to", "[variants][pan]")
{
    for (auto stereo : {true, false})
    {
        DYNAMIC_SECTION((stereo ? "stereo" : "mono") << " first variant hard right")
        {
            UnisonPanFixture f({stereo, true});
            f.soloAndPan(0, 1.f);
            requireOnlyRight(f.render());
        }
    }
}

TEST_CASE("Unpanned unison variants reach both sides", "[variants][pan]")
{
    for (auto stereo : {true, false})
    {
        DYNAMIC_SECTION((stereo ? "stereo" : "mono") << " second variant centred")
        {
            UnisonPanFixture f({true, stereo});
            f.soloAndPan(1, 0.f);
            auto e = f.render();
            INFO("left " << e.l << " right " << e.r);
            REQUIRE(e.l > 1e-2);
            REQUIRE(e.r > 1e-2);
        }
    }
}
