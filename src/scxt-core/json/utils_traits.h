/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_CORE_JSON_UTILS_TRAITS_H
#define SCXT_SRC_SCXT_CORE_JSON_UTILS_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "utils.h"
#include "scxt_traits.h"

namespace scxt::json
{
/*
 * Leave this one un-convererd to SC_STREAMDEF because of the double template
 */
template <int idType> struct scxt_traits<ID<idType>>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const ID<idType> &t)
    {
        v = {{"i", t.id}, {"t", t.idType}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, ID<idType> &result)
    {
        const auto &object = v.get_object();
        if (SC_UNSTREAMING_FROM_PRIOR_TO(0x2024'08'06))
        {
            if (result.idType != v.at("idType").template as<int32_t>())
            {
                throw std::logic_error("Unstreaming different result types");
            }
            v.at("id").to(result.id);
        }
        else
        {
            if (result.idType != v.at("t").template as<int32_t>())
            {
                throw std::logic_error("Unstreaming different result types");
            }
            v.at("i").to(result.id);
        }
    }
};

SC_STREAMDEF(scxt::SampleID,
             SC_FROM(v = {{"m", from.md5}, {"a", from.multiAddress}, {"p", from.pathHash}});
             , SC_TO(
                   auto vs = v.find("m"); if (vs) {
                       std::string m5;
                       vs->to(m5);
                       strncpy(to.md5, m5.c_str(), scxt::SampleID::md5len + 1);
                       findIf(v, "a", to.multiAddress);
                       findIf(v, "p", to.pathHash);
                   } else {
                       std::string legType{"t"}, legID{"i"};
                       if (SC_UNSTREAMING_FROM_PRIOR_TO(0x2024'08'06))
                       {
                           legType = "idType";
                           legID = "id";
                       }

                       if (6 != v.at(legType).template as<int32_t>())
                       {
                           SCLOG("Unstreamed a non-6 type from legacy");
                       }
                       int32_t id;
                       v.at(legID).to(id);
                       to.setAsLegacy(id);
                   }));

} // namespace scxt::json
#endif // SHORTCIRCUIT_UTILS_TRAITS_H
