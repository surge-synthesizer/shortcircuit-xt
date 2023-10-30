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

#ifndef SCXT_SRC_JSON_SCXT_TRAITS_H
#define SCXT_SRC_JSON_SCXT_TRAITS_H

#include "tao/json/traits.hpp"

namespace scxt::json
{
template <typename T> struct scxt_traits : public tao::json::traits<T>
{
};
using scxt_value = tao::json::basic_value<scxt_traits>;

template <typename V, typename R> bool findIf(V &v, const std::string &key, R &r)
{
    auto vs = v.find(key);
    if (vs)
    {
        vs->to(r);
        return true;
    }
    return false;
}

template <typename V, typename R> bool findEnumIf(V &v, const std::string &key, R &r)
{
    auto vs = v.find(key);
    if (vs)
    {
        int rv;
        vs->to(rv);
        r = (R)rv;
        return true;
    }
    return false;
}

/**
 * Extract json element key from type v onto r, using d as the default
 * if the key is not
 * @param key
 * @param d
 * @param r
 */
template <typename V, typename D, typename R>
void findOrSet(V &v, const std::string &key, const D &d, R &r)
{
    if (!findIf(v, key, r))
        r = d;
}

template <typename V, typename R> void findOrDefault(V &v, const std::string &key, R &r)
{
    findOrSet(v, key, R{}, r);
}

#define FIND(x, y) findIf(v, #y, x.y)
#define FINDOR(x, y, d) findOrSet(v, #y, d, x.y);

#define STREAM_ENUM(E, toS, fromS)                                                                 \
    template <> struct scxt_traits<E>                                                              \
    {                                                                                              \
        template <template <typename...> class Traits>                                             \
        static void assign(tao::json::basic_value<Traits> &v, const E &s)                          \
        {                                                                                          \
            v = {{#E, toS(s)}};                                                                    \
        }                                                                                          \
        template <template <typename...> class Traits>                                             \
        static void to(const tao::json::basic_value<Traits> &v, E &r)                              \
        {                                                                                          \
            std::string s;                                                                         \
            findIf(v, #E, s);                                                                      \
            r = fromS(s);                                                                          \
        }                                                                                          \
    };

} // namespace scxt::json
#endif // SHORTCIRCUIT_SCXT_TRAITS_H
