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
typedef std::tuple<int, bool, dsp::processor::ProcessorControlDescription,
                   dsp::processor::ProcessorStorage>
    processorDataResponsePayload_t;

SERIAL_TO_CLIENT(ProcessorMetadataAndData, s2c_respond_single_processor_metadata_and_data,
                 processorDataResponsePayload_t, onProcessorDataAndMetadata);

// C2S set processor type (sends back data and metadata)
typedef std::pair<int32_t, int32_t> setProcessorPayload_t;
inline void setProcessorType(const setProcessorPayload_t &whichToType, const engine::Engine &engine,
                             messaging::MessageController &cont)
{
    const auto &[w, id] = whichToType;
    auto sz = engine.getSelectionManager()->currentlySelectedZones();
    auto lz = engine.getSelectionManager()->currentLeadZone(engine);

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
                        which, true, z->processorDescription[which], z->processorStorage[which]},
                    *(engine.getMessageController()));
                serializationSendToClient(messaging::client::s2c_update_zone_voice_matrix_metadata,
                                          modulation::getVoiceModMatrixMetadata(*z),
                                          *(engine.getMessageController()));
            });
    }
    else
    {
        // This means you have a lead zone and no selected zones
        assert(sz.empty());
    }
}
CLIENT_TO_SERIAL(SetSelectedProcessorType, c2s_set_processor_type, setProcessorPayload_t,
                 setProcessorType(payload, engine, cont));

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
            [a = *lz, t = to, f = from](auto &engine) {
                const auto &z =
                    engine.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone);
                auto fs = z->processorStorage[f];
                auto ts = z->processorStorage[t];

                z->setProcessorType(f, (dsp::processor::ProcessorType)ts.type);
                z->setProcessorType(t, (dsp::processor::ProcessorType)fs.type);

                z->processorStorage[f] = ts;
                z->processorStorage[t] = fs;
            },
            [a = *lz, t = to, f = from](const auto &engine) {
                const auto &z =
                    engine.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone);
                serializationSendToClient(
                    messaging::client::s2c_respond_single_processor_metadata_and_data,
                    messaging::client::ProcessorMetadataAndData::s2c_payload_t{
                        t, true, z->processorDescription[t], z->processorStorage[t]},
                    *(engine.getMessageController()));

                serializationSendToClient(
                    messaging::client::s2c_respond_single_processor_metadata_and_data,
                    messaging::client::ProcessorMetadataAndData::s2c_payload_t{
                        f, true, z->processorDescription[f], z->processorStorage[f]},
                    *(engine.getMessageController()));
                serializationSendToClient(messaging::client::s2c_update_zone_voice_matrix_metadata,
                                          modulation::getVoiceModMatrixMetadata(*z),
                                          *(engine.getMessageController()));
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
