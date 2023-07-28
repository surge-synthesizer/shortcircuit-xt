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

#ifndef SCXT_SRC_MESSAGING_CLIENT_MODULATION_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_MODULATION_MESSAGES_H

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

// which row, what data, and force a full update
typedef std::tuple<int, modulation::VoiceModMatrix::Routing, bool> indexedRowUpdate_t;
inline void indexedRoutingRowUpdated(const indexedRowUpdate_t &payload,
                                     const engine::Engine &engine, MessageController &cont)
{
    // TODO Selected Zone State
    const auto &[i, r, b] = payload;
    auto sz = engine.getSelectionManager()->currentlySelectedZones();
    if (!sz.empty())
    {
        cont.scheduleAudioThreadCallback(
            [index = i, row = r, zs = sz](auto &eng) {
                for (const auto &z : zs)
                    eng.getPatch()
                        ->getPart(z.part)
                        ->getGroup(z.group)
                        ->getZone(z.zone)
                        ->routingTable[index] = row;
            },
            [doUpdate = b](auto &eng) {
                if (doUpdate)
                {
                    auto lz = eng.getSelectionManager()->currentLeadZone(eng);
                    if (lz.has_value())
                        eng.getSelectionManager()->sendDisplayDataForZonesBasedOnLead(
                            lz->part, lz->group, lz->zone);
                }
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
    auto sz = engine.getSelectionManager()->currentlySelectedZones();
    if (!sz.empty())
    {
        cont.scheduleAudioThreadCallback([row = r, index = i, zs = sz](auto &eng) {
            for (const auto &[p, g, z] : zs)
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
