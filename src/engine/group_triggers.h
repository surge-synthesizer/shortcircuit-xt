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

#ifndef SCXT_SRC_ENGINE_GROUP_TRIGGERS_H
#define SCXT_SRC_ENGINE_GROUP_TRIGGERS_H

#include <memory>
#include <array>
#include <variant>

#include "configuration.h"
#include "utils.h"

#include "datamodel/metadata.h"

namespace scxt::engine
{

struct Engine;
struct Part;
struct Group;

/**
 * Maintain the state of keys ccs etc... for a particular instrument
 */
struct GroupTriggerInstrumentState
{
};

enum struct GroupTriggerID : int32_t
{
    NONE,

    MACRO,
    MIDICC // if MIDICC is no longer last, adjust fromStringGfroupTRiggerID iteration
};

std::string toStringGroupTriggerID(const GroupTriggerID &p);
GroupTriggerID fromStringGroupTriggerID(const std::string &p);

struct GroupTriggerStorage
{
    GroupTriggerID id{GroupTriggerID::NONE};
    using argInterface_t = int32_t; // for now and just use ms for miliseconds and scale

    static constexpr int32_t numArgs{2};
    std::array<argInterface_t, numArgs> args{0, 0};
};
/**
 * Calculate if a group is triggered
 */
struct GroupTrigger
{
    GroupTriggerInstrumentState &state;
    GroupTriggerStorage &storage;
    GroupTrigger(GroupTriggerInstrumentState &onState, GroupTriggerStorage &onStorage)
        : state(onState), storage(onStorage)
    {
    }
    virtual ~GroupTrigger() = default;
    virtual bool value(const std::unique_ptr<Engine> &, const std::unique_ptr<Group> &) const = 0;
    virtual datamodel::pmd argMetadata(int argNo) const = 0;
};

std::unique_ptr<GroupTrigger> makeGroupTrigger(GroupTriggerID, GroupTriggerInstrumentState &,
                                               GroupTriggerStorage &);
std::string getGroupTriggerDisplayName(GroupTriggerID);

struct GroupTriggerConditions
{
    std::array<GroupTriggerStorage, scxt::triggerConditionsPerGroup> storage;
    std::array<bool, scxt::triggerConditionsPerGroup> active;

    enum struct Conjunction : int32_t
    {
        AND,
        OR,
        AND_NOT,
        OR_NOT // add one to end or change position update the from in fromString
    };

    static std::string toStringGroupConditionsConjunction(const Conjunction &p);
    static Conjunction fromStringConditionsConjunction(const std::string &p);

    std::array<Conjunction, scxt::triggerConditionsPerGroup - 1> conjunctions;

    void setupOnUnstream(GroupTriggerInstrumentState &);
    std::array<std::unique_ptr<GroupTrigger>, scxt::triggerConditionsPerGroup> conditions;
};

} // namespace scxt::engine
#endif // GROUP_TRIGGERS_H
