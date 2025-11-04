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

#ifndef SCXT_SRC_MESSAGING_CLIENT_MISSING_RESOLUTION_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_MISSING_RESOLUTION_MESSAGES_H

#include "messaging/client/client_serial.h"
#include "messaging/client/detail/client_json_details.h"
#include "json/engine_traits.h"
#include "json/datamodel_traits.h"
#include "selection/selection_manager.h"
#include "engine/missing_resolution.h"
#include "client_macros.h"

namespace scxt::messaging::client
{
SERIAL_TO_CLIENT(SendMissingResolutionWorkItemList, s2c_send_missing_resolution_workitem_list,
                 std::vector<engine::MissingResolutionWorkItem>, onMissingResolutionWorkItemList);

using resolveSamplePayload_t = std::tuple<engine::MissingResolutionWorkItem, std::string>;
inline void doResolveSample(const resolveSamplePayload_t &payload, engine::Engine &e,
                            messaging::MessageController &cont)
{
    auto mwi = std::get<0>(payload);
    auto p = fs::path(fs::u8path(std::get<1>(payload)));

    if (mwi.isMultiUsed)
    {
        engine::resolveMultiFileMissingWorkItem(e, mwi, p);
    }
    else
    {
        engine::resolveSingleFileMissingWorkItem(e, mwi, p);
    }
    e.getSampleManager()->purgeUnreferencedSamples();
    e.sendFullRefreshToClient();
}
CLIENT_TO_SERIAL(ResolveSample, c2s_resolve_sample, resolveSamplePayload_t,
                 doResolveSample(payload, engine, cont));

} // namespace scxt::messaging::client
#endif // MISSING_RESOLUTION_MESSAGES_H
