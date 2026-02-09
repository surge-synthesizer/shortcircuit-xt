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

#ifndef SCXT_SRC_SCXT_CORE_SELECTION_SELECTION_MANAGER_H
#define SCXT_SRC_SCXT_CORE_SELECTION_SELECTION_MANAGER_H

#include <optional>
#include <vector>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "utils.h"
#include "dsp/processor/processor.h"

namespace scxt::engine
{
struct Engine;
}
namespace scxt::messaging
{
struct MessageController;
}
namespace scxt::selection
{
struct SelectionManager
{
    engine::Engine &engine;
    SelectionManager(engine::Engine &e) : engine(e) {}

    enum MainSelection
    {
        MULTI,
        PART
    };

    struct ZoneAddress
    {
        ZoneAddress() {}
        ZoneAddress(int32_t p, int32_t g, int32_t z) : part(p), group(g), zone(z) {}
        int32_t part{-1};
        int32_t group{-1};
        int32_t zone{-1};

        bool operator!=(const ZoneAddress &other) const { return !(*this == other); }
        bool operator==(const ZoneAddress &other) const
        {
            return part == other.part && group == other.group && zone == other.zone;
        }

        bool operator<(const ZoneAddress &other) const
        {
            if (part != other.part)
                return part < other.part;
            if (group != other.group)
                return group < other.group;
            if (zone != other.zone)
                return zone < other.zone;
            return false;
        }

        // PGZ is in the engine
        bool isIn(const engine::Engine &e) const;
        // PG is in the engine, Z is anything
        bool isInPartGroup(const engine::Engine &e) const;
        // PGZ is in the engine or G and-or Z are -1
        bool isInWithPartials(const engine::Engine &e) const;

        friend std::ostream &operator<<(std::ostream &os, const ZoneAddress &z)
        {
            os << "zoneaddr[p=" << z.part << ",g=" << z.group << ",z=" << z.zone << "]";
            return os;
        }
        struct Hash
        {
            size_t operator()(const ZoneAddress &z) const
            {
                auto pn = z.part;
                auto gn = (z.group >= 0 ? z.group : (1 << 10) - 1);
                auto zn = (z.zone >= 0 ? z.zone : (1 << 10) - 1);
                int64_t value = pn + (gn << 6) + (zn << (6 + 10));
                return std::hash<int64_t>()(value);
            }
        };
    };

    struct SelectActionContents
    {
        SelectActionContents() {}
        SelectActionContents(int32_t p, int32_t g, int32_t z) : part(p), group(g), zone(z) {}
        SelectActionContents(int32_t p, int32_t g, int32_t z, bool s, bool d, bool ld)
            : part(p), group(g), zone(z), selecting(s), distinct(d), selectingAsLead(ld)
        {
        }
        SelectActionContents(const ZoneAddress &z)
            : part(z.part), group(z.group), zone(z.zone), selecting(true), distinct(true),
              selectingAsLead(true)
        {
        }
        SelectActionContents(const ZoneAddress &z, bool s, bool d, bool ld)
            : part(z.part), group(z.group), zone(z.zone), selecting(s), distinct(d),
              selectingAsLead(ld)
        {
        }
        int32_t part{-1};
        int32_t group{-1};
        int32_t zone{-1};
        bool selecting{true};       // am i selecting (T) or deselecting (F) this zone
        bool distinct{true};        // Is this a single selection or a multi-selection gesture
        bool selectingAsLead{true}; // Should I force this selection to be lead?
        bool forZone{true};         // does this target the zone selection set (T) or group (F)

        static SelectActionContents deselectSentinel() { return {-1, -1, -1, true, true, false}; }

        bool isDeselectSentinel() const
        {
            return part == -1 && group == -1 && zone == -1 && selecting && distinct &&
                   !selectingAsLead;
        }

        friend std::ostream &operator<<(std::ostream &os, const SelectActionContents &z)
        {
            os << "select[p=" << z.part << ",g=" << z.group << ",z=" << z.zone
               << ",sel=" << z.selecting << ",dis=" << z.distinct << ",ld=" << z.selectingAsLead
               << ",forZone=" << z.forZone << "]";
            return os;
        }

        operator ZoneAddress() const { return {part, group, zone}; }
        ZoneAddress addr() const { return {part, group, zone}; }
    };

    void applySelectActions(const std::vector<SelectActionContents> &v);
    void applySelectActions(const SelectActionContents &v)
    {
        applySelectActions(std::vector<SelectActionContents>{v});
    }
    void guaranteeConsistencyAfterDeletes(const engine::Engine &, bool zoneDeleted,
                                          const ZoneAddress &addressDeleted);
    void selectPart(int16_t part);
    void clearAllSelections();

  protected:
    enum ConsistencyCheck
    {
        PROCESSOR_TYPE,
        MATRIX_ROW,
        PROC_ROUTING,
        OUTPUT_ROUTING,
        LFO_SHAPE
    };
    bool acrossSelectionConsistency(bool forZone, ConsistencyCheck whichCheck, int index);

    std::vector<SelectActionContents>
    transformSelectionActions(const std::vector<SelectActionContents> &);
    void adjustInternalStateForAction(const SelectActionContents &);
    void guaranteeSelectedLead();
    void guaranteeSelectedLeadSomewhereIn(int part, int group, int zone);
    void debugDumpSelectionState();

  public:
    int16_t selectedPart{0};
    typedef std::unordered_set<ZoneAddress, ZoneAddress::Hash> selectedZones_t;
    selectedZones_t currentlySelectedZones() { return allSelectedZones[selectedPart]; }
    // This will have -1 for every zone of course
    selectedZones_t currentlySelectedGroups() { return allSelectedGroups[selectedPart]; }
    std::optional<ZoneAddress> currentLeadZone(const engine::Engine &e) const
    {
        if (leadZone[selectedPart].isIn(e))
            return leadZone[selectedPart];
        return std::nullopt;
    }
    std::optional<ZoneAddress> currentLeadGroup(const engine::Engine &e) const
    {
        if (leadGroup[selectedPart].isInPartGroup(e))
            return leadGroup[selectedPart];
        return std::nullopt;
    }
    std::pair<int, int> bestPartGroupForNewSample(const engine::Engine &e);

    int currentlySelectedPart(const engine::Engine &e) const { return selectedPart; }

    void sendSelectedZonesToClient();
    void sendGroupZoneMappingForSelectedPart();
    void sendClientDataForLeadSelectionState();
    void sendDisplayDataForZonesBasedOnLead(int part, int group, int zone);
    void sendDisplayDataForNoZoneSelected();
    void sendDisplayDataForGroupsBasedOnLead(int part, int group);
    void sendDisplayDataForNoGroupSelected();
    void sendSelectedPartMacrosToClient();
    void sendOtherTabsSelectionToClient();
    void configureAndSendZoneOrGroupModMatrixMetadata(int p, int g, int z);
    template <typename Config, typename Mat, typename RT>
    void configureMatrixInternal(bool forZone, Mat &m, RT &routingTable);

    void copyZoneOrGroupProcessorLeadToAll(bool forZone, int which);

  public:
    using otherTabSelection_t = std::unordered_map<std::string, std::string>;
    otherTabSelection_t otherTabSelection;
    std::array<selectedZones_t, scxt::numParts> allSelectedZones, allSelectedGroups,
        allDisplayGroups;
    std::array<ZoneAddress, scxt::numParts> leadZone, leadGroup;
};
} // namespace scxt::selection

#endif // SHORTCIRCUIT_SELECTIONMANAGER_H
