//
// Created by Paul Walker on 2/3/23.
//

#ifndef SHORTCIRCUIT_DSP_TRAITS_H
#define SHORTCIRCUIT_DSP_TRAITS_H

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
             {"intParams", t.intParams}};
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
    }
};
} // namespace scxt::json
#endif // SHORTCIRCUIT_DSP_TRAITS_H
