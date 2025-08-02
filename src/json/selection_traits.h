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

#ifndef SCXT_SRC_JSON_SELECTION_TRAITS_H
#define SCXT_SRC_JSON_SELECTION_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
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

SC_STREAMDEF(selection::SelectionManager, SC_FROM({
                 auto &e = from;
                 v = {{"zones", e.allSelectedZones},
                      {"leadZone", e.leadZone},
                      {"groups", e.allSelectedGroups},
                      {"tabs", e.otherTabSelection},
                      {"selectedPart", e.selectedPart}};
             }),
             SC_TO({
                 auto &z = to;
                 if (SC_UNSTREAMING_FROM_PRIOR_TO(0x2024'07'24))
                 {
                     z.selectedPart = 0;
                     findIf(v, "tabs", z.otherTabSelection);
                     findIf(v, "zones", z.allSelectedZones[0]);
                     findIf(v, "groups", z.allSelectedGroups[0]);
                     findIf(v, "leadZone", z.leadZone[0]);
                 }
                 else
                 {
                     findIf(v, "tabs", z.otherTabSelection);
                     findIf(v, "zones", z.allSelectedZones);
                     findIf(v, "groups", z.allSelectedGroups);
                     findIf(v, "leadZone", z.leadZone);
                     findIf(v, "selectedPart", z.selectedPart);
                 }

                 selection::SelectionManager::ZoneAddress za;

                 if (!z.allSelectedZones[z.selectedPart].empty())
                 {
                     selection::SelectionManager::SelectActionContents sac{
                         z.leadZone[z.selectedPart]};
                     sac.distinct = false;
                     z.selectAction(sac);
                 }
             }));
} // namespace scxt::json

#endif // SHORTCIRCUIT_SELECTION_TRAITS_H
