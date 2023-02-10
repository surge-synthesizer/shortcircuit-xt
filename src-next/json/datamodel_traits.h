//
// Created by Paul Walker on 2/11/23.
//

#ifndef SHORTCIRCUIT_DATAMODEL_TRAITS_H
#define SHORTCIRCUIT_DATAMODEL_TRAITS_H

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
