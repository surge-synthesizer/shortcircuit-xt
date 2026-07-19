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

#ifndef SCXT_SRC_SCXT_CORE_UNDO_MANAGER_PAYLOAD_UNDOABLE_ITEMS_H
#define SCXT_SRC_SCXT_CORE_UNDO_MANAGER_PAYLOAD_UNDOABLE_ITEMS_H

/*
 * The consolidating pattern for value-style undo. Almost every undoable
 * edit in the synth is "copy a value-semantics subtree of each selected
 * zone or group, and on restore write the copies back on the audio thread
 * then refresh the UI from the lead selection". PayloadUndoableItem<Spec>
 * implements that store/restore/makeRedo/describe cycle once; a Spec just
 * names the subtree and (rarely) customizes the write or its audio-thread
 * side effect.
 *
 * A Spec provides:
 *   static constexpr bool forZone;
 *   using value_t = ...;            // copyable subtree
 *   static std::string name();
 *   static value_t &ref(engine::Engine &, const ZoneAddress &, int32_t index);
 * and optionally:
 *   static void write(engine::Engine &, const ZoneAddress &, int32_t,
 *                     const value_t &);  // when plain assignment isn't enough
 *   static void postWrite(engine::Engine &, const ZoneAddress &, int32_t);
 *                                        // audio-thread side effect after write
 *   static void extraClientRefresh(const engine::Engine &);
 *                                  // serial-thread client refresh sent on
 *                                  // undo/redo restore, beyond refreshLeadDisplay
 *
 * For the common case the ZoneMemberSpec / GroupMemberSpec (and Indexed)
 * bases derive everything from a member pointer.
 */

#include "undoable_items.h"
#include "undo.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "engine/group.h"
#include "engine/zone.h"
#include "engine/bus.h"
#include "messaging/messaging.h"

namespace scxt::undo
{
using ZoneAddress = selection::SelectionManager::ZoneAddress;

inline engine::Zone &zoneAt(engine::Engine &e, const ZoneAddress &a)
{
    return *(e.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone));
}

inline engine::Group &groupAt(engine::Engine &e, const ZoneAddress &a)
{
    return *(e.getPatch()->getPart(a.part)->getGroup(a.group));
}

// serial-thread refresh of every pane for the lead zone or group, plus the
// part mapping summary for zones. Defined in payload_undoable_items.cpp.
void refreshLeadDisplay(const engine::Engine &e, bool forZone);
void resendPGZStructure(const engine::Engine &e);

// Spec customization points
template <typename S>
concept SpecHasRead = requires(engine::Engine &e, const ZoneAddress &a, int32_t i) {
    { S::read(e, a, i) };
};
template <typename S>
concept SpecHasWrite = requires(engine::Engine &e, const ZoneAddress &a, int32_t i,
                                const typename S::value_t &v) { S::write(e, a, i, v); };
template <typename S>
concept SpecHasPostWrite =
    requires(engine::Engine &e, const ZoneAddress &a, int32_t i) { S::postWrite(e, a, i); };
template <typename S>
concept SpecHasExtraClientRefresh = requires(const engine::Engine &e) { S::extraClientRefresh(e); };
template <typename S>
concept SpecHasExtraClientRefreshAddr =
    requires(const engine::Engine &e, const std::vector<ZoneAddress> &sel, int32_t i) {
        S::extraClientRefresh(e, sel, i);
    };
template <typename S>
concept SpecSkipsLeadRefresh = requires { S::skipLeadRefresh; };

template <typename Spec> struct PayloadUndoableItem : MultiSelectUndoBaseItem
{
    int32_t index{-1};
    std::vector<typename Spec::value_t> cached;

    void store(engine::Engine &e, int32_t idx, const std::vector<ZoneAddress> &sel)
    {
        forZone = Spec::forZone;
        index = idx;
        selectionList = sel;
        cached.clear();
        cached.reserve(sel.size());
        for (const auto &a : sel)
        {
            if constexpr (SpecHasRead<Spec>)
                cached.push_back(Spec::read(e, a, idx));
            else
                cached.push_back(Spec::ref(e, a, idx));
        }
    }

    void restore(engine::Engine &e) override
    {
        auto idx = index;
        auto sel = selectionList;
        auto vals = std::move(cached);

        e.getMessageController()->scheduleAudioThreadCallback(
            [idx, sel, vals](auto &eng) {
                for (size_t i = 0; i < sel.size() && i < vals.size(); ++i)
                {
                    if constexpr (SpecHasWrite<Spec>)
                        Spec::write(eng, sel[i], idx, vals[i]);
                    else
                        Spec::ref(eng, sel[i], idx) = vals[i];

                    if constexpr (SpecHasPostWrite<Spec>)
                        Spec::postWrite(eng, sel[i], idx);
                }
            },
            [sel, idx](const auto &eng) {
                if constexpr (!SpecSkipsLeadRefresh<Spec>)
                    refreshLeadDisplay(eng, Spec::forZone);
                if constexpr (SpecHasExtraClientRefresh<Spec>)
                    Spec::extraClientRefresh(eng);
                if constexpr (SpecHasExtraClientRefreshAddr<Spec>)
                    Spec::extraClientRefresh(eng, sel, idx);
            });
    }

    std::unique_ptr<UndoableItem> makeRedo(engine::Engine &e) override
    {
        auto redo = std::make_unique<PayloadUndoableItem<Spec>>();
        redo->store(e, index, selectionList);
        return redo;
    }

    std::string describe() const override
    {
        auto res = Spec::name();
        if (index >= 0)
            res += " " + std::to_string(index);
        res +=
            " [" + std::to_string(selectionList.size()) + (Spec::forZone ? " zones]" : " groups]");
        return res;
    }
};

template <typename Spec> inline std::string gestureTag(int32_t index)
{
    return Spec::name() + "/" + std::to_string(index);
}

// Undo push for an explicit address list (e.g. a non-selection-based target)
template <typename Spec>
inline void pushPayloadUndoFor(engine::Engine &e, const std::vector<ZoneAddress> &sel,
                               int32_t index = -1, UndoGesture g = UndoGesture::Discrete)
{
    if (sel.empty())
        return;

    auto tag = gestureTag<Spec>(index);
    if (g == UndoGesture::Discrete && e.undoManager.gestureCovers(tag))
    {
        // skip the snapshot work, but notify the host of the state change
        e.markDirty();
        return;
    }

    undo::pushUndoTagged<PayloadUndoableItem<Spec>>(e, tag, g, index, sel);
}

// One-line undo push for handler call sites: snapshot the current selection
template <typename Spec>
inline void pushPayloadUndo(engine::Engine &e, int32_t index = -1,
                            UndoGesture g = UndoGesture::Discrete)
{
    std::vector<ZoneAddress> sel;
    if constexpr (Spec::forZone)
    {
        auto sz = e.getSelectionManager()->currentlySelectedZones();
        sel.assign(sz.begin(), sz.end());
    }
    else
    {
        auto sg = e.getSelectionManager()->currentlySelectedGroups();
        sel.assign(sg.begin(), sg.end());
    }
    pushPayloadUndoFor<Spec>(e, sel, index, g);
}

// for the zoneOrGroup message family where forZone rides in the payload
template <typename ZSpec, typename GSpec>
inline void pushZoneOrGroupPayloadUndo(engine::Engine &e, bool forZone, int32_t index = -1,
                                       UndoGesture g = UndoGesture::Discrete)
{
    if (forZone)
        pushPayloadUndo<ZSpec>(e, index, g);
    else
        pushPayloadUndo<GSpec>(e, index, g);
}

// Spec bases which derive scope and value type from a member pointer
namespace payload_detail
{
template <typename O, auto M>
using member_value_t = std::remove_reference_t<decltype(std::declval<O &>().*M)>;
template <typename O, auto M>
using element_value_t = std::remove_reference_t<decltype((std::declval<O &>().*M)[0])>;
} // namespace payload_detail

template <auto M> struct ZoneMemberSpec
{
    static constexpr bool forZone{true};
    using value_t = payload_detail::member_value_t<engine::Zone, M>;
    static value_t &ref(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        return zoneAt(e, a).*M;
    }
};

template <auto M> struct GroupMemberSpec
{
    static constexpr bool forZone{false};
    using value_t = payload_detail::member_value_t<engine::Group, M>;
    static value_t &ref(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        return groupAt(e, a).*M;
    }
};

template <auto M> struct ZoneIndexedMemberSpec
{
    static constexpr bool forZone{true};
    using value_t = payload_detail::element_value_t<engine::Zone, M>;
    static value_t &ref(engine::Engine &e, const ZoneAddress &a, int32_t idx)
    {
        return (zoneAt(e, a).*M)[idx];
    }
};

template <auto M> struct GroupIndexedMemberSpec
{
    static constexpr bool forZone{false};
    using value_t = payload_detail::element_value_t<engine::Group, M>;
    static value_t &ref(engine::Engine &e, const ZoneAddress &a, int32_t idx)
    {
        return (groupAt(e, a).*M)[idx];
    }
};

/*
 * The concrete specs, one per editable subtree
 */

struct ZoneMappingSpec : ZoneMemberSpec<&engine::Zone::mapping>
{
    static std::string name() { return "Zone Mapping"; }
};

struct ZoneOutputInfoSpec : ZoneMemberSpec<&engine::Zone::outputInfo>
{
    static std::string name() { return "Zone Output"; }
};

struct GroupOutputInfoSpec : GroupMemberSpec<&engine::Group::outputInfo>
{
    static std::string name() { return "Group Output"; }
    // replay the side effects of the polyphony / midi channel setters
    static void postWrite(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        auto &g = groupAt(e, a);
        g.resetPolyAndPlaymode(e);
        g.onGroupMidiChannelSubscriptionChanged();
    }
    // mute state shows in the part group zone structure display
    static void extraClientRefresh(const engine::Engine &e) { resendPGZStructure(e); }
};

struct ZoneVariantsSpec : ZoneMemberSpec<&engine::Zone::variantData>
{
    static std::string name() { return "Zone Samples"; }
    static void postWrite(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        auto &z = zoneAt(e, a);
        for (auto i = 0U; i < maxVariantsPerZone; ++i)
            z.attachToSample(*(e.getSampleManager()), i, engine::Zone::NONE);
    }
};

struct ZoneEgSpec : ZoneIndexedMemberSpec<&engine::Zone::egStorage>
{
    static std::string name() { return "Zone Envelope"; }
};

struct GroupEgSpec : GroupIndexedMemberSpec<&engine::Group::gegStorage>
{
    static std::string name() { return "Group Envelope"; }
};

struct ZoneModStorageSpec : ZoneIndexedMemberSpec<&engine::Zone::modulatorStorage>
{
    static std::string name() { return "Zone Modulator"; }
};

struct GroupModStorageSpec : GroupIndexedMemberSpec<&engine::Group::modulatorStorage>
{
    static std::string name() { return "Group Modulator"; }
    static void postWrite(engine::Engine &e, const ZoneAddress &a, int32_t idx)
    {
        auto &g = groupAt(e, a);
        g.resetLFOs(idx);
        g.rePrepareAndBindGroupMatrix();
    }
};

struct ZoneMiscModSpec : ZoneMemberSpec<&engine::Zone::miscSourceStorage>
{
    static std::string name() { return "Zone MiscMod"; }
};

struct GroupMiscModSpec : GroupMemberSpec<&engine::Group::miscSourceStorage>
{
    static std::string name() { return "Group MiscMod"; }
};

struct ZoneAudioModSpec : ZoneMemberSpec<&engine::Zone::audioSourceStorage>
{
    static std::string name() { return "Zone AudioMod"; }
};

struct GroupAudioModSpec : GroupMemberSpec<&engine::Group::audioSourceStorage>
{
    static std::string name() { return "Group AudioMod"; }
};

struct ZoneModRowSpec
{
    static constexpr bool forZone{true};
    using value_t = voice::modulation::Matrix::RoutingTable::Routing;
    static std::string name() { return "Zone Mod Matrix Row"; }
    static value_t &ref(engine::Engine &e, const ZoneAddress &a, int32_t idx)
    {
        return zoneAt(e, a).routingTable.routes[idx];
    }
    static void postWrite(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        zoneAt(e, a).onRoutingChanged();
    }
};

struct GroupModRowSpec
{
    static constexpr bool forZone{false};
    using value_t = modulation::GroupMatrix::RoutingTable::Routing;
    static std::string name() { return "Group Mod Matrix Row"; }
    static value_t &ref(engine::Engine &e, const ZoneAddress &a, int32_t idx)
    {
        return groupAt(e, a).routingTable.routes[idx];
    }
    static void postWrite(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        auto &g = groupAt(e, a);
        g.onRoutingChanged();
        g.rePrepareAndBindGroupMatrix();
    }
};

struct ZoneModTableSpec
{
    static constexpr bool forZone{true};
    using value_t = decltype(voice::modulation::Matrix::RoutingTable::routes);
    static std::string name() { return "Zone Mod Matrix"; }
    static value_t &ref(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        return zoneAt(e, a).routingTable.routes;
    }
    static void postWrite(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        zoneAt(e, a).onRoutingChanged();
    }
};

struct GroupModTableSpec
{
    static constexpr bool forZone{false};
    using value_t = decltype(modulation::GroupMatrix::RoutingTable::routes);
    static std::string name() { return "Group Mod Matrix"; }
    static value_t &ref(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        return groupAt(e, a).routingTable.routes;
    }
    static void postWrite(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        auto &g = groupAt(e, a);
        g.onRoutingChanged();
        g.rePrepareAndBindGroupMatrix();
    }
};

struct ZoneProcessorSpec
{
    static constexpr bool forZone{true};
    using value_t = dsp::processor::ProcessorStorage;
    static std::string name() { return "Zone Processor"; }
    static value_t &ref(engine::Engine &e, const ZoneAddress &a, int32_t idx)
    {
        return zoneAt(e, a).processorStorage[idx];
    }
    static void write(engine::Engine &e, const ZoneAddress &a, int32_t idx, const value_t &v)
    {
        auto &z = zoneAt(e, a);
        z.setProcessorType(idx, v.type);
        z.processorStorage[idx] = v;
    }
};

struct GroupProcessorSpec
{
    static constexpr bool forZone{false};
    using value_t = dsp::processor::ProcessorStorage;
    static std::string name() { return "Group Processor"; }
    static value_t &ref(engine::Engine &e, const ZoneAddress &a, int32_t idx)
    {
        return groupAt(e, a).processorStorage[idx];
    }
    static void write(engine::Engine &e, const ZoneAddress &a, int32_t idx, const value_t &v)
    {
        auto &g = groupAt(e, a);
        g.setProcessorType(idx, v.type);
        g.processorStorage[idx] = v;
    }
};

/*
 * Swapping (or copying) processor slots also rewires the routing table, so
 * the swap snapshot carries both touched slots plus the routes. The two
 * slots are packed into the item index as to | (from << 8).
 */
inline int32_t encodeProcessorPair(int32_t to, int32_t from) { return to | (from << 8); }

template <bool ForZone> struct ProcessorSwapSpec
{
    static constexpr bool forZone{ForZone};
    using owner_t = std::conditional_t<ForZone, engine::Zone, engine::Group>;
    using routes_t =
        std::conditional_t<ForZone, decltype(voice::modulation::Matrix::RoutingTable::routes),
                           decltype(modulation::GroupMatrix::RoutingTable::routes)>;
    using value_t =
        std::tuple<dsp::processor::ProcessorStorage, dsp::processor::ProcessorStorage, routes_t>;
    static std::string name() { return ForZone ? "Zone Processor Swap" : "Group Processor Swap"; }

    static owner_t &owner(engine::Engine &e, const ZoneAddress &a)
    {
        if constexpr (ForZone)
            return zoneAt(e, a);
        else
            return groupAt(e, a);
    }

    static value_t read(engine::Engine &e, const ZoneAddress &a, int32_t idx)
    {
        auto &o = owner(e, a);
        return {o.processorStorage[idx & 0xff], o.processorStorage[(idx >> 8) & 0xff],
                o.routingTable.routes};
    }

    static void write(engine::Engine &e, const ZoneAddress &a, int32_t idx, const value_t &v)
    {
        auto &o = owner(e, a);
        auto t = idx & 0xff;
        auto f = (idx >> 8) & 0xff;
        o.setProcessorType(t, std::get<0>(v).type);
        o.processorStorage[t] = std::get<0>(v);
        o.setProcessorType(f, std::get<1>(v).type);
        o.processorStorage[f] = std::get<1>(v);
        o.routingTable.routes = std::get<2>(v);
        o.onRoutingChanged();
        if constexpr (!ForZone)
            o.rePrepareAndBindGroupMatrix();
    }
};

using ZoneProcessorSwapSpec = ProcessorSwapSpec<true>;
using GroupProcessorSwapSpec = ProcessorSwapSpec<false>;

struct GroupTriggerConditionsSpec : GroupMemberSpec<&engine::Group::triggerConditions>
{
    static std::string name() { return "Group Trigger Conditions"; }
    static void postWrite(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        auto &pt = e.getPatch()->getPart(a.part);
        groupAt(e, a).triggerConditions.setupOnUnstream(pt->groupTriggerInstrumentState);
        pt->guaranteeKeyswitchLatchCoherence(e);
    }
};

/*
 * Mixer and part scope specs. These are not selection based; the target
 * rides in the address as {part-or-minus-1, bus-or-minus-1, fx-slot} and
 * the item index only distinguishes gesture tags.
 */
// pack a mixer target into the begin-edit / gesture index
inline int32_t mixerUndoIndex(int bus, int part, int slot)
{
    return (slot & 0xff) | (((bus + 1) & 0xff) << 8) | (((part + 1) & 0xff) << 16);
}

struct BusEffectSpec
{
    static constexpr bool forZone{false};
    static constexpr bool skipLeadRefresh{true};
    using value_t = engine::BusEffectStorage;
    static std::string name() { return "Bus Effect"; }
    static value_t &ref(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        if (a.group >= 0)
            return e.getPatch()
                ->busses.busByAddress((engine::BusAddress)a.group)
                .busEffectStorage[a.zone];
        return e.getPatch()->getPart(a.part)->partEffectStorage[a.zone];
    }
    static void write(engine::Engine &e, const ZoneAddress &a, int32_t, const value_t &v)
    {
        if (a.group >= 0)
        {
            auto &b = e.getPatch()->busses.busByAddress((engine::BusAddress)a.group);
            b.setBusEffectType(e, a.zone, v.type);
            b.busEffectStorage[a.zone] = v;
        }
        else
        {
            auto &pt = e.getPatch()->getPart(a.part);
            pt->setBusEffectType(e, a.zone, v.type);
            pt->partEffectStorage[a.zone] = v;
        }
    }
    static void extraClientRefresh(const engine::Engine &e, const std::vector<ZoneAddress> &sel,
                                   int32_t);
};

struct BusSendSpec
{
    static constexpr bool forZone{false};
    static constexpr bool skipLeadRefresh{true};
    using value_t = engine::Bus::BusSendStorage;
    static std::string name() { return "Bus Send"; }
    static value_t &ref(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        return e.getPatch()->busses.busByAddress((engine::BusAddress)a.group).busSendStorage;
    }
    static void postWrite(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        e.getPatch()->busses.busByAddress((engine::BusAddress)a.group).resetSendState();
        e.getPatch()->busses.reconfigureSolo();
        e.getPatch()->busses.reconfigureOutputBusses();
    }
    static void extraClientRefresh(const engine::Engine &e, const std::vector<ZoneAddress> &sel,
                                   int32_t);
};

struct PartConfigSpec
{
    static constexpr bool forZone{false};
    static constexpr bool skipLeadRefresh{true};
    using value_t = engine::Part::PartConfiguration;
    static std::string name() { return "Part Configuration"; }
    static value_t &ref(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        return e.getPatch()->getPart(a.part)->configuration;
    }
    static void postWrite(engine::Engine &e, const ZoneAddress &a, int32_t)
    {
        e.onPartConfigurationUpdated();
    }
    static void extraClientRefresh(const engine::Engine &e, const std::vector<ZoneAddress> &sel,
                                   int32_t);
};

/*
 * Aliases retained from the pre-consolidation item-per-edit classes
 */
using ZoneMappingDataUndoableItem = PayloadUndoableItem<ZoneMappingSpec>;
using ZoneModRowChangeItem = PayloadUndoableItem<ZoneModRowSpec>;
using GroupModRowChangeItem = PayloadUndoableItem<GroupModRowSpec>;
using ZoneModTableSnapshotItem = PayloadUndoableItem<ZoneModTableSpec>;
using GroupModTableSnapshotItem = PayloadUndoableItem<GroupModTableSpec>;
using ZoneProcessorTypeChangeItem = PayloadUndoableItem<ZoneProcessorSpec>;
using GroupProcessorTypeChangeItem = PayloadUndoableItem<GroupProcessorSpec>;

} // namespace scxt::undo

#endif // SCXT_SRC_SCXT_CORE_UNDO_MANAGER_PAYLOAD_UNDOABLE_ITEMS_H
