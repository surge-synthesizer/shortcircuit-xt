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

#include "processor.h"
#include "datamodel/metadata.h"
#include "processor_defs.h"

#include <functional>
#include <new>
#include <cassert>
#include <unordered_map>

namespace scxt::dsp::processor
{

/**
 * This code blob uses constexpr expansion to allow us to convert a runtime integer to
 * a template argument without a big case statement.
 */
namespace detail
{

/**
 * Booleans
 */

using boolOp_t = bool (*)();
using constCharOp_t = const char *(*)();

template <size_t I> bool implIsProcessorImplemented()
{
    if constexpr (I == ProcessorType::proct_none)
        return true;
    else
    {
        static_assert(sizeof(typename ProcessorImplementor<(ProcessorType)I>::T) <
                      processorMemoryBufferSize);

        return !std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value;
    }
}

template <size_t... Is> auto isProcessorImplemented(size_t ft, std::index_sequence<Is...>)
{
    constexpr boolOp_t fnc[] = {detail::implIsProcessorImplemented<Is>...};
    return fnc[ft]();
}

template <size_t I> const char *implGetProcessorName()
{
    if constexpr (I == ProcessorType::proct_none)
        return "Off";

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return "error";
    else
        return ProcessorImplementor<(ProcessorType)I>::T::processorName;
}

template <size_t... Is> auto getProcessorName(size_t ft, std::index_sequence<Is...>)
{
    constexpr constCharOp_t fnc[] = {detail::implGetProcessorName<Is>...};
    return fnc[ft]();
}

template <size_t I> const char *implGetProcessorStreamingName()
{
    if constexpr (I == ProcessorType::proct_none)
        return "none";

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return "error";
    else
        return ProcessorImplementor<(ProcessorType)I>::T::processorStreamingName;
}

template <size_t... Is> auto getProcessorStreamingName(size_t ft, std::index_sequence<Is...>)
{
    constexpr constCharOp_t fnc[] = {detail::implGetProcessorStreamingName<Is>...};
    return fnc[ft]();
}

template <size_t I> const char *implGetProcessorDisplayGroup()
{
    if constexpr (I == ProcessorType::proct_none)
        return "none";

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return "error";
    else
        return ProcessorImplementor<(ProcessorType)I>::T::processorDisplayGroup;
}

template <size_t... Is> auto getProcessorDisplayGroup(size_t ft, std::index_sequence<Is...>)
{
    constexpr constCharOp_t fnc[] = {detail::implGetProcessorDisplayGroup<Is>...};
    return fnc[ft]();
}
template <size_t I>
Processor *returnSpawnOnto(uint8_t *m, engine::MemoryPool *mp, float *fp, int *ip)
{
    if constexpr (I == ProcessorType::proct_none)
        return nullptr;

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
    {
        return nullptr;
    }
    else
    {
        auto mem = new (m) typename ProcessorImplementor<(ProcessorType)I>::T(mp, fp, ip);
        return mem;
    }
}

template <size_t... Is>
auto spawnOnto(size_t ft, uint8_t *m, engine::MemoryPool *mp, float *fp, int *ip,
               std::index_sequence<Is...>)
{
    using FuncType = Processor *(*)(uint8_t *, engine::MemoryPool *, float *, int *);
    constexpr FuncType arFuncs[] = {detail::returnSpawnOnto<Is>...};
    return arFuncs[ft](m, mp, fp, ip);
}
} // namespace detail

bool isProcessorImplemented(ProcessorType id)
{
    return detail::isProcessorImplemented(
        id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

const char *getProcessorName(ProcessorType id)
{
    return detail::getProcessorName(
        id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

const char *getProcessorStreamingName(ProcessorType id)
{
    return detail::getProcessorStreamingName(
        id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

const char *getProcessorDisplayGroup(ProcessorType id)
{
    return detail::getProcessorDisplayGroup(
        id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

std::optional<ProcessorType> fromProcessorStreamingName(const std::string &s)
{
    // A bit gross but hey
    for (auto i = 0; i < proct_num_types; ++i)
    {
        if (getProcessorStreamingName((ProcessorType)i) == s)
            return (ProcessorType)i;
    }
    return {};
}

processorList_t getAllProcessorDescriptions()
{
    processorList_t res;
    for (auto i = 0U; i < (size_t)ProcessorType::proct_num_types; ++i)
    {
        auto pt = (ProcessorType)i;
        if (isProcessorImplemented(pt))
        {
            res.push_back({pt, getProcessorStreamingName(pt), getProcessorName(pt),
                           getProcessorDisplayGroup(pt)});
        }
    }
    std::sort(res.begin(), res.end(), [](const auto &a, const auto &b) {
        if (a.displayName == "Off")
            return true;
        if (b.displayName == "Off")
            return false;

        if (a.displayGroup == b.displayGroup)
        {
            return a.displayName < b.displayName;
        }
        return a.displayGroup < b.displayGroup;
    });
    return res;
}

/**
 * Spawn with in-place new onto a pre-allocated block. The memory must
 * be a 16byte aligned block of at least size processorMemoryBufferSize.
 */
Processor *spawnProcessorInPlace(ProcessorType id, engine::MemoryPool *mp, uint8_t *memory,
                                 size_t memorySize, float *fp, int *ip)
{
    assert(memorySize >= processorMemoryBufferSize);
    return detail::spawnOnto(id, memory, mp, fp, ip,
                             std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

/**
 * Spawn a Processor, potentially allocating memory. Call this if canInPlaceNew
 * returns false.
 *
 * TODO: Make it so we can remove this
 */
Processor *spawnProcessorAllocating(int id, float *fp, int *ip, bool stereo)
{
    assert(false);
    return 0;
}

/**
 * Whetner a Processor is spawned in place or onto fresh memory, release it here.
 */
void unspawnProcessor(Processor *f)
{
    if (!f)
        return;

    f->~Processor();
}

ProcessorControlDescription Processor::getControlDescription() const
{
    ProcessorControlDescription res;
    res.type = getType();
    res.typeDisplayName = getName();
    res.numFloatParams = getFloatParameterCount();

    for (int i = 0; i < res.numFloatParams; ++i)
    {
        res.floatControlDescriptions[i] = ctrlmode_desc[i];
    }
    for (int i = res.numFloatParams; i < maxProcessorFloatParams; ++i)
    {
        res.floatControlDescriptions[i] = datamodel::pmd().withType(datamodel::pmd::NONE);
    }

    res.numIntParams = getIntParameterCount();
    for (int i = 0; i < res.numIntParams; ++i)
    {
        std::unordered_map<int, std::string> dVals;
        for (int j = 0; j < getIntParameterChoicesCount(i); ++j)
        {
            dVals[j] = getIntParameterChoicesLabel(i, j);
        }

        auto pd = datamodel::pmd()
                      .withType(datamodel::pmd::INT)
                      .withRange(0, getIntParameterChoicesCount(i) - 1)
                      .withDefault(0)
                      .withName(getIntParameterLabel(i))
                      .withUnorderedMapFormatting(dVals);

        res.intControlDescriptions[i] = pd;
    }
    for (int i = res.numIntParams; i < maxProcessorIntParams; ++i)
    {
        res.intControlDescriptions[i] = {};
    }
    return res;
}

} // namespace scxt::dsp::processor