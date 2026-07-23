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

#ifndef SCXT_SRC_SCXT_CORE_ENGINE_MACROS_H
#define SCXT_SRC_SCXT_CORE_ENGINE_MACROS_H

#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <string>
#include <fmt/core.h>

#include "sst/basic-blocks/params/ParamMetadata.h"
#include "configuration.h"
#include "utils.h"

namespace scxt::engine
{
struct Macro
{
    float value{0.f}; // -1 .. 1

    int16_t part{-1}, index{-1};

    enum Mode
    {
        UNIPOLAR, // 0 .. 1
        BIPOLAR,  // -1 .. 1
        TOGGLE,   // 0 or 1 only; snapped at 0.5
    } mode{UNIPOLAR};
    DECLARE_ENUM_STRING(Mode);

    std::string name{};

    bool isBipolar() const { return mode == BIPOLAR; }
    bool isToggle() const { return mode == TOGGLE; }

    void setMode(Mode m)
    {
        mode = m;
        setValueConstrained(value);
    }
    void setValueConstrained(float f)
    {
        if (mode == TOGGLE)
            value = (f > 0.5f ? 1.f : 0.f);
        else
            value = std::clamp(f, isBipolar() ? -1.f : 0.f, 1.f);
    }
    void setValue01(float f)
    {
        assert(f >= 0.f && f <= 1.f);
        if (isBipolar())
        {
            setValueConstrained(f * 2 - 1.0);
        }
        else
        {
            setValueConstrained(f);
        }
    }
    float getValue01For(float vv) const
    {
        float res = vv;
        if (isBipolar())
        {
            res = (res + 1) * 0.5;
        }
        return std::clamp(res, 0.f, 1.f);
    }
    float getValue01() const { return getValue01For(value); }

    float valueFromString(const std::string &s) const
    {
        if (mode == TOGGLE)
            return boolFromString(s) ? 1.f : 0.f;
        auto sv = std::atof(s.c_str());
        return std::clamp((float)sv, isBipolar() ? -1.f : 0.f, 1.f);
    }

    float value01FromString(const std::string &s) const
    {
        auto v = valueFromString(s);
        if (isBipolar())
            v = (v + 1) * 0.5;
        return v;
    }

    std::string value01ToString(float dv) const
    {
        if (mode == TOGGLE)
            return dv > 0.5f ? "On" : "Off";
        auto fv = dv;
        if (isBipolar())
            fv = fv * 2 - 1;
        return fmt::format("{:.4f}", fv);
    }

    static bool boolFromString(const std::string &s)
    {
        auto t = s;
        std::transform(t.begin(), t.end(), t.begin(), [](auto c) { return std::tolower(c); });
        if (t == "on" || t == "true" || t == "yes")
            return true;
        if (t == "off" || t == "false" || t == "no")
            return false;
        return std::atof(t.c_str()) > 0.5;
    }

    static std::string defaultNameFor(int index) { return "Macro " + std::to_string(index + 1); }
    std::string pluginParameterNameFor(int p, int i) const
    {
        auto dn = defaultNameFor(i);
        if (name == dn)
        {
            return std::string("P" + std::to_string(p + 1) + " " + name);
        }
        else
        {
            return std::string("P" + std::to_string(p + 1) + " M" + std::to_string(i + 1) + " - " +
                               name);
        }
    }

    /*
     * Clap ID to Macro ID handling. This lives here so the engine can generate the
     * associated clap id and so on outside the wrapper
     */
    static constexpr uint32_t firstMacroParam{0x30000};
    static constexpr uint32_t macroParamPartStride{0x100};
    static_assert(scxt::macrosPerPart < macroParamPartStride,
                  "If you hit this you need to figure out what to do with those higher macros");
    static bool isMacroID(uint32_t id)
    {
        if (id < firstMacroParam)
            return false;
        auto off = id - firstMacroParam;
        return (off / macroParamPartStride) < scxt::numParts &&
               (off % macroParamPartStride) < scxt::macrosPerPart;
    }
    static int macroIDToPart(uint32_t id) { return (id - firstMacroParam) / macroParamPartStride; }
    static int macroIDToIndex(uint32_t id) { return (id - firstMacroParam) % macroParamPartStride; }
    static uint32_t partIndexToMacroID(uint32_t pt, uint32_t id)
    {
        return firstMacroParam + pt * macroParamPartStride + id;
    }
};
} // namespace scxt::engine
#endif // SHORTCIRCUITXT_MACROS_H
