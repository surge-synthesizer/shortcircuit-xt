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

#ifndef SCXT_SRC_JSON_DATAMODEL_TRAITS_H
#define SCXT_SRC_JSON_DATAMODEL_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "scxt_traits.h"
#include "extensions.h"

#include "datamodel/adsr_storage.h"

namespace scxt::json
{

template <> struct scxt_traits<datamodel::AdsrStorage>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v,
                       const datamodel::AdsrStorage &t)
    {
        v = { {"a", t.a},
             {"d", t.d},
             {"s", t.s},
             {"r", t.r},
             {"isDigital", t.isDigital},
             {"aShape", t.aShape},
             {"dShape", t.dShape},
             {"rShape", t.rShape}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v,
                   datamodel::AdsrStorage &result)
    {
        v.at("a").to(result.a);
        v.at("d").to(result.d);
        v.at("s").to(result.s);
        v.at("r").to(result.r);
        v.at("isDigital").to(result.isDigital);
        v.at("aShape").to(result.aShape);
        v.at("dShape").to(result.dShape);
        v.at("rShape").to(result.rShape);
    }
};
} // namespace scxt::json
#endif // SHORTCIRCUIT_DATAMODEL_TRAITS_H
