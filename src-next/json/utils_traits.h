//
// Created by Paul Walker on 2/3/23.
//

#ifndef SHORTCIRCUIT_UTILS_TRAITS_H
#define SHORTCIRCUIT_UTILS_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "utils.h"

template <int idType> struct tao::json::traits<scxt::ID<idType>>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::ID<idType> &t)
    {
        v = {{"id", t.id}, {"idType", t.idType}, {"idDisplayName", t.display_name()}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::ID<idType> &result)
    {
        const auto &object = v.get_object();
        if(result.idType != v.at("idType").template as<int32_t>())
        {
            throw std::logic_error("Unstreaming different result types");
        }
        v.at("id").to(result.id);
    }
};

#endif // SHORTCIRCUIT_UTILS_TRAITS_H
