/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_ENGINE_MACROS_H
#define SCXT_SRC_ENGINE_MACROS_H

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <fmt/core.h>

#include "sst/basic-blocks/params/ParamMetadata.h"
#include "configuration.h"

namespace scxt::engine
{
struct Macro
{
    float value{0.f}; // -1 .. 1

    bool isBipolar{false}; // The value is expected in range -1..1
    bool isStepped{false}; // The value has discrete stepped value
    size_t stepCount{1};   // how many steps between min and max.
                           // So 1 means a 0..1 or -1..1 toggle
    std::string name{};

    void setIsBipolar(bool b)
    {
        isBipolar = b;
        setValueConstrained(value);
    }
    void setValueConstrained(float f) { value = std::clamp(f, isBipolar ? -1.f : 0.f, 1.f); }
    void setValue01(float f)
    {
        assert(f >= 0.f && f <= 1.f);
        if (isBipolar)
        {
            setValueConstrained(f * 2 - 1.0);
        }
        else
        {
            setValueConstrained(f);
        }
    }
    float getValue01() const
    {
        float res = value;
        if (isBipolar)
        {
            res = (res + 1) * 0.5;
        }
        return std::clamp(res, 0.f, 1.f);
    }

    float valueFromString(const std::string &s) const
    {
        auto sv = std::atof(s.c_str());
        return std::clamp((float)sv, isBipolar ? -1.f : 0.f, 1.f);
    }

    float value01FromString(const std::string &s) const
    {
        auto v = valueFromString(s);
        if (isBipolar)
            v = (v + 1) * 0.5;
        return v;
    }

    std::string value01ToString(float dv) const
    {
        auto fv = dv;
        if (isBipolar)
            fv = fv * 2 - 1;
        return fmt::format("{:.4f}", fv);
    }

    static std::string defaultNameFor(int index) { return "Macro " + std::to_string(index + 1); }
    std::string pluginParameterNameFor(int part, int index) const
    {
        auto dn = defaultNameFor(index);
        if (name == dn)
        {
            return std::string("P" + std::to_string(part + 1) + " " + name);
        }
        else
        {
            return std::string("P" + std::to_string(part + 1) + " M" + std::to_string(index + 1) +
                               " - " + name);
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
        return id >= firstMacroParam &&
               id < firstMacroParam + macroParamPartStride * (scxt::numParts + 1);
    }
    static int macroIDToPart(uint32_t id) { return (id - firstMacroParam) / macroParamPartStride; }
    static int macroIDToIndex(uint32_t id) { return (id - firstMacroParam) % macroParamPartStride; }
    static uint32_t partIndexToMacroID(uint32_t part, uint32_t index)
    {
        return firstMacroParam + part * macroParamPartStride + index;
    }
};
} // namespace scxt::engine
#endif // SHORTCIRCUITXT_MACROS_H
