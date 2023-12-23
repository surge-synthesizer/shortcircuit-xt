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
                    /*
                    serializationSendToClient(
                        messaging::client::s2c_update_zone_matrix_metadata,
                        modulation::getVoiceModMatrixMetadata(*z),
                        *(engine.getMessageController()));*/
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
                                              modulation::getVoiceModMatrixMetadata(*z),
                                              *(engine.getMessageController()));
                });
        }
    }
}
CLIENT_TO_SERIAL(SetSelectedProcessorType, c2s_set_processor_type, setProcessorPayload_t,
                 setProcessorType(payload, engine, cont));

CLIENT_TO_SERIAL(CopyProcessorLeadToAll, c2s_copy_processor_lead_to_all, int,
                 engine.getSelectionManager()->copyZoneProcessorLeadToAll(payload));

// C2S set processor storage
typedef std::pair<int32_t, dsp::processor::ProcessorStorage> setProcessorStoragePayload_t;
inline void setProcessorStorage(const setProcessorStoragePayload_t &payload,
                                const engine::Engine &engine, messaging::MessageController &cont)
{
    const auto &[w, st] = payload;
    auto sz = engine.getSelectionManager()->currentlySelectedZones();

    if (!sz.empty())
    {
        cont.scheduleAudioThreadCallback([zs = sz, which = w, storage = st](auto &e) {
            for (const auto &a : zs)
            {
                const auto &z = e.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone);
                z->processorStorage[which] = storage;
            }
        });
    }
}
CLIENT_TO_SERIAL(SetSelectedProcessorStorage, c2s_update_single_processor_data,
                 setProcessorStoragePayload_t, setProcessorStorage(payload, engine, cont));

enum struct ProcessorValueIndex : uint32_t
{
    param0,
    mix = param0 + dsp::processor::maxProcessorFloatParams
};
// for-zone, which processor, which index, value
typedef std::tuple<bool, int32_t, uint32_t, float> setProcessorSingleValuePayload_t;
inline void setProcessorSingleValue(const setProcessorSingleValuePayload_t &payload,
                                    const engine::Engine &engine,
                                    messaging::MessageController &cont)
{
    const auto &[forzone, w, idx, val] = payload;
    auto sz = engine.getSelectionManager()->currentlySelectedZones();

    if (forzone && !!sz.empty())
    {
        cont.scheduleAudioThreadCallback([zs = sz, which = w, validx = idx, newval = val](auto &e) {
            for (const auto &a : zs)
            {
                const auto &z = e.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone);
                if (validx == (uint32_t)ProcessorValueIndex::mix)
                {
                    z->mUILag.setNewDestination(&(z->processorStorage[which].mix), newval);
                }
                else
                {
                    auto fidx = (int)validx - (int)ProcessorValueIndex::param0;
                    assert(fidx < dsp::processor::maxProcessorFloatParams);
                    z->mUILag.setNewDestination(&(z->processorStorage[which].floatParams[fidx]),
                                                newval);
                }
                if (!z->isActive())
                {
                    z->mUILag.instantlySnap();
                }
            }
        });
        return;
    }

    auto sg = engine.getSelectionManager()->currentlySelectedGroups();
    if (!forzone && !sg.empty())
    {
        cont.scheduleAudioThreadCallback([gs = sg, which = w, validx = idx, newval = val](auto &e) {
            for (const auto &a : gs)
            {
                const auto &g = e.getPatch()->getPart(a.part)->getGroup(a.group);
                if (validx == (uint32_t)ProcessorValueIndex::mix)
                {
                    g->mUILag.setNewDestination(&(g->processorStorage[which].mix), newval);
                }
                else
                {
                    auto fidx = (int)validx - (int)ProcessorValueIndex::param0;
                    assert(fidx < dsp::processor::maxProcessorFloatParams);
                    g->mUILag.setNewDestination(&(g->processorStorage[which].floatParams[fidx]),
                                                newval);
                }
            }
        });
    }
}
CLIENT_TO_SERIAL(SetSelectedProcessorSingleValue, c2s_update_single_processor_single_value,
                 setProcessorSingleValuePayload_t, setProcessorSingleValue(payload, engine, cont));

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

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_PROCESSOR_MESSAGES_H
