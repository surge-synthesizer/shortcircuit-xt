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
 * Dragging an effect from one slot onto another. The payload's last field says how it
 * resolves - swap exchanges the two, move clears the source, copy leaves it alone - and
 * both the part and the bus handler have to honour it the same way.
 */

#include "catch2/catch2.hpp"

#include "console_harness.h"
#include "engine/bus.h"
#include "engine/engine.h"
#include "engine/part.h"

namespace cmsg = scxt::messaging::client;
using scxt::engine::AvailableBusEffects;

namespace
{
// A param value distinct from any default, so the assertions can tell a real storage copy
// from a slot which merely ended up with the right effect type.
constexpr float marker{0.375f};

struct FXFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    FXFixture()
    {
        th.start();
        th.stepUI();
    }

    // bus >= 0 addresses a bus, otherwise part is used - the same convention the messages use
    void setSlot(int bus, int part, int slot, AvailableBusEffects type, float p0)
    {
        th.sendToSerialization(cmsg::SetBusEffectToType({bus, part, slot, (int)type}));
        th.stepUI(10);

        auto bes = storage(bus, part, slot);
        bes.params[0] = p0;
        th.sendToSerialization(cmsg::SetBusEffectStorage({bus, part, slot, bes}));
        th.stepUI(10);
    }

    scxt::engine::BusEffectStorage storage(int bus, int part, int slot)
    {
        if (bus >= 0)
            return th.engine->getPatch()
                ->busses.busByAddress((scxt::engine::BusAddress)bus)
                .busEffectStorage[slot];
        return th.engine->getPatch()->getPart(part)->partEffectStorage[slot];
    }
};
} // namespace

TEST_CASE("A part fx drag resolves per its action", "[fx_drag]")
{
    SECTION("swap exchanges the two slots")
    {
        FXFixture f;
        f.setSlot(-1, 0, 0, AvailableBusEffects::reverb1, marker);
        f.setSlot(-1, 0, 1, AvailableBusEffects::delay, marker * 2);

        f.th.sendToSerialization(cmsg::SwapPartFX({0, 0, 0, 1, cmsg::fx_swap}));
        f.th.stepUI(20);

        REQUIRE(f.storage(-1, 0, 0).type == AvailableBusEffects::delay);
        REQUIRE(f.storage(-1, 0, 0).params[0] == marker * 2);
        REQUIRE(f.storage(-1, 0, 1).type == AvailableBusEffects::reverb1);
        REQUIRE(f.storage(-1, 0, 1).params[0] == marker);
    }

    SECTION("move clears the source")
    {
        FXFixture f;
        f.setSlot(-1, 0, 0, AvailableBusEffects::reverb1, marker);
        f.setSlot(-1, 0, 1, AvailableBusEffects::delay, marker * 2);

        f.th.sendToSerialization(cmsg::SwapPartFX({0, 0, 0, 1, cmsg::fx_move}));
        f.th.stepUI(20);

        REQUIRE(f.storage(-1, 0, 0).type == AvailableBusEffects::none);
        REQUIRE(f.storage(-1, 0, 0).params[0] == 0.f);
        REQUIRE(f.storage(-1, 0, 1).type == AvailableBusEffects::reverb1);
        REQUIRE(f.storage(-1, 0, 1).params[0] == marker);
    }

    SECTION("copy leaves the source alone")
    {
        FXFixture f;
        f.setSlot(-1, 0, 0, AvailableBusEffects::reverb1, marker);
        f.setSlot(-1, 0, 1, AvailableBusEffects::delay, marker * 2);

        f.th.sendToSerialization(cmsg::SwapPartFX({0, 0, 0, 1, cmsg::fx_copy}));
        f.th.stepUI(20);

        REQUIRE(f.storage(-1, 0, 0).type == AvailableBusEffects::reverb1);
        REQUIRE(f.storage(-1, 0, 0).params[0] == marker);
        REQUIRE(f.storage(-1, 0, 1).type == AvailableBusEffects::reverb1);
        REQUIRE(f.storage(-1, 0, 1).params[0] == marker);
    }

    SECTION("an unknown action is refused rather than guessed at")
    {
        FXFixture f;
        f.setSlot(-1, 0, 0, AvailableBusEffects::reverb1, marker);
        f.setSlot(-1, 0, 1, AvailableBusEffects::delay, marker * 2);

        f.th.sendToSerialization(cmsg::SwapPartFX({0, 0, 0, 1, 74}));
        f.th.stepUI(20);

        REQUIRE(f.storage(-1, 0, 0).type == AvailableBusEffects::reverb1);
        REQUIRE(f.storage(-1, 0, 1).type == AvailableBusEffects::delay);
    }
}

TEST_CASE("A bus fx drag resolves per its action", "[fx_drag]")
{
    constexpr int bus{scxt::engine::MAIN_0};

    SECTION("swap exchanges the two slots")
    {
        FXFixture f;
        f.setSlot(bus, -1, 0, AvailableBusEffects::phaser, marker);
        f.setSlot(bus, -1, 1, AvailableBusEffects::flanger, marker * 2);

        f.th.sendToSerialization(cmsg::SwapBusFX({bus, 0, bus, 1, cmsg::fx_swap}));
        f.th.stepUI(20);

        REQUIRE(f.storage(bus, -1, 0).type == AvailableBusEffects::flanger);
        REQUIRE(f.storage(bus, -1, 0).params[0] == marker * 2);
        REQUIRE(f.storage(bus, -1, 1).type == AvailableBusEffects::phaser);
        REQUIRE(f.storage(bus, -1, 1).params[0] == marker);
    }

    SECTION("move clears the source")
    {
        FXFixture f;
        f.setSlot(bus, -1, 0, AvailableBusEffects::phaser, marker);
        f.setSlot(bus, -1, 1, AvailableBusEffects::flanger, marker * 2);

        f.th.sendToSerialization(cmsg::SwapBusFX({bus, 0, bus, 1, cmsg::fx_move}));
        f.th.stepUI(20);

        REQUIRE(f.storage(bus, -1, 0).type == AvailableBusEffects::none);
        REQUIRE(f.storage(bus, -1, 0).params[0] == 0.f);
        REQUIRE(f.storage(bus, -1, 1).type == AvailableBusEffects::phaser);
        REQUIRE(f.storage(bus, -1, 1).params[0] == marker);
    }

    SECTION("copy leaves the source alone")
    {
        FXFixture f;
        f.setSlot(bus, -1, 0, AvailableBusEffects::phaser, marker);
        f.setSlot(bus, -1, 1, AvailableBusEffects::flanger, marker * 2);

        f.th.sendToSerialization(cmsg::SwapBusFX({bus, 0, bus, 1, cmsg::fx_copy}));
        f.th.stepUI(20);

        REQUIRE(f.storage(bus, -1, 0).type == AvailableBusEffects::phaser);
        REQUIRE(f.storage(bus, -1, 0).params[0] == marker);
        REQUIRE(f.storage(bus, -1, 1).type == AvailableBusEffects::phaser);
        REQUIRE(f.storage(bus, -1, 1).params[0] == marker);
    }

    SECTION("a drag across two busses moves too")
    {
        FXFixture f;
        constexpr int other{scxt::engine::PART_0};
        f.setSlot(bus, -1, 0, AvailableBusEffects::phaser, marker);
        f.setSlot(other, -1, 0, AvailableBusEffects::flanger, marker * 2);

        f.th.sendToSerialization(cmsg::SwapBusFX({bus, 0, other, 0, cmsg::fx_move}));
        f.th.stepUI(20);

        REQUIRE(f.storage(bus, -1, 0).type == AvailableBusEffects::none);
        REQUIRE(f.storage(other, -1, 0).type == AvailableBusEffects::phaser);
        REQUIRE(f.storage(other, -1, 0).params[0] == marker);
    }
}
