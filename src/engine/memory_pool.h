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

#ifndef SCXT_SRC_ENGINE_MEMORY_POOL_H
#define SCXT_SRC_ENGINE_MEMORY_POOL_H

#include <cstdint>
#include <cctype>
#include <unordered_map>
#include <unordered_set>

#include "utils.h"

namespace scxt::engine
{
struct MemoryPool : MoveableOnly<MemoryPool>
{
    typedef uint8_t data_t;

    ~MemoryPool();

    void preReservePool(size_t blockSize);

    data_t *checkoutBlock(size_t blockSize);
    void returnBlock(data_t *block, size_t blockSize);

  private:
    template <size_t N = 10> static inline size_t nearestBlock(size_t x)
    {
        return ((x >> N) + 1) * (1 << N);
    }
    void growBlock(size_t blockSize);
    std::unordered_map<size_t, std::unordered_set<data_t *>> cache;

    int64_t debugCheckouts{0}, debugReturns{0};
};
} // namespace scxt::engine

#endif // SHORTCIRCUIT_MEMORY_POOL_H
