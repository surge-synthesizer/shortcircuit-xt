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

#ifndef SCXT_SRC_SCXT_CORE_JSON_SELECTION_TRAITS_H
#define SCXT_SRC_SCXT_CORE_JSON_SELECTION_TRAITS_H

#include <tao/json/contrib/traits.hpp>

#include "stream.h"
#include "extensions.h"

#include "selection/selection_manager.h"

#include "scxt_traits.h"

namespace scxt::json
{

SC_STREAMDEF(scxt::selection::SelectionManager::SelectActionContents, SC_FROM({
                 auto &e = from;
                 assert(SC_STREAMING_FOR_IN_PROCESS);
                 v = {{"p", e.part},      {"g", e.group},    {"z", e.zone},
                      {"s", e.selecting}, {"d", e.distinct}, {"sl", e.selectingAsLead},
                      {"fz", e.forZone}};
             }),
             SC_TO({
                 auto &z = to;
                 findIf(v, "p", z.part);
                 findIf(v, "g", z.group);
                 findIf(v, "z", z.zone);
                 findIf(v, "s", z.selecting);
                 findIf(v, "d", z.distinct);
                 findIf(v, "sl", z.selectingAsLead);
                 findIf(v, "fz", z.forZone);
             }));

SC_STREAMDEF(scxt::selection::SelectionManager::ZoneAddress, SC_FROM({
                 if (t.part == -1 && t.group == -1 && t.zone == -1)
                 {
                     v = tao::json::empty_object;
                 }
                 else
                 {
                     v = {{"p", t.part}, {"g", t.group}, {"z", t.zone}};
                 }
             }),
             SC_TO({
                 findOrSet(v, {"p", "part"}, -1, result.part);
                 findOrSet(v, {"g", "group"}, -1, result.group);
                 findOrSet(v, {"z", "zone"}, -1, result.zone);
             }));

SC_STREAMDEF(selection::SelectionManager::PerPartState, SC_FROM({
                 auto &e = from;
                 v = {{"zones", e.selectedZones},   {"groups", e.selectedGroups},
                      {"dgroups", e.displayGroups}, {"leadZone", e.leadZone},
                      {"leadGroup", e.leadGroup},   {"collapsedGroups", e.collapsedGroups}};
             }),
             SC_TO({
                 auto &z = to;
                 findIf(v, "zones", z.selectedZones);
                 findIf(v, "groups", z.selectedGroups);
                 findIf(v, "dgroups", z.displayGroups);
                 findIf(v, "leadZone", z.leadZone);
                 findIf(v, "leadGroup", z.leadGroup);
                 findIf(v, "collapsedGroups", z.collapsedGroups);
             }));

SC_STREAMDEF(
    selection::SelectionManager, SC_FROM({
        auto &e = from;
        v = {{"state", e.state}, {"tabs", e.otherTabSelection}, {"selectedPart", e.selectedPart}};
    }),
    SC_TO({
        auto &z = to;
        if (SC_UNSTREAMING_FROM_PRIOR_TO(0x2024'07'24))
        {
            z.selectedPart = 0;
            findIf(v, "tabs", z.otherTabSelection);
            findIf(v, "zones", z.state[0].selectedZones);
            findIf(v, "groups", z.state[0].selectedGroups);
            findIf(v, "leadZone", z.state[0].leadZone);
        }
        else if (SC_UNSTREAMING_FROM_PRIOR_TO(0x2026'05'29))
        {
            // Legacy array-of-arrays format (stable 0x2024'07'24 ..
            // 0x2026'05'21): read each parallel array, then scatter by
            // part index into z.state[p].field. leadGroup was never
            // streamed in this era, so it stays default.
            findIf(v, "tabs", z.otherTabSelection);
            findIf(v, "selectedPart", z.selectedPart);

            std::array<selection::SelectionManager::selectedZones_t, scxt::numParts> tmpZ, tmpG,
                tmpDG;
            std::array<selection::SelectionManager::ZoneAddress, scxt::numParts> tmpLZ;
            std::array<selection::SelectionManager::collapsedGroupSet_t, scxt::numParts> tmpCG;

            findIf(v, "zones", tmpZ);
            findIf(v, "groups", tmpG);
            findIf(v, "dgroups", tmpDG);
            findIf(v, "leadZone", tmpLZ);
            findIf(v, "collapsedGroups", tmpCG);

            for (int p = 0; p < scxt::numParts; ++p)
            {
                z.state[p].selectedZones = std::move(tmpZ[p]);
                z.state[p].selectedGroups = std::move(tmpG[p]);
                z.state[p].displayGroups = std::move(tmpDG[p]);
                z.state[p].leadZone = tmpLZ[p];
                z.state[p].collapsedGroups = std::move(tmpCG[p]);
            }
        }
        else
        {
            findIf(v, "tabs", z.otherTabSelection);
            findIf(v, "selectedPart", z.selectedPart);
            findIf(v, "state", z.state);
        }

        if (!z.state[z.selectedPart].selectedZones.empty())
        {
            selection::SelectionManager::SelectActionContents sac{z.state[z.selectedPart].leadZone};
            sac.distinct = false;
            sac.selectingAsLead = true;
            sac.selecting = true;
            z.applySelectActions({sac});
        }
    }));
} // namespace scxt::json

#endif // SHORTCIRCUIT_SELECTION_TRAITS_H
