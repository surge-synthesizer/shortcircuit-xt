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

#ifndef SCXT_SRC_SCXT_CORE_ENGINE_MEMORY_POOL_H
#define SCXT_SRC_SCXT_CORE_ENGINE_MEMORY_POOL_H

#include <cstdint>
#include <cctype>
#include <unordered_map>
#include <unordered_set>
#include <queue>

#include "utils.h"

namespace scxt::engine
{
struct MemoryPool : MoveableOnly<MemoryPool>
{
    typedef uint8_t data_t;

    ~MemoryPool();

    void preReservePool(size_t blockSize);
    void preReserveSingleInstancePool(size_t blockSize);

    data_t *checkoutBlock(size_t blockSize);
    void returnBlock(data_t *block, size_t blockSize);

  private:
    template <size_t N = 10> static inline size_t nearestBlock(size_t x)
    {
        return ((x >> N) + 1) * (1 << N);
    }
    void growBlock(size_t blockSize, bool initialize);
    struct SinglePool : MoveableOnly<SinglePool>
    {
        size_t associatedSize;
        size_t initialPoolSize, growSize;
        std::queue<data_t *> data;
    };
    using pool_t = SinglePool;
    using cache_t = std::unordered_map<size_t, pool_t>;
    cache_t cache;

    int64_t debugCheckouts{0}, debugReturns{0};
};
} // namespace scxt::engine

#endif // SHORTCIRCUIT_MEMORY_POOL_H
