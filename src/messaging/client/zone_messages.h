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

#ifndef SCXT_SRC_MESSAGING_CLIENT_ZONE_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_ZONE_MESSAGES_H

#include "messaging/client/detail/client_json_details.h"
#include "json/engine_traits.h"
#include "json/datamodel_traits.h"
#include "selection/selection_manager.h"

namespace scxt::messaging::client
{
typedef std::tuple<int, bool, datamodel::AdsrStorage> adsrViewResponsePayload_t;
SERIAL_TO_CLIENT(AdsrSelectedZoneView, s2c_respond_zone_adsr_view, adsrViewResponsePayload_t,
                 onEnvelopeUpdated);

typedef std::tuple<int, datamodel::AdsrStorage> adsrSelectedZoneC2SPayload_t;
inline void adsrSelectedZoneUpdate(const adsrSelectedZoneC2SPayload_t &payload,
                                   const engine::Engine &engine, MessageController &cont)
{
    // TODO Selected Zone State
    const auto &[e, adsr] = payload;
    auto sz = engine.getSelectionManager()->getSelectedZone();
    if (sz.has_value())
    {
        auto [ps, gs, zs] = *sz;
        cont.scheduleAudioThreadCallback([p = ps, g = gs, z = zs, ew = e, adsrv = adsr](auto &eng) {
            if (ew == 0)
                eng.getPatch()->getPart(p)->getGroup(g)->getZone(z)->aegStorage = adsrv;
            if (ew == 1)
                eng.getPatch()->getPart(p)->getGroup(g)->getZone(z)->eg2Storage = adsrv;
        });
    }
}
CLIENT_TO_SERIAL(AdsrSelectedZoneUpdateRequest, c2s_update_zone_adsr_view,
                 adsrSelectedZoneC2SPayload_t, adsrSelectedZoneUpdate(payload, engine, cont));

typedef std::tuple<bool, engine::Zone::ZoneMappingData> mappingSelectedZoneViewResposne_t;
SERIAL_TO_CLIENT(MappingSelectedZoneView, s2c_respond_zone_mapping,
                 mappingSelectedZoneViewResposne_t, onMappingUpdated);

typedef std::tuple<bool, engine::Zone::AssociatedSampleArray> sampleSelectedZoneViewResposne_t;
SERIAL_TO_CLIENT(SampleSelectedZoneView, s2c_respond_zone_samples, sampleSelectedZoneViewResposne_t,
                 onSamplesUpdated);

inline void mappingSelectedZoneUpdate(const engine::Zone::ZoneMappingData &payload,
                                      const engine::Engine &engine, MessageController &cont)
{
    // TODO Selected Zone State
    const auto &mapping = payload;
    auto sz = engine.getSelectionManager()->getSelectedZone();
    if (sz.has_value())
    {
        auto [ps, gs, zs] = *sz;
        cont.scheduleAudioThreadCallback(
            [p = ps, g = gs, z = zs, mapv = mapping](auto &eng) {
                eng.getPatch()->getPart(p)->getGroup(g)->getZone(z)->mapping = mapv;
            },
            [p = ps, g = gs](const auto &eng) {
                serializationSendToClient(
                    messaging::client::s2c_send_selected_group_zone_mapping_summary,
                    eng.getPatch()->getPart(p)->getGroup(g)->getZoneMappingSummary(),
                    *(eng.getMessageController()));
            });
    }
}
CLIENT_TO_SERIAL(MappingSelectedZoneUpdateRequest, c2s_update_zone_mapping,
                 engine::Zone::ZoneMappingData, mappingSelectedZoneUpdate(payload, engine, cont));

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_ZONE_MESSAGES_H
