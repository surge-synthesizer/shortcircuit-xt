/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
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
#include "engine.h"

namespace scxt::engine
{

std::string toStringGroupTriggerID(const GroupTriggerID &p)
{
    if (p >= GroupTriggerID::MACRO && (int)p < (int)GroupTriggerID::MACRO + scxt::macrosPerPart)
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
    case GroupTriggerID::KEYSWITCH_LATCH:
        return "ksL";
    case GroupTriggerID::KEYSWITCH_MOMENTARY:
        return "ksM";
    case GroupTriggerID::PROGRAM_CHANGE:
        return "pgm";
    case GroupTriggerID::PITCH_BEND:
        return "pbnd";
    case GroupTriggerID::ROUND_ROBIN_CYCLE:
        return "rr";
    case GroupTriggerID::ROUND_ROBIN_RANDOM:
        return "rrR";
    case GroupTriggerID::ROUND_ROBIN_SHUFFLE:
        return "rrS";
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

    bool conditionHolds(const Engine &, const Group &g, int16_t, int16_t) const override
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

    bool conditionHolds(const Engine &, const Group &g, int16_t, int16_t) const override
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

struct GTProgramChange : GroupTrigger
{
    int lb{0}, ub{0};
    GTProgramChange(GroupTriggerID id, GroupTriggerInstrumentState &onState,
                    GroupTriggerStorage &onStorage)
        : GroupTrigger(id, onState, onStorage)
    {
    }

    // The last program change the part saw, which is 0 until one arrives
    bool conditionHolds(const Engine &, const Group &g, int16_t, int16_t) const override
    {
        assert(g.parentPart);
        auto pc = g.parentPart->lastProgramChange;
        return pc >= lb && pc <= ub;
    }

    void storageAdjusted() override
    {
        lb = (int)std::round(storage.args[0]);
        ub = (int)std::round(storage.args[1]);
        if (lb > ub)
            std::swap(lb, ub);
    }
};

struct GTPitchBend : GroupTrigger
{
    // Signed 14 bit, matching the part - not the -1..1 float the DSP uses
    int lb{0}, ub{0};
    GTPitchBend(GroupTriggerID id, GroupTriggerInstrumentState &onState,
                GroupTriggerStorage &onStorage)
        : GroupTrigger(id, onState, onStorage)
    {
    }

    bool conditionHolds(const Engine &, const Group &g, int16_t, int16_t) const override
    {
        assert(g.parentPart);
        auto pb = g.parentPart->pitchBend14Bit;
        return pb >= lb && pb <= ub;
    }

    void storageAdjusted() override
    {
        lb = (int)std::round(storage.args[0]);
        ub = (int)std::round(storage.args[1]);
        if (lb > ub)
            std::swap(lb, ub);
    }
};

struct GTKeyswitchLatch : GroupTrigger
{
    GTKeyswitchLatch(GroupTriggerID id, GroupTriggerInstrumentState &onState,
                     GroupTriggerStorage &onStorage)
        : GroupTrigger(id, onState, onStorage)
    {
    }

    // Holds when the incoming key is my switch key. That key selects the group rather than
    // sounding it, so a holding latch suppresses play.
    bool conditionHolds(const Engine &, const Group &g, int16_t, int16_t midiKey) const override
    {
        return midiKey == (int)std::round(storage.args[0]);
    }

    bool holdingSuppressesPlay() const override { return true; }

    void storageAdjusted() override {}
};

struct GTKeyswitchMomentary : GroupTrigger
{
    GTKeyswitchMomentary(GroupTriggerID id, GroupTriggerInstrumentState &onState,
                         GroupTriggerStorage &onStorage)
        : GroupTrigger(id, onState, onStorage)
    {
    }

    // Holds while my switch key is physically held down, which is what lets the group play
    bool conditionHolds(const Engine &e, const Group &g, int16_t ch, int16_t midiKey) const override
    {
        auto vsKey = (int)std::round(storage.args[0]);
        if (ch < 0 || ch >= 16 || vsKey < 0 || vsKey >= 128)
            return true;

        SCLOG_IF(groupTrigggers, "Evaluating momentary at "
                                     << ch << " " << vsKey << " "
                                     << e.voiceManager.heldMIDIKeyByChannel[ch][vsKey]);
        return e.voiceManager.heldMIDIKeyByChannel[ch][vsKey];
    }

    void storageAdjusted() override {}
};

/*
 * The round robin trigger does no work at all. Which slot of a set is live is a question about all
 * of the set's groups at once, so the part answers it once per note in advanceRoundRobinSets and
 * this just reads the answer - which keeps it const, branch free and allocation free.
 *
 * All three kinds share a class; the kind only picks which space of sets to look in and which
 * field of the answer to read.
 */
struct GTRoundRobin : GroupTrigger
{
    int kind{0}, set{0}, ordinal{1};
    bool isCycle{true};
    GTRoundRobin(GroupTriggerID id, GroupTriggerInstrumentState &onState,
                 GroupTriggerStorage &onStorage)
        : GroupTrigger(id, onState, onStorage)
    {
    }

    bool conditionHolds(const Engine &, const Group &g, int16_t, int16_t) const override
    {
        const auto &st = state.roundRobin[kind][set];
        return isCycle ? (st.ordinal == ordinal) : (st.winner == g.id);
    }

    void storageAdjusted() override
    {
        kind = std::max(roundRobinKindIndex(id), 0);
        isCycle = (id == GroupTriggerID::ROUND_ROBIN_CYCLE);
        set = std::clamp((int)std::round(storage.args[0]), 0, (int)maxRoundRobinSets - 1);
        ordinal = std::clamp((int)std::round(storage.args[1]), 1, (int)maxRoundRobinOrdinal);
    }
};

GroupTrigger *makeGroupTrigger(GroupTriggerID id, GroupTriggerInstrumentState &gis,
                               GroupTriggerStorage &st, GroupTriggerBuffer &bf)
{
    if (id >= GroupTriggerID::MACRO && (int)id < (int)GroupTriggerID::MACRO + scxt::macrosPerPart)
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
        CS(GroupTriggerID::KEYSWITCH_LATCH, GTKeyswitchLatch);
        CS(GroupTriggerID::KEYSWITCH_MOMENTARY, GTKeyswitchMomentary);
        CS(GroupTriggerID::PROGRAM_CHANGE, GTProgramChange);
        CS(GroupTriggerID::PITCH_BEND, GTPitchBend);
        CS(GroupTriggerID::ROUND_ROBIN_CYCLE, GTRoundRobin);
        CS(GroupTriggerID::ROUND_ROBIN_RANDOM, GTRoundRobin);
        CS(GroupTriggerID::ROUND_ROBIN_SHUFFLE, GTRoundRobin);
    default:
        return nullptr;
    }
    SCLOG_IF(groupTrigggers, "Unable to create group trigger for id " << (int)id);
    return nullptr;
}

std::string getGroupTriggerDisplayName(GroupTriggerID id)
{
    if (id >= GroupTriggerID::MACRO && (int)id < (int)GroupTriggerID::MACRO + scxt::macrosPerPart)
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
    case GroupTriggerID::KEYSWITCH_LATCH:
        return "KSWITCH";
    case GroupTriggerID::KEYSWITCH_MOMENTARY:
        return "KSW/MOM";
    case GroupTriggerID::PROGRAM_CHANGE:
        return "PROGRAM";
    case GroupTriggerID::PITCH_BEND:
        return "PBEND";
    case GroupTriggerID::ROUND_ROBIN_CYCLE:
        return "RR/CYC";
    case GroupTriggerID::ROUND_ROBIN_RANDOM:
        return "RR/RND";
    case GroupTriggerID::ROUND_ROBIN_SHUFFLE:
        return "RR/SHF";
    default:
    {
        SCLOG_IF(groupTrigggers, "Un-named group trigger id=" << (int)id);
        return "ERROR";
    }
    }
    return "ERROR";
}

void GroupTriggerConditions::setupOnUnstream(GroupTriggerInstrumentState &gis)
{
    bool allNone{true};
    containsKeySwitchLatch = false;
    roundRobinKind = GroupTriggerID::NONE;
    roundRobinSet = 0;
    roundRobinOrdinal = 1;
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
            if (s.id == GroupTriggerID::KEYSWITCH_LATCH)
                containsKeySwitchLatch = true;

            // First active round robin row wins; a group in two cycles at once means nothing
            if (active[i] && isRoundRobinTriggerID(s.id) && !inRoundRobin())
            {
                roundRobinKind = s.id;
                roundRobinSet =
                    (int16_t)std::clamp((int)std::round(s.args[0]), 0, (int)maxRoundRobinSets - 1);
                roundRobinOrdinal =
                    (int16_t)std::clamp((int)std::round(s.args[1]), 1, (int)maxRoundRobinOrdinal);
            }

            if (!conditions[i] || conditions[i]->getID() != s.id)
            {
                conditions[i] = makeGroupTrigger(s.id, gis, s, conditionBuffers[i]);
                assert(conditions[i]);
            }
        }
        if (conditions[i])
            conditions[i]->storageAdjusted();
    }
    alwaysReturnsTrue = allNone;
}

bool GroupTriggerConditions::evaluate(const Engine &e, const Group &g, int16_t channel,
                                      int16_t midiKey, bool skipRoundRobin) const
{
    if (alwaysReturnsTrue)
        return true;

    auto v = true;
    for (int i = 0; i < triggerConditionsPerGroup; ++i)
    {
        // FIXME - conjunctions
        if (active[i] && conditions[i])
        {
            if (skipRoundRobin && isRoundRobinTriggerID(conditions[i]->getID()))
                continue;
            v = v & conditions[i]->groupShouldPlay(e, g, channel, midiKey);
        }
    }
    return v;
}

bool GroupTriggerConditions::groupShouldPlay(const Engine &e, const Group &g, int16_t channel,
                                             int16_t midiKey) const
{
    return evaluate(e, g, channel, midiKey, false);
}

bool GroupTriggerConditions::groupShouldPlayIgnoringRoundRobin(const Engine &e, const Group &g,
                                                               int16_t channel,
                                                               int16_t midiKey) const
{
    return evaluate(e, g, channel, midiKey, true);
}

bool GroupTriggerConditions::isKeySwitchKey(int16_t midiKey) const
{
    for (int i = 0; i < triggerConditionsPerGroup; ++i)
    {
        const auto &s = storage[i];
        if (active[i] &&
            (s.id == GroupTriggerID::KEYSWITCH_LATCH ||
             s.id == GroupTriggerID::KEYSWITCH_MOMENTARY) &&
            (int)std::round(s.args[0]) == midiKey)
        {
            return true;
        }
    }
    return false;
}

bool GroupTriggerConditions::isKeySwitchLatchKey(int16_t midiKey) const
{
    for (int i = 0; i < triggerConditionsPerGroup; ++i)
    {
        const auto &s = storage[i];
        if (active[i] && s.id == GroupTriggerID::KEYSWITCH_LATCH &&
            (int)std::round(s.args[0]) == midiKey)
        {
            return true;
        }
    }
    return false;
}

int16_t GroupTriggerConditions::firstKeySwitchLatchKey() const
{
    for (int i = 0; i < triggerConditionsPerGroup; ++i)
    {
        if (active[i] && storage[i].id == GroupTriggerID::KEYSWITCH_LATCH)
            return (int16_t)std::round(storage[i].args[0]);
    }
    return -1;
}

bool GroupTriggerConditions::keySwitchLatchHolds(const Engine &e, const Group &g, int16_t channel,
                                                 int16_t midiKey) const
{
    for (int i = 0; i < triggerConditionsPerGroup; ++i)
    {
        if (active[i] && conditions[i] &&
            conditions[i]->getID() == GroupTriggerID::KEYSWITCH_LATCH &&
            conditions[i]->conditionHolds(e, g, channel, midiKey))
        {
            return true;
        }
    }
    return false;
}

} // namespace scxt::engine