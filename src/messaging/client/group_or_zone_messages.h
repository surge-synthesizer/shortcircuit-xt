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

#ifndef SCXT_SRC_MESSAGING_CLIENT_GROUP_OR_ZONE_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_GROUP_OR_ZONE_MESSAGES_H

namespace scxt::messaging::client
{
// structure here is (forZone, whichEG, active, data)
typedef std::tuple<bool, int, bool, modulation::modulators::AdsrStorage> adsrViewResponsePayload_t;
SERIAL_TO_CLIENT(AdsrGroupOrZoneUpdate, s2c_update_group_or_zone_adsr_view,
                 adsrViewResponsePayload_t, onGroupOrZoneEnvelopeUpdated);

CLIENT_TO_SERIAL_CONSTRAINED(UpdateZoneOrGroupEGFloatValue, c2s_update_zone_or_group_eg_float_value,
                             detail::indexedZoneOrGroupDiffMsg_t<float>,
                             modulation::modulators::AdsrStorage,
                             detail::updateZoneOrGroupIndexedMemberValue(&engine::Zone::egStorage,
                                                                         &engine::Group::gegStorage,
                                                                         payload, engine, cont));

CLIENT_TO_SERIAL_CONSTRAINED(
    UpdateZoneOrGroupModStorageFloatValue, c2s_update_zone_or_group_modstorage_float_value,
    detail::indexedZoneOrGroupDiffMsg_t<float>, modulation::ModulatorStorage,
    detail::updateZoneOrGroupIndexedMemberValue(&engine::Zone::modulatorStorage,
                                                &engine::Group::modulatorStorage, payload, engine,
                                                cont));

CLIENT_TO_SERIAL_CONSTRAINED(
    UpdateZoneOrGroupModStorageBoolValue, c2s_update_zone_or_group_modstorage_bool_value,
    detail::indexedZoneOrGroupDiffMsg_t<bool>, modulation::ModulatorStorage,
    detail::updateZoneOrGroupIndexedMemberValue(&engine::Zone::modulatorStorage,
                                                &engine::Group::modulatorStorage, payload, engine,
                                                cont));

// int updates can change shape. For now lets just always assume they do
CLIENT_TO_SERIAL_CONSTRAINED(
    UpdateZoneOrGroupModStorageInt16TValue, c2s_update_zone_or_group_modstorage_int16_t_value,
    detail::indexedZoneOrGroupDiffMsg_t<int16_t>, modulation::ModulatorStorage,
    detail::updateZoneOrGroupIndexedMemberValue(
        &engine::Zone::modulatorStorage, &engine::Group::modulatorStorage, payload, engine, cont,
        [payload](auto &eng) {
            auto forZone = std::get<0>(payload);
            if (forZone)
            {
                // ToDo: Have to do the group side of this later
                auto lz = eng.getSelectionManager()->currentLeadZone(eng);
                if (lz.has_value())
                {
                    auto &z =
                        eng.getPatch()->getPart(lz->part)->getGroup(lz->group)->getZone(lz->zone);
                    serializationSendToClient(messaging::client::s2c_update_zone_matrix_metadata,
                                              voice::modulation::getVoiceMatrixMetadata(*z),
                                              *(eng.getMessageController()));
                }
            }
            else
            {
                // ToDo: Have to do the group side of this later
                auto lg = eng.getSelectionManager()->currentLeadGroup(eng);
            }
        },
        nullptr, // no need to do a voice update
        [payload](auto &engine, const auto &gs) {
            auto idx = std::get<1>(payload);
            assert(engine.getMessageController()->threadingChecker.isAudioThread());
            for (auto [p, g, z] : gs)
            {
                auto &grp = engine.getPatch()->getPart(p)->getGroup(g);
                grp->resetLFOs(idx);
                grp->rePrepareAndBindGroupMatrix();
            }
        }))

using renameGroupZonePayload_t = std::tuple<selection::SelectionManager::ZoneAddress, std::string>;
inline void doRenameGroup(const renameGroupZonePayload_t &payload, const engine::Engine &engine,
                          MessageController &cont)
{
    const auto &[p, g, z] = std::get<0>(payload);
    engine.getPatch()->getPart(p)->getGroup(g)->name = std::get<1>(payload);
    serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(), cont);
}
CLIENT_TO_SERIAL(RenameGroup, c2s_rename_group, renameGroupZonePayload_t,
                 doRenameGroup(payload, engine, cont));

inline void doRenameZone(const renameGroupZonePayload_t &payload, const engine::Engine &engine,
                         MessageController &cont)
{
    const auto &[p, g, z] = std::get<0>(payload);
    engine.getPatch()->getPart(p)->getGroup(g)->getZone(z)->givenName = std::get<1>(payload);
    serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(), cont);
    serializationSendToClient(s2c_send_selected_group_zone_mapping_summary,
                              engine.getPatch()->getPart(p)->getZoneMappingSummary(),
                              *(engine.getMessageController()));
}
CLIENT_TO_SERIAL(RenameZone, c2s_rename_zone, renameGroupZonePayload_t,
                 doRenameZone(payload, engine, cont));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUITXT_GROUP_OR_ZONE_MESSAGES_H
