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

#include <filesystem>
#include <string>

#include "console_harness.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "selection/selection_manager.h"

#ifndef SCXT_TEST_SOURCE_DIR
#define SCXT_TEST_SOURCE_DIR ""
#endif

namespace cmsg = scxt::messaging::client;

namespace
{
struct VariantClipboardFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    // one zone holding Beep, Kick and Hat, in that order, selected as lead
    VariantClipboardFixture()
    {
        th.start();
        th.stepUI();

        send(cmsg::AddSampleWithRange({sample("Beep.wav"), 60, 48, 72, 0, 127}));
        send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));
        send(cmsg::AddSampleInZone({sample("Kick.wav"), 0, 0, 0, 1}));
        send(cmsg::AddSampleInZone({sample("Hat.wav"), 0, 0, 0, 2}));
        REQUIRE(zone().getNumSampleLoaded() == 3);
    }

    template <typename T> void send(const T &msg, size_t drainSteps = 30)
    {
        th.sendToSerialization(msg);
        th.stepUI(drainSteps);
    }

    static std::string sample(const std::string &n)
    {
        namespace fs = std::filesystem;
        auto p = fs::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" / "next" / n;
        REQUIRE(fs::exists(p));
        return p.u8string();
    }

    scxt::engine::Zone &zone()
    {
        return *th.engine->getPatch()->getPart(0)->getGroup(0)->getZone(0);
    }

    std::string nameAt(int v)
    {
        auto &z = zone();
        if (!z.variantData.variants[v].active || !z.samplePointers[v])
            return "";
        return z.samplePointers[v]->getPath().filename().u8string();
    }
};
} // namespace

TEST_CASE("Pasting a variant puts it after the variant named", "[variants][clipboard]")
{
    VariantClipboardFixture f;

    f.send(cmsg::CopyVariant(2));
    REQUIRE(f.th.engine->clipboard.getClipboardType() ==
            scxt::engine::Clipboard::ContentType::VARIANT);

    f.zone().variantData.variants[2].pan = 0.25f;
    f.send(cmsg::PasteVariant(0));
    REQUIRE(f.zone().getNumSampleLoaded() == 4);
    REQUIRE(f.nameAt(0) == "Beep.wav");
    REQUIRE(f.nameAt(1) == "Hat.wav");
    REQUIRE(f.nameAt(2) == "Kick.wav");
    REQUIRE(f.nameAt(3) == "Hat.wav");
    // the copy keeps what the variant held when it was copied
    REQUIRE(f.zone().variantData.variants[1].pan == Approx(0.f));

    f.send(cmsg::Undo(true));
    REQUIRE(f.zone().getNumSampleLoaded() == 3);
    REQUIRE(f.nameAt(1) == "Kick.wav");
    REQUIRE(f.nameAt(2) == "Hat.wav");
}

TEST_CASE("A cut variant pastes back even after its sample was otherwise unused",
          "[variants][clipboard]")
{
    VariantClipboardFixture f;

    auto beepID = f.zone().variantData.variants[0].sampleID;
    auto hatID = f.zone().variantData.variants[2].sampleID;

    // hat goes without passing through the clipboard, beep is cut
    f.send(cmsg::DeleteVariant(2));
    f.send(cmsg::CopyVariant(0));
    f.send(cmsg::DeleteVariant(0));
    REQUIRE(f.zone().getNumSampleLoaded() == 1);
    REQUIRE(f.nameAt(0) == "Kick.wav");

    // stopping a preview purges every sample nothing holds
    f.send(cmsg::PreviewBrowserSample({0, 1.f, {}}));
    REQUIRE(!f.th.engine->getSampleManager()->getSample(hatID));
    REQUIRE(f.th.engine->getSampleManager()->getSample(beepID));

    f.send(cmsg::PasteVariant(0));
    REQUIRE(f.zone().getNumSampleLoaded() == 2);
    REQUIRE(f.nameAt(0) == "Kick.wav");
    REQUIRE(f.nameAt(1) == "Beep.wav");
}

TEST_CASE("Pasting past the end of the variants appends", "[variants][clipboard]")
{
    VariantClipboardFixture f;

    f.send(cmsg::CopyVariant(1));
    f.send(cmsg::PasteVariant(scxt::maxVariantsPerZone));
    REQUIRE(f.zone().getNumSampleLoaded() == 4);
    REQUIRE(f.nameAt(3) == "Kick.wav");
}
