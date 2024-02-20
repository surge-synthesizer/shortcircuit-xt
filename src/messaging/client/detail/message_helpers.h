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

#ifndef SCXT_SRC_MESSAGING_CLIENT_DETAIL_MESSAGE_HELPERS_H
#define SCXT_SRC_MESSAGING_CLIENT_DETAIL_MESSAGE_HELPERS_H

#include <tuple>

namespace scxt::messaging::client::detail
{

template <typename VT> using diffMsg_t = std::tuple<ptrdiff_t, VT>;

template <typename VT, typename M>
inline void
updateZoneLeadMemberValue(M m, const diffMsg_t<VT> &payload, const engine::Engine &engine,
                          MessageController &cont,
                          std::function<void(const engine::Engine &)> responseCB = nullptr)
{
    auto sz = engine.getSelectionManager()->currentLeadZone(engine);
    if (sz.has_value())
    {
        cont.scheduleAudioThreadCallback(
            [zs = sz, payload, m](auto &eng) {
                auto [d, v] = payload;
                auto [p, g, z] = *zs; // did had value check before we started
                auto &zn = eng.getPatch()->getPart(p)->getGroup(g)->getZone(z);
                auto &dat = *zn.*m;
                static_assert(std::is_standard_layout_v<std::remove_reference_t<decltype(dat)>>);
                assert(d <= sizeof(dat) - sizeof(v));
                assert(d >= 0);
                if constexpr (std::is_same_v<VT, float>)
                {
                    if (!zn->isActive())
                    {
                        *(VT *)(((uint8_t *)&dat) + d) = v;
                    }
                    else
                    {
                        zn->mUILag.setNewDestination((VT *)(((uint8_t *)&dat) + d), v);
                    }
                }
                else
                {
                    *(VT *)(((uint8_t *)&dat) + d) = v;
                }
            },
            responseCB);
    }
}

template <typename VT, typename M>
inline void updateZoneMemberValue(M m, const diffMsg_t<VT> &payload, const engine::Engine &engine,
                                  MessageController &cont,
                                  std::function<void(const engine::Engine &)> responseCB = nullptr)
{
    auto sz = engine.getSelectionManager()->currentlySelectedZones();
    if (!sz.empty())
    {
        cont.scheduleAudioThreadCallback(
            [zs = sz, payload, m](auto &eng) {
                auto [d, v] = payload;
                for (const auto &[p, g, z] : zs)
                {
                    auto &zn = eng.getPatch()->getPart(p)->getGroup(g)->getZone(z);
                    auto &dat = *zn.*m;
                    static_assert(
                        std::is_standard_layout_v<std::remove_reference_t<decltype(dat)>>);
                    assert(d <= sizeof(dat) - sizeof(v));
                    assert(d >= 0);
                    if constexpr (std::is_same_v<VT, float>)
                    {
                        if (!zn->isActive())
                        {
                            *(VT *)(((uint8_t *)&dat) + d) = v;
                        }
                        else
                        {
                            zn->mUILag.setNewDestination((VT *)(((uint8_t *)&dat) + d), v);
                        }
                    }
                    else
                    {
                        *(VT *)(((uint8_t *)&dat) + d) = v;
                    }
                }
            },
            responseCB);
    }
}

template <typename VT, typename M>
inline void updateGroupMemberValue(M m, const diffMsg_t<VT> &payload, const engine::Engine &engine,
                                   MessageController &cont,
                                   std::function<void(const engine::Engine &)> responseCB = nullptr)
{
    auto sg = engine.getSelectionManager()->currentlySelectedGroups();
    if (!sg.empty())
    {
        cont.scheduleAudioThreadCallback(
            [gs = sg, payload, m](auto &eng) {
                auto [d, v] = payload;
                for (const auto &[p, g, z] : gs)
                {
                    auto &grp = eng.getPatch()->getPart(p)->getGroup(g);
                    auto &dat = *grp.*m;
                    static_assert(
                        std::is_standard_layout_v<std::remove_reference_t<decltype(dat)>>);
                    assert(d <= sizeof(dat) - sizeof(v));
                    assert(d >= 0);
                    if constexpr (std::is_same_v<VT, float>)
                    {
                        // TODO: Group UI Lag
                        *(VT *)(((uint8_t *)&dat) + d) = v;
                    }
                    else
                    {
                        *(VT *)(((uint8_t *)&dat) + d) = v;
                    }
                }
            },
            responseCB);
    }
}

template <typename VT> using indexedDiffMsg_t = std::tuple<size_t, ptrdiff_t, VT>;

template <typename VT, typename M>
inline void
updateZoneIndexedMemberValue(M m, const indexedDiffMsg_t<VT> &payload, const engine::Engine &engine,
                             MessageController &cont,
                             std::function<void(const engine::Engine &)> responseCB = nullptr)
{
    auto sz = engine.getSelectionManager()->currentlySelectedZones();
    if (!sz.empty())
    {
        cont.scheduleAudioThreadCallback(
            [zs = sz, payload, m](auto &eng) {
                auto [idx, d, v] = payload;
                for (const auto &[p, g, z] : zs)
                {
                    auto &zn = eng.getPatch()->getPart(p)->getGroup(g)->getZone(z);
                    auto &dat = (*zn.*m)[idx];
                    static_assert(
                        std::is_standard_layout_v<std::remove_reference_t<decltype(dat)>>);
                    assert(d <= sizeof(dat) - sizeof(v));
                    assert(d >= 0);
                    if constexpr (std::is_same_v<VT, float>)
                    {
                        if (!zn->isActive())
                        {
                            *(VT *)(((uint8_t *)&dat) + d) = v;
                        }
                        else
                        {
                            zn->mUILag.setNewDestination((VT *)(((uint8_t *)&dat) + d), v);
                        }
                    }
                    else
                    {
                        *(VT *)(((uint8_t *)&dat) + d) = v;
                    }
                }
            },
            responseCB);
    }
}

template <typename VT, typename M>
inline void
updateGroupIndexedMemberValue(M m, const indexedDiffMsg_t<VT> &payload,
                              const engine::Engine &engine, MessageController &cont,
                              std::function<void(const engine::Engine &)> responseCB = nullptr)
{
    auto sg = engine.getSelectionManager()->currentlySelectedGroups();
    if (!sg.empty())
    {
        cont.scheduleAudioThreadCallback(
            [gs = sg, payload, m](auto &eng) {
                auto [idx, d, v] = payload;
                for (const auto &[p, g, z] : gs)
                {
                    auto &grp = eng.getPatch()->getPart(p)->getGroup(g);
                    auto &dat = (*grp.*m)[idx];
                    static_assert(
                        std::is_standard_layout_v<std::remove_reference_t<decltype(dat)>>);
                    assert(d <= sizeof(dat) - sizeof(v));
                    assert(d >= 0);
                    if constexpr (std::is_same_v<VT, float>)
                    {
                        // TODO: Group UI Lag
                        *(VT *)(((uint8_t *)&dat) + d) = v;
                    }
                    else
                    {
                        *(VT *)(((uint8_t *)&dat) + d) = v;
                    }
                }
            },
            responseCB);
    }
}

template <typename VT> using indexedZoneOrGroupDiffMsg_t = std::tuple<bool, size_t, ptrdiff_t, VT>;

template <typename VT, typename MZ, typename MG>
inline void updateZoneOrGroupIndexedMemberValue(
    MZ mz, MG mg, const indexedZoneOrGroupDiffMsg_t<VT> &payload, const engine::Engine &engine,
    MessageController &cont, std::function<void(const engine::Engine &)> responseCB = nullptr)
{
    auto isZone = std::get<0>(payload);
    auto underT =
        indexedDiffMsg_t<VT>{std::get<1>(payload), std::get<2>(payload), std::get<3>(payload)};
    if (isZone)
    {
        updateZoneIndexedMemberValue(mz, underT, engine, cont, responseCB);
    }
    else
    {
        updateGroupIndexedMemberValue(mg, underT, engine, cont, responseCB);
    }
}
} // namespace scxt::messaging::client::detail
#endif // SHORTCIRCUITXT_MESSAGE_HELPERS_H
