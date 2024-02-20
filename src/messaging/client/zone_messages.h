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

#ifndef SCXT_SRC_MESSAGING_CLIENT_ZONE_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_ZONE_MESSAGES_H

#include "messaging/client/detail/client_json_details.h"
#include "messaging/client/detail/message_helpers.h"
#include "json/engine_traits.h"
#include "json/datamodel_traits.h"
#include "selection/selection_manager.h"

namespace scxt::messaging::client
{

typedef std::tuple<bool, engine::Zone::ZoneMappingData> mappingSelectedZoneViewResposne_t;
SERIAL_TO_CLIENT(MappingSelectedZoneView, s2c_respond_zone_mapping,
                 mappingSelectedZoneViewResposne_t, onMappingUpdated);

/*
 * For now we are keeping this 'bulk' message around since the drag-a-field
 * ui actually uses it (rather than figure out which edge is changed) but we
 * also have per-field updates for the UI elements below and associated
 * bound metadata
 */
inline void mappingSelectedZoneUpdate(const engine::Zone::ZoneMappingData &payload,
                                      const engine::Engine &engine, MessageController &cont)
{
    // TODO Selected Zone State
    const auto &mapping = payload;
    auto sz = engine.getSelectionManager()->currentLeadZone(engine);
    if (sz.has_value())
    {
        cont.scheduleAudioThreadCallback(
            [zs = *sz, mapv = mapping](auto &eng) {
                auto [p, g, z] = zs;
                eng.getPatch()->getPart(p)->getGroup(g)->getZone(z)->mapping = mapv;
            },
            [p = sz->part](const auto &eng) {
                serializationSendToClient(
                    messaging::client::s2c_send_selected_group_zone_mapping_summary,
                    eng.getPatch()->getPart(p)->getZoneMappingSummary(),
                    *(eng.getMessageController()));
            });
    }
}
CLIENT_TO_SERIAL(MappingSelectedZoneUpdateRequest, c2s_update_zone_mapping,
                 engine::Zone::ZoneMappingData, mappingSelectedZoneUpdate(payload, engine, cont));

// Updating mapping rather than try and calculate the image client side just
// resend the resulting mapped zones with selection state back to the UI
CLIENT_TO_SERIAL_CONSTRAINED(UpdateZoneMappingFloatValue, c2s_update_zone_mapping_float,
                             detail::diffMsg_t<float>, engine::Zone::ZoneMappingData,
                             detail::updateZoneLeadMemberValue(&engine::Zone::mapping, payload,
                                                               engine, cont));
CLIENT_TO_SERIAL_CONSTRAINED(
    UpdateZoneMappingInt16TValue, c2s_update_zone_mapping_int16_t, detail::diffMsg_t<int16_t>,
    engine::Zone::ZoneMappingData,
    detail::updateZoneLeadMemberValue(
        &engine::Zone::mapping, payload, engine, cont, [](const auto &eng) {
            auto pt = eng.getSelectionManager()->currentlySelectedPart(eng);
            serializationSendToClient(
                messaging::client::s2c_send_selected_group_zone_mapping_summary,
                eng.getPatch()->getPart(pt)->getZoneMappingSummary(),
                *(eng.getMessageController()));
        }));

typedef std::tuple<bool, engine::Zone::AssociatedSampleArray> sampleSelectedZoneViewResposne_t;
SERIAL_TO_CLIENT(SampleSelectedZoneView, s2c_respond_zone_samples, sampleSelectedZoneViewResposne_t,
                 onSamplesUpdated);

using associatedSampleVariationPayload_t = std::tuple<size_t, engine::Zone::AssociatedSample>;
inline void samplesSelectedZoneUpdate(const associatedSampleVariationPayload_t &payload,
                                      const engine::Engine &engine, MessageController &cont)
{
    // TODO Selected Zone State
    const auto &samples = payload;
    auto sz = engine.getSelectionManager()->currentLeadZone(engine);
    if (sz.has_value())
    {
        auto [ps, gs, zs] = *sz;
        cont.scheduleAudioThreadCallback([p = ps, g = gs, z = zs, sampv = samples](auto &eng) {
            auto &[idx, smp] = sampv;
            eng.getPatch()->getPart(p)->getGroup(g)->getZone(z)->sampleData[idx] = smp;
        });
    }
}
CLIENT_TO_SERIAL(SamplesSelectedZoneUpdateRequest, c2s_update_zone_samples,
                 associatedSampleVariationPayload_t,
                 samplesSelectedZoneUpdate(payload, engine, cont));

using zoneOutputInfoUpdate_t = std::pair<bool, engine::Zone::ZoneOutputInfo>;
SERIAL_TO_CLIENT(ZoneOutputInfoUpdated, s2c_update_zone_output_info, zoneOutputInfoUpdate_t,
                 onZoneOutputInfoUpdated);

CLIENT_TO_SERIAL_CONSTRAINED(UpdateZoneOutputFloatValue, c2s_update_zone_output_float_value,
                             detail::diffMsg_t<float>, engine::Zone::ZoneOutputInfo,
                             detail::updateZoneMemberValue(&engine::Zone::outputInfo, payload,
                                                           engine, cont));

CLIENT_TO_SERIAL_CONSTRAINED(UpdateZoneOutputInt16TValue, c2s_update_zone_output_int16_t_value,
                             detail::diffMsg_t<int16_t>, engine::Zone::ZoneOutputInfo,
                             detail::updateZoneMemberValue(&engine::Zone::outputInfo, payload,
                                                           engine, cont));

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_ZONE_MESSAGES_H
