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

#include "group.h"
#include "part.h"
#include "utils.h"

namespace scxt::engine
{

std::string toStringGroupTriggerID(const GroupTriggerID &p)
{
    if (p >= GroupTriggerID::MACRO && (int)p <= (int)GroupTriggerID::MACRO + scxt::macrosPerPart)
    {
        return fmt::format("macro{}", (int)p - (int)GroupTriggerID::MACRO);
    }
    if (p >= GroupTriggerID::MIDICC && p <= GroupTriggerID::LAST_MIDICC)
    {
        return fmt::format("midcc{}", (int)p - (int)GroupTriggerID::MIDICC);
    }
    switch (p)
    {
        // lesson learned earlier was long names are not that handy debugging now the infra works
        // and just make big files so use compact ones here
    case GroupTriggerID::NONE:
        return "n";
    case GroupTriggerID::MACRO:
    case GroupTriggerID::MIDICC:
    case GroupTriggerID::LAST_MIDICC:
    {
        assert(false);
    }
    break;
    }
    return "n";
}
GroupTriggerID fromStringGroupTriggerID(const std::string &s)
{
    static auto inverse = makeEnumInverse<GroupTriggerID, toStringGroupTriggerID>(
        GroupTriggerID::NONE, GroupTriggerID::LAST_MIDICC);
    auto p = inverse.find(s);
    if (p == inverse.end())
    {
        return GroupTriggerID::NONE;
    }
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
    return "a";
}
GroupTriggerConditions::Conjunction
GroupTriggerConditions::fromStringConditionsConjunction(const std::string &s)
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

struct GTMacro : GroupTrigger
{
    int macro{0};
    float lb{0}, ub{1};
    GTMacro(GroupTriggerID id, GroupTriggerInstrumentState &onState, GroupTriggerStorage &onStorage,
            int macro)
        : macro(macro), GroupTrigger(id, onState, onStorage)
    {
    }

    bool value(const Engine &, const Group &g) const override
    {
        auto mv = g.parentPart->macros[macro].value;
        return mv >= lb && mv <= ub;
    }

    void storageAdjusted() override
    {
        lb = storage.args[0];
        ub = storage.args[1];
        if (lb > ub)
            std::swap(lb, ub);
    }
};

struct GTMIDI1CC : GroupTrigger
{
    int cc;
    float lb{0}, ub{1};
    GTMIDI1CC(GroupTriggerID id, GroupTriggerInstrumentState &onState,
              GroupTriggerStorage &onStorage, int cc)
        : cc(cc), GroupTrigger(id, onState, onStorage)
    {
    }

    bool value(const Engine &, const Group &g) const override
    {
        assert(g.parentPart);
        auto ccv = g.parentPart->midiCCValues[cc];
        return ccv >= lb && ccv <= ub;
    }

    void storageAdjusted() override
    {
        auto flb = storage.args[0];
        auto fub = storage.args[1];
        if (flb > fub)
            std::swap(flb, fub);
        lb = std::floor(flb) / 127.0;
        ub = std::ceil(fub) / 127.0;
    }
};

GroupTrigger *makeGroupTrigger(GroupTriggerID id, GroupTriggerInstrumentState &gis,
                               GroupTriggerStorage &st, GroupTriggerBuffer &bf)
{
    if (id >= GroupTriggerID::MACRO && (int)id <= (int)GroupTriggerID::MACRO + scxt::macrosPerPart)
    {
        static_assert(sizeof(GTMacro) < sizeof(GroupTriggerBuffer));
        return new (bf) GTMacro(id, gis, st, (int)id - (int)GroupTriggerID::MACRO);
    }
    if (id >= GroupTriggerID::MIDICC && id <= GroupTriggerID::LAST_MIDICC)
    {
        static_assert(sizeof(GTMIDI1CC) < sizeof(GroupTriggerBuffer));
        return new (bf) GTMIDI1CC(id, gis, st, (int)id - (int)GroupTriggerID::MIDICC);
    }

#define CS(id, tp)                                                                                 \
    static_assert(sizeof(tp) < sizeof(GroupTriggerBuffer));                                        \
    case id:                                                                                       \
        return new (bf) tp(id, gis, st);
    switch (id)
    {
    default:
        return nullptr;
    }
    return nullptr;
}

std::string getGroupTriggerDisplayName(GroupTriggerID id)
{
    if (id >= GroupTriggerID::MACRO && (int)id <= (int)GroupTriggerID::MACRO + scxt::macrosPerPart)
    {
        return fmt::format("MACRO {}", (int)id - (int)GroupTriggerID::MACRO + 1);
    }
    if (id >= GroupTriggerID::MIDICC && id <= GroupTriggerID::LAST_MIDICC)
    {
        return fmt::format("MIDICC {}", (int)id - (int)GroupTriggerID::MIDICC);
    }

    switch (id)
    {
    case GroupTriggerID::NONE:
        return "NONE";
    default:
        return "ERROR";
    }
    return "ERROR";
}

void GroupTriggerConditions::setupOnUnstream(GroupTriggerInstrumentState &gis)
{
    bool allNone{true};
    for (int i = 0; i < triggerConditionsPerGroup; ++i)
    {
        auto &s = storage[i];
        if (s.id == GroupTriggerID::NONE)
        {
            if (conditions[i])
                conditions[i]->~GroupTrigger();
            conditions[i] = nullptr;
        }
        else
        {
            allNone = false;

            if (!conditions[i] || conditions[i]->getID() != s.id)
            {
                conditions[i] = makeGroupTrigger(s.id, gis, s, conditionBuffers[i]);
            }
        }
        if (conditions[i])
            conditions[i]->storageAdjusted();
    }
    alwaysReturnsTrue = allNone;
}

bool GroupTriggerConditions::value(const Engine &e, const Group &g) const
{
    if (alwaysReturnsTrue)
        return true;

    auto v = true;
    for (int i = 0; i < triggerConditionsPerGroup; ++i)
    {
        // FIXME - conjunctions
        if (active[i] && conditions[i])
            v = v & conditions[i]->value(e, g);
    }
    return v;
}

} // namespace scxt::engine