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
#include <fstream>
#include <string>

#include "browser/browser.h"
#include "console_harness.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "messaging/client/client_messages.h"
#include "patch_io/patch_io.h"
#include "selection/selection_manager.h"

#ifndef SCXT_TEST_SOURCE_DIR
#define SCXT_TEST_SOURCE_DIR ""
#endif

namespace cmsg = scxt::messaging::client;
namespace fs = std::filesystem;
using scxt::patch_io::SaveStyles;
using CE = scxt::sample::compound::CompoundElement;

namespace
{
struct Harness
{
    scxt::clients::console_ui::ConsoleHarness th;

    Harness()
    {
        th.start();
        th.sync();
    }

    template <typename T> void send(const T &msg)
    {
        th.sendToSerialization(msg);
        th.sync();
    }

    scxt::engine::Part &part(int16_t p) { return *th.engine->getPatch()->getPart(p); }

    void activate(int16_t p)
    {
        auto cfg = part(p).configuration;
        cfg.active = true;
        send(cmsg::UpdatePartFullConfig({p, cfg}));
    }

    void name(int16_t p, const std::string &nm)
    {
        auto n = part(p).names;
        n.setName(nm);
        send(cmsg::UpdatePartNames({p, n}));
    }

    std::shared_ptr<scxt::sample::Sample> firstSampleOf(int16_t p)
    {
        const auto &g = part(p).getGroup(0);
        auto sid = g->getZone(0)->variantData.variants[0].sampleID;
        return th.engine->getSampleManager()->getSample(sid);
    }

    size_t loadedSampleCount()
    {
        return th.engine->getSampleManager()->getSampleAddressesAndIDs().size();
    }
};

std::string testSample(const std::string &n)
{
    auto p = fs::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" / "next" / n;
    REQUIRE(fs::exists(p));
    return p.u8string();
}

/*
 * Part 1 "Lead" plays Beep, part 2 "Drums" plays Kick. Written as a monolith multi, a
 * referencing multi, and part 2 alone as a monolith part.
 */
struct Files
{
    fs::path dir, mono, ref, part;

    Files()
    {
        dir = fs::temp_directory_path() / "scxt_patch_browser";
        fs::remove_all(dir);
        fs::create_directories(dir);
        mono = dir / "Kit.scm";
        ref = dir / "Kit Ref.scm";
        part = dir / "Drums.scp";

        Harness w;
        w.send(cmsg::AddSampleWithRange({testSample("Beep.wav"), 60, 48, 72, 0, 127}));
        w.name(0, "Lead");
        w.activate(1);
        w.send(cmsg::SelectPart(1));
        w.send(cmsg::AddSampleWithRange({testSample("Kick.wav"), 60, 48, 72, 0, 127}));
        w.name(1, "Drums");

        w.send(cmsg::SaveMulti({mono.u8string(), (int)SaveStyles::AS_MONOLITH}));
        w.send(cmsg::SaveMulti({ref.u8string(), (int)SaveStyles::NO_SAMPLES}));
        w.send(cmsg::SavePart({part.u8string(), 1, (int)SaveStyles::AS_MONOLITH}));

        REQUIRE(fs::exists(mono));
        REQUIRE(fs::exists(ref));
        REQUIRE(fs::exists(part));
    }
    ~Files()
    {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }

    static CE find(const std::vector<CE> &els, const std::string &nm)
    {
        for (const auto &e : els)
            if (e.name == nm)
                return e;
        FAIL("No element named " << nm);
        return {};
    }
};
} // namespace

TEST_CASE("Patch files open in the browser", "[patchbrowser]")
{
    namespace br = scxt::browser;
    REQUIRE(br::Browser::isExpandableInBrowser("x/Kit.scm"));
    REQUIRE(br::Browser::isExpandableInBrowser("x/Drums.scp"));
    REQUIRE(!br::Browser::isExpandableInBrowser("x/Beep.wav"));
}

TEST_CASE("A multi lists itself, its active parts and its samples", "[patchbrowser]")
{
    Files files;

    SECTION("A monolith carries its samples")
    {
        auto els = scxt::browser::Browser::expandForBrowser(files.mono);
        REQUIRE(els.size() == 5);

        REQUIRE(els[0].name == "Full Multi");
        REQUIRE(els[0].type == CE::INSTRUMENT);
        REQUIRE(els[0].sampleAddress.preset == -1);

        REQUIRE(els[1].name == "Part 1: Lead");
        REQUIRE(els[1].sampleAddress.preset == 0);
        REQUIRE(els[2].name == "Part 2: Drums");
        REQUIRE(els[2].sampleAddress.preset == 1);

        // the monolith sorts its samples by path
        REQUIRE(els[3].type == CE::SAMPLE);
        REQUIRE(els[3].name == "Beep.wav");
        REQUIRE(els[3].sampleAddress.type == scxt::sample::Sample::SCXT_FILE);
        REQUIRE(els[3].sampleAddress.region == 0);
        REQUIRE(els[4].name == "Kick.wav");
        REQUIRE(els[4].sampleAddress.region == 1);
    }

    SECTION("A referencing multi has no samples to show")
    {
        auto els = scxt::browser::Browser::expandForBrowser(files.ref);
        REQUIRE(els.size() == 3);
        REQUIRE(els[0].name == "Full Multi");
        REQUIRE(els[2].name == "Part 2: Drums");
    }

    SECTION("A file that isn't one says so")
    {
        auto bad = files.dir / "Broken.scm";
        {
            std::ofstream o(bad);
            o << "not a riff";
        }
        auto els = scxt::browser::Browser::expandForBrowser(bad);
        REQUIRE(els.size() == 1);
        REQUIRE(els[0].type == CE::ERROR_SENTINEL);
    }
}

TEST_CASE("A part file lists itself and its samples", "[patchbrowser]")
{
    Files files;
    auto els = scxt::browser::Browser::expandForBrowser(files.part);
    REQUIRE(els.size() == 2);
    // saving a part names it for its file
    REQUIRE(els[0].name == "Part: Drums");
    REQUIRE(els[0].type == CE::INSTRUMENT);
    REQUIRE(els[1].name == "Kick.wav");
    REQUIRE(els[1].type == CE::SAMPLE);
}

TEST_CASE("One part of a multi loads into the selected part", "[patchbrowser]")
{
    Files files;

    for (const auto &path : {files.mono, files.ref})
    {
        DYNAMIC_SECTION("from " << path.filename().u8string())
        {
            Harness h;
            h.activate(4);
            h.send(cmsg::SelectPart(4));

            auto drums = Files::find(scxt::patch_io::getSCXTCompoundList(path), "Part 2: Drums");
            h.send(cmsg::AddCompoundElementWithRange({drums, 60, 48, 72, 0, 127}));

            REQUIRE(h.part(4).getGroups().size() == 1);
            REQUIRE(std::string(h.part(4).names.name) == "Drums");
            REQUIRE(h.firstSampleOf(4)->displayName == "Kick.wav");
            // only the samples the part uses come along
            REQUIRE(h.loadedSampleCount() == 1);
            // it has no part file of its own
            REQUIRE(h.th.engine->getSelectionManager()->getPatchFiles().parts[4].path.empty());
            // and nothing else moved
            REQUIRE(h.part(0).getGroups().empty());
        }
    }
}

TEST_CASE("A part dropped on a card loads there and undoes", "[patchbrowser]")
{
    Files files;
    Harness h;

    h.send(cmsg::LoadPartFromMultiInto({files.mono.u8string(), (int16_t)0, (int16_t)6}));
    REQUIRE(h.part(6).getGroups().size() == 1);
    REQUIRE(std::string(h.part(6).names.name) == "Lead");
    REQUIRE(h.firstSampleOf(6)->type == scxt::sample::Sample::SCXT_FILE);

    h.send(cmsg::Undo(true));
    REQUIRE(h.part(6).getGroups().empty());
}

TEST_CASE("The whole of a patch file loads from its entry", "[patchbrowser]")
{
    Files files;
    Harness h;

    SECTION("Full Multi replaces the multi")
    {
        auto full = Files::find(scxt::patch_io::getSCXTCompoundList(files.mono), "Full Multi");
        h.send(cmsg::AddCompoundElementWithRange({full, 60, 48, 72, 0, 127}));

        REQUIRE(h.th.engine->getSelectionManager()->getPatchFiles().multi.path == files.mono);
        REQUIRE(std::string(h.part(1).names.name) == "Drums");
    }

    SECTION("A part file's entry loads into the selected part")
    {
        h.activate(2);
        h.send(cmsg::SelectPart(2));
        auto pt = Files::find(scxt::patch_io::getSCXTCompoundList(files.part), "Part: Drums");
        h.send(cmsg::AddCompoundElementWithRange({pt, 60, 48, 72, 0, 127}));

        REQUIRE(h.part(2).getGroups().size() == 1);
        REQUIRE(h.th.engine->getSelectionManager()->getPatchFiles().parts[2].path == files.part);
    }
}

TEST_CASE("A sample inside a monolith loads as a zone", "[patchbrowser]")
{
    Files files;
    Harness h;

    auto kick = Files::find(scxt::patch_io::getSCXTCompoundList(files.mono), "Kick.wav");
    h.send(cmsg::AddCompoundElementWithRange({kick, 60, 48, 72, 0, 127}));

    REQUIRE(h.part(0).getGroups().size() == 1);
    auto smp = h.firstSampleOf(0);
    REQUIRE(smp->type == scxt::sample::Sample::SCXT_FILE);
    REQUIRE(smp->displayName == "Kick.wav");
    REQUIRE(smp->getPath() == files.mono);
}
