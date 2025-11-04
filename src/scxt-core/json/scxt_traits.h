/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_CORE_JSON_SCXT_TRAITS_H
#define SCXT_SRC_SCXT_CORE_JSON_SCXT_TRAITS_H

#include <initializer_list>
#include "tao/json/traits.hpp"
#include "filesystem/import.h"

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

template <typename T, typename V, typename R>
void addToObject(V &v, const std::string &key, const R &val)
{
    v.insert({{key, T(val)}});
}
template <typename T, typename V, typename R>
void addUnlessDefault(V &v, const std::string &key, const R &defVal, const R &val)
{
    if (val != defVal)
        v.insert({{key, T(val)}});
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

/* Be careful using this, It applies to all streams in all context
 * so only use it for tightly managed enums which are used in one place
 * and so on
 */
#define STREAM_ENUM_WITH_DEFAULT(E, d, toS, fromS)                                                 \
    template <> struct scxt_traits<E>                                                              \
    {                                                                                              \
        template <template <typename...> class Traits>                                             \
        static void assign(tao::json::basic_value<Traits> &v, const E &s)                          \
        {                                                                                          \
            if (s == (d))                                                                          \
            {                                                                                      \
                v = tao::json::empty_object;                                                       \
            }                                                                                      \
            else                                                                                   \
            {                                                                                      \
                v = {{"e", toS(s)}};                                                               \
            };                                                                                     \
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
                if (!findIf(v, "e", s))                                                            \
                {                                                                                  \
                    s = toS(d);                                                                    \
                }                                                                                  \
            }                                                                                      \
            r = fromS(s);                                                                          \
        }                                                                                          \
    };

#define SC_FROM(...) __VA_ARGS__
#define SC_TO(...) __VA_ARGS__
#define SC_STREAMDEF(type, assignBlock, toBlock)                                                   \
    template <> struct scxt_traits<type>                                                           \
    {                                                                                              \
        using rt_t = type;                                                                         \
        template <template <typename...> class Traits>                                             \
        static void assign(tao::json::basic_value<Traits> &v, const rt_t &from)                    \
        {                                                                                          \
            using val_t = tao::json::basic_value<Traits>;                                          \
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

#define SC_STREAMING_FOR_IN_PROCESS                                                                \
    (engine::Engine::streamReason == engine::Engine::StreamReason::IN_PROCESS)
#define SC_STREAMING_FOR_DAW (engine::Engine::streamReason == engine::Engine::StreamReason::FOR_DAW)
#define SC_STREAMING_FOR_MULTI                                                                     \
    (engine::Engine::streamReason == engine::Engine::StreamReason::FOR_MULTI)
#define SC_STREAMING_FOR_PART                                                                      \
    (engine::Engine::streamReason == engine::Engine::StreamReason::FOR_PART)

#define SC_STREAMING_FOR_DAW_OR_MULTI (SC_STREAMING_FOR_DAW || SC_STREAMING_FOR_MULTI)

inline fs::path unstreamPathFromString(const std::string &s)
{
#if WINDOWS
    return fs::path(fs::u8path(s));
#else
    // A windows path with a drive letter needs adjusting
    // on a posix system, alas. We handle this at unstream
    // to keep the same-store-os case working properly.
    //
    // Also handle the \\server\foo case here
    if (s.length() > 2 && (s[1] == ':' && (s[2] == '\\') || (s[2] == '/')) ||
        (s[0] == s[1] && (s[0] == '\\' || s[0] == '/')))
    {
        // We know at this point we are a windows path, so
        // any \ in the string are a separator, unlike posix
        // where \ can be part of a filename
        std::string r, mnt;

        if (s[1] == ':')
        {
            // c:\foo\bar
            auto drv = s.substr(0, 1);
            r = s.substr(2);
            mnt = std::string("/mnt/") + drv;
        }
        else
        {
            // //server/foo/bar
            r = r.substr(1);
            mnt = "";
        }

        std::replace(r.begin(), r.end(), '\\', '/');
        auto res = fs::path(fs::u8path(mnt + r));
        return res;
    }
    else
    {
        return fs::path(fs::u8path(s));
    }
#endif
}

} // namespace scxt::json
#endif // SHORTCIRCUIT_SCXT_TRAITS_H
