#ifndef __SCXT_ENGINE_PATCH_H
#define __SCXT_ENGINE_PATCH_H

#include "utils.h"
#include "part.h"
#include <memory>
#include <array>
#include <sst/cpputils.h>

namespace scxt::engine
{
struct Patch : NonCopyable<Patch>
{
    Patch() : id(PatchID::next())
    {
        for (int i = 0; i < numParts; ++i)
        {
            parts[i] = std::make_unique<Part>(i);
        }
    }
    PatchID id;

    /**
     * Get a part by the index in array
     */
    const std::unique_ptr<Part> &getPart(int i) const
    {
        assert(i >= 0 && i < numParts);
        return parts[i];
    }

    /**
     * Get the index of a part by the PartID.
     */
    int getPartIndex(const PartID &pid) const
    {
        for (const auto &[idx, r] : sst::cpputils::enumerate(parts))
            if (r->id == pid)
                return idx;
        return -1;
    }

    /**
     * Get the part by part ID. May throw.
     */
    const std::unique_ptr<Part> &getPart(const PartID &pid) const
    {
        auto idx = getPartIndex(pid);
        if (idx < 0 || idx >= parts.size())
            throw SCXTError("Unable to locate part " + pid.to_string() + " in patch " +
                              id.to_string());
        return parts[idx];
    }
    static constexpr int numParts{16};

    typedef std::array<std::unique_ptr<Part>, numParts> partContainer_t;

    partContainer_t::iterator begin() noexcept { return parts.begin(); }
    partContainer_t::const_iterator cbegin() const noexcept { return parts.cbegin(); }

    partContainer_t::iterator end() noexcept { return parts.end(); }
    partContainer_t::const_iterator cend() const noexcept { return parts.cend(); }

  private:
    partContainer_t parts;
};
} // namespace scxt::engine

#endif