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

enum struct GroupTriggerID : int32_t
{
    NONE,

    KEYSWITCH_LATCH,
    KEYSWITCH_MOMENTARY,

    PROGRAM_CHANGE,
    PITCH_BEND,

    ROUND_ROBIN_CYCLE,
    ROUND_ROBIN_RANDOM,
    ROUND_ROBIN_SHUFFLE,

    // Leave these at the end please
    MACRO,
    MIDICC = MACRO + scxt::macrosPerPart,
    LAST_MIDICC = MIDICC + 128 // Leave this at the end please
};

inline bool isRoundRobinTriggerID(GroupTriggerID id)
{
    return id == GroupTriggerID::ROUND_ROBIN_CYCLE || id == GroupTriggerID::ROUND_ROBIN_RANDOM ||
           id == GroupTriggerID::ROUND_ROBIN_SHUFFLE;
}

/*
 * Each kind gets its own space of sets - cycle 1 (RR1), random 1 (RN1) and shuffle 1 (SH1) are
 * three unrelated round robins, not one set three groups disagree about. -1 for anything else.
 */
static constexpr int numRoundRobinKinds{3};
inline int roundRobinKindIndex(GroupTriggerID id)
{
    switch (id)
    {
    case GroupTriggerID::ROUND_ROBIN_CYCLE:
        return 0;
    case GroupTriggerID::ROUND_ROBIN_RANDOM:
        return 1;
    case GroupTriggerID::ROUND_ROBIN_SHUFFLE:
        return 2;
    default:
        return -1;
    }
}

// Which sets a note lands in: one bit per set, one mask per kind
using roundRobinMask_t = std::array<uint32_t, numRoundRobinKinds>;

std::string toStringGroupTriggerID(const GroupTriggerID &p);
GroupTriggerID fromStringGroupTriggerID(const std::string &p);

/*
 * When a group makes its voices. ON_NOTE_ON is what everything has always done. ON_NOTE_OFF
 * makes the press do nothing but be remembered, and sounds the group when the key comes back
 * up, playing it at the velocity it was pressed with. More modes (sustain pedal release and
 * friends) are coming, hence an enum rather than a bool - see issue #2186.
 */
enum struct VoiceCreationMode : int32_t
{
    ON_NOTE_ON = 0,
    ON_NOTE_OFF
};

std::string toStringVoiceCreationMode(const VoiceCreationMode &p);
VoiceCreationMode fromStringVoiceCreationMode(const std::string &p);

/**
 * Maintain the state of keys ccs etc... for a particular instrument
 */
struct GroupTriggerInstrumentState
{
    /*
     * Where each round robin set has got to, indexed [kind][set]. Runtime state, deliberately not
     * streamed - which press of the cycle a performance happens to be on is not part of the
     * instrument.
     */
    struct RoundRobinSetState
    {
        int32_t ordinal{0}; // CYCLE: the live ordinal. Ordinals start at 1, so 0 is "not yet"
        GroupID winner{};   // RANDOM / SHUFFLE: who won this note. The default id is -1, nobody
        uint32_t drawn{0};  // SHUFFLE: which member slots this pass has already used
    };
    std::array<std::array<RoundRobinSetState, scxt::maxRoundRobinSets>, numRoundRobinKinds>
        roundRobin{};

    void resetRoundRobin()
    {
        for (auto &k : roundRobin)
            k.fill(RoundRobinSetState());
    }
};

/*
 * How a client should draw a key: not a switch at all, a switch that isn't the selected
 * articulation, or the live one. Sent as int32 so the payload needs no enum streaming.
 */
enum struct KeySwitchDisplayState : int32_t
{
    NOT_A_SWITCH = 0,
    INACTIVE,
    ACTIVE
};
using partKeySwitchDisplay_t = std::array<int32_t, 128>;

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

    /*
     * Two separate questions. conditionHolds is "is this condition true right now" - is the
     * macro in range, is this the key I watch. groupShouldPlay is what findZone asks. They
     * differ for the keyswitch latch, which consumes its switch key rather than sounding on it.
     */
    virtual bool conditionHolds(const Engine &, const Group &, int16_t channel,
                                int16_t midiKey) const = 0;

    // true when a holding condition suppresses play rather than enabling it
    virtual bool holdingSuppressesPlay() const { return false; }

    bool groupShouldPlay(const Engine &e, const Group &g, int16_t channel, int16_t midiKey) const
    {
        auto h = conditionHolds(e, g, channel, midiKey);
        return holdingSuppressesPlay() ? !h : h;
    }

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

    /*
     * Not a condition - the conditions decide whether the group sounds, this decides which
     * note event asks them. It lives here because it is the same question ("when does this
     * group trigger") and it shares the client's trigger payload and undo entry.
     */
    VoiceCreationMode voiceCreationMode{VoiceCreationMode::ON_NOTE_ON};
    bool createsVoicesOnRelease() const
    {
        return voiceCreationMode == VoiceCreationMode::ON_NOTE_OFF;
    }

    bool alwaysReturnsTrue{true};
    bool containsKeySwitchLatch{false};

    /*
     * The round robin row, cached from storage. A group belongs to at most one set - the UI stops
     * you adding a second and this keeps the first - since being in two cycles at once is
     * meaningless. NONE means this group is in no round robin at all.
     */
    GroupTriggerID roundRobinKind{GroupTriggerID::NONE};
    int16_t roundRobinSet{0};
    int16_t roundRobinOrdinal{1};
    bool inRoundRobin() const { return roundRobinKind != GroupTriggerID::NONE; }

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

    /*
     * midiKey is the key as the MIDI stream delivered it, before any tuning remap. Conditions
     * are a performance gesture, not a pitch, so they must not move with MTS-ESP or transposition.
     */
    bool groupShouldPlay(const Engine &, const Group &, int16_t channel, int16_t midiKey) const;
    bool keySwitchLatchHolds(const Engine &, const Group &, int16_t channel, int16_t midiKey) const;

    /*
     * Everything except the round robin. The round robin has to know whether a note would have
     * played this group before it decides whether to spend a slot on it, and that question has to
     * be answered before the round robin itself is evaluated.
     */
    bool groupShouldPlayIgnoringRoundRobin(const Engine &, const Group &, int16_t channel,
                                           int16_t midiKey) const;

    /*
     * Storage-only queries. The client keeps a copy of this struct whose conditions are never
     * built, so anything the UI asks has to be answerable from storage and active alone.
     */
    bool isKeySwitchKey(int16_t midiKey) const;
    bool isKeySwitchLatchKey(int16_t midiKey) const;
    int16_t firstKeySwitchLatchKey() const; // -1 if this group has no latch

  protected:
    bool evaluate(const Engine &, const Group &, int16_t channel, int16_t midiKey,
                  bool skipRoundRobin) const;

    std::array<GroupTriggerBuffer, scxt::triggerConditionsPerGroup> conditionBuffers;
    std::array<GroupTrigger *, scxt::triggerConditionsPerGroup> conditions{};
};

} // namespace scxt::engine
#endif // GROUP_TRIGGERS_H
