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

#include "sfz_test_support.h"
#include "engine/group.h"

using namespace scxt::sfztest;
namespace eng = scxt::engine;

namespace
{
std::optional<eng::GroupTriggerStorage> triggerOf(const eng::Group &g, eng::GroupTriggerID id)
{
    const auto &tc = g.triggerConditions;
    for (int i = 0; i < scxt::triggerConditionsPerGroup; ++i)
        if (tc.active[i] && tc.storage[i].id == id)
            return tc.storage[i];
    return std::nullopt;
}

const eng::Zone &zoneAt(Fixture &f, size_t g, size_t z)
{
    auto &groups = f.part0().getGroups();
    REQUIRE(groups.size() > g);
    REQUIRE(groups[g]->getZones().size() > z);
    return *groups[g]->getZones()[z];
}

const WavSpec looped{"looped.wav", 4000, SmplSpec{48, std::make_pair(1000u, 2999u)}};
} // namespace

TEST_CASE("SFZ pan places hard panned mono pairs", "[importer][sfz]")
{
    const std::string sfz = R"(
<group> key=60
<region> sample=l.wav pan=-100
<region> sample=r.wav pan=100
)";
    auto p = buildSfz("pan", sfz, {{"l.wav", 1000}, {"r.wav", 1200}});

    Fixture f;
    f.load(p);

    CHECK(zoneAt(f, 0, 0).mapping.pan == Approx(-1.f));
    CHECK(zoneAt(f, 0, 1).mapping.pan == Approx(1.f));
}

TEST_CASE("SFZ tune and transpose stack", "[importer][sfz]")
{
    auto p = buildSfz("tune_transpose", "<region> sample=a.wav transpose=2 tune=50\n",
                      {{"a.wav", 1000}});

    Fixture f;
    f.load(p);

    CHECK(f.onlyZone().mapping.pitchOffset == Approx(2.5f));
}

TEST_CASE("SFZ key extends with hikey and lokey", "[importer][sfz]")
{
    const std::string sfz = R"(
<region> sample=a.wav key=60 hikey=71
<region> sample=a.wav key=60 lokey=62 hikey=70
)";
    auto p = buildSfz("key_ranges", sfz, {{"a.wav", 1000}});

    Fixture f;
    f.load(p);

    auto &a = zoneAt(f, 0, 0);
    CHECK(a.mapping.rootKey == 60);
    CHECK(a.mapping.keyboardRange.keyStart == 60);
    CHECK(a.mapping.keyboardRange.keyEnd == 71);

    // lokey above key's own end used to swap against it
    auto &b = zoneAt(f, 0, 1);
    CHECK(b.mapping.rootKey == 60);
    CHECK(b.mapping.keyboardRange.keyStart == 62);
    CHECK(b.mapping.keyboardRange.keyEnd == 70);
}

TEST_CASE("SFZ pitch_keycenter=sample reads the file's root", "[importer][sfz]")
{
    auto p = buildSfz("keycenter_sample",
                      "<region> sample=looped.wav lokey=40 hikey=50 "
                      "pitch_keycenter=sample\n",
                      {looped});

    Fixture f;
    f.load(p);

    auto &z = f.onlyZone();
    CHECK(z.mapping.rootKey == 48);
    CHECK(z.mapping.keyboardRange.keyStart == 40);
    CHECK(z.mapping.keyboardRange.keyEnd == 50);
}

TEST_CASE("SFZ loop_mode", "[importer][sfz]")
{
    using LM = eng::Zone::LoopMode;

    SECTION("unspecified keeps the file's loop")
    {
        auto p = buildSfz("loop_default", "<region> sample=looped.wav\n", {looped});
        Fixture f;
        f.load(p);
        auto &v = f.onlyZone().variantData.variants[0];
        CHECK(v.loopActive);
        CHECK(v.startLoop == 1000);
    }

    SECTION("no_loop overrides the file's loop")
    {
        auto p = buildSfz("loop_none", "<region> sample=looped.wav loop_mode=no_loop\n", {looped});
        Fixture f;
        f.load(p);
        CHECK_FALSE(f.onlyZone().variantData.variants[0].loopActive);
    }

    SECTION("loop_sustain loops while gated")
    {
        auto p = buildSfz("loop_sustain", "<region> sample=looped.wav loop_mode=loop_sustain\n",
                          {looped});
        Fixture f;
        f.load(p);
        auto &v = f.onlyZone().variantData.variants[0];
        CHECK(v.loopActive);
        CHECK(v.loopMode == LM::LOOP_WHILE_GATED);
    }

    SECTION("sfz2 spellings")
    {
        auto p = buildSfz("loop_alias",
                          "<region> sample=a.wav loopmode=loop_continuous loopstart=100 "
                          "loopend=800\n",
                          {{"a.wav", 1000}});
        Fixture f;
        f.load(p);
        auto &v = f.onlyZone().variantData.variants[0];
        CHECK(v.loopActive);
        CHECK(v.loopMode == LM::LOOP_DURING_VOICE);
        CHECK(v.startLoop == 100);
        CHECK(v.endLoop == 800);
    }

    SECTION("one_shot gates the amp envelope on the sample")
    {
        auto p = buildSfz("one_shot", "<region> sample=looped.wav loop_mode=one_shot\n", {looped});
        Fixture f;
        f.load(p);
        auto &z = f.onlyZone();
        CHECK_FALSE(z.variantData.variants[0].loopActive);
        CHECK(z.egStorage[0].gateMode ==
              scxt::modulation::modulators::AdsrStorage::GateMode::SAMPLE_GATED);
    }
}

TEST_CASE("SFZ loop_type and direction", "[importer][sfz]")
{
    auto p =
        buildSfz("loop_type", "<region> sample=looped.wav loop_type=alternate direction=reverse\n",
                 {looped});

    Fixture f;
    f.load(p);

    auto &v = f.onlyZone().variantData.variants[0];
    CHECK(v.loopDirection == eng::Zone::ALTERNATE_DIRECTIONS);
    CHECK(v.playReverse);
}

TEST_CASE("SFZ trigger=release", "[importer][sfz]")
{
    SECTION("on the group header")
    {
        const std::string sfz = R"(
<group> group_label=Piano
<region> sample=a.wav key=60
<group> group_label=Noise trigger=release
<region> sample=r.wav key=60
)";
        auto p = buildSfz("release_group", sfz, {{"a.wav", 1000}, {"r.wav", 500}});
        Fixture f;
        f.load(p);

        auto &groups = f.part0().getGroups();
        REQUIRE(groups.size() == 2);
        CHECK(groups[0]->triggerConditions.voiceCreationMode == eng::VoiceCreationMode::ON_NOTE_ON);
        CHECK(groups[1]->triggerConditions.voiceCreationMode ==
              eng::VoiceCreationMode::ON_NOTE_OFF);
    }

    SECTION("mixed in one group splits off a release group")
    {
        const std::string sfz = R"(
<group> group_label=Piano
<region> sample=a.wav key=60
<region> sample=r.wav key=60 trigger=release
<region> sample=a.wav key=62
)";
        auto p = buildSfz("release_mixed", sfz, {{"a.wav", 1000}, {"r.wav", 500}});
        Fixture f;
        f.load(p);

        auto &groups = f.part0().getGroups();
        REQUIRE(groups.size() == 2);
        CHECK(groups[0]->name == "Piano");
        CHECK(groups[0]->getZones().size() == 2);
        CHECK(groups[1]->name == "Piano Release");
        CHECK(groups[1]->getZones().size() == 1);
        CHECK(groups[1]->triggerConditions.voiceCreationMode ==
              eng::VoiceCreationMode::ON_NOTE_OFF);
    }
}

TEST_CASE("SFZ keyswitches", "[importer][sfz]")
{
    const std::string sfz = R"(
<global> sw_lokey=24 sw_hikey=26 sw_default=25
<group> sw_last=24 sw_label=Legato
<region> sample=a.wav key=60
<group> sw_last=25 sw_label=Staccato
<region> sample=b.wav key=60
<group> sw_down=26
<region> sample=c.wav key=62
)";
    auto p = buildSfz("keyswitch", sfz, {{"a.wav", 1000}, {"b.wav", 2000}, {"c.wav", 3000}});

    Fixture f;
    f.load(p);

    auto &groups = f.part0().getGroups();
    REQUIRE(groups.size() == 3);

    auto legato = triggerOf(*groups[0], eng::GroupTriggerID::KEYSWITCH_LATCH);
    REQUIRE(legato);
    CHECK(legato->args[0] == 24);
    CHECK(groups[0]->name == "Legato");

    auto stacc = triggerOf(*groups[1], eng::GroupTriggerID::KEYSWITCH_LATCH);
    REQUIRE(stacc);
    CHECK(stacc->args[0] == 25);

    auto mom = triggerOf(*groups[2], eng::GroupTriggerID::KEYSWITCH_MOMENTARY);
    REQUIRE(mom);
    CHECK(mom->args[0] == 26);

    // sw_default picks the live articulation
    CHECK(f.part0().configuration.defaultKeySwitchKey == 25);
    CHECK(groups[0]->mutedByLatch);
    CHECK_FALSE(groups[1]->mutedByLatch);
}

TEST_CASE("SFZ group and off_by choke", "[importer][sfz]")
{
    const std::string sfz = R"(
<group> group_label=Kit
<region> sample=k.wav key=36
<region> sample=c.wav key=42 group=1 hivel=63
<region> sample=c.wav key=42 group=1 lovel=64
<region> sample=o.wav key=46 group=1 off_by=1
)";
    auto p = buildSfz("choke", sfz, {{"k.wav", 1000}, {"c.wav", 2000}, {"o.wav", 3000}});

    Fixture f;
    f.load(p);

    auto &groups = f.part0().getGroups();
    REQUIRE(groups.size() == 3);

    // the kick isn't in a choke and stays put
    CHECK(groups[0]->outputInfo.exclusiveGroup == 0);
    CHECK(groups[0]->getZones().size() == 1);

    // scxt doesn't choke within a group, so each hat key gets one; velocity layers stay together
    CHECK(groups[1]->outputInfo.exclusiveGroup == 1);
    CHECK(groups[1]->getZones().size() == 2);
    CHECK(groups[2]->outputInfo.exclusiveGroup == 1);
    CHECK(groups[2]->getZones().size() == 1);

    CHECK(f.part0().configuration.numExclusiveGroups == 1);
}

TEST_CASE("SFZ seq_position on the group header cycles groups", "[importer][sfz]")
{
    const std::string sfz = R"(
<global> seq_length=2
<group> seq_position=1
<region> sample=a.wav key=60
<group> seq_position=2
<region> sample=b.wav key=60
)";
    auto p = buildSfz("seq_groups", sfz, {{"a.wav", 1000}, {"b.wav", 2000}});

    Fixture f;
    f.load(p);

    auto &groups = f.part0().getGroups();
    REQUIRE(groups.size() == 2);
    for (int i = 0; i < 2; ++i)
    {
        auto rr = triggerOf(*groups[i], eng::GroupTriggerID::ROUND_ROBIN_CYCLE);
        REQUIRE(rr);
        CHECK(rr->args[0] == 0);
        CHECK(rr->args[1] == i + 1);
        CHECK(groups[i]->getZones().size() == 1);
    }
}

TEST_CASE("SFZ master opcodes are inherited", "[importer][sfz]")
{
    const std::string sfz = R"(
<master> transpose=12
<group>
<region> sample=a.wav
<master>
<group>
<region> sample=a.wav
)";
    auto p = buildSfz("master", sfz, {{"a.wav", 1000}});

    Fixture f;
    f.load(p);

    CHECK(zoneAt(f, 0, 0).mapping.pitchOffset == Approx(12.f));
    CHECK(zoneAt(f, 1, 0).mapping.pitchOffset == Approx(0.f));
}
