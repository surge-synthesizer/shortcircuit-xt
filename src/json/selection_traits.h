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

template <> struct scxt_traits<scxt::selection::SelectionManager::SelectActionContents>
{
    typedef scxt::selection::SelectionManager::SelectActionContents za_t;
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const za_t &e)
    {
        v = {{"part", e.part},
             {"group", e.group},
             {"zone", e.zone},
             {"selecting", e.selecting},
             {"distinct", e.distinct},
             {"selectingAsLead", e.selectingAsLead},
             {"forZone", e.forZone},
             {"isContiguous", e.isContiguous},
             {"contiguousFrom", e.contiguousFrom}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, za_t &z)
    {
        findIf(v, "part", z.part);
        findIf(v, "group", z.group);
        findIf(v, "zone", z.zone);
        findIf(v, "selecting", z.selecting);
        findIf(v, "distinct", z.distinct);
        findIf(v, "selectingAsLead", z.selectingAsLead);
        findIf(v, "forZone", z.forZone);
        findIf(v, "isContiguous", z.isContiguous);
        findIf(v, "contiguousFrom", z.contiguousFrom);
    }
};

template <> struct scxt_traits<scxt::selection::SelectionManager::ZoneAddress>
{
    typedef scxt::selection::SelectionManager::ZoneAddress za_t;
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const za_t &e)
    {
        v = {{"part", e.part}, {"group", e.group}, {"zone", e.zone}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, za_t &z)
    {
        findIf(v, "part", z.part);
        findIf(v, "group", z.group);
        findIf(v, "zone", z.zone);
    }
};

template <> struct scxt_traits<selection::SelectionManager>
{
    typedef selection::SelectionManager sm_t;
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const sm_t &e)
    {
        v = {{"zones", e.allSelectedZones},
             {"leadZone", e.leadZone},
             {"groups", e.allSelectedGroups},
             {"tabs", e.otherTabSelection},
             {"selectedPart", e.selectedPart}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, sm_t &z)
    {
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
            selection::SelectionManager::SelectActionContents sac{z.leadZone[z.selectedPart]};
            sac.distinct = false;
            z.selectAction(sac);
        }
    }
};
} // namespace scxt::json

#endif // SHORTCIRCUIT_SELECTION_TRAITS_H
