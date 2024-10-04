/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#include "group_triggers.h"
#include "utils.h"

namespace scxt::engine
{

std::string toStringGroupTriggerID(const GroupTriggerID &p)
{
    switch (p)
    {
        // lesson learned earlier was long names are not that handy debugging now the infra works
        // and just make big files so use compact ones here
    case GroupTriggerID::NONE:
        return "n";
    case GroupTriggerID::MACRO:
        return "mcr";
    case GroupTriggerID::MIDICC:
        return "mcc";
    }
}
GroupTriggerID fromStringGroupTriggerID(const std::string &s)
{
    static auto inverse = makeEnumInverse<GroupTriggerID, toStringGroupTriggerID>(
        GroupTriggerID::NONE, GroupTriggerID::MIDICC);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return GroupTriggerID::NONE;
    return p->second;
}

std::string GroupTriggerConditions::toStringGroupConditionsConjunction(const Conjunction &p)
{
    switch (p)
    {
    case Conjunction::AND:
        return "a";
    case Conjunction::OR:
        return "o";
    case Conjunction::AND_NOT:
        return "a!";
    case Conjunction::OR_NOT:
        return "o!";
    }
    return "";
}
GroupTriggerConditions::Conjunction fromStringConditionsConjunction(const std::string &s)
{
    static auto inverse =
        makeEnumInverse<GroupTriggerConditions::Conjunction,
                        GroupTriggerConditions::toStringGroupConditionsConjunction>(
            GroupTriggerConditions::Conjunction::AND, GroupTriggerConditions::Conjunction::OR_NOT);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return GroupTriggerConditions::Conjunction::AND;
    return p->second;
}

std::unique_ptr<GroupTrigger> makeGroupTrigger(GroupTriggerID, GroupTriggerInstrumentState &,
                                               GroupTriggerStorage &)
{
    return {};
}

// THIS NEEDS ARGUMENTS of the state and so on
void GroupTriggerConditions::setupOnUnstream(GroupTriggerInstrumentState &gis)
{
    SCLOG("Setup on Unstream");
    for (int i = 0; i < triggerConditionsPerGroup; ++i)
    {
        auto &s = storage[i];
        if (s.id == GroupTriggerID::NONE)
        {
            conditions[i].reset();
        }
        else
        {
            conditions[i] = makeGroupTrigger(s.id, gis, s);
        }
    }
}

} // namespace scxt::engine