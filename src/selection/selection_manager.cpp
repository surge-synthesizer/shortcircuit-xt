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

#include "selection_manager.h"
#include <iostream>
#include "engine/engine.h"
#include "messaging/messaging.h"
#include "messaging/client/client_serial.h"

namespace scxt::selection
{
void SelectionManager::singleSelect(const ZoneAddress &a)
{
    auto [p, g, z] = a;
    if (z >= 0 && g >= 0 && p >= 0)
    {
        singleSelection = a;
        // TODO: The 'full zone' becomes a single function obviously
        const auto &zp = engine.getPatch()->getPart(p)->getGroup(g)->getZone(z);
        serializationSendToClient(
            messaging::client::s2c_respond_zone_adsr_view,
            messaging::client::AdsrSelectedZoneView::s2c_payload_t{0, true, zp->aegStorage},
            *(engine.getMessageController()));
        serializationSendToClient(
            messaging::client::s2c_respond_zone_adsr_view,
            messaging::client::AdsrSelectedZoneView::s2c_payload_t{1, true, zp->eg2Storage},
            *(engine.getMessageController()));
    }
    else
    {
        singleSelection = {};
        serializationSendToClient(
            messaging::client::s2c_respond_zone_adsr_view,
            messaging::client::AdsrSelectedZoneView::s2c_payload_t{0, false, {}},
            *(engine.getMessageController()));
        serializationSendToClient(
            messaging::client::s2c_respond_zone_adsr_view,
            messaging::client::AdsrSelectedZoneView::s2c_payload_t{1, false, {}},
            *(engine.getMessageController()));
    }
}
} // namespace scxt::selection