/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#include <initializer_list>
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

template <typename V, typename R>
bool findIf(V &v, const std::initializer_list<std::string> &keys, R &r)
{
    for (const auto &key : keys)
    {
        auto vs = v.find(key);
        if (vs)
        {
            vs->to(r);
            return true;
        }
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

template <typename V, typename D, typename R>
void findOrSet(V &v, const std::initializer_list<std::string> &keys, const D &d, R &r)
{
    if (!findIf(v, keys, r))
        r = d;
}

template <typename V, typename R> void findOrDefault(V &v, const std::string &key, R &r)
{
    findOrSet(v, key, R{}, r);
}

#define STREAM_ENUM(E, toS, fromS)                                                                 \
    template <> struct scxt_traits<E>                                                              \
    {                                                                                              \
        template <template <typename...> class Traits>                                             \
        static void assign(tao::json::basic_value<Traits> &v, const E &s)                          \
        {                                                                                          \
            v = {{"e", toS(s)}};                                                                   \
        }                                                                                          \
        template <template <typename...> class Traits>                                             \
        static void to(const tao::json::basic_value<Traits> &v, E &r)                              \
        {                                                                                          \
            std::string s;                                                                         \
            if (SC_UNSTREAMING_FROM_PRIOR_TO(0x2024'08'04))                                        \
            {                                                                                      \
                findIf(v, #E, s);                                                                  \
            }                                                                                      \
            else                                                                                   \
            {                                                                                      \
                findIf(v, "e", s);                                                                 \
            }                                                                                      \
            r = fromS(s);                                                                          \
        }                                                                                          \
    };

#define SC_INSERT(x, a, b) x.insert({{a, tao::json::basic_value<Traits>(b)}});
#define SC_FROM(...) __VA_ARGS__
#define SC_TO(...) __VA_ARGS__
#define SC_STREAMDEF(type, assignBlock, toBlock)                                                   \
    template <> struct scxt_traits<type>                                                           \
    {                                                                                              \
        using rt_t = type;                                                                         \
        template <template <typename...> class Traits>                                             \
        static void assign(tao::json::basic_value<Traits> &v, const rt_t &from)                    \
        {                                                                                          \
            auto &t = from;                                                                        \
            assignBlock                                                                            \
        }                                                                                          \
        template <template <typename...> class Traits>                                             \
        static void to(const tao::json::basic_value<Traits> &v, rt_t &to)                          \
        {                                                                                          \
            auto &result = to;                                                                     \
            toBlock                                                                                \
        }                                                                                          \
    };

#define SC_UNSTREAMING_FROM_THIS_OR_OLDER(x)                                                       \
    engine::Engine::isFullEngineUnstream &&engine::Engine::fullEngineUnstreamStreamingVersion <= x

#define SC_UNSTREAMING_FROM_PRIOR_TO(x)                                                            \
    engine::Engine::isFullEngineUnstream &&engine::Engine::fullEngineUnstreamStreamingVersion < x

} // namespace scxt::json
#endif // SHORTCIRCUIT_SCXT_TRAITS_H
