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

#ifndef SCXT_SRC_CLIENTS_PERF_HARNESS_PERF_FNV_H
#define SCXT_SRC_CLIENTS_PERF_HARNESS_PERF_FNV_H

#include <cstddef>
#include <cstdint>

namespace scxt::perf
{
inline constexpr uint64_t kFnvOffsetBasis = 0xcbf29ce484222325ULL;
inline constexpr uint64_t kFnvPrime = 0x100000001b3ULL;

inline void fnv1a(uint64_t &h, const void *data, std::size_t n)
{
    auto *p = static_cast<const std::uint8_t *>(data);
    for (std::size_t i = 0; i < n; ++i)
    {
        h ^= p[i];
        h *= kFnvPrime;
    }
}

inline std::uint64_t fnv1a(const void *data, std::size_t n)
{
    std::uint64_t h = kFnvOffsetBasis;
    fnv1a(h, data, n);
    return h;
}
} // namespace scxt::perf
#endif
