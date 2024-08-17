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

#ifndef SCXT_SRC_MESSAGING_CLIENT_PROCESSOR_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_PROCESSOR_MESSAGES_H

#include <cassert>
#include "client_macros.h"
#include "selection/selection_manager.h"

namespace scxt::messaging::client
{
// C2S Request for Processor I S2C Metadata and Processor Storage
// forZone, which, active, description, storage
typedef std::tuple<bool, int, bool, dsp::processor::ProcessorControlDescription,
                   dsp::processor::ProcessorStorage>
    processorDataResponsePayload_t;

SERIAL_TO_CLIENT(ProcessorMetadataAndData, s2c_respond_single_processor_metadata_and_data,
                 processorDataResponsePayload_t, onGroupOrZoneProcessorDataAndMetadata);

// zone, lead type, leadName, all types
typedef std::tuple<int, int, std::string, std::set<int32_t>> processorMismatchPayload_t;
SERIAL_TO_CLIENT(ProcessorsMismatched, s2c_notify_mismatched_processors_for_zone,
                 processorMismatchPayload_t, onZoneProcessorDataMismatch);

// C2S set processor type (sends back data and metadata)
// tuple is forzone, whichprocessor, type
typedef std::tuple<bool, int32_t, int32_t> setProcessorPayload_t;
inline void setProcessorType(const setProcessorPayload_t &whichToType, const engine::Engine &engine,
                             messaging::MessageController &cont)
{
    const auto &[forZone, w, id] = whichToType;
    if (!forZone)
    {
        auto sg = engine.getSelectionManager()->currentlySelectedGroups();
        auto lg = engine.getSelectionManager()->currentLeadGroup(engine);
        assert(sg.empty() || lg.has_value());
        if (!sg.empty())
        {
            cont.scheduleAudioThreadCallback(
                [gs = sg, which = w, type = id](auto &e) {
                    for (const auto &a : gs)
                    {
                        const auto &g = e.getPatch()->getPart(a.part)->getGroup(a.group);
                        g->setProcessorType(which, (dsp::processor::ProcessorType)type);
                    }
                },
                [a = *lg, which = w](const auto &engine) {
                    const auto &g = engine.getPatch()->getPart(a.part)->getGroup(a.group);
                    serializationSendToClient(
                        messaging::client::s2c_respond_single_processor_metadata_and_data,
                        messaging::client::ProcessorMetadataAndData::s2c_payload_t{
                            false, which, true, g->processorDescription[which],
                            g->processorStorage[which]},
                        *(engine.getMessageController()));
                    serializationSendToClient(messaging::client::s2c_update_group_matrix_metadata,
                                              modulation::getGroupMatrixMetadata(*g),
                                              *(engine.getMessageController()));
                });
        }
        return;
    }
    else
    {
        auto sz = engine.getSelectionManager()->currentlySelectedZones();
        auto lz = engine.getSelectionManager()->currentLeadZone(engine);

        assert(sz.empty() || lz.has_value());

        if (!sz.empty() && lz.has_value())
        {
            cont.scheduleAudioThreadCallback(
                [zs = sz, which = w, type = id](auto &e) {
                    for (const auto &a : zs)
                    {
                        const auto &z =
                            e.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone);
                        z->setProcessorType(which, (dsp::processor::ProcessorType)type);
                    }
                },
                [a = *lz, which = w](const auto &engine) {
                    const auto &z =
                        engine.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone);
                    serializationSendToClient(
                        messaging::client::s2c_respond_single_processor_metadata_and_data,
                        messaging::client::ProcessorMetadataAndData::s2c_payload_t{
                            true, which, true, z->processorDescription[which],
                            z->processorStorage[which]},
                        *(engine.getMessageController()));
                    serializationSendToClient(messaging::client::s2c_update_zone_matrix_metadata,
                                              voice::modulation::getVoiceMatrixMetadata(*z),
                                              *(engine.getMessageController()));
                });
        }
    }
}
CLIENT_TO_SERIAL(SetSelectedProcessorType, c2s_set_processor_type, setProcessorPayload_t,
                 setProcessorType(payload, engine, cont));

CLIENT_TO_SERIAL(CopyProcessorLeadToAll, c2s_copy_processor_lead_to_all, int,
                 engine.getSelectionManager()->copyZoneProcessorLeadToAll(payload));

CLIENT_TO_SERIAL_CONSTRAINED(
    UpdateZoneOrGroupProcessorFloatValue, c2s_update_single_processor_float_value,
    detail::indexedZoneOrGroupDiffMsg_t<float>, dsp::processor::ProcessorStorage,
    detail::updateZoneOrGroupIndexedMemberValue(&engine::Zone::processorStorage,
                                                &engine::Group::processorStorage, payload, engine,
                                                cont));

CLIENT_TO_SERIAL_CONSTRAINED(
    UpdateZoneOrGroupProcessorInt32TValue, c2s_update_single_processor_int32_t_value,
    detail::indexedZoneOrGroupDiffMsg_t<int32_t>, dsp::processor::ProcessorStorage,
    detail::updateZoneOrGroupIndexedMemberValue(
        &engine::Zone::processorStorage, &engine::Group::processorStorage, payload, engine, cont,
        nullptr,
        [payload](auto &e, auto &sz) {
            auto wp = std::get<1>(payload);
            for (const auto &a : sz)
            {
                const auto &z = e.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone);
                z->checkOrAdjustIntConsistency(wp);
            }
        },
        [payload](auto &e, auto &sg) {
            auto wp = std::get<1>(payload);
            for (const auto &a : sg)
            {
                const auto &g = e.getPatch()->getPart(a.part)->getGroup(a.group);
                g->checkOrAdjustIntConsistency(wp);
            }
        }));

CLIENT_TO_SERIAL_CONSTRAINED(
    UpdateZoneOrGroupProcessorBoolValue, c2s_update_single_processor_bool_value,
    detail::indexedZoneOrGroupDiffMsg_t<bool>, dsp::processor::ProcessorStorage,
    detail::updateZoneOrGroupIndexedMemberValue(
        &engine::Zone::processorStorage, &engine::Group::processorStorage, payload, engine, cont,
        nullptr,
        [payload](auto &e, auto &sz) {
            auto wp = std::get<1>(payload);
            for (const auto &a : sz)
            {
                const auto &z = e.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone);
                z->checkOrAdjustBoolConsistency(wp);
            }
        },
        [payload](auto &e, auto &sg) {
            auto wp = std::get<1>(payload);
            for (const auto &a : sg)
            {
                const auto &g = e.getPatch()->getPart(a.part)->getGroup(a.group);
                g->checkOrAdjustBoolConsistency(wp);
            }
        }));

// C2S set processor type (sends back data and metadata)
typedef std::pair<int32_t, int32_t> processorPair_t;
inline void swapZoneProcessors(const processorPair_t &whichToType, const engine::Engine &engine,
                               messaging::MessageController &cont)
{
    const auto &[to, from] = whichToType;
    auto sz = engine.getSelectionManager()->currentlySelectedZones();
    auto lz = engine.getSelectionManager()->currentLeadZone(engine);

    if (!sz.empty() && lz.has_value())
    {
        cont.scheduleAudioThreadCallback(
            [q = sz, t = to, f = from](auto &engine) {
                for (const auto &a : q)
                {
                    const auto &z =
                        engine.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone);
                    auto fs = z->processorStorage[f];
                    auto ts = z->processorStorage[t];

                    z->setProcessorType(f, (dsp::processor::ProcessorType)ts.type);
                    z->setProcessorType(t, (dsp::processor::ProcessorType)fs.type);

                    z->processorStorage[f] = ts;
                    z->processorStorage[t] = fs;

                    z->updateRoutingTableAfterProcessorSwap(f, t);

                    z->checkOrAdjustBoolConsistency(f);
                    z->checkOrAdjustIntConsistency(f);
                    z->checkOrAdjustBoolConsistency(t);
                    z->checkOrAdjustIntConsistency(t);
                }
            },
            [a = *lz](const auto &engine) {
                engine.getSelectionManager()->sendDisplayDataForZonesBasedOnLead(a.part, a.group,
                                                                                 a.zone);
            });
    }
    else
    {
        // This means you have a lead zone and no selected zones
        assert(sz.empty());
    }
}
CLIENT_TO_SERIAL(SwapZoneProcessors, c2s_swap_zone_processors, processorPair_t,
                 swapZoneProcessors(payload, engine, cont));

inline void swapGroupProcessors(const processorPair_t &whichToType, const engine::Engine &engine,
                                messaging::MessageController &cont)
{
    const auto &[to, from] = whichToType;
    auto sg = engine.getSelectionManager()->currentlySelectedGroups();
    auto lg = engine.getSelectionManager()->currentLeadGroup(engine);

    if (!sg.empty() && lg.has_value())
    {
        cont.scheduleAudioThreadCallback(
            [q = sg, t = to, f = from](auto &engine) {
                for (const auto &a : q)
                {
                    const auto &g = engine.getPatch()->getPart(a.part)->getGroup(a.group);
                    auto fs = g->processorStorage[f];
                    auto ts = g->processorStorage[t];

                    g->setProcessorType(f, (dsp::processor::ProcessorType)ts.type);
                    g->setProcessorType(t, (dsp::processor::ProcessorType)fs.type);

                    g->processorStorage[f] = ts;
                    g->processorStorage[t] = fs;

                    g->updateRoutingTableAfterProcessorSwap(f, t);

                    g->checkOrAdjustBoolConsistency(f);
                    g->checkOrAdjustIntConsistency(f);
                    g->checkOrAdjustBoolConsistency(t);
                    g->checkOrAdjustIntConsistency(t);

                    g->rePrepareAndBindGroupMatrix();
                }
            },
            [a = *lg](const auto &engine) {
                engine.getSelectionManager()->sendDisplayDataForSingleGroup(a.part, a.group);
            });
    }
    else
    {
        // This means you have a lead zone and no selected zones
        assert(sg.empty());
    }
}

CLIENT_TO_SERIAL(SwapGroupProcessors, c2s_swap_group_processors, processorPair_t,
                 swapGroupProcessors(payload, engine, cont));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_PROCESSOR_MESSAGES_H
