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
#include <fstream>
#include <string>
#include <vector>

#include "console_harness.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "voice/voice.h"
#include "messaging/client/client_messages.h"
#include "test_utils.h"

namespace cmsg = scxt::messaging::client;

namespace
{
cmsg::addSampleSpec_t beep(int root, int keyLo, int keyHi)
{
    return {samplePath("next/Beep.wav").u8string(), root, keyLo, keyHi, 0, 127, false};
}

struct DropFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    DropFixture()
    {
        th.start();
        th.stepUI();
    }

    scxt::engine::Part &part() { return *th.engine->getPatch()->getPart(0); }

    size_t zonesIn(size_t group)
    {
        if (group >= part().getGroups().size())
            return 0;
        return part().getGroup(group)->getZones().size();
    }

    // adds land after an audio thread hop, and a drain is not a barrier
    bool drainUntilZones(size_t group, size_t n)
    {
        for (int i = 0; i < 300; ++i)
        {
            th.stepUI(1);
            if (zonesIn(group) == n)
            {
                th.stepUI(20);
                return true;
            }
        }
        return false;
    }

    void resetCounts()
    {
        th.editor->structureUpdateCount = 0;
        th.editor->selectionStateCount = 0;
        th.editor->receivedByteCount = 0;
    }

    scxt::engine::Zone &zone() { return *part().getGroup(0)->getZone(0); }

    // one zone holding Beep in its first variant, selected as lead
    void makeLeadZone()
    {
        th.sendToSerialization(cmsg::AddSamples({{beep(60, 48, 72)}, -1, -1}));
        REQUIRE(drainUntilZones(0, 1));
        th.sendToSerialization(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));
        th.stepUI(30);
        REQUIRE(zone().getNumSampleLoaded() == 1);
    }

    bool drainUntilVariants(size_t n)
    {
        for (int i = 0; i < 300; ++i)
        {
            th.stepUI(1);
            if ((size_t)zone().getNumSampleLoaded() == n)
            {
                th.stepUI(20);
                return true;
            }
        }
        return false;
    }

    std::string variantName(int v)
    {
        auto &z = zone();
        if (!z.variantData.variants[v].active || !z.samplePointers[v])
            return "";
        return z.samplePointers[v]->getPath().filename().u8string();
    }
};

// variants 1 to n of the lead zone, cycling the non Beep test samples
std::vector<cmsg::addSampleInZoneSpec_t> variantSamples(int n)
{
    static const std::array<std::string, 3> names{"Kick.wav", "Hat.wav", "PulseSaw.wav"};
    std::vector<cmsg::addSampleInZoneSpec_t> res;
    for (int i = 0; i < n; ++i)
        res.push_back({samplePath("next/" + names[i % names.size()]).u8string(), i + 1});
    return res;
}

std::vector<cmsg::addSampleSpec_t> beeps(int n)
{
    return std::vector<cmsg::addSampleSpec_t>(n, beep(60, 48, 72));
}
} // namespace

TEST_CASE("A multi sample drop sends one structure and one selection update", "[drop]")
{
    DropFixture f;
    f.resetCounts();

    f.th.sendToSerialization(cmsg::AddSamples({beeps(50), -1, -1}));
    REQUIRE(f.drainUntilZones(0, 50));

    REQUIRE(f.th.editor->structureUpdateCount == 1);
    REQUIRE(f.th.editor->selectionStateCount == 1);
}

TEST_CASE("A multi sample drop costs about what a single sample drop costs", "[drop]")
{
    size_t oneFile{0}, manyFiles{0};
    {
        DropFixture f;
        f.resetCounts();
        f.th.sendToSerialization(cmsg::AddSamples({beeps(1), -1, -1}));
        REQUIRE(f.drainUntilZones(0, 1));
        oneFile = f.th.editor->receivedByteCount;
    }
    {
        DropFixture f;
        f.resetCounts();
        f.th.sendToSerialization(cmsg::AddSamples({beeps(50), -1, -1}));
        REQUIRE(f.drainUntilZones(0, 50));
        manyFiles = f.th.editor->receivedByteCount;
    }
    REQUIRE(oneFile > 0);
    REQUIRE(manyFiles < 2 * oneFile);
}

TEST_CASE("A multi sample drop gives each zone its own mapping", "[drop]")
{
    DropFixture f;

    std::vector<cmsg::addSampleSpec_t> specs;
    for (int i = 0; i < 5; ++i)
        specs.push_back(beep(40 + i, 40 + i, 40 + i));

    f.th.sendToSerialization(cmsg::AddSamples({specs, -1, -1}));
    REQUIRE(f.drainUntilZones(0, 5));

    const auto &zones = f.part().getGroup(0)->getZones();
    for (int i = 0; i < 5; ++i)
    {
        INFO("zone " << i);
        REQUIRE(zones[i]->mapping.rootKey == 40 + i);
        REQUIRE(zones[i]->mapping.keyboardRange.keyStart == 40 + i);
        REQUIRE(zones[i]->mapping.keyboardRange.keyEnd == 40 + i);
    }
}

TEST_CASE("A multi sample drop into an explicit group lands in that group", "[drop]")
{
    DropFixture f;
    for (int i = 0; i < 3; ++i)
        f.th.sendToSerialization(cmsg::CreateGroup(0));
    f.th.stepUI(30);
    REQUIRE(f.part().getGroups().size() == 3);

    // the middle group is neither the first nor the most recently created one
    f.th.sendToSerialization(cmsg::AddSamples({beeps(4), 0, 1}));
    REQUIRE(f.drainUntilZones(1, 4));

    REQUIRE(f.zonesIn(0) == 0);
    REQUIRE(f.zonesIn(2) == 0);
}

TEST_CASE("A multi sample drop is a single undo step", "[drop][undo]")
{
    DropFixture f;
    auto baseSize = f.th.engine->undoManager.undoStackSize();

    f.th.sendToSerialization(cmsg::AddSamples({beeps(3), -1, -1}));
    REQUIRE(f.drainUntilZones(0, 3));
    REQUIRE(f.th.engine->undoManager.undoStackSize() == baseSize + 1);

    f.th.sendToSerialization(cmsg::Undo(true));
    f.th.stepUI(30);
    REQUIRE(f.part().getGroups().empty());
}

TEST_CASE("A multi sample variant drop sends one selection update", "[drop][variants]")
{
    DropFixture f;
    f.makeLeadZone();
    f.resetCounts();

    f.th.sendToSerialization(cmsg::AddSamplesInZone({variantSamples(6), {}, 0, 0, 0}));
    REQUIRE(f.drainUntilVariants(7));

    REQUIRE(f.th.editor->selectionStateCount == 1);
}

TEST_CASE("A multi sample variant drop costs about what a single variant drop costs",
          "[drop][variants]")
{
    size_t oneFile{0}, manyFiles{0};
    {
        DropFixture f;
        f.makeLeadZone();
        f.resetCounts();
        f.th.sendToSerialization(cmsg::AddSamplesInZone({variantSamples(1), {}, 0, 0, 0}));
        REQUIRE(f.drainUntilVariants(2));
        oneFile = f.th.editor->receivedByteCount;
    }
    {
        DropFixture f;
        f.makeLeadZone();
        f.resetCounts();
        f.th.sendToSerialization(
            cmsg::AddSamplesInZone({variantSamples(scxt::maxVariantsPerZone - 1), {}, 0, 0, 0}));
        REQUIRE(f.drainUntilVariants(scxt::maxVariantsPerZone));
        manyFiles = f.th.editor->receivedByteCount;
    }
    REQUIRE(oneFile > 0);
    REQUIRE(manyFiles < 2 * oneFile);
}

TEST_CASE("A multi sample variant drop fills the variants it names", "[drop][variants]")
{
    namespace compound = scxt::sample::compound;

    auto sf2P = samplePath("harpsi.sf2");
    std::vector<cmsg::addCompoundInZoneSpec_t> compounds;
    for (const auto &el : scxt::browser::Browser::expandForBrowser(sf2P))
    {
        if (el.type == compound::CompoundElement::SAMPLE)
        {
            compounds.push_back({el, 2});
            break;
        }
    }
    REQUIRE(compounds.size() == 1);

    DropFixture f;
    f.makeLeadZone();

    std::vector<cmsg::addSampleInZoneSpec_t> samples{{samplePath("next/Kick.wav").u8string(), 1},
                                                     {samplePath("next/Hat.wav").u8string(), 3}};
    f.th.sendToSerialization(cmsg::AddSamplesInZone({samples, compounds, 0, 0, 0}));
    REQUIRE(f.drainUntilVariants(4));

    REQUIRE(f.variantName(0) == "Beep.wav");
    REQUIRE(f.variantName(1) == "Kick.wav");
    REQUIRE(f.variantName(2) == "harpsi.sf2");
    REQUIRE(f.variantName(3) == "Hat.wav");
}

TEST_CASE("A multi sample variant drop ignores variants out of range", "[drop][variants]")
{
    DropFixture f;
    f.makeLeadZone();

    auto kick = samplePath("next/Kick.wav").u8string();
    std::vector<cmsg::addSampleInZoneSpec_t> samples{
        {kick, -1}, {kick, scxt::maxVariantsPerZone}, {kick, 1}};
    f.th.sendToSerialization(cmsg::AddSamplesInZone({samples, {}, 0, 0, 0}));
    REQUIRE(f.drainUntilVariants(2));
    REQUIRE(f.variantName(1) == "Kick.wav");
}

TEST_CASE("A multi sample variant drop is a single undo step", "[drop][variants][undo]")
{
    DropFixture f;
    f.makeLeadZone();
    auto baseSize = f.th.engine->undoManager.undoStackSize();

    f.th.sendToSerialization(cmsg::AddSamplesInZone({variantSamples(3), {}, 0, 0, 0}));
    REQUIRE(f.drainUntilVariants(4));
    REQUIRE(f.th.engine->undoManager.undoStackSize() == baseSize + 1);

    f.th.sendToSerialization(cmsg::Undo(true));
    f.th.stepUI(30);
    REQUIRE(f.zone().getNumSampleLoaded() == 1);
}

/*
 * Browser auto-load (#2303): the browser swaps the sample under the lead zone as the
 * selection moves, rather than adding a zone. So the zone's geometry has to survive, the
 * variants have to collapse, and each swap has to be one undo step.
 */
namespace
{
// a 16 bit mono wav, with a smpl chunk when asked: no checked in test sample carries loop
// points, and they are all at the same rate
fs::path writeWav(const fs::path &dir, const std::string &name, uint32_t rate, int32_t loopStart,
                  int32_t loopEnd)
{
    constexpr int32_t frames{1024};
    const bool withLoop = loopEnd > loopStart;
    fs::create_directories(dir);
    auto path = dir / name;
    std::ofstream o(path, std::ios::binary);

    auto u32 = [&o](uint32_t v) { o.write((const char *)&v, 4); };
    auto u16 = [&o](uint16_t v) { o.write((const char *)&v, 2); };

    const uint32_t dataBytes = frames * 2;
    const uint32_t smplBytes = 36 + 24;

    o.write("RIFF", 4);
    u32(4 + (8 + 16) + (8 + dataBytes) + (withLoop ? 8 + smplBytes : 0));
    o.write("WAVE", 4);

    o.write("fmt ", 4);
    u32(16);
    u16(1); // pcm
    u16(1); // mono
    u32(rate);
    u32(rate * 2);
    u16(2);
    u16(16);

    o.write("data", 4);
    u32(dataBytes);
    for (int32_t i = 0; i < frames; ++i)
        u16((uint16_t)(int16_t)std::lround(8000 * std::sin(i * 0.05)));

    if (withLoop)
    {
        o.write("smpl", 4);
        u32(smplBytes);
        for (int32_t i = 0; i < 9; ++i)
            u32(i == 3 ? 60u : (i == 7 ? 1u : 0u)); // unity note, then one loop
        u32(0);
        u32(0); // forward
        u32((uint32_t)loopStart);
        u32((uint32_t)(loopEnd - 1)); // the reader adds the one back
        u32(0);
        u32(0);
    }
    return path;
}

fs::path writeLoopedWav(const fs::path &dir, int32_t loopStart, int32_t loopEnd)
{
    return writeWav(dir, "Looped.wav", 48000, loopStart, loopEnd);
}

std::string autoLoadPath(const std::string &name) { return samplePath("next/" + name).u8string(); }
} // namespace

TEST_CASE("Browser auto-load replaces the lead zone's sample", "[drop][autoload]")
{
    DropFixture f;
    f.makeLeadZone();
    REQUIRE(f.variantName(0) == "Beep.wav");

    f.th.sendToSerialization(cmsg::AutoLoadSampleIntoLeadZone(autoLoadPath("Kick.wav")));
    f.th.stepUI(30);

    REQUIRE(f.variantName(0) == "Kick.wav");
    REQUIRE(f.zone().getNumSampleLoaded() == 1);
}

TEST_CASE("Browser auto-load leaves the zone geometry alone", "[drop][autoload]")
{
    DropFixture f;
    f.th.sendToSerialization(cmsg::AddSamples({{beep(48, 36, 60)}, -1, -1}));
    REQUIRE(f.drainUntilZones(0, 1));
    f.th.sendToSerialization(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));
    f.th.stepUI(30);

    auto &z = f.zone();
    z.mapping.velocityRange = {20, 100};
    z.mapping.keyboardRange.fadeStart = 3;
    z.mapping.velocityRange.fadeEnd = 7;

    // the looped wav carries a unity note of 60, which must not become the root key
    auto dir = fs::temp_directory_path() / "scxt-auto-load-tests";
    fs::remove_all(dir);
    auto looped = writeLoopedWav(dir, 100, 200);

    f.th.sendToSerialization(cmsg::AutoLoadSampleIntoLeadZone(looped.u8string()));
    f.th.stepUI(30);

    REQUIRE(f.variantName(0) == "Looped.wav");
    REQUIRE(z.mapping.rootKey == 48);
    REQUIRE(z.mapping.keyboardRange.keyStart == 36);
    REQUIRE(z.mapping.keyboardRange.keyEnd == 60);
    REQUIRE(z.mapping.keyboardRange.fadeStart == 3);
    REQUIRE(z.mapping.velocityRange.velStart == 20);
    REQUIRE(z.mapping.velocityRange.velEnd == 100);
    REQUIRE(z.mapping.velocityRange.fadeEnd == 7);

    fs::remove_all(dir);
}

TEST_CASE("Browser auto-load collapses the zone to one variant", "[drop][autoload][variants]")
{
    DropFixture f;
    f.makeLeadZone();
    f.th.sendToSerialization(cmsg::AddSamplesInZone({variantSamples(3), {}, 0, 0, 0}));
    REQUIRE(f.drainUntilVariants(4));

    f.th.sendToSerialization(cmsg::AutoLoadSampleIntoLeadZone(autoLoadPath("Hat.wav")));
    f.th.stepUI(30);

    REQUIRE(f.zone().getNumSampleLoaded() == 1);
    REQUIRE(f.variantName(0) == "Hat.wav");
    for (auto i = 1U; i < scxt::maxVariantsPerZone; ++i)
    {
        INFO("variant " << i);
        REQUIRE(!f.zone().variantData.variants[i].active);
    }
}

TEST_CASE("Browser auto-load takes the loop from the sample, or leaves none",
          "[drop][autoload][loop]")
{
    DropFixture f;
    f.makeLeadZone();

    auto dir = fs::temp_directory_path() / "scxt-auto-load-tests";
    fs::remove_all(dir);
    auto looped = writeLoopedWav(dir, 100, 200);

    SECTION("a sample with loop points brings them in")
    {
        f.th.sendToSerialization(cmsg::AutoLoadSampleIntoLeadZone(looped.u8string()));
        f.th.stepUI(30);

        const auto &v = f.zone().variantData.variants[0];
        REQUIRE(v.loopActive);
        REQUIRE(v.startLoop == 100);
        REQUIRE(v.endLoop == 200);
        REQUIRE(v.startSample == 0);
    }

    SECTION("a sample with none loses the loop the last one had")
    {
        f.th.sendToSerialization(cmsg::AutoLoadSampleIntoLeadZone(looped.u8string()));
        f.th.stepUI(30);
        REQUIRE(f.zone().variantData.variants[0].loopActive);

        f.th.sendToSerialization(cmsg::AutoLoadSampleIntoLeadZone(autoLoadPath("Kick.wav")));
        f.th.stepUI(30);

        const auto &v = f.zone().variantData.variants[0];
        REQUIRE(f.variantName(0) == "Kick.wav");
        REQUIRE(!v.loopActive);
        REQUIRE(v.startSample == 0);
    }

    fs::remove_all(dir);
}

TEST_CASE("Browser auto-load is one undo step each time", "[drop][autoload][undo]")
{
    DropFixture f;
    f.makeLeadZone();
    f.th.sendToSerialization(cmsg::AddSamplesInZone({variantSamples(2), {}, 0, 0, 0}));
    REQUIRE(f.drainUntilVariants(3));

    auto baseSize = f.th.engine->undoManager.undoStackSize();

    f.th.sendToSerialization(cmsg::AutoLoadSampleIntoLeadZone(autoLoadPath("Hat.wav")));
    f.th.stepUI(30);
    REQUIRE(f.th.engine->undoManager.undoStackSize() == baseSize + 1);

    f.th.sendToSerialization(cmsg::AutoLoadSampleIntoLeadZone(autoLoadPath("Kick.wav")));
    f.th.stepUI(30);
    REQUIRE(f.th.engine->undoManager.undoStackSize() == baseSize + 2);

    // the second swap backs out to the first, and the first back to all three variants
    f.th.sendToSerialization(cmsg::Undo(true));
    f.th.stepUI(30);
    REQUIRE(f.variantName(0) == "Hat.wav");

    f.th.sendToSerialization(cmsg::Undo(true));
    f.th.stepUI(30);
    REQUIRE(f.zone().getNumSampleLoaded() == 3);
    REQUIRE(f.variantName(0) == "Beep.wav");
}

namespace
{
/*
 * Auto-load under a sounding voice. The browser can swap the sample while the zone is
 * still making noise, so this drives the engine directly rather than through the harness's
 * audio thread: every block here is one the test asked for.
 */
struct SoundingZone
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Zone *zone{nullptr};
    fs::path dir;

    SoundingZone()
    {
        dir = fs::temp_directory_path() / "scxt-auto-load-rates";
        fs::remove_all(dir);

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

        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        eng->getSelectionManager()->applySelectActions({0, 0, 0, true, true, true});
    }

    ~SoundingZone()
    {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }

    fs::path wav(const std::string &name, uint32_t rate) { return writeWav(dir, name, rate, 0, 0); }

    void autoLoad(const fs::path &p)
    {
        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        REQUIRE(eng->autoLoadSampleIntoLeadZone(p));
        // one block to pick the swap up off the queue
        eng->processAudio();
    }

    /*
     * How fast the newest voice on the zone eats its sample, per sample of output. An
     * oversampled voice runs its generator twice per output sample, so its ratio is half
     * the one you hear - fold that back in or two voices at the same pitch disagree.
     */
    double startNoteAndGetRate(int key)
    {
        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
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
        return (double)newest->GD[0].ratio * (newest->useOversampling ? 2 : 1);
    }
};
} // namespace

TEST_CASE("Browser auto-load tunes the next note to the sample it just loaded", "[drop][autoload]")
{
    /*
     * Swapping under a sounding voice and then playing had the new note come out at the
     * rate of the sample before it - an octave up between a 48k and a 24k file.
     */
    SoundingZone f;
    auto fast = f.wav("Fast.wav", 48000);
    auto slow = f.wav("Slow.wav", 24000);

    f.autoLoad(fast);
    auto atRoot = f.startNoteAndGetRate(60);
    REQUIRE(atRoot == Approx(1 << 24).epsilon(0.001));

    // the first note is still sounding while both of these land
    f.autoLoad(slow);
    f.autoLoad(fast);

    REQUIRE(f.startNoteAndGetRate(60) == Approx(atRoot).epsilon(0.001));
}

TEST_CASE("Browser auto-load with no zone to load into does nothing", "[drop][autoload]")
{
    DropFixture f;
    auto baseSize = f.th.engine->undoManager.undoStackSize();

    f.th.sendToSerialization(cmsg::AutoLoadSampleIntoLeadZone(autoLoadPath("Kick.wav")));
    f.th.stepUI(30);

    REQUIRE(f.part().getGroups().empty());
    REQUIRE(f.th.engine->undoManager.undoStackSize() == baseSize);
}

TEST_CASE("A zone whose voices end outside the group's walk leaves the group", "[drop][voices]")
{
    SoundingZone f;
    f.autoLoad(f.wav("Fast.wav", 48000));
    f.startNoteAndGetRate(60);

    auto &part = *f.eng->getPatch()->getPart(0);
    auto &group = *part.getGroup(0);
    REQUIRE(group.activeZones == 1);
    REQUIRE(part.activeGroups == 1);

    f.zone->terminateAllVoices();
    REQUIRE(!f.zone->isActive());
    REQUIRE(group.activeZones == 0);

    // the group still has to fade out and hand itself back to the part
    for (int i = 0; i < 200; ++i)
        f.eng->processAudio();
    REQUIRE(part.activeGroups == 0);

    REQUIRE(f.startNoteAndGetRate(60) == Approx(1 << 24).epsilon(0.001));
    REQUIRE(group.activeZones == 1);
    REQUIRE(part.activeGroups == 1);
}

TEST_CASE("A variant drop onto a sounding zone leaves the group", "[drop][variants][voices]")
{
    SoundingZone f;
    f.autoLoad(f.wav("Fast.wav", 48000));
    f.startNoteAndGetRate(60);

    auto &group = *f.eng->getPatch()->getPart(0)->getGroup(0);
    REQUIRE(group.activeZones == 1);

    {
        auto bypass = f.eng->getMessageController()->threadingChecker.bypassChecksInScope();
        f.eng->loadSamplesIntoZone({{1, f.wav("Slow.wav", 24000), std::nullopt}}, 0, 0, 0);
        f.eng->processAudio();
    }
    REQUIRE(!f.zone->isActive());
    REQUIRE(group.activeZones == 0);
}
