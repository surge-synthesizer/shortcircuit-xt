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

#include "processor.h"
#include "datamodel/metadata.h"
#include "processor_defs.h"
#include "engine/engine.h"

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
 * Generic wrapper to eliminate duplication between impl* and wrapper functions.
 * Takes a return type and variadic function pointers, then indexes into them.
 * Each wrapper function explicitly passes its impl function instantiations.
 */
template <typename ReturnType, ReturnType (*...Funcs)()> auto genericWrapper(size_t ft)
{
    constexpr ReturnType (*fnc[])() = {Funcs...};
    return fnc[ft]();
}

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

template <size_t I> const char *implGetProcessorName()
{
    if constexpr (I == ProcessorType::proct_none)
        return scxt::dsp::processor::noneDisplayName;

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return "error";
    else
        return ProcessorImplementor<(ProcessorType)I>::T::processorName;
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

template <size_t I> std::string implGetProcessorSSTDisplayName()
{
    if constexpr (I == ProcessorType::proct_none)
        return "None";

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return "";
    else
        return ProcessorImplementor<(ProcessorType)I>::T::displayName;
}

template <size_t I> int16_t implGetProcessorFloatParamCount()
{
    if constexpr (I == ProcessorType::proct_none)
        return 0;

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return 0;
    else
        return ProcessorImplementor<(ProcessorType)I>::T::numFloatParams;
}

template <size_t I> int16_t implGetProcessorIntParamCount()
{
    if constexpr (I == ProcessorType::proct_none)
        return 0;

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return 0;
    else
        return ProcessorImplementor<(ProcessorType)I>::T::numIntParams;
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

template <size_t I> float implGetProcessorDefaultMix()
{
    return ProcessorDefaultMix<(ProcessorType)I>::defaultMix;
}

template <size_t I> bool implGetForGroupOnly()
{
    return ProcessorForGroupOnly<(ProcessorType)I>::isGroupOnly;
}

template <size_t I> int16_t implGetStreamingVersion()
{
    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
        return 0;
    else
        return ProcessorImplementor<(ProcessorType)I>::T::streamingVersion;
}

template <typename T>
concept HasThreeArgRemap = requires(int sv, float *fv, int *iv) {
    { T::remapParametersForStreamingVersion(sv, fv, iv) } -> std::same_as<void>;
};

template <typename T>
concept HasFourArgRemap = requires(int sv, float *fv, int *iv, uint32_t *featv) {
    { T::remapParametersForStreamingVersion(sv, fv, iv, featv) } -> std::same_as<void>;
};

template <size_t I> remapFn_t implGetRemapFn()
{
    using PT = typename ProcessorImplementor<(ProcessorType)I>::T;

    if constexpr (std::is_same<PT, unimpl_t>::value)
        return nullptr;
    else if constexpr (HasThreeArgRemap<PT>)
        return
            [](auto a, auto *b, auto *c, auto) { PT::remapParametersForStreamingVersion(a, b, c); };
    else if constexpr (HasFourArgRemap<PT>)
        return PT::remapParametersForStreamingVersion;
}

template <size_t I>
Processor *returnSpawnOnto(engine::Engine *e, uint8_t *m, engine::MemoryPool *mp,
                           const ProcessorStorage &ps, float *f, int *i, bool needsMetadata)
{
    if constexpr (I == ProcessorType::proct_none)
        return nullptr;

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::T, unimpl_t>::value)
    {
        return nullptr;
    }
    else
    {
        auto mem = new (m)
            typename ProcessorImplementor<(ProcessorType)I>::T(e, mp, ps, f, i, needsMetadata);
        return mem;
    }
}

template <size_t... Is>
auto spawnOnto(size_t ft, engine::Engine *e, uint8_t *m, engine::MemoryPool *mp,
               const ProcessorStorage &ps, float *f, int *i, bool needsMetadata,
               std::index_sequence<Is...>)
{
    using FuncType = Processor *(*)(engine::Engine *, uint8_t *, engine::MemoryPool *,
                                    const ProcessorStorage &, float *, int *, bool);
    constexpr FuncType arFuncs[] = {detail::returnSpawnOnto<Is>...};
    return arFuncs[ft](e, m, mp, ps, f, i, needsMetadata);
}

template <size_t I>
Processor *returnSpawnOntoOS(engine::Engine *e, uint8_t *m, engine::MemoryPool *mp,
                             const ProcessorStorage &ps, float *f, int *i, bool needsMetadata)
{
    if constexpr (I == ProcessorType::proct_none)
        return nullptr;

    if constexpr (std::is_same<typename ProcessorImplementor<(ProcessorType)I>::TOS,
                               unimpl_t>::value)
    {
        return nullptr;
    }
    else
    {
        auto mem = new (m)
            typename ProcessorImplementor<(ProcessorType)I>::TOS(e, mp, ps, f, i, needsMetadata);
        return mem;
    }
}

template <size_t... Is>
auto spawnOntoOS(size_t ft, engine::Engine *e, uint8_t *m, engine::MemoryPool *mp,
                 const ProcessorStorage &ps, float *f, int *i, bool needsMetadata,
                 std::index_sequence<Is...>)
{
    using FuncType = Processor *(*)(engine::Engine *, uint8_t *, engine::MemoryPool *,
                                    const ProcessorStorage &, float *, int *, bool);
    constexpr FuncType arFuncs[] = {detail::returnSpawnOntoOS<Is>...};
    return arFuncs[ft](e, m, mp, ps, f, i, needsMetadata);
}
} // namespace detail

bool isProcessorImplemented(ProcessorType id)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return detail::genericWrapper<bool, detail::implIsProcessorImplemented<Is>...>(ft);
    }(id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

const char *getProcessorName(ProcessorType id)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return detail::genericWrapper<const char *, detail::implGetProcessorName<Is>...>(ft);
    }(id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

const char *getProcessorStreamingName(ProcessorType id)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return detail::genericWrapper<const char *, detail::implGetProcessorStreamingName<Is>...>(
            ft);
    }(id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

remapFn_t getProcessorRemapParametersFromStreamingVersion(ProcessorType id)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return detail::genericWrapper<remapFn_t, detail::implGetRemapFn<Is>...>(ft);
    }(id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

const char *getProcessorDisplayGroup(ProcessorType id)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return detail::genericWrapper<const char *, detail::implGetProcessorDisplayGroup<Is>...>(
            ft);
    }(id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

float getProcessorDefaultMix(ProcessorType id)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return detail::genericWrapper<float, detail::implGetProcessorDefaultMix<Is>...>(ft);
    }(id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

bool getProcessorGroupOnly(ProcessorType id)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return detail::genericWrapper<bool, detail::implGetForGroupOnly<Is>...>(ft);
    }(id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

int16_t getProcessorStreamingVersion(ProcessorType id)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return detail::genericWrapper<int16_t, detail::implGetStreamingVersion<Is>...>(ft);
    }(id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

int16_t getProcessorFloatParamCount(ProcessorType id)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return detail::genericWrapper<int16_t, detail::implGetProcessorFloatParamCount<Is>...>(ft);
    }(id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

int16_t getProcessorIntParamCount(ProcessorType id)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return detail::genericWrapper<int16_t, detail::implGetProcessorIntParamCount<Is>...>(ft);
    }(id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
}

std::string getProcessorSSTVoiceDisplayName(ProcessorType id)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return detail::genericWrapper<std::string, detail::implGetProcessorSSTDisplayName<Is>...>(
            ft);
    }(id, std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
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
            auto pn = getProcessorName(pt);
            auto spn = getProcessorSSTVoiceDisplayName(pt);
            res.push_back({pt, getProcessorStreamingName(pt), getProcessorName(pt),
                           getProcessorDisplayGroup(pt), getProcessorGroupOnly(pt)});
        }
    }
    std::sort(res.begin(), res.end(), [](const auto &a, const auto &b) {
        if (a.displayName == scxt::dsp::processor::noneDisplayName)
            return true;
        if (b.displayName == scxt::dsp::processor::noneDisplayName)
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
Processor *spawnProcessorInPlace(ProcessorType id, engine::Engine *e, engine::MemoryPool *mp,
                                 uint8_t *memory, size_t memorySize, const ProcessorStorage &ps,
                                 float *f, int *i, bool oversample, bool needsMetadata)
{
    assert(memorySize >= processorMemoryBufferSize);
    if (id == proct_none)
    {
        return nullptr;
    }
    if (oversample)
    {
        return detail::spawnOntoOS(
            id, e, memory, mp, ps, f, i, needsMetadata,
            std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
    }
    else
    {
        return detail::spawnOnto(
            id, e, memory, mp, ps, f, i, needsMetadata,
            std::make_index_sequence<(size_t)ProcessorType::proct_num_types>());
    }
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
    res.numIntParams = getIntParameterCount();

    res.floatControlDescriptions = floatParameterMetaData;
    res.intControlDescriptions = intParameterMetaData;

    res.supportsKeytrack = false;
    res.requiresConsistencyCheck = false;

    return res;
}

} // namespace scxt::dsp::processor
