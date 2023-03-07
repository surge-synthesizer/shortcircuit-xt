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
inline void adsrSelectedViewResponse(const int &which, const engine::Engine &engine,
                                     MessageController &cont)
{
    // TODO Selected Zone State
    // const auto &selectedZone = engine.getPatch()->getPart(0)->getGroup(0)->getZone(0);
    auto addr = engine.getSelectionManager()->getSelectedZone();

    if (addr.has_value())
    {
        const auto &selectedZone =
            engine.getPatch()->getPart(addr->part)->getGroup(addr->group)->getZone(addr->zone);
        if (which == 0)
            serializationSendToClient(
                s2c_respond_zone_adsr_view,
                adsrViewResponsePayload_t{which, true, selectedZone->aegStorage}, cont);
        if (which == 1)
            serializationSendToClient(
                s2c_respond_zone_adsr_view,
                adsrViewResponsePayload_t{which, true, selectedZone->eg2Storage}, cont);
    }
    else
    {
        // It is a wee bit wasteful re-sending a default ADSR here but easier
        // than two messages
        serializationSendToClient(s2c_respond_zone_adsr_view,
                                  adsrViewResponsePayload_t{which, false, {}}, cont);
    }
}
CLIENT_SERIAL_REQUEST_RESPONSE(AdsrSelectedZoneView, c2s_request_zone_adsr_view, int,
                               s2c_respond_zone_adsr_view, adsrViewResponsePayload_t,
                               adsrSelectedViewResponse(payload, engine, cont), onEnvelopeUpdated);

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
inline void mappingSelectedZoneViewSerial(const engine::Engine &engine, MessageController(&cont))
{
    // TODO Selected Zone State
    // const auto &selectedZone = engine.getPatch()->getPart(0)->getGroup(0)->getZone(0);
    auto addr = engine.getSelectionManager()->getSelectedZone();

    if (addr.has_value())
    {
        const auto &selectedZone =
            engine.getPatch()->getPart(addr->part)->getGroup(addr->group)->getZone(addr->zone);
        serializationSendToClient(s2c_respond_zone_mapping,
                                  mappingSelectedZoneViewResposne_t{true, selectedZone->mapping},
                                  cont);
    }
    else
    {
        serializationSendToClient(s2c_respond_zone_mapping,
                                  mappingSelectedZoneViewResposne_t{false, {}}, cont);
    }
}
CLIENT_SERIAL_REQUEST_RESPONSE(MappingSelectedZoneView, c2s_request_zone_mapping, uint8_t,
                               s2c_respond_zone_mapping, mappingSelectedZoneViewResposne_t,
                               mappingSelectedZoneViewSerial(engine, cont), onMappingUpdated);

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
