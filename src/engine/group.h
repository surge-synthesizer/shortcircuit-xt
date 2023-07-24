/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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
#ifndef SCXT_SRC_ENGINE_GROUP_H
#define SCXT_SRC_ENGINE_GROUP_H

#include <memory>
#include <vector>
#include <cassert>

#include <sst/cpputils.h>

#include "utils.h"
#include "zone.h"
#include "selection/selection_manager.h"
#include "datamodel/adsr_storage.h"
#include "bus.h"

namespace scxt::engine
{
struct Part;
struct Engine;

struct Group : MoveableOnly<Group>
{
    Group() : id(GroupID::next()), name(id.to_string()) {}
    GroupID id;

    std::string name{};
    Part *parentPart{nullptr};

    BusAddress routeTo{DEFAULT_BUS};

    float output alignas(16)[2][blockSize];
    void process(Engine &onto);

    // ToDo editable name
    std::string getName() const { return name; }

    size_t addZone(std::unique_ptr<Zone> &z)
    {
        z->parentGroup = this;
        zones.push_back(std::move(z));
        return zones.size();
    }

    size_t addZone(std::unique_ptr<Zone> &&z)
    {
        z->parentGroup = this;
        zones.push_back(std::move(z));
        return zones.size();
    }

    void clearZones() { zones.clear(); }

    int getZoneIndex(const ZoneID &zid) const
    {
        for (const auto &[idx, r] : sst::cpputils::enumerate(zones))
            if (r->id == zid)
                return idx;
        return -1;
    }

    const std::unique_ptr<Zone> &getZone(int idx) const
    {
        assert(idx >= 0 && idx < zones.size());
        return zones[idx];
    }

    const std::unique_ptr<Zone> &getZone(const ZoneID &zid) const
    {
        auto idx = getZoneIndex(zid);
        if (idx < 0 || idx >= zones.size())
            throw SCXTError("Unable to locate part " + zid.to_string() + " in patch " +
                            id.to_string());
        return zones[idx];
    }

    std::unique_ptr<Zone> removeZone(ZoneID &zid)
    {
        auto idx = getZoneIndex(zid);
        if (idx < 0 || idx >= zones.size())
            return {};

        auto res = std::move(zones[idx]);
        zones.erase(zones.begin() + idx);
        res->parentGroup = nullptr;
        return res;
    }

    bool isActive() { return activeZones != 0; }
    void addActiveZone();
    void removeActiveZone();

    static constexpr int egPerGroup{2};
    std::array<datamodel::AdsrStorage, egPerGroup> gegStorage;

    uint32_t activeZones{0};

    typedef std::vector<std::unique_ptr<Zone>> zoneContainer_t;

    const zoneContainer_t &getZones() const { return zones; }

    zoneContainer_t::iterator begin() noexcept { return zones.begin(); }
    zoneContainer_t::const_iterator cbegin() const noexcept { return zones.cbegin(); }

    zoneContainer_t::iterator end() noexcept { return zones.end(); }
    zoneContainer_t::const_iterator cend() const noexcept { return zones.cend(); }

  private:
    zoneContainer_t zones;
};
} // namespace scxt::engine

#endif