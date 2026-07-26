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

#ifndef SCXT_TESTS_TEST_UTILS_H
#define SCXT_TESTS_TEST_UTILS_H

/*
 * Small helpers shared by more than one test file. They live in a header rather than being
 * repeated per .cpp because a unity build folds every test into one translation unit, where
 * duplicate definitions collide.
 */

#include <filesystem>
#include <string>
#include <memory>

#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "voice/voice.h"

#ifndef SCXT_TEST_SOURCE_DIR
#define SCXT_TEST_SOURCE_DIR ""
#endif

constexpr double TEST_SAMPLE_RATE = 48000.0;

inline std::filesystem::path samplePath(const std::string &relative)
{
    return std::filesystem::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" / relative;
}

/*
 * An engine driven directly on the test thread - no ConsoleHarness, no background audio
 * thread - which is enough to exercise real voice creation.
 */
inline scxt::engine::Engine *makeEngine()
{
    auto *e = new scxt::engine::Engine();
    e->prepareToPlay(TEST_SAMPLE_RATE);
    // Pin tuning to 12-TET so any MTS-ESP master running on the dev box
    // doesn't remap test keys out of the zone ranges.
    e->midikeyRetuner.setTuningMode(scxt::tuning::MidikeyRetuner::TWELVE_TET);
    return e;
}

// A zone with no sample. The engine still makes voices for these; the generator is silent.
inline void addBlankZoneToGroup(scxt::engine::Part &part, int groupIdx, int keyLo, int keyHi)
{
    auto z = std::make_unique<scxt::engine::Zone>();
    z->mapping.keyboardRange.keyStart = keyLo;
    z->mapping.keyboardRange.keyEnd = keyHi;
    z->mapping.velocityRange.velStart = 0;
    z->mapping.velocityRange.velEnd = 127;
    z->initialize();
    part.getGroup(groupIdx)->addZone(z);
}

// Voices that exist and have not been choked
inline int countLiveVoicesInGroup(const scxt::engine::Group &grp)
{
    int n = 0;
    for (const auto &zone : grp.getZones())
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            const auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned && v->terminationSequence < 0)
                n++;
        }
    return n;
}

// Live voices for one specific key. A released voice still rings, so a test that presses
// several keys in sequence has to ask about the key it cares about, not the whole group.
inline int countLiveVoicesForKey(const scxt::engine::Group &grp, int key)
{
    int n = 0;
    for (const auto &zone : grp.getZones())
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            const auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned && v->terminationSequence < 0 && v->key == key)
                n++;
        }
    return n;
}

#endif // SCXT_TESTS_TEST_UTILS_H
