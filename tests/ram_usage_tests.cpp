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

#include <filesystem>
#include <memory>
#include <vector>

#include "catch2/catch2.hpp"

#include "engine/memory_pool.h"
#include "messaging/messaging.h"
#include "test_utils.h"

TEST_CASE("Memory pool tracks the memory it has grown", "[memory-pool]")
{
    scxt::engine::MemoryPool mp;
    REQUIRE(mp.getAllocatedBytes() == 0);

    static constexpr size_t request{1000};
    mp.preReservePool(request);

    auto reserved = mp.getAllocatedBytes();
    REQUIRE(reserved > 0);

    // blocks are rounded up to a power of two, so the total is a whole number of them
    REQUIRE(reserved % 1024 == 0);

    std::vector<scxt::engine::MemoryPool::data_t *> held;
    auto firstBlock = mp.checkoutBlock(request);
    REQUIRE(firstBlock);
    held.push_back(firstBlock);
    REQUIRE(mp.getAllocatedBytes() == reserved);

    // drain past the initial reservation and the pool has to grow for real
    while (mp.getAllocatedBytes() == reserved && held.size() < 1024)
    {
        auto b = mp.checkoutBlock(request);
        REQUIRE(b);
        held.push_back(b);
    }
    auto grown = mp.getAllocatedBytes();
    REQUIRE(grown > reserved);
    REQUIRE(grown % 1024 == 0);

    // a returned block stays ours, so the total is unchanged
    for (auto *b : held)
        mp.returnBlock(b, request);
    REQUIRE(mp.getAllocatedBytes() == grown);
}

TEST_CASE("Ram usage reaches the shared ui state", "[memory-pool]")
{
    auto eng = std::unique_ptr<scxt::engine::Engine>(makeEngine());
    const auto &ram = eng->sharedUIMemoryState.ramUsage;

    eng->processAudio();
    REQUIRE(ram.sampleMemory == 0);
    REQUIRE(ram.total() == ram.memoryPool);

    auto p = samplePath("WavStereo48k.wav");
    REQUIRE(std::filesystem::exists(p));

    auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
    auto sid = eng->getSampleManager()->loadSampleByPath(p);
    REQUIRE(sid.has_value());

    auto loadedBytes = eng->getSampleManager()->sampleMemoryInBytes.load();
    REQUIRE(loadedBytes > 0);

    // the engine mirrors both halves into the shared state as it processes
    eng->getMemoryPool()->preReservePool(1000);
    eng->processAudio();

    REQUIRE(ram.sampleMemory == loadedBytes);
    REQUIRE(ram.memoryPool == eng->getMemoryPool()->getAllocatedBytes());
    REQUIRE(ram.memoryPool > 0);
    REQUIRE(ram.total() == ram.sampleMemory + ram.memoryPool);
}
