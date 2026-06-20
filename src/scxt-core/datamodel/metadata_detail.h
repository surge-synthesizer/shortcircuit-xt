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
// Sentinel pd matching no field: passed at static-init so a single descFor call flows through
// every SC_FIELD block and seeds each field's static (off the realtime thread).
static constexpr ptrdiff_t kSeedAll = -1;

template <typename P>
const sst::basic_blocks::params::ParamMetaData &descFor(const P &, ptrdiff_t pd, bool realtime)
{
    // If you hit this assert you need an SC_DESCRIBE for a w.r.t b
    assert(false);
    static const sst::basic_blocks::params::ParamMetaData err =
        sst::basic_blocks::params::ParamMetaData()
            .withType(sst::basic_blocks::params::ParamMetaData::NONE)
            .withName("ERROR");
    return err;
}

// Deferred seeding. Each SC_DESCRIBE self-registers a seed node at static-init time, but that is
// only a cheap pointer link into this intrusive list - no ParamMetaData is built. seedAllMetadata()
// (call once, off the realtime thread, e.g. from the Engine ctor) walks the list and forces each
// descFor<T> to construct its field statics. This keeps the heavy pmd construction out of DLL load
// so it doesn't slow DAW plugin scanning.
struct MetadataSeed;
inline MetadataSeed *&metadataSeedHead()
{
    static MetadataSeed *head{nullptr};
    return head;
}
struct MetadataSeed
{
    using Fn = void (*)();
    explicit MetadataSeed(Fn f) : fn(f), next(metadataSeedHead()) { metadataSeedHead() = this; }
    Fn fn;
    MetadataSeed *next;
};
inline void seedAllMetadata()
{
    static const bool once = []() {
        for (auto *n = metadataSeedHead(); n; n = n->next)
            n->fn();
        return true;
    }();
    (void)once;
}

} // namespace scxt::datamodel::detail

// Token-paste helpers so the per-SC_DESCRIBE seed object gets a unique (__COUNTER__-based) name.
// __COUNTER__ rather than __LINE__ so two SC_DESCRIBE on the same line in different headers can't
// collide within a translation unit.
#define SC_DESC_CAT2(a, b) a##b
#define SC_DESC_CAT(a, b) SC_DESC_CAT2(a, b)

#define SC_DESCRIBE(T, ...)                                                                        \
    template <>                                                                                    \
    inline const sst::basic_blocks::params::ParamMetaData &scxt::datamodel::detail::descFor<T>(    \
        const T &payload, ptrdiff_t(pd), bool realtime)                                            \
    {                                                                                              \
        using data = T;                                                                            \
        static_assert(std::is_standard_layout_v<T>);                                               \
        __VA_ARGS__;                                                                               \
                                                                                                   \
        if (pd != scxt::datamodel::detail::kSeedAll)                                               \
        {                                                                                          \
            SCLOG_IF(warnings, "Failed to bind " << pd << " position element of " << #T);          \
            assert(false);                                                                         \
        }                                                                                          \
        static const pmd scEmpty;                                                                  \
        return scEmpty;                                                                            \
    }                                                                                              \
    namespace                                                                                      \
    {                                                                                              \
    /* Registers (does not run) the seed; descFor<T> is forced from seedAllMetadata(). */          \
    [[maybe_unused]] const scxt::datamodel::detail::MetadataSeed SC_DESC_CAT(scSeed_,              \
                                                                             __COUNTER__){[]() {   \
        scxt::datamodel::detail::descFor<T>(T{}, scxt::datamodel::detail::kSeedAll, false);        \
    }};                                                                                            \
    }

// The pmd is built once into a block-scoped static (block scope -> a fixed name works even for
// nested/dotted field names like keyboardRange.fadeEnd, which are only valid inside offsetof).
// descFor returns it by const ref, so binding never builds or copies a pmd on the realtime thread.
#define SC_FIELD(x, ...)                                                                           \
    {                                                                                              \
        static const pmd scField = __VA_ARGS__;                                                    \
        if (pd == offsetof(data, x))                                                               \
            return scField;                                                                        \
    }

// Payload-dependent field. base is the static (range etc.); dyn is a narrow member write (e.g.
// scField.defaultVal = ...) done only off the realtime thread - the realtime path reads the
// seeded range and never writes, so its min/max reads never race the (distinct) default write.
#define SC_FIELD_COMPUTED(x, base, dyn)                                                            \
    {                                                                                              \
        static pmd scField = base;                                                                 \
        if (pd == offsetof(data, x))                                                               \
        {                                                                                          \
            if (!realtime)                                                                         \
            {                                                                                      \
                dyn;                                                                               \
            }                                                                                      \
            return scField;                                                                        \
        }                                                                                          \
    }

// Describe x[i] as elements for i 0...N (the description must not depend on the index fai)
#define SC_FIELD_ARRAY(x, N, ...)                                                                  \
    {                                                                                              \
        static const pmd scField = __VA_ARGS__;                                                    \
        auto bsOff = offsetof(data, x);                                                            \
        auto szof = sizeof(decltype(data::x)::value_type);                                         \
        for (size_t fai = 0; fai < N; fai++)                                                       \
            if (pd == bsOff + fai * szof)                                                          \
                return scField;                                                                    \
    }

// Describe x[i].y as elements for i 0...N (the description must not depend on the index fai)
#define SC_FIELD_ARRAY_MEMBER(x, y, N, ...)                                                        \
    {                                                                                              \
        static const pmd scField = __VA_ARGS__;                                                    \
        auto bsOff = offsetof(data, x);                                                            \
        auto memOff = offsetof(decltype(data::x)::value_type, y);                                  \
        auto szof = sizeof(decltype(data::x)::value_type);                                         \
        for (size_t fai = 0; fai < N; fai++)                                                       \
            if (pd == bsOff + fai * szof + memOff)                                                 \
                return scField;                                                                    \
    }
#endif // SHORTCIRCUITXT_METADATA_DETAIL_H
