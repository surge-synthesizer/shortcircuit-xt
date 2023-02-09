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

#ifndef SCXT_SRC_JSON_EXTENSIONS_H
#define SCXT_SRC_JSON_EXTENSIONS_H

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
