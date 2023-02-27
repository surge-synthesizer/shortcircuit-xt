//
// Created by Paul Walker on 2/27/23.
//

#ifndef SHORTCIRCUIT_MODULATION_MESSAGES_H
#define SHORTCIRCUIT_MODULATION_MESSAGES_H

#include "messaging/client/detail/client_json_details.h"
#include "json/engine_traits.h"
#include "json/datamodel_traits.h"
#include "selection/selection_manager.h"

namespace scxt::messaging::client
{

SERIAL_TO_CLIENT(UpdateZoneVoiceMatrixMetadata, s2c_update_zone_voice_matrix_metadata,
                 modulation::voiceModMatrixMetadata_t, onZoneVoiceMatrixMetadata);

SERIAL_TO_CLIENT(UpdateZoneVoiceMatrix, s2c_update_zone_voice_matrix,
                 modulation::VoiceModMatrix::routingTable_t, onZoneVoiceMatrix);

typedef std::pair<int, modulation::VoiceModMatrix::Routing> indexedRowUpdate_t;
inline void indexedRoutingRowUpdated(const indexedRowUpdate_t &payload,
                                     const engine::Engine &engine, MessageController &cont)
{
    // TODO Selected Zone State
    const auto &[i, r] = payload;
    auto sz = engine.getSelectionManager()->getSelectedZone();
    if (sz.has_value())
    {
        auto [ps, gs, zs] = *sz;
        cont.scheduleAudioThreadCallback([index = i, row = r, p = ps, g = gs, z = zs](auto &eng) {
            eng.getPatch()->getPart(p)->getGroup(g)->getZone(z)->routingTable[index] = row;
        });
    }
}
CLIENT_TO_SERIAL(IndexedRoutingRowUpdated, c2s_update_zone_routing_row, indexedRowUpdate_t,
                 indexedRoutingRowUpdated(payload, engine, cont));

typedef std::tuple<bool, int, modulation::modulators::StepLFOStorage> indexedLfoUpdate_t;
inline void indexedLfoUpdated(const indexedLfoUpdate_t &payload, const engine::Engine &engine,
                              MessageController &cont)
{
    const auto &[active, i, r] = payload;
    auto sz = engine.getSelectionManager()->getSelectedZone();
    if (sz.has_value())
    {
        auto [ps, gs, zs] = *sz;
        cont.scheduleAudioThreadCallback([index = i, row = r, p = ps, g = gs, z = zs](auto &eng) {
            eng.getPatch()->getPart(p)->getGroup(g)->getZone(z)->lfoStorage[index] = row;
        });
    }
}
CLIENT_TO_SERIAL(IndexedLfoUpdated, c2s_update_individual_lfo, indexedLfoUpdate_t,
                 indexedLfoUpdated(payload, engine, cont));
SERIAL_TO_CLIENT(UpdateZoneLfo, s2c_update_zone_individual_lfo, indexedLfoUpdate_t,
                 onZoneLfoUpdated);

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_MODULATION_MESSAGES_H
