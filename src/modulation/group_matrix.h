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

#ifndef SCXT_SRC_MODULATION_GROUP_MATRIX_H
#define SCXT_SRC_MODULATION_GROUP_MATRIX_H

#include <string>
#include <array>
#include "utils.h"
#include "base_matrix.h"

namespace scxt::engine
{
struct Group;
} // namespace scxt::engine

namespace scxt::modulation
{

enum GroupModMatrixSource
{
    gms_none,

    numGroupMatrixSources
};

enum GroupModMatrixDestinationType
{
    gmd_none,

    numGroupMatrixDestinations
};

std::string getGroupModMatrixDestStreamingName(const GroupModMatrixDestinationType &dest);
std::optional<GroupModMatrixDestinationType>
fromGroupModMatrixDestStreamingName(const std::string &s);

std::string getGroupModMatrixSourceStreamingName(const GroupModMatrixSource &dest);
std::optional<GroupModMatrixSource> fromGroupModMatrixSourceStreamingName(const std::string &s);

struct GroupModMatrixDestinationAddress
{
    static constexpr int maxIndex{4}; // 4 processors per zone
    static constexpr int maxDestinations{maxIndex * numGroupMatrixDestinations};
    GroupModMatrixDestinationType type{gmd_none};
    size_t index{0};

    // want in order, not index interleaved, so we can look at FP as a block etc...
    operator size_t() const { return (size_t)type + numGroupMatrixDestinations * index; }

    static constexpr inline size_t destIndex(GroupModMatrixDestinationType t, size_t idx)
    {
        return (size_t)t + numGroupMatrixDestinations * idx;
    }

    bool operator==(const GroupModMatrixDestinationAddress &other) const
    {
        return other.type == type && other.index == index;
    }
};
struct GroupModMatrixTraits
{
    static constexpr int numModMatrixSlots{6};
    typedef GroupModMatrixSource SourceEnum;
    static constexpr SourceEnum SourceEnumNoneValue{gms_none};
    typedef GroupModMatrixDestinationAddress DestAddress;
    typedef GroupModMatrixDestinationType DestEnum;
    static constexpr DestEnum DestEnumNoneValue{gmd_none};
};
struct GroupModMatrix : public MoveableOnly<GroupModMatrix>, ModMatrix<GroupModMatrixTraits>
{
    GroupModMatrix() { clear(); }
};
} // namespace scxt::modulation
#endif // SHORTCIRCUITXT_GROUP_MATRIX_H
