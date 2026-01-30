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

#ifndef SCXT_SRC_SCXT_CORE_ENGINE_GROUP_TRIGGERS_H
#define SCXT_SRC_SCXT_CORE_ENGINE_GROUP_TRIGGERS_H

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

    KEYSWITCH_LATCH,
    KEYSWITCH_MOMENTARY,

    // Leave these at the end please
    MACRO,
    MIDICC = MACRO + scxt::macrosPerPart,
    LAST_MIDICC = MIDICC + 128 // Leave this at the end please
};

std::string toStringGroupTriggerID(const GroupTriggerID &p);
GroupTriggerID fromStringGroupTriggerID(const std::string &p);

struct GroupTriggerStorage
{
    GroupTriggerID id{GroupTriggerID::NONE};
    using argInterface_t = float;

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
    GroupTriggerID id;
    GroupTrigger(GroupTriggerID id, GroupTriggerInstrumentState &onState,
                 GroupTriggerStorage &onStorage)
        : state(onState), storage(onStorage), id(id)
    {
    }
    GroupTriggerID getID() const { return id; }
    virtual ~GroupTrigger() = default;
    virtual bool groupShouldPlay(const Engine &, const Group &, int16_t channel,
                                 int16_t key) const = 0;
    virtual void storageAdjusted() = 0;

    /*
     * At one point I had considered making these self describing but our
     * trigger types are small enough I just went with an int32 as each data
     * element and then set up based on type in the UI in GroupTriggersCard.cpp,
     * since I don't think adding trigger types will be anything like adding
     * modulator or processor types in cadence. If I'm wrong add something like
     * this again.
     */
    // virtual datamodel::pmd argMetadata(int argNo) const = 0;
};

// FIXME - make this sized more intelligently
using GroupTriggerBuffer = uint8_t[2048];

GroupTrigger *makeGroupTrigger(GroupTriggerID, GroupTriggerInstrumentState &, GroupTriggerStorage &,
                               GroupTriggerBuffer &);
std::string getGroupTriggerDisplayName(GroupTriggerID);

struct GroupTriggerConditions
{
    GroupTriggerConditions()
    {
        std::fill(conditions.begin(), conditions.end(), nullptr);
        std::fill(active.begin(), active.end(), true);
    }
    std::array<GroupTriggerStorage, scxt::triggerConditionsPerGroup> storage{};
    std::array<bool, scxt::triggerConditionsPerGroup> active{};

    bool alwaysReturnsTrue{true};
    bool containsKeySwitchLatch{false};

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

    bool groupShouldPlay(const Engine &, const Group &, int16_t channel, int16_t key) const;
    bool wasPlaySupressedByKeySwitch(const Engine &, const Group &, int16_t channel,
                                     int16_t key) const;

  protected:
    std::array<GroupTriggerBuffer, scxt::triggerConditionsPerGroup> conditionBuffers;
    std::array<GroupTrigger *, scxt::triggerConditionsPerGroup> conditions{};
};

} // namespace scxt::engine
#endif // GROUP_TRIGGERS_H
