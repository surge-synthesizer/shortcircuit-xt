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

#include <cmath>
#include <set>

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
                    // plain, shift, alt, over the keyboard, over its lower half
                    for (auto variant : {0, 1, 2, 3, 4})
                    {
                        auto g = at(n, key, top);
                        g.shift = (variant == 1);
                        g.alt = (variant == 2);
                        g.overKeyboard = (variant >= 3);
                        g.inLowerKeyboardHalf = (variant == 4);
                        INFO("n " << n << " key " << key << " fromTop " << top << " variant "
                                  << variant);
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

TEST_CASE("Drop Geometry Span Ladder")
{
    SECTION("the pull runs from a single key to the whole keyboard")
    {
        REQUIRE(scxt::engine::zoneSpanAt(1.f) == 0);
        REQUIRE(scxt::engine::zoneSpanAt(0.f) == 127);
    }

    SECTION("the first 70% of the pull runs 0 to 24 evenly")
    {
        // fromTop is the other way up, so 70% of the pull ends at 0.3
        REQUIRE(scxt::engine::zoneSpanAt(0.3f) == 24);
        REQUIRE(scxt::engine::zoneSpanAt(0.65f) == 12);
        REQUIRE(scxt::engine::zoneSpanAt(1.f - 0.35f) == 12);

        // evenly: equal steps of pull give equal steps of span
        for (int i = 0; i <= 10; ++i)
        {
            auto pull = 0.7f * i / 10;
            INFO("pull " << pull);
            REQUIRE(scxt::engine::zoneSpanAt(1.f - pull) == (int16_t)std::round(pull / 0.7f * 24));
        }
    }

    SECTION("the last 30% climbs in octaves")
    {
        std::set<int> wide;
        for (float pull = 0.7001f; pull <= 1.f; pull += 0.001f)
            wide.insert(scxt::engine::zoneSpanAt(1.f - pull));

        std::set<int> expected{36, 48, 60, 72, 84, 96, 108, 120, 127};
        REQUIRE(wide == expected);
    }

    SECTION("span never widens as the cursor moves down")
    {
        auto prev = scxt::engine::zoneSpanAt(0.f);
        for (float t = 0.f; t <= 1.f; t += 0.005f)
        {
            auto w = scxt::engine::zoneSpanAt(t);
            INFO("fromTop " << t << " span " << w << " previous " << prev);
            REQUIRE(w <= prev);
            prev = w;
        }
    }

    SECTION("only the top 10% slides the start to make room")
    {
        REQUIRE(scxt::engine::isFitToKeyboardAt(0.f));
        REQUIRE(scxt::engine::isFitToKeyboardAt(0.05f));
        REQUIRE(!scxt::engine::isFitToKeyboardAt(0.2f));
        REQUIRE(!scxt::engine::isFitToKeyboardAt(1.f));
    }
}

TEST_CASE("Drop Geometry Anchors Left")
{
    SECTION("the cursor is the left edge of the spread, which extends right")
    {
        auto g = at(4, 40, 0.6);
        auto r = scxt::engine::dropRangesFor(g);
        auto w = scxt::engine::zoneSpanAt(0.6) + 1;

        REQUIRE(r.size() == 4);
        REQUIRE(r[0].keyLo == 40);
        for (size_t i = 0; i < r.size(); ++i)
        {
            INFO("zone " << i << " width " << w);
            REQUIRE(r[i].keyLo == 40 + (int)i * w);
            REQUIRE(r[i].keyHi == r[i].keyLo + w - 1);
        }
    }

    SECTION("the root of every zone in a spread is its own left edge")
    {
        for (float top : {0.2f, 0.5f, 0.8f})
        {
            auto r = scxt::engine::dropRangesFor(at(5, 30, top));
            for (const auto &e : r)
            {
                INFO("fromTop " << top);
                REQUIRE(e.root == e.keyLo);
            }
        }
    }

    SECTION("a single sample still starts at the cursor but roots in the middle")
    {
        auto span = scxt::engine::zoneSpanAt(0.5);
        auto r = scxt::engine::dropRangesFor(at(1, 48, 0.5));
        REQUIRE(r.size() == 1);
        REQUIRE(r[0].keyLo == 48);
        REQUIRE(r[0].keyHi == 48 + span);
        REQUIRE(r[0].root == (48 + 48 + span) / 2);
    }
}

TEST_CASE("Drop Geometry Scrunches The Overflow")
{
    SECTION("what does not fit piles onto the last note")
    {
        // wide zones from high up the keyboard cannot all fit
        auto r = scxt::engine::dropRangesFor(at(8, 120, 0.3));
        REQUIRE(r.size() == 8);
        requireWellFormed(r);
        REQUIRE(r.back().keyLo == 127);
        REQUIRE(r.back().keyHi == 127);
    }

    SECTION("a zone straddling the end is truncated, not moved")
    {
        auto w = scxt::engine::zoneSpanAt(0.5) + 1;
        auto r = scxt::engine::dropRangesFor(at(1, (float)(127 - w / 2), 0.5));
        REQUIRE(r[0].keyHi == 127);
        REQUIRE(r[0].keyLo == 127 - w / 2);
    }

    SECTION("dropping on the last key still gives one usable zone each")
    {
        auto r = scxt::engine::dropRangesFor(at(6, 127, 0.5));
        REQUIRE(r.size() == 6);
        requireWellFormed(r);
        for (const auto &e : r)
        {
            REQUIRE(e.keyLo == 127);
            REQUIRE(e.keyHi == 127);
        }
    }
}

TEST_CASE("Drop Geometry Fills The Keyboard At The Top")
{
    SECTION("the top of the pull gives every sample the whole keyboard")
    {
        auto r = scxt::engine::dropRangesFor(at(5, 60, 0.f));
        REQUIRE(r.size() == 5);
        for (const auto &e : r)
        {
            REQUIRE(e.keyLo == 0);
            REQUIRE(e.keyHi == 127);
            REQUIRE(e.velLo == 0);
            REQUIRE(e.velHi == 127);
        }
    }

    SECTION("overlapped zones stay separate rather than collapsing to variants")
    {
        // one range is the signal for a variant stack, so overlap must not send it
        auto r = scxt::engine::dropRangesFor(at(5, 60, 0.f));
        REQUIRE(r.size() == 5);
    }

    SECTION("the root stays under the cursor rather than snapping to zero")
    {
        auto r = scxt::engine::dropRangesFor(at(3, 72, 0.f));
        for (const auto &e : r)
            REQUIRE(e.root == 72);
    }
}

TEST_CASE("Drop Geometry Over The Keyboard")
{
    SECTION("the upper half distributes over velocity on one key")
    {
        auto g = at(4, 55, 1.f);
        g.overKeyboard = true;
        auto r = scxt::engine::dropRangesFor(g);

        REQUIRE(r.size() == 4);
        REQUIRE(r.front().velLo == 0);
        REQUIRE(r.back().velHi == 127);
        for (size_t i = 0; i < r.size(); ++i)
        {
            INFO("band " << i);
            REQUIRE(r[i].keyLo == 55);
            REQUIRE(r[i].keyHi == 55);
            if (i > 0)
                REQUIRE(r[i].velLo == r[i - 1].velHi + 1);
        }
    }

    SECTION("the lower half overlaps on one key with no velocity split")
    {
        auto g = at(4, 55, 1.f);
        g.overKeyboard = true;
        g.inLowerKeyboardHalf = true;
        auto r = scxt::engine::dropRangesFor(g);

        REQUIRE(r.size() == 4);
        for (const auto &e : r)
        {
            REQUIRE(e.keyLo == 55);
            REQUIRE(e.keyHi == 55);
            REQUIRE(e.velLo == 0);
            REQUIRE(e.velHi == 127);
        }
    }

    SECTION("the keyboard overrides the width the travel would have picked")
    {
        auto g = at(2, 55, 0.f); // would be the full overlap band in the mapping area
        g.overKeyboard = true;
        g.inLowerKeyboardHalf = true;
        auto r = scxt::engine::dropRangesFor(g);
        REQUIRE(r[0].keyLo == 55);
        REQUIRE(r[0].keyHi == 55);
    }
}

TEST_CASE("Drop Geometry Velocity Bend")
{
    auto split = [](int n, float bend) {
        auto g = at(n, 60, 0.5);
        g.shift = true;
        g.velocityBend = bend;
        return scxt::engine::dropRangesFor(g);
    };

    SECTION("a bend still tiles the whole velocity range")
    {
        for (float bend : {-1.f, -0.5f, 0.f, 0.5f, 1.f})
        {
            for (int n : {2, 3, 5, 9})
            {
                auto r = split(n, bend);
                INFO("bend " << bend << " n " << n);
                REQUIRE((int)r.size() == n);
                requireWellFormed(r);
                REQUIRE(r.front().velLo == 0);
                REQUIRE(r.back().velHi == 127);
                for (size_t i = 1; i < r.size(); ++i)
                    REQUIRE(r[i].velLo == r[i - 1].velHi + 1);
            }
        }
    }

    SECTION("no bend leaves the bands even")
    {
        auto r = split(4, 0.f);
        REQUIRE(r[0].velHi == 32);
        REQUIRE(r[1].velHi == 64);
        REQUIRE(r[2].velHi == 95);
    }

    SECTION("convex widens the soft layers, concave widens the loud ones")
    {
        auto even = split(4, 0.f);
        auto convex = split(4, 1.f);
        auto concave = split(4, -1.f);

        REQUIRE(convex[0].velHi > even[0].velHi);
        REQUIRE(concave[0].velHi < even[0].velHi);
    }

    SECTION("the bend is monotonic in its own parameter")
    {
        auto prev = split(5, -1.f)[0].velHi;
        for (float bend = -1.f; bend <= 1.f; bend += 0.1f)
        {
            auto v = split(5, bend)[0].velHi;
            INFO("bend " << bend << " first band ends at " << v << " previous " << prev);
            REQUIRE(v >= prev);
            prev = v;
        }
    }

    SECTION("bend is ignored when the drop is not splitting over velocity")
    {
        auto g = at(4, 60, 0.5);
        g.velocityBend = 1.f;
        for (const auto &e : scxt::engine::dropRangesFor(g))
        {
            REQUIRE(e.velLo == 0);
            REQUIRE(e.velHi == 127);
        }
    }
}

TEST_CASE("Drop Geometry Start Slides Only At The Top")
{
    SECTION("a single sample at the top of the pull fills the keyboard from anywhere")
    {
        for (float key : {0.f, 40.f, 60.f, 100.f, 127.f})
        {
            auto r = scxt::engine::dropRangesFor(at(1, key, 0.f));
            INFO("dropped at key " << key);
            REQUIRE(r.size() == 1);
            REQUIRE(r[0].keyLo == 0);
            REQUIRE(r[0].keyHi == 127);
        }
    }

    SECTION("just inside the top the start slides only as far as it needs to")
    {
        auto span = scxt::engine::zoneSpanAt(0.05f);
        REQUIRE(scxt::engine::isFitToKeyboardAt(0.05f));

        // low down there is room, so the zone stays under the cursor
        auto low = scxt::engine::dropRangesFor(at(1, 3, 0.05f));
        REQUIRE(low[0].keyLo == 3);

        // high up there is not, so it slides back to fit rather than truncating
        auto high = scxt::engine::dropRangesFor(at(1, 120, 0.05f));
        REQUIRE(high[0].keyLo == 127 - span);
        REQUIRE(high[0].keyHi == 127);
    }

    SECTION("below the top the zone truncates instead of sliding")
    {
        // 0.2 is outside the top 10% but still in the octave region
        REQUIRE(!scxt::engine::isFitToKeyboardAt(0.2f));
        auto r = scxt::engine::dropRangesFor(at(1, 120, 0.2f));
        REQUIRE(r[0].keyLo == 120);
        REQUIRE(r[0].keyHi == 127);
    }

    SECTION("a spread's roots follow the cursor once the start has slid away from it")
    {
        auto r = scxt::engine::dropRangesFor(at(3, 90, 0.f));
        for (const auto &e : r)
        {
            REQUIRE(e.keyLo == 0);
            REQUIRE(e.root == 90);
        }
    }
}

TEST_CASE("Drop Geometry Single Sample Roots In The Middle")
{
    SECTION("one sample centres its root across the pull")
    {
        for (float top : {0.1f, 0.35f, 0.6f, 0.9f})
        {
            auto r = scxt::engine::dropRangesFor(at(1, 50, top));
            INFO("fromTop " << top << " key " << r[0].keyLo << ".." << r[0].keyHi);
            REQUIRE(r.size() == 1);
            REQUIRE(r[0].root == (r[0].keyLo + r[0].keyHi) / 2);
        }
    }

    SECTION("a one key zone still roots on that key")
    {
        auto r = scxt::engine::dropRangesFor(at(1, 50, 1.f));
        REQUIRE(r[0].keyLo == 50);
        REQUIRE(r[0].keyHi == 50);
        REQUIRE(r[0].root == 50);
    }

    SECTION("filling the keyboard with one sample roots it in the middle")
    {
        auto r = scxt::engine::dropRangesFor(at(1, 100, 0.f));
        REQUIRE(r[0].keyLo == 0);
        REQUIRE(r[0].keyHi == 127);
        REQUIRE(r[0].root == 63);
    }

    SECTION("two samples go back to the lower edge")
    {
        auto r = scxt::engine::dropRangesFor(at(2, 50, 0.6));
        REQUIRE(r.size() == 2);
        for (const auto &e : r)
            REQUIRE(e.root == e.keyLo);
    }

    SECTION("a variant stack is a multi sample drop, so it roots at the edge")
    {
        auto g = at(4, 50, 0.6);
        g.alt = true;
        auto r = scxt::engine::dropRangesFor(g);
        REQUIRE(r.size() == 1);
        REQUIRE(r[0].root == r[0].keyLo);
    }
}
