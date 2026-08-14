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
 * Variant selection tactics - the round robin / random cycle state machine in
 * Zone::advanceVariantIndex. RANDOM_CYCLE promises each variant plays once before any
 * repeats, which also means it must not repeat across a cycle boundary.
 */

#include "catch2/catch2.hpp"

#include <memory>
#include <set>
#include <vector>

#include "configuration.h"
#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"

#include "test_utils.h"

using Zone = scxt::engine::Zone;

namespace
{
/*
 * A zone whose variants are loaded as far as the variant tactics are concerned.
 * advanceVariantIndex only asks getNumSampleLoaded() for a count, so marking the slots
 * active is enough to drive it and keeps this test off the sample loader.
 */
Zone *addVariantZone(scxt::engine::Part &part, int groupIdx, int nVariants,
                     Zone::VariantPlaybackMode mode, int key)
{
    auto z = std::make_unique<Zone>();
    z->mapping.keyboardRange.keyStart = key;
    z->mapping.keyboardRange.keyEnd = key;
    z->mapping.velocityRange.velStart = 0;
    z->mapping.velocityRange.velEnd = 127;
    for (int i = 0; i < nVariants; ++i)
        z->variantData.variants[i].active = true;
    z->variantData.variantPlaybackMode = mode;
    z->initialize();

    auto *res = z.get();
    part.getGroup(groupIdx)->addZone(z);
    return res;
}

/*
 * Press and release one key n times, collecting the variant each press chose. The zone has no
 * sample attached so no voice actually starts, but the note on still runs the tactic.
 */
std::vector<int> variantsFromKeyPresses(int nVariants, Zone::VariantPlaybackMode mode, int n,
                                        uint32_t seed = 8675309)
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    eng->rng.reseed(seed); // so a failure reproduces

    constexpr int key{60};
    auto &part = *eng->getPatch()->getPart(0);
    part.addGroup();
    auto *zone = addVariantZone(part, 0, nVariants, mode, key);

    std::vector<int> out;
    out.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        eng->processNoteOnEvent(0, 0, key, -1, 1.f, 0.f);
        out.push_back(zone->sampleIndex);
        eng->processNoteOffEvent(0, 0, key, -1, 0.f);
    }
    return out;
}

/*
 * The same sequence taken straight off the tactic rather than through a note on. Cheap enough
 * to sweep seeds and variant counts with, where the key press version is not.
 */
std::vector<int> variantsFromTactic(int nVariants, Zone::VariantPlaybackMode mode, int n,
                                    uint32_t seed)
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    eng->rng.reseed(seed);

    auto &part = *eng->getPatch()->getPart(0);
    part.addGroup();
    auto *zone = addVariantZone(part, 0, nVariants, mode, 60);

    std::vector<int> out;
    out.reserve(n);
    for (int i = 0; i < n; ++i)
        out.push_back(zone->advanceVariantIndex());
    return out;
}

// count of adjacent equal entries
int backToBackRepeats(const std::vector<int> &seq)
{
    int n{0};
    for (size_t i = 1; i < seq.size(); ++i)
        if (seq[i] == seq[i - 1])
            n++;
    return n;
}
} // namespace

/*
 * Both cycle properties off one 10000 press run - a press through the engine is not free, so
 * don't pay for the sequence twice.
 */
TEST_CASE("A five variant random cycle over 10000 key presses", "[variants]")
{
    constexpr int nVariants{5};
    constexpr int nPresses{10000};

    auto seq = variantsFromKeyPresses(nVariants, Zone::RANDOM_CYCLE, nPresses);
    REQUIRE(seq.size() == nPresses);

    // every variant plays once per cycle
    for (size_t base = 0; base + nVariants <= seq.size(); base += nVariants)
    {
        std::set<int> cycle(seq.begin() + base, seq.begin() + base + nVariants);
        INFO("cycle starting at press " << base);
        REQUIRE(cycle.size() == nVariants);
        REQUIRE(*cycle.begin() == 0);
        REQUIRE(*cycle.rbegin() == nVariants - 1);
    }

    // and no variant repeats back to back. The only place that can happen is the cycle
    // boundary, where the refill re-picks the variant the last cycle ended on.
    int firstAt{-1};
    for (size_t i = 1; i < seq.size() && firstAt < 0; ++i)
        if (seq[i] == seq[i - 1])
            firstAt = (int)i;

    INFO("first back to back repeat at press " << firstAt << " (variant "
                                               << (firstAt >= 0 ? seq[firstAt] : -1) << ")");
    REQUIRE(backToBackRepeats(seq) == 0);
}

/*
 * The boundary repeat only shows up when the refill happens to draw the variant the last cycle
 * ended on, so sweep seeds and cycle lengths rather than trusting one lucky sequence.
 */
TEST_CASE("Random cycle holds across seeds and variant counts", "[variants]")
{
    // three is the smallest count that reaches the tactic at all - one and two variants are
    // special cased above it
    for (int nVariants = 3; nVariants <= (int)scxt::maxVariantsPerZone; ++nVariants)
    {
        for (uint32_t seed : {1u, 17u, 1729u, 8675309u})
        {
            INFO("nVariants " << nVariants << " seed " << seed);
            auto seq = variantsFromTactic(nVariants, Zone::RANDOM_CYCLE, 2000, seed);
            REQUIRE(backToBackRepeats(seq) == 0);

            int malformedCycles{0};
            for (size_t base = 0; base + nVariants <= seq.size(); base += nVariants)
            {
                std::set<int> cycle(seq.begin() + base, seq.begin() + base + nVariants);
                if ((int)cycle.size() != nVariants)
                    malformedCycles++;
            }
            REQUIRE(malformedCycles == 0);
        }
    }
}

TEST_CASE("Forward round robin walks the variants in order", "[variants]")
{
    constexpr int nVariants{5};

    auto seq = variantsFromKeyPresses(nVariants, Zone::FORWARD_RR, 100);
    for (size_t i = 1; i < seq.size(); ++i)
    {
        INFO("press " << i);
        REQUIRE(seq[i] == (seq[i - 1] + 1) % nVariants);
    }
}

TEST_CASE("Random no repeat never repeats a variant back to back", "[variants]")
{
    constexpr int nVariants{5};

    auto seq = variantsFromKeyPresses(nVariants, Zone::RANDOM_NOREPEAT, 2000);
    int repeats{0};
    for (size_t i = 1; i < seq.size(); ++i)
        if (seq[i] == seq[i - 1])
            repeats++;
    REQUIRE(repeats == 0);
}
