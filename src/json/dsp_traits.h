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
