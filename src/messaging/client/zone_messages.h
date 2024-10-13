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
inline void doUpdateLeadZoneMapping(const engine::Zone::ZoneMappingData &payload,
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
CLIENT_TO_SERIAL(UpdateLeadZoneMapping, c2s_update_lead_zone_mapping, engine::Zone::ZoneMappingData,
                 doUpdateLeadZoneMapping(payload, engine, cont));

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

typedef std::tuple<bool, engine::Zone::Variants> sampleSelectedZoneViewResposne_t;
SERIAL_TO_CLIENT(SampleSelectedZoneView, s2c_respond_zone_samples, sampleSelectedZoneViewResposne_t,
                 onSamplesUpdated);

using updateLeadZoneSingleVariantPayload_t = std::tuple<size_t, engine::Zone::SingleVariant>;
inline void doUpdateLeadZoneSingleVariant(const updateLeadZoneSingleVariantPayload_t &payload,
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
            eng.getPatch()->getPart(p)->getGroup(g)->getZone(z)->variantData.variants[idx] = smp;
        });
    }
}
CLIENT_TO_SERIAL(UpdateLeadZoneSingleVariant, c2s_update_lead_zone_single_variant,
                 updateLeadZoneSingleVariantPayload_t,
                 doUpdateLeadZoneSingleVariant(payload, engine, cont));

using normalizeVariantAmplitudePayload_t = std::tuple<size_t, bool>;
inline void doNormalizeVariantAmplitude(const normalizeVariantAmplitudePayload_t &payload,
                                        const engine::Engine &engine, MessageController &cont)
{
    const auto &samples = payload;
    auto sz = engine.getSelectionManager()->currentLeadZone(engine);
    if (sz.has_value())
    {
        auto [ps, gs, zs] = *sz;
        cont.scheduleAudioThreadCallback([p = ps, g = gs, z = zs, sampv = samples](auto &eng) {
            auto &[idx, use_peak] = sampv;
            eng.getPatch()->getPart(p)->getGroup(g)->getZone(z)->setNormalizedSampleLevel(use_peak,
                                                                                          idx);
        });
    }
}
CLIENT_TO_SERIAL(NormalizeVariantAmplitude, c2s_normalize_variant_amplitude,
                 normalizeVariantAmplitudePayload_t,
                 doNormalizeVariantAmplitude(payload, engine, cont));

using clearVariantAmplitudeNormalizationPayload_t = size_t;
inline void
doClearVariantAmplitudeNormalization(const clearVariantAmplitudeNormalizationPayload_t &payload,
                                     const engine::Engine &engine, MessageController &cont)
{
    const auto &samples = payload;
    auto sz = engine.getSelectionManager()->currentLeadZone(engine);
    if (sz.has_value())
    {
        auto [ps, gs, zs] = *sz;
        cont.scheduleAudioThreadCallback([p = ps, g = gs, z = zs, sampv = samples](auto &eng) {
            auto &idx = sampv;
            eng.getPatch()->getPart(p)->getGroup(g)->getZone(z)->clearNormalizedSampleLevel(idx);
        });
    }
}
CLIENT_TO_SERIAL(ClearVariantAmplitudeNormalization, c2s_clear_variant_amplitude_normalization,
                 clearVariantAmplitudeNormalizationPayload_t,
                 doClearVariantAmplitudeNormalization(payload, engine, cont));

CLIENT_TO_SERIAL_CONSTRAINED(UpdateZoneVariantsInt16TValue, c2s_update_zone_variants_int16_t,
                             detail::diffMsg_t<int16_t>, engine::Zone::Variants,
                             detail::updateZoneMemberValue(&engine::Zone::variantData, payload,
                                                           engine, cont));

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

using addBlankZonePayload_t = std::array<int, 6>;
inline void doAddBlankZone(const addBlankZonePayload_t &payload, engine::Engine &engine,
                           MessageController &cont)
{
    auto [part, group, ks, ke, vs, ve] = payload;
    engine.createEmptyZone({ks, ke}, {vs, ve});
}

CLIENT_TO_SERIAL(AddBlankZone, c2s_add_blank_zone, addBlankZonePayload_t,
                 doAddBlankZone(payload, engine, cont));

using deleteVariantPayload_t = int;
inline void doDeleteVariant(const deleteVariantPayload_t &payload, engine::Engine &engine,
                            MessageController &cont)
{
    const auto &samples = payload;
    auto sz = engine.getSelectionManager()->currentLeadZone(engine);
    if (sz.has_value())
    {
        auto [ps, gs, zs] = *sz;
        cont.scheduleAudioThreadCallback(
            [p = ps, g = gs, z = zs, var = payload](auto &eng) {
                const auto &zone = eng.getPatch()->getPart(p)->getGroup(g)->getZone(z);
                zone->deleteVariant(var);
                eng.getSampleManager()->purgeUnreferencedSamples();
            },
            [p = ps, g = gs, z = zs](auto &e) {
                SCLOG_ONCE("Delete variant could be optimized to not sending so much back");
                e.sendFullRefreshToClient();
            });
    }
}
CLIENT_TO_SERIAL(DeleteVariant, c2s_delete_variant, deleteVariantPayload_t,
                 doDeleteVariant(payload, engine, cont))

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_ZONE_MESSAGES_H
