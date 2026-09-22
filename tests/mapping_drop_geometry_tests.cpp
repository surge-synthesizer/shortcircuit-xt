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

#include "engine/drop_mapping.h"

namespace
{
scxt::engine::DropGeometry at(int n, float key, float fromTop)
{
    scxt::engine::DropGeometry g;
    g.nElements = n;
    g.key = key;
    g.fromTop = fromTop;
    return g;
}

// every range has to be a usable zone on a 0..127 keyboard
void requireWellFormed(const std::vector<scxt::engine::DropRange> &rs)
{
    REQUIRE(!rs.empty());
    for (const auto &r : rs)
    {
        INFO("root " << r.root << " key " << r.keyLo << ".." << r.keyHi << " vel " << r.velLo
                     << ".." << r.velHi);
        REQUIRE(r.keyLo >= 0);
        REQUIRE(r.keyHi <= 127);
        REQUIRE(r.keyLo <= r.keyHi);
        REQUIRE(r.root >= 0);
        REQUIRE(r.root <= 127);
        REQUIRE(r.velLo >= 0);
        REQUIRE(r.velHi <= 127);
        REQUIRE(r.velLo <= r.velHi);
    }
}
} // namespace

TEST_CASE("Drop Geometry Is Always Well Formed")
{
    SECTION("across the whole gesture surface")
    {
        for (int n : {1, 2, 3, 4, 5, 7, 12, 16, 64, 126, 127, 128, 200})
        {
            for (float key = 0.f; key <= 127.f; key += 7.f)
            {
                for (float top = 0.f; top <= 1.f; top += 0.05f)
                {
                    auto g = at(n, key, top);
                    for (auto mods : {0, 1, 2})
                    {
                        g.shift = (mods == 1);
                        g.alt = (mods == 2);
                        INFO("n " << n << " key " << key << " fromTop " << top << " mods " << mods);
                        requireWellFormed(scxt::engine::dropRangesFor(g));
                    }
                }
            }
        }
    }

    SECTION("degenerate element counts do not produce an empty result")
    {
        requireWellFormed(scxt::engine::dropRangesFor(at(0, 60, 0.5)));
        requireWellFormed(scxt::engine::dropRangesFor(at(-1, 60, 0.5)));
    }
}

TEST_CASE("Drop Geometry Element Counts")
{
    SECTION("one range per element when spreading over the keyboard")
    {
        for (int n : {2, 3, 5, 16})
        {
            auto r = scxt::engine::dropRangesFor(at(n, 60, 0.5));
            REQUIRE((int)r.size() == n);
        }
    }

    SECTION("one range per element when spreading over velocity")
    {
        for (int n : {2, 3, 5, 16})
        {
            auto g = at(n, 60, 0.5);
            g.shift = true;
            auto r = scxt::engine::dropRangesFor(g);
            REQUIRE((int)r.size() == n);
        }
    }

    SECTION("alt collapses to a single shared range")
    {
        auto g = at(7, 60, 0.5);
        g.alt = true;
        REQUIRE(scxt::engine::dropRangesFor(g).size() == 1);
    }

    SECTION("a mapped instrument ignores the gesture entirely")
    {
        auto g = at(7, 20, 0.1);
        g.isMappedInstrument = true;
        auto r = scxt::engine::dropRangesFor(g);
        REQUIRE(r.size() == 1);
        REQUIRE(r[0].keyLo == 0);
        REQUIRE(r[0].keyHi == 127);
    }
}

TEST_CASE("Drop Geometry Keyboard Spread")
{
    SECTION("two elements at the top of the travel stay on the keyboard")
    {
        // the pair is wider than the keyboard here, which used to underflow to 1.8e19
        auto r = scxt::engine::dropRangesFor(at(2, 60, 0.f));
        REQUIRE(r.size() == 2);
        requireWellFormed(r);
        REQUIRE(r[0].keyLo == 0);
    }

    SECTION("zones run left to right and do not leave gaps")
    {
        auto r = scxt::engine::dropRangesFor(at(4, 60, 0.5));
        REQUIRE(r.size() == 4);
        for (size_t i = 1; i < r.size(); ++i)
        {
            INFO("zone " << i);
            REQUIRE(r[i].keyLo >= r[i - 1].keyLo);
            REQUIRE(r[i].keyLo == r[i - 1].keyHi + 1);
        }
    }

    SECTION("each root sits inside its own zone")
    {
        for (int n : {2, 3, 5, 9})
        {
            auto r = scxt::engine::dropRangesFor(at(n, 64, 0.4));
            for (const auto &e : r)
            {
                INFO("n " << n << " root " << e.root);
                REQUIRE(e.root >= e.keyLo);
                REQUIRE(e.root <= e.keyHi);
            }
        }
    }

    SECTION("dropping at the bottom of the travel gives single key zones")
    {
        auto r = scxt::engine::dropRangesFor(at(3, 60, 1.f));
        REQUIRE(r.size() == 3);
        for (const auto &e : r)
            REQUIRE(e.keyLo == e.keyHi);
    }
}

TEST_CASE("Drop Geometry Velocity Spread")
{
    SECTION("velocity covers 0 to 127 with no gaps and no overlap")
    {
        for (int n : {2, 3, 4, 8})
        {
            auto g = at(n, 60, 0.5);
            g.shift = true;
            auto r = scxt::engine::dropRangesFor(g);
            REQUIRE((int)r.size() == n);
            REQUIRE(r.front().velLo == 0);
            REQUIRE(r.back().velHi == 127);
            for (size_t i = 1; i < r.size(); ++i)
            {
                INFO("n " << n << " band " << i);
                REQUIRE(r[i].velLo == r[i - 1].velHi + 1);
            }
        }
    }

    SECTION("every band shares one key range")
    {
        auto g = at(5, 60, 0.3);
        g.shift = true;
        auto r = scxt::engine::dropRangesFor(g);
        for (const auto &e : r)
        {
            REQUIRE(e.keyLo == r[0].keyLo);
            REQUIRE(e.keyHi == r[0].keyHi);
            REQUIRE(e.root == r[0].root);
        }
    }
}
