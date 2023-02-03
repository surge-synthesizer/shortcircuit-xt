//
// Created by Paul Walker on 2/3/23.
//

#ifndef SHORTCIRCUIT_JSON_EXTENSIONS_H
#define SHORTCIRCUIT_JSON_EXTENSIONS_H

#include <tao/json/basic_value.hpp>
#include <tao/json/contrib/traits.hpp>
#include <functional>

// A set of templates which I use to help with some common pattersn
template <template <typename...> class Traits, typename Source, typename Predicate>
typename tao::json::basic_value<Traits> toIndexedArrayIf(const Source &s, Predicate f)
{
    auto t = tao::json::basic_value<Traits>::array({});

    size_t idx{0};
    for (const auto &r : s)
    {
        if (f(r))
        {
            const tao::json::basic_value<Traits> vel = {{"idx", idx}, {"entry", r}};
            t.get_array().push_back(vel);
        }
        idx++;
    }
    return t;
}

template <template <typename...> class Traits, typename Target>
void fromIndexedArray(const tao::json::basic_value<Traits> &s, Target &t)
{
    const auto &arr = s.get_array();


    for (int i = 0U; i < arr.size(); ++i)
    {
        auto elo = arr[i].get_object();
        size_t idx;
        elo.at("idx").to(idx);
        elo.at("entry").to(t[idx]);
    }
}
#endif // SHORTCIRCUIT_EXTENSIONS_H
