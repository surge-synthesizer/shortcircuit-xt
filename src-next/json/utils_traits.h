/*
 * ShortCircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortCircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortCircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortCircuitXT is inspired by, and shares some code with, the
 * commercial product ShortCircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for ShortCircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortCircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_JSON_UTILS_TRAITS_H
#define SCXT_SRC_JSON_UTILS_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "utils.h"
#include "scxt_traits.h"

namespace scxt::json
{
template <int idType> struct scxt_traits<ID<idType>>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const ID<idType> &t)
    {
        v = {{"id", t.id}, {"idType", t.idType}, {"idDisplayName", t.display_name()}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, ID<idType> &result)
    {
        const auto &object = v.get_object();
        if (result.idType != v.at("idType").template as<int32_t>())
        {
            throw std::logic_error("Unstreaming different result types");
        }
        v.at("id").to(result.id);
    }
};
} // namespace scxt::json
#endif // SHORTCIRCUIT_UTILS_TRAITS_H
