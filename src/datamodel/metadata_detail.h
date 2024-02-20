//
// Created by Paul Walker on 2/19/24.
//

#ifndef SHORTCIRCUITXT_METADATA_DETAIL_H
#define SHORTCIRCUITXT_METADATA_DETAIL_H

#include "sst/basic-blocks/params/ParamMetadata.h"

namespace scxt::datamodel::detail
{
template <typename P, typename L> ptrdiff_t off(const P &p, L l)
{
    auto pd = (uint8_t *)&(p.*l) - (uint8_t *)&p;
    return pd;
}
template <typename P, typename V> ptrdiff_t offV(const P &p, const V &l)
{
    auto pd = (uint8_t *)&(l) - (uint8_t *)&p;
    return pd;
}
template <typename P> sst::basic_blocks::params::ParamMetaData descFor(ptrdiff_t pd)
{
    assert(false);
    return sst::basic_blocks::params::ParamMetaData()
        .withType(sst::basic_blocks::params::ParamMetaData::NONE)
        .withName("ERROR");
}

} // namespace scxt::datamodel::detail

#define SC_DESCRIBE(T, ...)                                                                        \
    template <>                                                                                    \
    inline sst::basic_blocks::params::ParamMetaData scxt::datamodel::detail::descFor<T>(           \
        ptrdiff_t(pd))                                                                             \
    {                                                                                              \
        using data = T;                                                                            \
        static_assert(std::is_standard_layout_v<T>);                                               \
        __VA_ARGS__;                                                                               \
                                                                                                   \
        SCLOG("Failed to bind " << pd << " position element of " << #T);                           \
        assert(false);                                                                             \
        return pmd();                                                                              \
    }

#define SC_FIELD(x, ...)                                                                           \
    if (pd == offsetof(data, x))                                                                   \
    {                                                                                              \
        return __VA_ARGS__;                                                                        \
    }

#endif // SHORTCIRCUITXT_METADATA_DETAIL_H
