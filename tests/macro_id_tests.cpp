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

#include "engine/macros.h"

using mac = scxt::engine::Macro;

TEST_CASE("Macro CLAP Param IDs", "[macros]")
{
    SECTION("Valid IDs round trip")
    {
        for (uint32_t p = 0; p < scxt::numParts; ++p)
        {
            for (uint32_t m = 0; m < scxt::macrosPerPart; ++m)
            {
                auto id = mac::partIndexToMacroID(p, m);
                REQUIRE(mac::isMacroID(id));
                REQUIRE(mac::macroIDToPart(id) == (int)p);
                REQUIRE(mac::macroIDToIndex(id) == (int)m);
            }
        }
    }

    SECTION("Out of range IDs are rejected")
    {
        REQUIRE_FALSE(mac::isMacroID(0));
        REQUIRE_FALSE(mac::isMacroID(mac::firstMacroParam - 1));

        // index past macrosPerPart within an otherwise valid part stride
        REQUIRE_FALSE(mac::isMacroID(mac::partIndexToMacroID(0, scxt::macrosPerPart)));
        REQUIRE_FALSE(mac::isMacroID(mac::partIndexToMacroID(3, mac::macroParamPartStride - 1)));

        // part at and past numParts
        REQUIRE_FALSE(mac::isMacroID(mac::partIndexToMacroID(scxt::numParts, 0)));
        REQUIRE_FALSE(mac::isMacroID(mac::partIndexToMacroID(scxt::numParts + 1, 0)));

        REQUIRE_FALSE(mac::isMacroID(0xFFFFFFFF));
    }
}
