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

#include "group_matrix.h"
#include "engine/group.h"
#include <stdexcept>

namespace scxt::modulation
{

std::string getGroupModMatrixDestStreamingName(const GroupModMatrixDestinationType &dest)
{
    switch (dest)
    {
    case gmd_none:
        return "gmd_none";

    case gmd_grouplevel:
        return "gmd_grouplevel";

    case gmd_LFO_Rate:
        return "gmd_lforate";

    case numGroupMatrixDestinations:
        throw std::logic_error("Can't convert numGroupMatrixDestinations to string");
    }

    throw std::logic_error("Invalid enum");
}
std::optional<GroupModMatrixDestinationType>
fromGroupModMatrixDestStreamingName(const std::string &s)
{
    for (int i = gmd_none; i < numGroupMatrixDestinations; ++i)
    {
        if (getGroupModMatrixDestStreamingName((GroupModMatrixDestinationType)i) == s)
        {
            return ((GroupModMatrixDestinationType)i);
        }
    }
    return gmd_none;
}

std::string getGroupModMatrixSourceStreamingName(const GroupModMatrixSource &dest)
{
    switch (dest)
    {
    case gms_none:
        return "gms_none";
    case gms_EG1:
        return "gms_EG1";
    case gms_EG2:
        return "gms_EG2";
    case gms_LFO1:
        return "gms_LFO1";
    case gms_LFO2:
        return "gms_LFO2";
    case gms_LFO3:
        return "gms_LFO3";

    case numGroupMatrixSources:
        throw std::logic_error("Don't call with numGroupMatrixSources");
    }

    throw std::logic_error("Mysterious unhandled condition");
}
std::optional<GroupModMatrixSource> fromGroupModMatrixSourceStreamingName(const std::string &s)
{
    for (int i = gms_none; i < numGroupMatrixSources; ++i)
    {
        if (getGroupModMatrixSourceStreamingName((GroupModMatrixSource)i) == s)
        {
            return ((GroupModMatrixSource)i);
        }
    }
    return gms_none;
}

void GroupModMatrix::copyBaseValuesFromGroup(engine::Group &g)
{
    baseValues[destIndex(gmd_grouplevel, 0)] = g.level;
    for (int i = 0; i < engine::lfosPerGroup; ++i)
    {
        baseValues[destIndex(gmd_LFO_Rate, i)] = g.lfoStorage[i].rate;
    }
}

void GroupModMatrix::updateModulatorUsed(engine::Group &g) const
{
    // TODO: Call this only when the mod matrix morphs. For now run them all
    for (const auto &r : g.routingTable)
    {
        g.gegUsed[0] = g.gegUsed[0] || r.src == gms_EG1 || r.srcVia == gms_EG1;
        g.gegUsed[1] = g.gegUsed[1] || r.src == gms_EG2 || r.srcVia == gms_EG2;
        g.lfoUsed[0] = g.lfoUsed[0] || r.src == gms_LFO1 || r.srcVia == gms_LFO1;
        g.lfoUsed[1] = g.lfoUsed[1] || r.src == gms_LFO2 || r.srcVia == gms_LFO2;
        g.lfoUsed[2] = g.lfoUsed[2] || r.src == gms_LFO3 || r.srcVia == gms_LFO3;
    }

    g.anyModulatorUsed =
        g.gegUsed[0] || g.gegUsed[1] || g.lfoUsed[0] || g.lfoUsed[1] || g.lfoUsed[2];
}

void GroupModMatrix::assignSourcesFromGroup(engine::Group &g)
{
    sourcePointers[gms_LFO1] = &g.lfos[0].output;
    sourcePointers[gms_LFO2] = &g.lfos[1].output;
    sourcePointers[gms_LFO3] = &g.lfos[2].output;

    sourcePointers[gms_EG1] = &g.gegEvaluators[0].outBlock0;
    sourcePointers[gms_EG2] = &g.gegEvaluators[1].outBlock0;
}

std::string getGroupModMatrixSourceDisplayName(const GroupModMatrixSource &dest)
{
    switch (dest)
    {
    case gms_none:
        return "";

    case gms_EG1:
        return "Grp EG1";
    case gms_EG2:
        return "Grp EG2";

    case gms_LFO1:
        return "Grp LFO1";
    case gms_LFO2:
        return "Grp LFO2";
    case gms_LFO3:
        return "Grp LFO3";

    case numGroupMatrixSources:
        throw std::logic_error("Don't call with numGroupMatrixSources");
    }

    throw std::logic_error("Mysterious unhandled condition");
}

groupModMatrixSourceNames_t getGroupModMatrixSourceNames(const engine::Group &g)
{
    groupModMatrixSourceNames_t res;
    for (int s = (int)gms_none; s < numGroupMatrixSources; ++s)
    {
        auto gms = (GroupModMatrixSource)s;
        res.emplace_back(gms, getGroupModMatrixSourceDisplayName(gms));
    }
    return res;
}

int getGroupModMatrixDestIndexCount(const GroupModMatrixDestinationType &t)
{
    switch (t)
    {
    case gmd_grouplevel:
        return 1;

    case gmd_LFO_Rate:
        return engine::lfosPerGroup;

    case gmd_none:
        return 1;
    case numGroupMatrixDestinations:
        throw std::logic_error("Don't call index cound with num");
    }
    assert(false);
    return 1;
}

std::optional<std::string>
getGroupModMatrixDestDisplayName(const GroupModMatrixDestinationAddress &dest,
                                 const engine::Group &g)
{
    // TODO: This is obviously .... wrong
    /*
     * These are a bit trickier since they are indexed so
     */
    const auto &[gmd, idx] = dest;

    if (gmd == gmd_none)
    {
        return "";
    }

    if (gmd == gmd_grouplevel)
    {
        return "Output/Level";
    }

    if (gmd == gmd_LFO_Rate)
    {
        return fmt::format("GLFO {}/Rate", idx + 1);
    }
    assert(false);
    return fmt::format("ERR {} {}", gmd, idx);
}

groupModMatrixDestinationNames_t getGroupModulationDestinationNames(const engine::Group &g)
{
    groupModMatrixDestinationNames_t res;
    // TODO code this way better - index on the 'outside' sorts but is inefficient
    int maxIndex = std::max({2, engine::processorCount, engine::lfosPerGroup});
    for (int idx = 0; idx < maxIndex; ++idx)
    {
        for (int i = gmd_none; i < numGroupMatrixDestinations; ++i)
        {
            auto vd = (GroupModMatrixDestinationType)i;
            auto ic = getGroupModMatrixDestIndexCount(vd);
            if (idx < ic)
            {
                auto addr = GroupModMatrixDestinationAddress{vd, (size_t)idx};
                auto dn = getGroupModMatrixDestDisplayName(addr, g);
                if (dn.has_value())
                    res.emplace_back(addr, *dn);
            }
        }
    }
    return res;
}
} // namespace scxt::modulation
