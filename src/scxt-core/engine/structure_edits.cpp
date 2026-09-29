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

#include "engine.h"
#include "part.h"
#include "group.h"
#include "zone.h"
#include "json/engine_traits.h"
#include "json/stream.h"
#include <tao/json/to_string.hpp>
#include "messaging/messaging.h"
#include "messaging/client/client_serial.h"
#include "messaging/client/structure_messages.h"
#include "messaging/audio/audio_messages.h"
#include "undo_manager/payload_undoable_items.h"
#include "undo_manager/structure_undoable_items.h"

namespace scxt::engine
{
namespace structure_edit_detail
{
using ZoneAddress = selection::SelectionManager::ZoneAddress;
namespace cmsg = messaging::client;

std::string freeName(const std::string &base, const std::set<std::string> &taken, bool copyStyle)
{
    for (int count = 0;; ++count)
    {
        auto n = base;
        if (copyStyle && count == 1)
            n += " (copy)";
        else if (copyStyle && count > 1)
            n += " (copy " + std::to_string(count) + ")";
        else if (!copyStyle && count > 0)
            n += " (" + std::to_string(count + 1) + ")";
        if (taken.find(n) == taken.end())
            return n;
    }
}

std::set<std::string> groupNames(const Part &p)
{
    std::set<std::string> res;
    for (const auto &g : p.getGroups())
        res.insert(g->name);
    return res;
}

std::set<std::string> zoneNames(const Part &p)
{
    std::set<std::string> res;
    for (const auto &g : p.getGroups())
        for (const auto &z : *g)
            res.insert(z->getName());
    return res;
}

// valid zones of one part in the order the part holds them
std::vector<ZoneAddress> validZonesInPart(const Engine &e, const std::vector<ZoneAddress> &addrs,
                                          int16_t part)
{
    std::vector<ZoneAddress> res;
    for (const auto &a : addrs)
        if (a.part == part && e.isValidZoneAddress(a))
            res.push_back(a);
    std::sort(res.begin(), res.end());
    res.erase(std::unique(res.begin(), res.end()), res.end());
    return res;
}

std::unique_ptr<Zone> cloneZone(Engine &e, const Zone &src, const std::string &name)
{
    auto v = json::scxt_value(src);
    auto res = std::make_unique<Zone>();
    v.to(*res);
    res->engine = &e;
    res->setupOnUnstream(e);
    res->givenName = name;
    return res;
}

std::unique_ptr<Group> cloneGroup(Engine &e, const Part &part, const Group &src, bool withZones)
{
    auto v = json::scxt_value(src);
    auto res = std::make_unique<Group>(e.rng);
    res->parentPart = const_cast<Part *>(&part);
    res->setSampleRate(part.getSampleRate());
    v.to(*res);
    if (!withZones)
        res->clearZones();
    res->warmup();
    return res;
}

std::unique_ptr<Group> freshGroup(Engine &e, const Part &part, const std::string &name)
{
    auto res = std::make_unique<Group>(e.rng);
    res->parentPart = const_cast<Part *>(&part);
    res->setSampleRate(part.getSampleRate());
    res->warmup();
    res->name = name;
    return res;
}

Group *groupWithID(Part &p, const GroupID &id)
{
    auto idx = p.getGroupIndex(id);
    return idx < 0 ? nullptr : p.getGroup(idx).get();
}

std::unique_ptr<Zone> takeZone(Engine &e, Part &p, const ZoneID &id)
{
    for (auto &g : p)
    {
        if (g->getZoneIndex(id) < 0)
            continue;
        e.terminateVoicesForZone(*g->getZone(id));
        return g->removeZone(id);
    }
    return {};
}

void sendStructure(const Engine &e, int16_t part)
{
    cmsg::serializationSendToClient(cmsg::s2c_send_pgz_structure, e.getPartGroupZoneStructure(),
                                    *(e.getMessageController()));
    cmsg::serializationSendToClient(cmsg::s2c_send_selected_group_zone_mapping_summary,
                                    e.getPatch()->getPart(part)->getZoneMappingSummary(),
                                    *(e.getMessageController()));
}

// copies take the selection, as a paste does
void selectByIdentity(const Engine &e, int16_t part, const std::vector<ZoneID> &zones,
                      const std::vector<GroupID> &groups)
{
    const auto &pt = e.getPatch()->getPart(part);
    std::vector<selection::SelectionManager::SelectActionContents> acts;
    for (int g = 0; g < (int)pt->getGroups().size(); ++g)
    {
        const auto &grp = pt->getGroup(g);
        if (std::find(groups.begin(), groups.end(), grp->id) != groups.end())
            acts.emplace_back(ZoneAddress{part, g, -1}, true, acts.empty(), acts.empty());
        for (int z = 0; z < (int)grp->getZones().size(); ++z)
            if (std::find(zones.begin(), zones.end(), grp->getZone(z)->id) != zones.end())
                acts.emplace_back(ZoneAddress{part, g, z}, true, acts.empty(), acts.empty());
    }
    // a lone group goes through as a group click, which brings its zones
    if (acts.size() == 1)
    {
        e.getSelectionManager()->applySelectActions(acts[0]);
        return;
    }
    for (const auto &a : acts)
        e.getSelectionManager()->applySelectActions(a);
}
} // namespace structure_edit_detail

using namespace structure_edit_detail;

void Engine::moveZonesTo(const std::vector<ZoneAddress> &addrs, const ZoneAddress &at, bool copy)
{
    assert(messageController->threadingChecker.isSerialThread());
    if (at.part < 0 || at.part >= numParts)
        return;
    auto part = (int16_t)at.part;
    auto src = validZonesInPart(*this, addrs, part);
    if (src.empty())
        return;

    auto &pt = getPatch()->getPart(part);
    std::vector<ZoneID> srcIDs;
    for (const auto &a : src)
        srcIDs.push_back(pt->getGroup(a.group)->getZone(a.zone)->id);

    Group *newGroup{nullptr};
    std::optional<GroupID> target;
    std::optional<ZoneID> before;
    if (isValidGroupAddress(at))
    {
        const auto &tg = pt->getGroup(at.group);
        target = tg->id;
        // moved zones leave, so land before the first zone from at which stays put
        for (int z = std::max(at.zone, 0); at.zone >= 0 && z < (int)tg->getZones().size(); ++z)
        {
            auto id = tg->getZone(z)->id;
            if (copy || std::find(srcIDs.begin(), srcIDs.end(), id) == srcIDs.end())
            {
                before = id;
                break;
            }
        }
    }
    else
    {
        auto gn = groupNames(*pt);
        newGroup = freshGroup(*this, *pt, freeName("New Group", gn, false)).release();
        target = newGroup->id;
    }

    std::vector<Zone *> clones;
    std::vector<ZoneID> cloneIDs;
    if (copy)
    {
        auto names = zoneNames(*pt);
        for (const auto &a : src)
        {
            const auto &z = pt->getGroup(a.group)->getZone(a.zone);
            auto n = freeName(z->getName(), names, true);
            names.insert(n);
            auto c = cloneZone(*this, *z, n);
            cloneIDs.push_back(c->id);
            clones.push_back(c.release());
        }
    }

    auto snap = selectionManager->snapshotIdentities(part);
    undo::pushPartStreamUndo(*this, part, copy ? "Copy Zones" : "Move Zones");

    messageController->scheduleAudioThreadCallbackUnderStructureLock(
        [part, srcIDs, clones, newGroup, target, before](auto &e) {
            auto &p = e.getPatch()->getPart(part);
            if (newGroup)
            {
                std::unique_ptr<Group> g(newGroup);
                p->addGroup(g);
                p->getGroups().back()->updatePolyphonyGroupParent(e);
            }

            std::vector<std::unique_ptr<Zone>> items;
            if (!clones.empty())
            {
                for (auto *c : clones)
                    items.emplace_back(c);
            }
            else
            {
                for (const auto &id : srcIDs)
                    if (auto z = takeZone(e, *p, id))
                        items.push_back(std::move(z));
            }

            auto *tg = groupWithID(*p, *target);
            if (!tg)
                return;
            auto idx = before.has_value() ? tg->getZoneIndex(*before) : -1;
            if (idx < 0)
                idx = (int)tg->getZones().size();
            for (auto &z : items)
                tg->insertZone(z, idx++);
        },
        [part, snap, cloneIDs](auto &e) {
            e.getSelectionManager()->restoreIdentities(snap);
            if (!cloneIDs.empty())
                selectByIdentity(e, part, cloneIDs, {});
            sendStructure(e, part);
        });
}

void Engine::moveGroupsTo(int16_t part, const std::vector<int32_t> &groupsIn, int32_t before,
                          bool copy)
{
    assert(messageController->threadingChecker.isSerialThread());
    if (part < 0 || part >= numParts)
        return;
    auto &pt = getPatch()->getPart(part);
    auto ng = (int32_t)pt->getGroups().size();

    std::vector<int32_t> groups;
    for (auto g : groupsIn)
        if (g >= 0 && g < ng)
            groups.push_back(g);
    std::sort(groups.begin(), groups.end());
    groups.erase(std::unique(groups.begin(), groups.end()), groups.end());
    if (groups.empty())
        return;

    std::vector<GroupID> srcIDs;
    for (auto g : groups)
        srcIDs.push_back(pt->getGroup(g)->id);

    std::optional<GroupID> beforeID;
    for (auto g = std::max(before, 0); before >= 0 && g < ng; ++g)
    {
        if (copy || std::find(groups.begin(), groups.end(), g) == groups.end())
        {
            beforeID = pt->getGroup(g)->id;
            break;
        }
    }

    // a move which lands everything where it already is does nothing
    if (!copy)
    {
        auto at = beforeID.has_value() ? pt->getGroupIndex(*beforeID) : ng;
        auto contiguous = groups.back() - groups.front() + 1 == (int32_t)groups.size();
        if (contiguous && (at == groups.back() + 1 || at == groups.front()))
            return;
    }

    std::vector<Group *> clones;
    std::vector<GroupID> cloneIDs;
    if (copy)
    {
        auto names = groupNames(*pt);
        for (auto g : groups)
        {
            const auto &og = pt->getGroup(g);
            auto c = cloneGroup(*this, *pt, *og, true);
            c->name = freeName(og->name, names, true);
            names.insert(c->name);
            cloneIDs.push_back(c->id);
            clones.push_back(c.release());
        }
    }

    auto snap = selectionManager->snapshotIdentities(part);
    undo::pushPartStreamUndo(*this, part, copy ? "Copy Groups" : "Move Groups");

    messageController->scheduleAudioThreadCallbackUnderStructureLock(
        [part, srcIDs, clones, beforeID](auto &e) {
            auto &p = e.getPatch()->getPart(part);
            std::vector<std::unique_ptr<Group>> items;
            if (!clones.empty())
            {
                for (auto *c : clones)
                    items.emplace_back(c);
            }
            else
            {
                for (const auto &id : srcIDs)
                {
                    auto idx = p->getGroupIndex(id);
                    if (idx < 0)
                        continue;
                    e.terminateVoicesForGroup(*p->getGroup(idx));
                    items.push_back(p->removeGroup(id));
                }
            }

            auto idx = beforeID.has_value() ? p->getGroupIndex(*beforeID) : -1;
            if (idx < 0)
                idx = (int)p->getGroups().size();
            for (auto &g : items)
            {
                p->insertGroup(g, idx);
                p->getGroup(idx)->updatePolyphonyGroupParent(e);
                idx++;
            }
        },
        [part, snap, cloneIDs](auto &e) {
            e.getSelectionManager()->restoreIdentities(snap);
            if (!cloneIDs.empty())
                selectByIdentity(e, part, {}, cloneIDs);
            sendStructure(e, part);
        });
}

void Engine::moveZonesToNewGroups(const std::vector<ZoneAddress> &addrsIn, bool eachOwn,
                                  bool cloneLeadGroup)
{
    assert(messageController->threadingChecker.isSerialThread());
    auto addrs = addrsIn;
    if (addrs.empty())
    {
        auto sz = selectionManager->currentlySelectedZones();
        addrs.assign(sz.begin(), sz.end());
    }
    if (addrs.empty())
        return;

    auto part = (int16_t)addrs.front().part;
    if (part < 0 || part >= numParts)
        return;
    auto src = validZonesInPart(*this, addrs, part);
    if (src.empty())
        return;
    auto &pt = getPatch()->getPart(part);

    const Group *pattern{nullptr};
    if (cloneLeadGroup)
    {
        auto lg = selectionManager->currentLeadGroup(*this);
        pattern = (lg.has_value() && lg->part == part && isValidGroupAddress(*lg))
                      ? pt->getGroup(lg->group).get()
                      : pt->getGroup(src.front().group).get();
    }

    auto names = groupNames(*pt);
    auto nameFor = [&](const ZoneAddress &a) {
        std::string base;
        if (eachOwn)
            base = pt->getGroup(a.group)->getZone(a.zone)->getName();
        else if (pattern)
            base = pattern->name;
        else
            base = "New Group";
        auto n = freeName(base, names, pattern && !eachOwn);
        names.insert(n);
        return n;
    };

    std::vector<Group *> newGroups;
    std::vector<std::pair<ZoneID, GroupID>> moves;
    for (const auto &a : src)
    {
        if (newGroups.empty() || eachOwn)
        {
            auto g = pattern ? cloneGroup(*this, *pt, *pattern, false) : freshGroup(*this, *pt, "");
            g->name = nameFor(a);
            newGroups.push_back(g.release());
        }
        moves.emplace_back(pt->getGroup(a.group)->getZone(a.zone)->id, newGroups.back()->id);
    }

    auto snap = selectionManager->snapshotIdentities(part);
    undo::pushPartStreamUndo(*this, part, "Move Zones to New Group");

    messageController->scheduleAudioThreadCallbackUnderStructureLock(
        [part, newGroups, moves](auto &e) {
            auto &p = e.getPatch()->getPart(part);
            for (auto *ng : newGroups)
            {
                std::unique_ptr<Group> g(ng);
                p->addGroup(g);
                p->getGroups().back()->updatePolyphonyGroupParent(e);
            }
            for (const auto &[zid, gid] : moves)
            {
                auto z = takeZone(e, *p, zid);
                auto *tg = groupWithID(*p, gid);
                if (z && tg)
                    tg->addZone(z);
            }
        },
        [part, snap](auto &e) {
            e.getSelectionManager()->restoreIdentities(snap);
            sendStructure(e, part);
        });
}

void Engine::deleteZonesWithMissingSamples(int16_t part)
{
    assert(messageController->threadingChecker.isSerialThread());
    if (part < 0 || part >= numParts)
        return;
    auto &pt = getPatch()->getPart(part);

    std::vector<ZoneAddress> doomed;
    std::vector<ZoneID> ids;
    for (int g = 0; g < (int)pt->getGroups().size(); ++g)
    {
        const auto &grp = pt->getGroup(g);
        for (int z = 0; z < (int)grp->getZones().size(); ++z)
        {
            if (grp->getZone(z)->missingSampleCount() > 0)
            {
                doomed.push_back({part, g, z});
                ids.push_back(grp->getZone(z)->id);
            }
        }
    }
    if (doomed.empty())
        return;

    auto snap = selectionManager->snapshotIdentities(part);
    undo::pushUndo<undo::ZonesRestoreItem>(*this, doomed);

    messageController->scheduleAudioThreadCallbackUnderStructureLock(
        [part, ids](auto &e) {
            auto &p = e.getPatch()->getPart(part);
            for (const auto &id : ids)
            {
                if (auto z = takeZone(e, *p, id))
                    e.getMessageController()->sendItemForDeletion(
                        z.release(),
                        messaging::audio::AudioToSerialization::ToBeDeleted::engine_Zone);
            }
        },
        [part, snap](auto &e) {
            e.getSelectionManager()->restoreIdentities(snap);
            sendStructure(e, part);
        });
}

namespace structure_edit_detail
{
// the lower zone fades out and the upper fades in across the span both share
template <typename R, typename O>
void autoCrossfade(std::vector<Zone::ZoneMappingData> &m, R range, O other)
{
    std::vector<int16_t> fadeIn(m.size(), -1), fadeOut(m.size(), -1);
    for (size_t i = 0; i < m.size(); ++i)
    {
        for (size_t j = 0; j < m.size(); ++j)
        {
            if (i == j)
                continue;
            auto [iLo, iHi] = range(m[i]);
            auto [jLo, jHi] = range(m[j]);
            auto [oiLo, oiHi] = other(m[i]);
            auto [ojLo, ojHi] = other(m[j]);
            if (oiHi < ojLo || ojHi < oiLo)
                continue;
            // i is the lower zone, overlapping but not containing j
            if (!(iLo < jLo && iHi >= jLo && iHi < jHi))
                continue;
            auto w = (int16_t)(iHi - jLo + 1);
            fadeOut[i] = std::max(fadeOut[i], w);
            fadeIn[j] = std::max(fadeIn[j], w);
        }
    }
    for (size_t i = 0; i < m.size(); ++i)
    {
        auto [lo, hi] = range(m[i]);
        auto span = (int16_t)(hi - lo + 1);
        auto &fs = range.fadeStart(m[i]);
        auto &fe = range.fadeEnd(m[i]);
        if (fadeIn[i] >= 0)
            fs = fadeIn[i];
        if (fadeOut[i] >= 0)
            fe = fadeOut[i];
        if (fs + fe > span)
        {
            // share what there is when both ends want more than the zone has
            fs = std::min(fs, (int16_t)(span / 2));
            fe = std::min(fe, (int16_t)(span - fs));
        }
    }
}

struct KeyAxis
{
    std::pair<int, int> operator()(const Zone::ZoneMappingData &d) const
    {
        return {d.keyboardRange.keyStart, d.keyboardRange.keyEnd};
    }
    int16_t &fadeStart(Zone::ZoneMappingData &d) const { return d.keyboardRange.fadeStart; }
    int16_t &fadeEnd(Zone::ZoneMappingData &d) const { return d.keyboardRange.fadeEnd; }
};
struct VelAxis
{
    std::pair<int, int> operator()(const Zone::ZoneMappingData &d) const
    {
        return {d.velocityRange.velStart, d.velocityRange.velEnd};
    }
    int16_t &fadeStart(Zone::ZoneMappingData &d) const { return d.velocityRange.fadeStart; }
    int16_t &fadeEnd(Zone::ZoneMappingData &d) const { return d.velocityRange.fadeEnd; }
};
} // namespace structure_edit_detail

void Engine::applyZoneBatchOp(ZoneBatchOp op, const std::vector<ZoneAddress> &which)
{
    assert(messageController->threadingChecker.isSerialThread());
    std::vector<ZoneAddress> sz(which.begin(), which.end());
    if (sz.empty())
    {
        auto cs = selectionManager->currentlySelectedZones();
        sz.assign(cs.begin(), cs.end());
    }
    std::vector<ZoneAddress> zones;
    for (const auto &a : sz)
        if (isValidZoneAddress(a))
            zones.push_back(a);
    std::sort(zones.begin(), zones.end());
    if (zones.empty())
        return;

    std::vector<Zone::ZoneMappingData> m;
    for (const auto &a : zones)
        m.push_back(getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone)->mapping);

    switch (op)
    {
    case ROOT_TO_FIRST_KEY:
    case ROOT_TO_CENTER_KEY:
    case ROOT_TO_LAST_KEY:
        for (auto &d : m)
        {
            const auto &kr = d.keyboardRange;
            d.rootKey = op == ROOT_TO_FIRST_KEY  ? kr.keyStart
                        : op == ROOT_TO_LAST_KEY ? kr.keyEnd
                                                 : (int16_t)((kr.keyStart + kr.keyEnd) / 2);
        }
        break;
    case AUTO_KEY_CROSSFADES:
        autoCrossfade(m, KeyAxis{}, VelAxis{});
        break;
    case AUTO_VELOCITY_CROSSFADES:
        autoCrossfade(m, VelAxis{}, KeyAxis{});
        break;
    case REMOVE_KEY_CROSSFADES:
        for (auto &d : m)
            d.keyboardRange.fadeStart = d.keyboardRange.fadeEnd = 0;
        break;
    case REMOVE_VELOCITY_CROSSFADES:
        for (auto &d : m)
            d.velocityRange.fadeStart = d.velocityRange.fadeEnd = 0;
        break;
    }

    undo::pushPayloadUndoFor<undo::ZoneMappingSpec>(*this, zones);
    messageController->scheduleAudioThreadCallback(
        [zones, m](auto &e) {
            for (size_t i = 0; i < zones.size(); ++i)
            {
                const auto &a = zones[i];
                e.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone)->mapping = m[i];
            }
        },
        [](const auto &e) { undo::refreshLeadDisplay(e, true); });
}

void Engine::setLinkZoneSelection(const std::vector<ZoneAddress> &groupsIn, bool link)
{
    assert(messageController->threadingChecker.isSerialThread());
    std::vector<ZoneAddress> groups;
    for (const auto &a : groupsIn)
        if (isValidGroupAddress(a))
            groups.push_back({a.part, a.group, -1});
    if (groups.empty())
        return;

    undo::pushPayloadUndoFor<undo::GroupLinkZoneSelectionSpec>(*this, groups);
    for (const auto &a : groups)
        getPatch()->getPart(a.part)->getGroup(a.group)->linkZoneSelection = link;

    // a group linked with one of its zones selected takes the rest along now
    if (link)
    {
        std::vector<selection::SelectionManager::SelectActionContents> acts;
        auto sel = selectionManager->currentlySelectedZones();
        for (const auto &z : sel)
        {
            for (const auto &g : groups)
            {
                if (z.part == g.part && z.group == g.group)
                    acts.emplace_back(z, true, false, false);
            }
        }
        if (!acts.empty())
            selectionManager->applySelectActions(acts);
    }

    undo::resendPGZStructure(*this);
    undo::refreshLeadDisplay(*this, true);
}

void Engine::initializePart(int16_t part)
{
    assert(messageController->threadingChecker.isSerialThread());
    if (part < 0 || part >= numParts)
        return;

    undo::pushPartStreamUndo(*this, part, "Initialize Part");

    messageController->stopAudioThreadThenRunOnSerial([part](const auto &engine) {
        auto &e = const_cast<Engine &>(engine);
        try
        {
            e.immediatelyTerminateAllVoices();

            auto &pt = e.getPatch()->getPart(part);
            // the slot is layout rather than sound, so it keeps its own of these
            auto config = pt->configuration;

            std::string fresh;
            {
                Part blank(part);
                fresh = tao::json::to_string(json::scxt_value(blank));
            }
            json::unstreamPartState(e, part, fresh, false, false);

            pt->configuration.channel = config.channel;
            pt->configuration.routeTo = config.routeTo;
            pt->configuration.active = config.active;
            e.onPartConfigurationUpdated();

            e.getSelectionManager()->state[part] = {};
            e.getSelectionManager()->guaranteeConsistencyAfterDeletes(e, true, {part, -1, -1});
            e.getSelectionManager()->clearPartFile(part);
            e.getSelectionManager()->sendPatchFilesToClient();
            e.sendPartNamesToClient(part);
            e.sendFullRefreshToClient();
        }
        catch (std::exception &err)
        {
            RAISE_ERROR_ENGINE(e, "Initialize Part Error",
                               std::string("Unable to initialize the part ") + err.what());
        }
        e.getMessageController()->restartAudioThreadFromSerial();
    });
}

void Engine::duplicatePart(int16_t part)
{
    assert(messageController->threadingChecker.isSerialThread());
    if (part < 0 || part >= numParts)
        return;

    int16_t target{-1};
    for (int16_t p = 0; p < numParts && target < 0; ++p)
        if (!getPatch()->getPart(p)->configuration.active)
            target = p;
    if (target < 0)
    {
        messageController->reportErrorToClient(
            "Unable to Duplicate Part", "All sixteen part slots are in use.", __FILE__, __LINE__);
        return;
    }

    undo::pushPartStreamUndo(*this, target, "Duplicate Part");

    prepareToStream();
    std::string payload;
    {
        auto sg = Engine::StreamGuard(Engine::FOR_PART);
        payload = tao::json::to_string(json::scxt_value(*getPatch()->getPart(part)));
    }

    messageController->stopAudioThreadThenRunOnSerial([target, payload](const auto &engine) {
        auto &e = const_cast<Engine &>(engine);
        try
        {
            json::unstreamPartState(e, target, payload, false, true);
            auto &pt = e.getPatch()->getPart(target);
            pt->configuration.active = true;
            e.onPartConfigurationUpdated();

            e.getSelectionManager()->guaranteeConsistencyAfterDeletes(e, true, {target, -1, -1});
            e.getSelectionManager()->clearPartFile(target);
            e.getSelectionManager()->sendPatchFilesToClient();
            e.sendPartNamesToClient(target);
            e.sendFullRefreshToClient();
        }
        catch (std::exception &err)
        {
            RAISE_ERROR_ENGINE(e, "Duplicate Part Error",
                               std::string("Unable to duplicate the part ") + err.what());
        }
        e.getMessageController()->restartAudioThreadFromSerial();
    });
}

} // namespace scxt::engine
