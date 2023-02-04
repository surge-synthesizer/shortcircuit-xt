#ifndef __SCXT_ENGINE_PART_H
#define __SCXT_ENGINE_PART_H

#include <memory>
#include <vector>
#include <optional>
#include <cassert>

#include "utils.h"
#include "group.h"

namespace scxt::engine
{
struct Patch;

struct Part : MoveableOnly<Part>
{
    Part(int16_t c) : id(PartID::next()), channel(c) { addGroup(); }

    PartID id;
    int16_t channel;
    Patch *parentPatch{nullptr};

    float output alignas(16)[maxOutputs][2][blockSize];
    void process();

    // TODO: have a channel mode like OMNI and MPE and everything
    static constexpr int16_t omniChannel{-1};

    // TODO: Multiple outputs
    size_t getNumOutputs() const { return 1; }

    size_t addGroup()
    {
        auto g = std::make_unique<Group>();
        g->parentPart = this;
        groups.push_back(std::move(g));
        return groups.size();
    }

    // TODO GroupID -> index
    // TODO: Remove Group by both - Copy from group basically

    const std::unique_ptr<Group> &getGroup(size_t i) const
    {
        assert(i < groups.size());
        return groups[i];
    }

    uint32_t activeGroups{0};
    bool isActive() { return activeGroups != 0; }
    void addActiveGroup() { activeGroups++; }
    void removeActiveGroup()
    {
        assert(activeGroups);
        activeGroups--;
    }

    // TODO: A group by ID which throws an SCXTError

    typedef std::vector<std::unique_ptr<Group>> groupContainer_t;

    const groupContainer_t &getGroups() const { return groups; }
    void clearGroups() { groups.clear(); }

    groupContainer_t::iterator begin() noexcept { return groups.begin(); }
    groupContainer_t::const_iterator cbegin() const noexcept { return groups.cbegin(); }

    groupContainer_t::iterator end() noexcept { return groups.end(); }
    groupContainer_t::const_iterator cend() const noexcept { return groups.cend(); }

  private:
    groupContainer_t groups;
};
} // namespace scxt::engine

#endif