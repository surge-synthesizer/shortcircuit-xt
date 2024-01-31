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

#include "memory_pool.h"
#include <cassert>

namespace scxt::engine
{
/*
 * A massive TODO here. Right now this allocates on the audio thread but we
 * can instead have this have a message contrller refecence at construction time
 * and instead try to ask the audio thread to allocate.
 */

MemoryPool::~MemoryPool()
{
    for (auto &[sz, s] : cache)
    {
        for (auto &p : s)
        {
            delete p;
        }
    }

    assert(debugCheckouts == debugReturns);
}

#define ALLOCATE_IMPLEMENTATION 1
#if ALLOCATE_IMPLEMENTATION
void MemoryPool::preReservePool(size_t blockSize) {}
MemoryPool::data_t *MemoryPool::checkoutBlock(size_t blockSize)
{
    // Technically don't need this in the naive impl but lets avoid some bugs
    debugCheckouts++;
    auto ubs = nearestBlock(blockSize);
    return new data_t[ubs];
}
void MemoryPool::returnBlock(data_t *block, size_t blockSize)
{
    debugReturns++;
    delete[] block;
}
#endif

} // namespace scxt::engine