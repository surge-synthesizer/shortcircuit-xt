#ifndef __SCXT_ENGINE_PART_H
#define __SCXT_ENGINE_PART_H

#include <memory>
#include <vector>
#include <optional>

#include "utils.h"
#include "group.h"

namespace scxt::engine
{
struct Part : NonCopyable<Part>
{
    Part(int c) : id(PartID::next()), channel(c) { addGroup(); }

    PartID id;
    int16_t channel;
    static constexpr int omniChannel{-1};

    size_t addGroup()
    {
        auto g = std::make_unique<Group>();
        g->parentPart = this;
        groups.push_back(std::move(g));
        return groups.size();
    }

    // TODO GroupID -> index
    // TODO: Remove Group by both - Copy from group basically

    const std::unique_ptr<Group> &getGroup(int i) const
    {
        assert(i >= 0 && i < groups.size());
        return groups[i];
    }

    // TODO: A group by ID which throws an SCXTError

    typedef std::vector<std::unique_ptr<Group>> groupContainer_t;

    groupContainer_t::iterator begin() noexcept { return groups.begin(); }
    groupContainer_t::const_iterator cbegin() const noexcept { return groups.cbegin(); }

    groupContainer_t::iterator end() noexcept { return groups.end(); }
    groupContainer_t::const_iterator cend() const noexcept { return groups.cend(); }

  private:
    groupContainer_t groups;
};
} // namespace scxt::engine

#endif