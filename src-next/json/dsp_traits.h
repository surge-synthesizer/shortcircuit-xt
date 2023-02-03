//
// Created by Paul Walker on 2/3/23.
//

#ifndef SHORTCIRCUIT_DSP_TRAITS_H
#define SHORTCIRCUIT_DSP_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "dsp/filter/filter.h"

template <> struct tao::json::traits<scxt::dsp::filter::FilterStorage>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::dsp::filter::FilterStorage &t)
    {
        v = {{"type", scxt::dsp::filter::getFilterStreamingName(t.type)},
             {"mix", t.mix},
             {"floatParams", t.floatParams},
             {"intParams", t.intParams}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v,
                   scxt::dsp::filter::FilterStorage &result)
    {
        const auto &object = v.get_object();

        auto optType =
            scxt::dsp::filter::fromFilterStreamingName(v.at("type").template as<std::string>());

        if (optType.has_value())
            result.type = *optType;
        else
            result.type = scxt::dsp::filter::ft_none;

        v.at("mix").to(result.mix);
        v.at("floatParams").to(result.floatParams);
        v.at("intParams").to(result.intParams);
    }
};
#endif // SHORTCIRCUIT_DSP_TRAITS_H
