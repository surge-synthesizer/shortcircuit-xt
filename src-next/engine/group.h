#ifndef __SCXT_ENGINE_GROUP_H
#define __SCXT_ENGINE_GROUP_H

#include <memory>
#include <vector>
#include <cassert>

#include <sst/cpputils.h>

#include "utils.h"
#include "zone.h"

namespace scxt::engine
{
struct Part;
struct Group : NonCopyable<Group>
{
    Group() : id(GroupID::next()) {}
    GroupID id;

    Part *parentPart{nullptr};

    float output alignas(16)[maxOutputs][2][blockSize];
    void process();

    // TODO: Multi-output
    size_t getNumOutputs() const { return 1; }

    size_t addZone(std::unique_ptr<Zone> &z)
    {
        z->parentGroup = this;
        zones.push_back(std::move(z));
        return zones.size();
    }

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

    uint32_t activeZones{0};

    typedef std::vector<std::unique_ptr<Zone>> zoneContainer_t;
    zoneContainer_t::iterator begin() noexcept { return zones.begin(); }
    zoneContainer_t::const_iterator cbegin() const noexcept { return zones.cbegin(); }

    zoneContainer_t::iterator end() noexcept { return zones.end(); }
    zoneContainer_t::const_iterator cend() const noexcept { return zones.cend(); }

  private:
    zoneContainer_t zones;
};
} // namespace scxt::engine

#endif