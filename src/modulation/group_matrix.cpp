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
#include <stdexcept>

namespace scxt::modulation
{

std::string getGroupModMatrixDestStreamingName(const GroupModMatrixDestinationType &dest)
{
    switch (dest)
    {
    case gmd_none:
        return "gmd_none";

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
        return "vms_none";

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

} // namespace scxt::modulation
