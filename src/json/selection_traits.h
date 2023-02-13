//
// Created by Paul Walker on 2/13/23.
//

#ifndef SHORTCIRCUIT_SELECTION_TRAITS_H
#define SHORTCIRCUIT_SELECTION_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "stream.h"
#include "extensions.h"

#include "selection/selection_manager.h"

#include "scxt_traits.h"

namespace scxt::json
{
template <> struct scxt_traits<scxt::selection::SelectionManager::ZoneAddress>
{
    typedef scxt::selection::SelectionManager::ZoneAddress za_t;
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const za_t &e)
    {
        v = {{"part", e.part}, {"group", e.group}, {"zone", e.zone}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, za_t &z)
    {
        v.at("part").to(z.part);
        v.at("group").to(z.group);
        v.at("zone").to(z.zone);
    }
};
} // namespace scxt::json

#endif // SHORTCIRCUIT_SELECTION_TRAITS_H
