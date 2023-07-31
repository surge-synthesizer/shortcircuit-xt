/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#ifndef SCXT_SRC_MODULATION_BASE_MATRIX_H
#define SCXT_SRC_MODULATION_BASE_MATRIX_H

#include <string>
#include <optional>
#include <vector>
#include <array>

namespace scxt::modulation
{

enum ModMatrixCurve
{
    modc_none,
    modc_cube,

    numModMatrixCurves
};

std::string getModMatrixCurveStreamingName(const ModMatrixCurve &);
std::optional<ModMatrixCurve> fromModMatrixCurveStreamingName(const std::string &);
std::string getModMatrixCurveDisplayName(const ModMatrixCurve &dest);
typedef std::vector<std::pair<ModMatrixCurve, std::string>> modMatrixCurveNames_t;
modMatrixCurveNames_t getModMatrixCurveNames();

template <typename T> struct ModMatrix
{
    static_assert(std::is_enum<typename T::SourceEnum>::value, "Trait class must define a source");
    static_assert(
        std::is_same<typename T::SourceEnum,
                     typename std::remove_const<decltype(T::SourceEnumNoneValue)>::type>::value,
        "Trait class must define a source none value");
    static_assert(std::is_class<typename T::DestAddress>::value, "Trait class must define a dest");
    static_assert(std::is_enum<typename T::DestEnum>::value, "Trait class must define a dest");
    static_assert(
        std::is_same<typename T::DestEnum,
                     typename std::remove_const<decltype(T::DestEnumNoneValue)>::type>::value,
        "Trait class must define a dest none value");
    static_assert(std::is_integral<decltype(T::DestAddress::maxDestinations)>::value,
                  "Dest Address myst define maxDestinations");

    static_assert(std::is_integral<decltype(T::numModMatrixSlots)>::value,
                  "T must define slot count");
    struct Routing
    {
        bool active{true};
        bool selConsistent{true};
        typename T::SourceEnum src{T::SourceEnumNoneValue};
        typename T::SourceEnum srcVia{T::SourceEnumNoneValue};
        typename T::DestAddress dst{T::DestEnumNoneValue};
        ModMatrixCurve curve{modc_none};
        // TODO curves
        float depth{0};

        bool operator==(const Routing &other) const
        {
            return src == other.src && dst == other.dst && depth == other.depth;
        }
        bool operator!=(const Routing &other) const { return !(*this == other); }
    };

    typedef std::array<Routing, T::numModMatrixSlots> routingTable_t;
    routingTable_t routingTable;

    float *getValuePtr(const typename T::DestAddress &dest) { return &modulatedValues[dest]; }

    float *getValuePtr(const typename T::DestEnum &type, size_t index)
    {
        return &modulatedValues[T::DestAddress::destIndex(type, index)];
    }

    float getValue(const typename T::DestAddress &dest) const { return modulatedValues[dest]; }

    float getValue(const typename T::DestEnum &type, size_t index) const
    {
        return modulatedValues[T::DestAddress::destIndex(type, index)];
    }

    static constexpr inline size_t destIndex(const typename T::DestEnum type, size_t index)
    {
        return T::DestAddress::destIndex(type, index);
    }

    void initializeModulationValues()
    {
        memcpy(modulatedValues, baseValues, sizeof(modulatedValues));
    }

    void clear()
    {
        memset(sourcePointers, 0, sizeof(sourcePointers));
        memset(baseValues, 0, sizeof(baseValues));
        memset(modulatedValues, 0, sizeof(modulatedValues));
    }

    void process()
    {
        memcpy(modulatedValues, baseValues, sizeof(modulatedValues));
        for (const auto &r : routingTable)
        {
            if (!r.active)
                continue;
            if (r.dst.type == T::DestEnumNoneValue || r.src == T::SourceEnumNoneValue)
                continue;
            if (!sourcePointers[r.src])
                continue;
            float viaval = 1.0;
            if (r.srcVia != T::SourceEnumNoneValue)
                viaval = *(sourcePointers[r.srcVia]);

            modulatedValues[r.dst] +=
                (*(sourcePointers[r.src])) * viaval * r.depth * depthScales[r.dst];
        }
    }

  protected:
    float *sourcePointers[T::DestAddress::maxDestinations];
    float depthScales[T::DestAddress::maxDestinations];
    float baseValues[T::DestAddress::maxDestinations];
    float modulatedValues[T::DestAddress::maxDestinations];
};

} // namespace scxt::modulation
#endif // SHORTCIRCUITXT_BASE_MATRIX_H
