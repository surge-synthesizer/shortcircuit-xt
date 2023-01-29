//
// Created by Paul Walker on 1/31/23.
//

#include "filter.h"
#include "filter_defs.h"

#include <functional>
#include <new>
#include <cassert>

namespace scxt::dsp::filter
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

template <size_t I> bool implCanInPlace()
{
    if constexpr (I == FilterType::ft_none)
        return true;

    if constexpr (std::is_same<typename FilterImplementor<(FilterType)I>::T, unimpl_t>::value)
        return false;
    else
        return sizeof(typename FilterImplementor<(FilterType)I>::T) < filterMemoryBufferSize;
}

template <size_t... Is> auto canInPlace(size_t ft, std::index_sequence<Is...>)
{
    constexpr boolOp_t fnc[] = {detail::implCanInPlace<Is>...};
    return fnc[ft]();
}

template <size_t I> bool implIsFilterImplemented()
{
    if constexpr (I == FilterType::ft_none)
        return true;

    return std::is_same<typename FilterImplementor<(FilterType)I>::T, unimpl_t>::value;
}

template <size_t... Is> auto isFilterImplemented(size_t ft, std::index_sequence<Is...>)
{
    constexpr boolOp_t fnc[] = {detail::implIsFilterImplemented<Is>...};
    return fnc[ft]();
}

template <size_t I> bool implIsZoneFilter()
{
    if constexpr (I == FilterType::ft_none)
        return true;

    if constexpr (std::is_same<typename FilterImplementor<(FilterType)I>::T, unimpl_t>::value)
        return false;
    else
        return FilterImplementor<(FilterType)I>::T::isZoneFilter;
}

template <size_t... Is> auto isZoneFilter(size_t ft, std::index_sequence<Is...>)
{
    constexpr boolOp_t fnc[] = {detail::implIsZoneFilter<Is>...};
    return fnc[ft]();
}

template <size_t I> bool implIsPartFilter()
{
    if constexpr (I == FilterType::ft_none)
        return true;

    if constexpr (std::is_same<typename FilterImplementor<(FilterType)I>::T, unimpl_t>::value)
        return false;
    else
        return FilterImplementor<(FilterType)I>::T::isPartFilter;
}

template <size_t... Is> auto isPartFilter(size_t ft, std::index_sequence<Is...>)
{
    constexpr boolOp_t fnc[] = {detail::implIsPartFilter<Is>...};
    return fnc[ft]();
}

template <size_t I> bool implIsFXFilter()
{
    if constexpr (I == FilterType::ft_none)
        return true;

    if constexpr (std::is_same<typename FilterImplementor<(FilterType)I>::T, unimpl_t>::value)
        return false;
    else
        return FilterImplementor<(FilterType)I>::T::isFXFilter;
}

template <size_t... Is> auto isFXFilter(size_t ft, std::index_sequence<Is...>)
{
    constexpr boolOp_t fnc[] = {detail::implIsFXFilter<Is>...};
    return fnc[ft]();
}

template <size_t I> const char *implGetFilterName()
{
    if constexpr (I == FilterType::ft_none)
        return "none";

    if constexpr (std::is_same<typename FilterImplementor<(FilterType)I>::T, unimpl_t>::value)
        return "error";
    else
        return FilterImplementor<(FilterType)I>::T::filterName;
}

template <size_t... Is> auto getFilterName(size_t ft, std::index_sequence<Is...>)
{
    constexpr constCharOp_t fnc[] = {detail::implGetFilterName<Is>...};
    return fnc[ft]();
}

template <size_t I> Filter *returnSpawnOnto(uint8_t *m, float *fp, int *ip, bool st)
{
    if constexpr (I == FilterType::ft_none)
        return nullptr;

    if constexpr (std::is_same<typename FilterImplementor<(FilterType)I>::T, unimpl_t>::value)
    {
        return nullptr;
    }
    else
    {
        auto mem = new (m) typename FilterImplementor<(FilterType)I>::T(fp, ip, st);
        return mem;
    }
}

template <size_t... Is>
auto spawnOnto(size_t ft, uint8_t *m, float *fp, int *ip, bool st, std::index_sequence<Is...>)
{
    using FuncType = Filter *(*)(uint8_t *, float *, int *, bool);
    constexpr FuncType arFuncs[] = {detail::returnSpawnOnto<Is>...};
    return arFuncs[ft](m, fp, ip, st);
}
} // namespace detail

bool canInPlaceNew(FilterType id)
{
    return detail::canInPlace(id, std::make_index_sequence<(size_t)FilterType::ft_num_types>());
}

bool isFilterImplemented(FilterType id)
{
    return detail::isFilterImplemented(
        id, std::make_index_sequence<(size_t)FilterType::ft_num_types>());
}

bool isZoneFilter(FilterType id)
{
    return detail::isZoneFilter(id, std::make_index_sequence<(size_t)FilterType::ft_num_types>());
}

bool isPartFilter(FilterType id)
{
    return detail::isPartFilter(id, std::make_index_sequence<(size_t)FilterType::ft_num_types>());
}

bool isFXFilter(FilterType id)
{
    return detail::isFXFilter(id, std::make_index_sequence<(size_t)FilterType::ft_num_types>());
}

const char *getFilterName(FilterType id)
{
    return detail::getFilterName(id, std::make_index_sequence<(size_t)FilterType::ft_num_types>());
}

/**
 * Spawn with in-place new onto a pre-allocated block. The memory must
 * be a 16byte aligned block of at least size filterMemoryBufferSize.
 */
Filter *spawnFilterInPlace(FilterType id, uint8_t *memory, size_t memorySize, float *fp, int *ip,
                           bool stereo)
{
    assert(memorySize >= filterMemoryBufferSize);
    return detail::spawnOnto(id, memory, fp, ip, stereo,
                             std::make_index_sequence<(size_t)FilterType::ft_num_types>());
}

/**
 * Spawn a filter, potentially allocating memory. Call this if canInPlaceNew
 * returns false.
 *
 * TODO: Make it so we can remove this
 */
Filter *spawnFilterAllocating(int id, float *fp, int *ip, bool stereo)
{
    assert(false);
    return 0;
}

/**
 * Whetner a filter is spawned in place or onto fresh memory, release it here.
 */
void unspawnFilter(Filter *f)
{
    if (!f)
        return;

    if (canInPlaceNew(f->getType()))
    {
        f->~Filter();
    }
    else
    {
        delete f;
    }
}
} // namespace scxt::dsp::filter