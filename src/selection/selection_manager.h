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
#include <string>
#include <unordered_map>

namespace scxt::engine
{
struct Engine;
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
    };

    std::optional<ZoneAddress> getSelectedZone() const { return singleSelection; }
    void singleSelect(const ZoneAddress &z);

    std::unordered_map<std::string, std::string> otherTabSelection;

  protected:
    MainSelection mainSelection{MULTI};
    std::vector<ZoneAddress> allSelectedZones;
    ZoneAddress leadZone;
    size_t selectedPart{0};
    std::map<size_t, std::vector<size_t>> selectedGroupByPart;

  private:
    std::optional<ZoneAddress> singleSelection;
};
} // namespace scxt::selection

#endif // SHORTCIRCUIT_SELECTIONMANAGER_H
