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

#ifndef SCXT_SRC_SCXT_CORE_DATAMODEL_METADATA_DETAIL_H
#define SCXT_SRC_SCXT_CORE_DATAMODEL_METADATA_DETAIL_H

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
template <typename P> sst::basic_blocks::params::ParamMetaData descFor(const P &, ptrdiff_t pd)
{
    // If you hit this assert you need an SC_DESCRIBE for a w.r.t b
    assert(false);
    return sst::basic_blocks::params::ParamMetaData()
        .withType(sst::basic_blocks::params::ParamMetaData::NONE)
        .withName("ERROR");
}

} // namespace scxt::datamodel::detail

#define SC_DESCRIBE(T, ...)                                                                        \
    template <>                                                                                    \
    inline sst::basic_blocks::params::ParamMetaData scxt::datamodel::detail::descFor<T>(           \
        const T &payload, ptrdiff_t(pd))                                                           \
    {                                                                                              \
        using data = T;                                                                            \
        static_assert(std::is_standard_layout_v<T>);                                               \
        __VA_ARGS__;                                                                               \
                                                                                                   \
        SCLOG_IF(warnings, "Failed to bind " << pd << " position element of " << #T);              \
        assert(false);                                                                             \
        return pmd();                                                                              \
    }

#define SC_FIELD(x, ...)                                                                           \
    if (pd == offsetof(data, x))                                                                   \
    {                                                                                              \
        return __VA_ARGS__;                                                                        \
    }

#define SC_FIELD_COMPUTED(x, ...)                                                                  \
    if (pd == offsetof(data, x))                                                                   \
    {                                                                                              \
        __VA_ARGS__;                                                                               \
    }

#endif // SHORTCIRCUITXT_METADATA_DETAIL_H
