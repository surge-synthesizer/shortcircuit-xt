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
 * Absolute zone bound edits - the type-in path behind c2s_apply_zone_delta with abs set.
 * Unlike the drag path these take the new bound outright rather than a delta, so the range
 * check in canApplyAbsoluteBoundEdit is the only thing keeping a client supplied value in
 * the MIDI range.
 *
 * The key dimensions carry their value in newX and the velocity dimensions in newY; the
 * caller zeroes the unused axis, so these pass 0 for it the way the UI does.
 */

#include "catch2/catch2.hpp"

#include "engine/zone.h"

using Zone = scxt::engine::Zone;
using scxt::engine::KeyboardRange;
using scxt::engine::VelocityRange;

namespace
{
// canApply and apply have to agree: anything canApply accepts must actually land, and
// anything it rejects must leave the range alone. Returns the range after the attempt.
KeyboardRange applyKey(Zone::ChangeDimension dim, int newX, KeyboardRange kr)
{
    VelocityRange vr;
    if (Zone::canApplyAbsoluteBoundEdit(dim, newX, 0, kr, vr))
        Zone::applyAbsoluteBoundEdit(dim, newX, 0, kr, vr);
    return kr;
}

VelocityRange applyVel(Zone::ChangeDimension dim, int newY, VelocityRange vr)
{
    KeyboardRange kr{48, 72};
    if (Zone::canApplyAbsoluteBoundEdit(dim, 0, newY, kr, vr))
        Zone::applyAbsoluteBoundEdit(dim, 0, newY, kr, vr);
    return vr;
}
} // namespace

TEST_CASE("An absolute key end edit stays in the MIDI range", "[bound_edit]")
{
    KeyboardRange kr{48, 72};

    SECTION("an in range value applies")
    {
        REQUIRE(Zone::canApplyAbsoluteBoundEdit(Zone::KEY_RANGE_END, 96, 0, kr, VelocityRange{}));
        REQUIRE(applyKey(Zone::KEY_RANGE_END, 96, kr).keyEnd == 96);
    }

    SECTION("the top of the range applies")
    {
        REQUIRE(applyKey(Zone::KEY_RANGE_END, 127, kr).keyEnd == 127);
    }

    SECTION("collapsing onto the start applies - a single key zone has start == end")
    {
        REQUIRE(applyKey(Zone::KEY_RANGE_END, 48, kr).keyEnd == 48);
    }

    SECTION("past the top of the keyboard is refused")
    {
        REQUIRE(!Zone::canApplyAbsoluteBoundEdit(Zone::KEY_RANGE_END, 128, 0, kr, VelocityRange{}));
        REQUIRE(applyKey(Zone::KEY_RANGE_END, 128, kr).keyEnd == 72);
        REQUIRE(applyKey(Zone::KEY_RANGE_END, 9999, kr).keyEnd == 72);
    }

    SECTION("below the start is refused")
    {
        REQUIRE(!Zone::canApplyAbsoluteBoundEdit(Zone::KEY_RANGE_END, 47, 0, kr, VelocityRange{}));
        REQUIRE(applyKey(Zone::KEY_RANGE_END, 47, kr).keyEnd == 72);
        REQUIRE(applyKey(Zone::KEY_RANGE_END, -5, kr).keyEnd == 72);
    }

    SECTION("the velocity axis does not decide a key edit")
    {
        // newY is the velocity coordinate and has no bearing here. A client which sends a
        // wild one must not change the outcome either way.
        REQUIRE(Zone::canApplyAbsoluteBoundEdit(Zone::KEY_RANGE_END, 96, 999, kr, VelocityRange{}));
        REQUIRE(
            !Zone::canApplyAbsoluteBoundEdit(Zone::KEY_RANGE_END, 128, 64, kr, VelocityRange{}));
    }
}

TEST_CASE("An absolute key start edit stays in the MIDI range", "[bound_edit]")
{
    KeyboardRange kr{48, 72};

    REQUIRE(applyKey(Zone::KEY_RANGE_START, 24, kr).keyStart == 24);
    REQUIRE(applyKey(Zone::KEY_RANGE_START, 0, kr).keyStart == 0);

    REQUIRE(applyKey(Zone::KEY_RANGE_START, -1, kr).keyStart == 48);
    REQUIRE(applyKey(Zone::KEY_RANGE_START, 128, kr).keyStart == 48);
    // start must stay below end
    REQUIRE(applyKey(Zone::KEY_RANGE_START, 80, kr).keyStart == 48);
}

TEST_CASE("An absolute velocity edit agrees with its own validation", "[bound_edit]")
{
    SECTION("full velocity is reachable")
    {
        VelocityRange vr{0, 100};
        REQUIRE(Zone::canApplyAbsoluteBoundEdit(Zone::VEL_RANGE_END, 0, 127, KeyboardRange{48, 72},
                                                vr));
        REQUIRE(applyVel(Zone::VEL_RANGE_END, 127, vr).velEnd == 127);
    }

    SECTION("a start one below the end is not a silent no-op")
    {
        // canApply accepts velEnd - 1, so apply has to write it rather than validate and drop it
        VelocityRange vr{0, 100};
        REQUIRE(Zone::canApplyAbsoluteBoundEdit(Zone::VEL_RANGE_START, 0, 99, KeyboardRange{48, 72},
                                                vr));
        REQUIRE(applyVel(Zone::VEL_RANGE_START, 99, vr).velStart == 99);
    }

    SECTION("out of range velocities are refused")
    {
        VelocityRange vr{0, 100};
        REQUIRE(applyVel(Zone::VEL_RANGE_END, 128, vr).velEnd == 100);
        REQUIRE(applyVel(Zone::VEL_RANGE_END, -1, vr).velEnd == 100);
        REQUIRE(applyVel(Zone::VEL_RANGE_START, 128, vr).velStart == 0);
        REQUIRE(applyVel(Zone::VEL_RANGE_START, -1, vr).velStart == 0);
        // and start may not cross end
        REQUIRE(applyVel(Zone::VEL_RANGE_START, 110, vr).velStart == 0);
    }
}
