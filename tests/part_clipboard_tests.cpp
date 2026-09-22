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
#include "messaging/client/client_messages.h"
#include "selection/selection_manager.h"

#ifndef SCXT_TEST_SOURCE_DIR
#define SCXT_TEST_SOURCE_DIR ""
#endif

namespace cmsg = scxt::messaging::client;

namespace
{
struct PartClipboardFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    // part 0 holds one zone with Beep, and is named; part 1 is active and empty
    PartClipboardFixture()
    {
        th.start();
        th.stepUI();

        activate(1);
        selectPart(0);
        send(cmsg::AddSampleWithRange({sample("Beep.wav"), 60, 48, 72, 0, 127}));
        REQUIRE(part(0).getGroups().size() == 1);

        setNames(0, "Cello Sustain", "warm and slow");
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

    scxt::engine::Part &part(int16_t p) { return *th.engine->getPatch()->getPart(p); }

    void selectPart(int16_t p) { send(cmsg::SelectPart(p)); }

    void activate(int16_t p)
    {
        auto cfg = part(p).configuration;
        cfg.active = true;
        send(cmsg::UpdatePartFullConfig({p, cfg}));
    }

    void setNames(int16_t p, const std::string &nm, const std::string &blurb)
    {
        auto n = part(p).names;
        n.setName(nm);
        snprintf(n.blurb, sizeof(n.blurb), "%s", blurb.c_str());
        send(cmsg::UpdatePartNames({p, n}));
    }

    std::string nameOf(int16_t p) { return std::string(part(p).names.name); }
    std::string blurbOf(int16_t p) { return std::string(part(p).names.blurb); }
};
} // namespace

TEST_CASE("Pasting a part carries its name and blurb", "[parts][clipboard]")
{
    PartClipboardFixture f;

    f.send(cmsg::CopyPart((int16_t)0));
    REQUIRE(f.th.engine->clipboard.getClipboardType() ==
            scxt::engine::Clipboard::ContentType::PART);

    REQUIRE(f.nameOf(1) == scxt::engine::Part::PartNames::defaultName);
    f.send(cmsg::PastePart((int16_t)1));

    REQUIRE(f.nameOf(1) == "Cello Sustain");
    REQUIRE(f.blurbOf(1) == "warm and slow");
}

TEST_CASE("Pasting a part carries its groups and zones", "[parts][clipboard]")
{
    PartClipboardFixture f;

    f.send(cmsg::CopyPart((int16_t)0));
    f.send(cmsg::PastePart((int16_t)1));

    REQUIRE(f.part(1).getGroups().size() == 1);
    REQUIRE(f.part(1).getGroup(0)->getZones().size() == 1);
    REQUIRE(f.part(1).getGroup(0)->getZone(0)->getNumSampleLoaded() == 1);
}

TEST_CASE("A pasted part keeps the destination's channel and routing", "[parts][clipboard]")
{
    PartClipboardFixture f;

    auto src = f.part(0).configuration;
    src.channel = 2;
    src.routeTo = scxt::engine::BusAddress::AUX_0;
    f.send(cmsg::UpdatePartFullConfig({(int16_t)0, src}));

    auto dst = f.part(1).configuration;
    dst.channel = 7;
    dst.routeTo = scxt::engine::BusAddress::MAIN_0;
    f.send(cmsg::UpdatePartFullConfig({(int16_t)1, dst}));

    f.send(cmsg::CopyPart((int16_t)0));
    f.send(cmsg::PastePart((int16_t)1));

    // the slot is part of the layout, so the destination keeps its own
    REQUIRE(f.part(1).configuration.channel == 7);
    REQUIRE(f.part(1).configuration.routeTo == scxt::engine::BusAddress::MAIN_0);
    // and the content came across anyway
    REQUIRE(f.nameOf(1) == "Cello Sustain");
}

TEST_CASE("Pasting a part is one undo entry", "[parts][clipboard][undo]")
{
    PartClipboardFixture f;

    f.setNames(1, "Scratch", "to be replaced");
    f.send(cmsg::CopyPart((int16_t)0));
    f.send(cmsg::PastePart((int16_t)1));
    REQUIRE(f.nameOf(1) == "Cello Sustain");
    REQUIRE(f.part(1).getGroups().size() == 1);

    f.send(cmsg::Undo(true));
    REQUIRE(f.nameOf(1) == "Scratch");
    REQUIRE(f.blurbOf(1) == "to be replaced");
    REQUIRE(f.part(1).getGroups().empty());
}

TEST_CASE("A pasted part forgets the destination slot's file", "[parts][clipboard]")
{
    PartClipboardFixture f;

    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "scxt_part_clipboard_file";
    fs::remove_all(dir);
    fs::create_directories(dir);

    f.selectPart(1);
    f.send(
        cmsg::AddSampleWithRange({PartClipboardFixture::sample("Kick.wav"), 60, 48, 72, 0, 127}));
    auto scp = dir / "Kick Part.scp";
    f.send(cmsg::SavePart({scp.u8string(), 1, (int)scxt::patch_io::SaveStyles::NO_SAMPLES}));
    REQUIRE(!f.th.engine->getSelectionManager()->getPatchFiles().parts[1].path.empty());

    f.send(cmsg::CopyPart((int16_t)0));
    f.send(cmsg::PastePart((int16_t)1));

    // else Save Part would write the pasted content over Kick Part.scp
    REQUIRE(f.th.engine->getSelectionManager()->getPatchFiles().parts[1].path.empty());

    fs::remove_all(dir);
}

TEST_CASE("A copied part pastes back after its source was cleared", "[parts][clipboard]")
{
    PartClipboardFixture f;

    auto beepID = f.part(0).getGroup(0)->getZone(0)->variantData.variants[0].sampleID;

    f.send(cmsg::CopyPart((int16_t)0));
    f.send(cmsg::ClearPart(0));
    REQUIRE(f.part(0).getGroups().empty());

    // stopping a preview purges every sample nothing holds
    f.send(cmsg::PreviewBrowserSample({0, 1.f, {}}));
    REQUIRE(f.th.engine->getSampleManager()->getSample(beepID));

    f.send(cmsg::PastePart((int16_t)1));
    REQUIRE(f.part(1).getGroups().size() == 1);
    REQUIRE(f.part(1).getGroup(0)->getZone(0)->getNumSampleLoaded() == 1);
}

// reachable straight from the menu, and it clears the groups the payload came from
TEST_CASE("A part can be pasted over itself", "[parts][clipboard]")
{
    PartClipboardFixture f;

    f.send(cmsg::CopyPart((int16_t)0));
    f.send(cmsg::PastePart((int16_t)0));

    REQUIRE(f.nameOf(0) == "Cello Sustain");
    REQUIRE(f.part(0).getGroups().size() == 1);
    REQUIRE(f.part(0).getGroup(0)->getZone(0)->getNumSampleLoaded() == 1);
}

TEST_CASE("Undoing a part deactivation restores its name", "[parts][undo]")
{
    PartClipboardFixture f;

    f.setNames(1, "Viola Short", "thin");
    f.send(cmsg::DeactivatePart((int32_t)1));
    REQUIRE(!f.part(1).configuration.active);

    f.send(cmsg::Undo(true));
    REQUIRE(f.part(1).configuration.active);
    REQUIRE(f.nameOf(1) == "Viola Short");
    REQUIRE(f.blurbOf(1) == "thin");
}
