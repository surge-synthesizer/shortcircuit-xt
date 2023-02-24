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

#ifndef SCXT_SRC_JSON_DSP_TRAITS_H
#define SCXT_SRC_JSON_DSP_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "dsp/processor/processor.h"
#include "scxt_traits.h"

namespace scxt::json
{

template <> struct scxt_traits<scxt::dsp::processor::ProcessorStorage>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v,
                       const scxt::dsp::processor::ProcessorStorage &t)
    {
        v = {{"type", scxt::dsp::processor::getProcessorStreamingName(t.type)},
             {"mix", t.mix},
             {"floatParams", t.floatParams},
             {"intParams", t.intParams},
             {"isBypassed", t.isBypassed}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v,
                   scxt::dsp::processor::ProcessorStorage &result)
    {
        const auto &object = v.get_object();

        auto optType = scxt::dsp::processor::fromProcessorStreamingName(
            v.at("type").template as<std::string>());

        if (optType.has_value())
            result.type = *optType;
        else
            result.type = scxt::dsp::processor::proct_none;

        v.at("mix").to(result.mix);
        v.at("floatParams").to(result.floatParams);
        v.at("intParams").to(result.intParams);
        findOr(v, "isBypassed", false, result.isBypassed);
    }
};

template <> struct scxt_traits<scxt::dsp::processor::ProcessorDescription>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v,
                       const scxt::dsp::processor::ProcessorDescription &t)
    {
        v = {
            {"id", (int32_t)t.id},          {"streamingName", t.streamingName},
            {"displayName", t.displayName}, {"isZone", t.isZone},
            {"isPart", t.isZone},           {"isFX", t.isZone},
        };
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v,
                   scxt::dsp::processor::ProcessorDescription &result)
    {
        int32_t tr;
        v.at("id").to(tr);
        result.id = (dsp::processor::ProcessorType)tr;
        v.at("streamingName").to(result.streamingName);
        v.at("displayName").to(result.displayName);
        v.at("isZone").to(result.isZone);
        v.at("isPart").to(result.isPart);
        v.at("isFX").to(result.isFX);
    }
};

template <> struct scxt_traits<scxt::dsp::processor::ProcessorControlDescription>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v,
                       const scxt::dsp::processor::ProcessorControlDescription &t)
    {
        v = {{"type",
              (int32_t)t.type}, // these are process-lifetime only so the type is safe to stream
             {"typeDisplayName", t.typeDisplayName},
             {"numFloatParams", t.numFloatParams},
             {"floatControlNames", t.floatControlNames},
             {"floatControlDescriptions", t.floatControlDescriptions},
             {"numIntParams", t.numIntParams},
             {"intControlNames", t.intControlNames},
             {"intControlDescriptions", t.intControlDescriptions}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v,
                   scxt::dsp::processor::ProcessorControlDescription &t)
    {
        int tInt{dsp::processor::proct_none};
        findOr(v, "type", (int32_t)dsp::processor::proct_none, tInt);
        t.type = (dsp::processor::ProcessorType)tInt;
        v.at("typeDisplayName").to(t.typeDisplayName);
        v.at("numFloatParams").to(t.numFloatParams);
        v.at("floatControlNames").to(t.floatControlNames);
        v.at("floatControlDescriptions").to(t.floatControlDescriptions);
        v.at("numIntParams").to(t.numIntParams);
        v.at("intControlNames").to(t.intControlNames);
        v.at("intControlDescriptions").to(t.intControlDescriptions);
    }
};
} // namespace scxt::json
#endif // SHORTCIRCUIT_DSP_TRAITS_H
