/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_SCXT_CORE_JSON_EXTENSIONS_H
#define SCXT_SRC_SCXT_CORE_JSON_EXTENSIONS_H

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

template <template <typename...> class Traits, typename Target>
void fromArrayWithSizeDifference(const tao::json::basic_value<Traits> &s, Target &t)
{
    const auto &arr = s.get_array();

    for (int i = 0U; i < arr.size() && i < t.size(); ++i)
    {
        arr[i].to(t[i]);
    }
}

template <typename R, typename Target>
void findIfArray(const R &v, const std::string &key, Target &t)
{
    auto vs = v.find(key);
    if (vs)
    {
        fromArrayWithSizeDifference(*vs, t);
    }
}
#endif // SHORTCIRCUIT_EXTENSIONS_H
