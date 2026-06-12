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

#include "payload_undoable_items.h"

#include "messaging/client/client_serial.h"
#include "messaging/client/detail/client_serial_impl.h"
#include "messaging/client/zone_messages.h"
#include "messaging/client/part_messages.h"

namespace scxt::undo
{

void refreshLeadDisplay(const engine::Engine &eng, bool forZone)
{
    if (forZone)
    {
        auto lz = eng.getSelectionManager()->currentLeadZone(eng);
        if (lz.has_value())
        {
            eng.getSelectionManager()->sendDisplayDataForZonesBasedOnLead(lz->part, lz->group,
                                                                          lz->zone);
        }
        auto pt = eng.getSelectionManager()->currentlySelectedPart(eng);
        messaging::client::serializationSendToClient(
            messaging::client::s2c_send_selected_group_zone_mapping_summary,
            eng.getPatch()->getPart(pt)->getZoneMappingSummary(), *(eng.getMessageController()));
    }
    else
    {
        auto lg = eng.getSelectionManager()->currentLeadGroup(eng);
        if (lg.has_value())
        {
            eng.getSelectionManager()->sendDisplayDataForGroupsBasedOnLead(lg->part, lg->group);
        }
    }
}

void BusEffectSpec::serialExtra(const engine::Engine &e, const std::vector<ZoneAddress> &sel,
                                int32_t)
{
    for (const auto &a : sel)
    {
        if (a.group >= 0)
            e.getPatch()
                ->busses.busByAddress((engine::BusAddress)a.group)
                .sendBusEffectInfoToClient(e, a.zone);
        else
            e.getPatch()->getPart(a.part)->sendBusEffectInfoToClient(e, a.zone);
    }
}

void BusSendSpec::serialExtra(const engine::Engine &e, const std::vector<ZoneAddress> &sel, int32_t)
{
    for (const auto &a : sel)
        e.getPatch()
            ->busses.busByAddress((engine::BusAddress)a.group)
            .sendBusSendStorageToClient(e);
}

void PartConfigSpec::serialExtra(const engine::Engine &e, const std::vector<ZoneAddress> &sel,
                                 int32_t)
{
    for (const auto &a : sel)
    {
        messaging::client::serializationSendToClient(
            messaging::client::s2c_send_part_configuration,
            messaging::client::partConfigurationPayload_t{
                (int16_t)a.part, e.getPatch()->getPart(a.part)->configuration},
            *(e.getMessageController()));
    }
}

void resendPGZStructure(const engine::Engine &e)
{
    messaging::client::serializationSendToClient(messaging::client::s2c_send_pgz_structure,
                                                 e.getPartGroupZoneStructure(),
                                                 *(e.getMessageController()));
}

} // namespace scxt::undo
