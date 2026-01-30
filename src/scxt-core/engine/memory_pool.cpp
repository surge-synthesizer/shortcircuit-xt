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

#include "memory_pool.h"
#include <cassert>
#include "configuration.h"

namespace scxt::engine
{
/*
 * A massive TODO here. Right now this allocates on the audio thread but we
 * can instead have this have a message contrller refecence at construction time
 * and instead try to ask the audio thread to allocate.
 */

MemoryPool::~MemoryPool()
{
    SCLOG_IF(memoryPool, "Destroying memory pool " << SCD(debugCheckouts) << SCD(debugReturns));
    assert(debugCheckouts == debugReturns);

    for (auto &[sz, s] : cache)
    {
        SCLOG_IF(memoryPool, "Cleaning up pool of size " << sz);
        while (!s.data.empty())
        {
            delete[] s.data.front();
            s.data.pop();
        }
    }
}

static constexpr size_t initialPoolSize{16};
MemoryPool::data_t *MemoryPool::checkoutBlock(size_t requestBlockSize)
{
    // Technically don't need this in the naive impl but lets avoid some bugs
    auto blockSize = nearestBlock(requestBlockSize);
    auto cacheP = cache.find(blockSize);
    SCLOG_IF(memoryPool,
             "Checkout Block " << blockSize << " from pool of size " << cacheP->second.data.size());
    assert(cacheP != cache.end()); // If you hit this you didn't pre-reserve
    if (cacheP == cache.end())
    {
        return nullptr;
    }
    assert(cacheP->second.associatedSize == blockSize);
    if (cacheP->second.data.empty())
    {
        growBlock(requestBlockSize, false);
        return checkoutBlock(requestBlockSize);
    }

    debugCheckouts++;
    auto res = cacheP->second.data.front();
    cacheP->second.data.pop();

    SCLOG_IF(memoryPool, blockSize << " : Post checkout size is " << cacheP->second.data.size());
    return res;
}
void MemoryPool::returnBlock(data_t *block, size_t requestBlockSize)
{
    debugReturns++;
    auto blockSize = nearestBlock(requestBlockSize);
    auto cacheP = cache.find(blockSize);
    assert(cacheP != cache.end()); // If you hit this you didn't pre-reserve

    cacheP->second.data.push(block);
    SCLOG_IF(memoryPool, "Return Block " << blockSize << " now have " << cacheP->second.data.size()
                                         << " entries");
}

void MemoryPool::growBlock(size_t requestBlockSize, bool initialize)
{
    auto blockSize = nearestBlock(requestBlockSize);
    SCLOG_IF(memoryPool, "Growing block size " << blockSize << " init=" << initialize);
    auto cacheP = cache.find(blockSize);
    assert(cacheP != cache.end()); // If you hit this you didn't pre-reserve

    auto gb = initialize ? cacheP->second.initialPoolSize : cacheP->second.growSize;

    SCLOG_IF(memoryPool, "Adding entries from " << cacheP->second.data.size() << " to " << gb);

    for (auto i = cacheP->second.data.size(); i < gb; ++i)
    {
        cacheP->second.data.push(new data_t[blockSize]);
    }
}

void MemoryPool::preReservePool(size_t requestBlockSize)
{
    auto blockSize = nearestBlock(requestBlockSize);
    SCLOG_IF(memoryPool, "preReserve Pool " << blockSize);
    auto cacheP = cache.find(blockSize);
    if (cacheP == cache.end())
    {
        auto cacheSet = pool_t();
        cacheSet.associatedSize = blockSize;
        cacheSet.initialPoolSize = initialPoolSize;
        cacheSet.growSize = initialPoolSize >> 1;

        cache.insert({blockSize, std::move(cacheSet)});
        growBlock(requestBlockSize, true);
    }
    SCLOG_IF(memoryPool,
             "preReserve complete " << blockSize << " " << cache[blockSize].data.size());
}

void MemoryPool::preReserveSingleInstancePool(size_t requestBlockSize)
{
    auto blockSize = nearestBlock(requestBlockSize);
    SCLOG_IF(memoryPool, "preReserve Single Instance Pool " << blockSize);

    auto cacheP = cache.find(blockSize);
    if (cacheP == cache.end())
    {
        auto cacheSet = pool_t();

        cacheSet.associatedSize = blockSize;
        cacheSet.initialPoolSize = 1;
        cacheSet.growSize = 1;

        cache.insert({blockSize, std::move(cacheSet)});
        growBlock(requestBlockSize, true);
    }
    // For single pool make sure its there if i pre-reserve it.
    const auto &pool = cache[blockSize];
    if (pool.data.size() < pool.initialPoolSize)
    {
        growBlock(requestBlockSize, true);
    }
    SCLOG_IF(memoryPool,
             "preReserve Single complete " << blockSize << " " << cache[blockSize].data.size());
}

} // namespace scxt::engine