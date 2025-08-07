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

#ifndef SCXT_SRC_SELECTION_SELECTION_MANAGER_H
#define SCXT_SRC_SELECTION_SELECTION_MANAGER_H

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

        bool forZone{true}; // does this target the zone selection set (T) or group (F)

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
        return {};
    }
    std::optional<ZoneAddress> currentLeadGroup(const engine::Engine &e) const
    {
        if (leadGroup[selectedPart].isInWithPartials(e))
            return leadGroup[selectedPart];
        return {};
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
    void configureAndSendZoneModMatrixMetadata(int p, int g, int z);

    // To ponder. Does this belong on this object or the engine?
    void copyZoneProcessorLeadToAll(int which);

  public:
    using otherTabSelection_t = std::unordered_map<std::string, std::string>;
    otherTabSelection_t otherTabSelection;
    std::array<selectedZones_t, scxt::numParts> allSelectedZones, allSelectedGroups;
    std::array<ZoneAddress, scxt::numParts> leadZone, leadGroup;
};
} // namespace scxt::selection

#endif // SHORTCIRCUIT_SELECTIONMANAGER_H
