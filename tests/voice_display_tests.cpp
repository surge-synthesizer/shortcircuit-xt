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
 * The voice display the sample editor draws its playheads from. A voice reports every variant
 * it is sounding and where each one is, so a unison stack shows a playhead on every tab and a
 * round robin voice shows one on the tab it picked.
 */

#include "catch2/catch2.hpp"

#include <filesystem>
#include <memory>

#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "messaging/messaging.h"
#include "voice/voice.h"

#include "test_utils.h"

namespace fs = std::filesystem;

using Zone = scxt::engine::Zone;
using Item = scxt::engine::Engine::SharedUIMemoryState::VoiceDisplayStateItem;

namespace
{
struct DisplayFixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    Zone *zone{nullptr};

    DisplayFixture(Zone::VariantPlaybackMode mode, int nVariants)
    {
        eng.reset(makeEngine());
        // the display is only written while a client is there to read it
        eng->getMessageController()->isClientConnected = true;

        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();
        auto *group = part.getGroup(0).get();

        auto z = std::make_unique<Zone>();
        z->mapping.keyboardRange = {0, 127};
        z->mapping.velocityRange = {0, 127};
        z->mapping.rootKey = 60;
        z->initialize();
        group->addZone(z);
        zone = group->getZone(0).get();

        // loadSampleByPath asserts it is on the serial thread; we are the only thread.
        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        auto sid = eng->getSampleManager()->loadSampleByPath(samplePath("WavStereo48k.wav"));
        REQUIRE(sid.has_value());

        for (int i = 0; i < nVariants; ++i)
        {
            zone->variantData.variants[i].sampleID = *sid;
            zone->variantData.variants[i].active = true;
            // ENDPOINTS only - MAPPING would let the wav's chunks overwrite our key range
            REQUIRE(zone->attachToSample(*eng->getSampleManager(), i,
                                         Zone::SampleInformationRead::ENDPOINTS));
            // staggered starts so every variant sits somewhere different
            zone->variantData.variants[i].startSample = 1000 * i;
        }
        REQUIRE(zone->getNumSampleLoaded() == nVariants);
        zone->variantData.variantPlaybackMode = mode;
    }

    void play(int key)
    {
        eng->processNoteOnEvent(0, 0, key, -1, 1.f, 0.f);
        eng->processAudio();
    }

    const Item &itemFor(int key) const
    {
        const auto &items = eng->sharedUIMemoryState.voiceDisplayItems;
        for (const auto &itm : items)
            if (itm.active && itm.midiNote == key)
                return itm;
        FAIL("no voice display item for key " << key);
        return items[0];
    }

    scxt::voice::Voice *voiceFor(int key) const
    {
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned && v->originalMidiKey == key)
                return v;
        }
        return nullptr;
    }
};
} // namespace

TEST_CASE("A unison voice shows a playhead on every variant", "[variants][display]")
{
    DisplayFixture f(Zone::UNISON, 4);
    f.play(60);

    auto *v = f.voiceFor(60);
    REQUIRE(v);
    REQUIRE(v->numGeneratorsActive == 4);

    const auto &itm = f.itemFor(60);
    REQUIRE(itm.numSamples == 4);
    for (int g = 0; g < 4; ++g)
    {
        INFO("generator " << g);
        REQUIRE(itm.sample[g] == g);
        REQUIRE(itm.samplePos[g] == v->GD[g].samplePos);
        REQUIRE(itm.samplePos[g] >= 1000 * g);
    }
}

TEST_CASE("A round robin voice shows its playhead on the variant it picked", "[variants][display]")
{
    DisplayFixture f(Zone::FORWARD_RR, 4);

    // forward round robin walks the variants in order, so the fourth note plays the fourth tab
    for (int n = 0; n < 4; ++n)
    {
        auto key = 60 + 2 * n;
        f.play(key);

        auto *v = f.voiceFor(key);
        REQUIRE(v);
        REQUIRE(v->sampleIndex == n);

        INFO("note " << n);
        const auto &itm = f.itemFor(key);
        REQUIRE(itm.numSamples == 1);
        REQUIRE(itm.sample[0] == n);
        REQUIRE(itm.samplePos[0] == v->GD[0].samplePos);
        REQUIRE(itm.samplePos[0] >= 1000 * n);
    }
}
