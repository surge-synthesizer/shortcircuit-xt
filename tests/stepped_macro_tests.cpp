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
#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>

#include "console_harness.h"
#include "engine/engine.h"
#include "engine/macros.h"
#include "json/engine_traits.h"
#include "messaging/client/client_messages.h"

namespace cmsg = scxt::messaging::client;
using mac = scxt::engine::Macro;

namespace
{
mac unstreamMacro(const std::string &s)
{
    tao::json::events::transformer<tao::json::events::to_basic_value<scxt::json::scxt_traits>>
        consumer;
    tao::json::events::from_string(consumer, s);
    mac res;
    consumer.value.to(res);
    return res;
}

bool isOnAStep(const mac &m)
{
    for (int i = 0; i < m.steps; ++i)
        if (m.value == Approx(m.valueForStepIndex(i)).margin(1e-6))
            return true;
    return false;
}
} // namespace

TEST_CASE("Stepped macro quantization", "[macros]")
{
    SECTION("Unipolar steps split the range into equal buckets")
    {
        mac m;
        m.setMode(mac::UNIPOLAR, 5);
        REQUIRE(m.isStepped());
        REQUIRE(!m.isBipolar());

        auto check = [&m](float in, float out) {
            INFO("in=" << in);
            m.setValueConstrained(in);
            REQUIRE(m.value == Approx(out).margin(1e-6));
        };
        check(0.0f, 0.0f);
        check(0.19f, 0.0f);
        check(0.21f, 0.25f);
        check(0.39f, 0.25f);
        check(0.41f, 0.5f);
        check(0.59f, 0.5f);
        check(0.61f, 0.75f);
        check(0.79f, 0.75f);
        check(0.81f, 1.0f);
        check(1.0f, 1.0f);
        check(-3.0f, 0.0f);
        check(7.0f, 1.0f);
    }

    SECTION("Bipolar odd steps always contain zero and both ends")
    {
        for (int16_t s : {3, 5, 9, 17, 25, 33})
        {
            INFO("steps=" << s);
            mac m;
            m.setMode(mac::BIPOLAR, s);
            m.setValueConstrained(0.f);
            REQUIRE(m.value == 0.f);
            m.setValue01(0.5f);
            REQUIRE(m.value == 0.f);
            m.setValueConstrained(-1.f);
            REQUIRE(m.value == -1.f);
            m.setValueConstrained(1.f);
            REQUIRE(m.value == 1.f);
        }
    }

    SECTION("Every step survives being constrained again")
    {
        for (auto md : {mac::UNIPOLAR, mac::BIPOLAR})
        {
            for (int16_t s : {3, 4, 5, 8, 9, 12, 13, 16, 17, 25, 33})
            {
                mac m;
                m.setMode(md, s);
                for (int i = 0; i < s; ++i)
                {
                    INFO("mode=" << md << " steps=" << s << " idx=" << i);
                    m.setValueConstrained(m.valueForStepIndex(i));
                    REQUIRE(m.stepIndex() == i);
                    auto v = m.value;
                    m.setValueConstrained(v);
                    REQUIRE(m.value == v);
                    m.setValue01(m.getValue01());
                    REQUIRE(m.value == v);
                }
            }
        }
    }

    SECTION("Changing mode quantizes the current value and clears steps")
    {
        mac m;
        m.setValueConstrained(0.6f);
        m.setMode(mac::UNIPOLAR, 3);
        REQUIRE(m.value == Approx(0.5f));

        m.setMode(mac::UNIPOLAR);
        REQUIRE(m.steps == 0);
        REQUIRE(!m.isStepped());
        m.setValueConstrained(0.6f);
        REQUIRE(m.value == Approx(0.6f));

        m.setMode(mac::TOGGLE, 8);
        REQUIRE(m.steps == 0);
        REQUIRE(!m.isStepped());
    }

    SECTION("Step counts are continuous or within the min and max")
    {
        REQUIRE(mac::validSteps(-4) == 0);
        REQUIRE(mac::validSteps(0) == 0);
        REQUIRE(mac::validSteps(1) == 0);
        REQUIRE(mac::validSteps(mac::minSteps) == mac::minSteps);
        REQUIRE(mac::validSteps(mac::maxSteps) == mac::maxSteps);
        REQUIRE(mac::validSteps(30000) == mac::maxSteps);

        mac m;
        m.setMode(mac::UNIPOLAR, 30000);
        REQUIRE(m.steps == mac::maxSteps);
        m.setValueConstrained(0.5f);
        REQUIRE(m.value == Approx(0.5f));

        m.setMode(mac::BIPOLAR, -4);
        REQUIRE(m.steps == 0);
        REQUIRE(!m.isStepped());

        m.setMode(mac::UNIPOLAR, 1);
        REQUIRE(m.steps == 0);
        REQUIRE(!m.isStepped());

        m.setMode(mac::UNIPOLAR, mac::minSteps);
        REQUIRE(m.isStepped());
        m.setValueConstrained(0.4f);
        REQUIRE(m.value == 0.f);
        m.setValueConstrained(0.6f);
        REQUIRE(m.value == 1.f);
    }

    SECTION("A single step is continuous")
    {
        mac m;
        m.steps = 1;
        REQUIRE(!m.isStepped());
        m.setValueConstrained(0.37f);
        REQUIRE(m.value == Approx(0.37f));
    }

    SECTION("Host strings show the step a value lands on")
    {
        mac m;
        m.setMode(mac::BIPOLAR, 3);
        REQUIRE(m.value01ToString(0.1f) == "-1/1 (-1.0000)");
        REQUIRE(m.value01ToString(0.5f) == "0/1 (.0000)");
        REQUIRE(m.value01ToString(0.9f) == "1/1 (1.0000)");

        m.setMode(mac::BIPOLAR, 25);
        REQUIRE(m.value01ToString(m.value01ForStepIndex(5)) == "-7/12 (-.5833)");
        REQUIRE(m.value01ToString(m.value01ForStepIndex(5)) == m.stepIndexToString(5));
    }

    SECTION("Host strings read back to the same step")
    {
        for (auto md : {mac::UNIPOLAR, mac::BIPOLAR})
        {
            for (int16_t s : {3, 4, 5, 8, 9, 12, 13, 16, 17, 25, 33})
            {
                mac m;
                m.setMode(md, s);
                for (int i = 0; i < s; ++i)
                {
                    auto str = m.value01ToString(m.value01ForStepIndex(i));
                    INFO("mode=" << md << " steps=" << s << " idx=" << i << " str=" << str);
                    auto v01 = m.value01FromString(str);
                    REQUIRE(v01 == Approx(m.value01ForStepIndex(i)).margin(1e-6));
                    REQUIRE(m.stepIndexFor01(v01) == i);
                }
            }
        }
    }

    SECTION("Host text snaps to the nearest step")
    {
        mac m;
        m.setMode(mac::BIPOLAR, 25);
        REQUIRE(m.valueFromString("-7/12") == Approx(-7.f / 12));
        REQUIRE(m.valueFromString("-.5833") == Approx(-7.f / 12));
        REQUIRE(m.valueFromString("(-.5833)") == Approx(-7.f / 12));
        REQUIRE(m.valueFromString(" -7 / 12 ") == Approx(-7.f / 12));
        REQUIRE(m.valueFromString("-0.6") == Approx(-7.f / 12));
        REQUIRE(m.valueFromString("3") == 1.f);
        REQUIRE(m.valueFromString("-40/12") == -1.f);
        REQUIRE(m.valueFromString("1/0") == Approx(1.f));
        REQUIRE(m.valueFromString("") == 0.f);
        REQUIRE(m.valueFromString("nan") == 0.f);
        REQUIRE(m.valueFromString("rubbish") == 0.f);

        m.setMode(mac::UNIPOLAR, 5);
        REQUIRE(m.value01FromString("0.2") == Approx(0.25f));
        REQUIRE(m.value01FromString("0.1") == 0.f);
        REQUIRE(m.value01FromString("3/4") == Approx(0.75f));
        REQUIRE(m.value01FromString("2/3") == Approx(0.75f));

        m.setMode(mac::UNIPOLAR);
        REQUIRE(m.valueFromString("1/4") == Approx(0.25f));
        REQUIRE(m.valueFromString("0.37") == Approx(0.37f));
    }

    SECTION("Step strings show a fraction and its value")
    {
        mac m;
        m.setMode(mac::BIPOLAR, 25);
        REQUIRE(m.stepIndexToString(5) == "-7/12 (-.5833)");
        REQUIRE(m.stepIndexToString(0) == "-12/12 (-1.0000)");
        REQUIRE(m.stepIndexToString(12) == "0/12 (.0000)");
        REQUIRE(m.stepIndexToString(18) == "6/12 (.5000)");
        REQUIRE(m.stepIndexToString(24) == "12/12 (1.0000)");

        m.setMode(mac::UNIPOLAR, 5);
        REQUIRE(m.stepIndexToString(0) == "0/4 (.0000)");
        REQUIRE(m.stepIndexToString(3) == "3/4 (.7500)");
        REQUIRE(m.stepIndexToString(4) == "4/4 (1.0000)");

        // no zero step so the halves stay in the fraction
        m.setMode(mac::BIPOLAR, 4);
        REQUIRE(m.stepIndexToString(1) == "-1/3 (-.3333)");
    }
}

TEST_CASE("Stepped macro streaming", "[macros]")
{
    SECTION("Steps round trip")
    {
        mac m;
        m.index = 2;
        m.setMode(mac::BIPOLAR, 9);
        m.setValueConstrained(0.25f);

        auto s = tao::json::to_string(scxt::json::scxt_value(m));
        auto r = unstreamMacro(s);
        REQUIRE(r.mode == mac::BIPOLAR);
        REQUIRE(r.steps == 9);
        REQUIRE(r.value == m.value);
    }

    SECTION("Continuous macros do not write steps")
    {
        mac m;
        m.index = 0;
        auto s = tao::json::to_string(scxt::json::scxt_value(m));
        REQUIRE(s.find("\"st\"") == std::string::npos);
    }

    SECTION("Oversized step counts are capped on load")
    {
        auto r = unstreamMacro(R"({"p":0,"i":1,"v":0.5,"st":30000})");
        REQUIRE(r.steps == mac::maxSteps);

        auto one = unstreamMacro(R"({"p":0,"i":1,"v":0.5,"st":1})");
        REQUIRE(one.steps == 0);
        REQUIRE(!one.isStepped());
    }

    SECTION("Macros saved before steps load as continuous")
    {
        auto r = unstreamMacro(R"({"p":0,"i":1,"v":0.3,"md":{"e":"bipolar"}})");
        REQUIRE(r.mode == mac::BIPOLAR);
        REQUIRE(r.steps == 0);
        REQUIRE(!r.isStepped());
    }
}

TEST_CASE("Stepped macro messages", "[macros]")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    auto &macro = th.engine->getPatch()->getPart(0)->macros[0];

    auto stepped = macro;
    stepped.setMode(mac::UNIPOLAR, 5);
    th.sendToSerialization(cmsg::SetMacroFullState({(int16_t)0, (int16_t)0, stepped}));
    th.stepUI();
    REQUIRE(macro.isStepped());
    REQUIRE(macro.steps == 5);

    th.sendToSerialization(cmsg::SetMacroValue({(int16_t)0, (int16_t)0, 0.55f}));
    th.stepUI(20);
    REQUIRE(isOnAStep(macro));
    REQUIRE(macro.value == Approx(0.5f).margin(1e-6));

    // a client bypassing setMode still cannot store an oversized count
    auto oversized = macro;
    oversized.steps = 30000;
    th.sendToSerialization(cmsg::SetMacroFullState({(int16_t)0, (int16_t)0, oversized}));
    th.stepUI();
    REQUIRE(macro.steps == mac::maxSteps);

    auto single = macro;
    single.steps = 1;
    th.sendToSerialization(cmsg::SetMacroFullState({(int16_t)0, (int16_t)0, single}));
    th.stepUI();
    REQUIRE(macro.steps == 0);
    REQUIRE(!macro.isStepped());

    for (int i = 0; i < 10 && th.engine->undoManager.hasUndoSteps(); ++i)
    {
        th.sendToSerialization(cmsg::Undo(true));
        th.stepUI(20);
    }
    REQUIRE(!macro.isStepped());
}
