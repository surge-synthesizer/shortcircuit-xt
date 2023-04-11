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

        bool operator==(const ZoneAddress &other) const
        {
            return part == other.part && group == other.group && zone == other.zone;
        }

        bool isIn(const engine::Engine &e);

        friend std::ostream &operator<<(std::ostream &os, const ZoneAddress &z)
        {
            os << "zoneaddr[p=" << z.part << ",g=" << z.group << ",z=" << z.zone << "]";
            return os;
        }
        struct Hash
        {
            size_t operator()(const ZoneAddress &z) const
            {
                int64_t value = z.part + (z.group << 6) + (z.zone << (6 + 10));
                return std::hash<int64_t>()(value);
            }
        };
    };

    struct SelectActionContents
    {
        SelectActionContents() {}
        SelectActionContents(int32_t p, int32_t g, int32_t z) : part(p), group(g), zone(z) {}
        SelectActionContents(int32_t p, int32_t g, int32_t z, bool s, bool d)
            : part(p), group(g), zone(z), selecting(s), distinct(d)
        {
        }
        SelectActionContents(const ZoneAddress &z)
            : part(z.part), group(z.group), zone(z.zone), selecting(true), distinct(true)
        {
        }
        int32_t part{-1};
        int32_t group{-1};
        int32_t zone{-1};
        bool selecting{true}; // am i selecting (T) or deselecting (F) this zone
        bool distinct{true};  // Is this a single selection or a multi-selection gesture

        friend std::ostream &operator<<(std::ostream &os, const SelectActionContents &z)
        {
            os << "select[p=" << z.part << ",g=" << z.group << ",z=" << z.zone
               << ",sel=" << z.selecting << "dis=" << z.distinct << "]";
            return os;
        }
    };

    void selectAction(const SelectActionContents &z)
    {
        SCDBGCOUT << __func__ << " Implement " << SCD(z) << std::endl;
        if (true || (z.distinct && z.selecting))
        {
            // HACK
            singleSelect({z.part, z.group, z.zone});
        }
    }

    int selectedPart{-1};
    typedef std::unordered_set<ZoneAddress, ZoneAddress::Hash> selectedZones_t;
    selectedZones_t currentlySelectedZones() { return allSelectedZones; }
    std::optional<ZoneAddress> currentLeadZone(const engine::Engine &e)
    {
        if (leadZone.isIn(e))
            return leadZone;
        return {};
    }
    void sendSelectionDataToClient(const messaging::MessageController &);

  private:
    void singleSelect(const ZoneAddress &z);

  public:
    std::unordered_map<std::string, std::string> otherTabSelection;
    selectedZones_t allSelectedZones;
    ZoneAddress leadZone;

  protected:
    MainSelection mainSelection{MULTI};
    std::map<size_t, std::set<size_t>> selectedGroupByPart;

  private:
    std::optional<ZoneAddress> singleSelection;
};
} // namespace scxt::selection

#endif // SHORTCIRCUIT_SELECTIONMANAGER_H
