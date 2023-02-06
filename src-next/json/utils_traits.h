//
// Created by Paul Walker on 2/3/23.
//

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
