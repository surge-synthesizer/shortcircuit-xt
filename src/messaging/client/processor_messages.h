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
inline void processorDataResponse(const int &which, const engine::Engine &engine,
                                  MessageController &cont)
{
    auto addr = engine.getSelectionManager()->getSelectedZone();
    assert(which < engine::processorsPerZone && which >= 0);
    if (addr.has_value() && which < engine::processorsPerZone && which >= 0)
    {
        const auto &selectedZone =
            engine.getPatch()->getPart(addr->part)->getGroup(addr->group)->getZone(addr->zone);
        serializationSendToClient(
            s2c_respond_single_processor_metadata_and_data,
            processorDataResponsePayload_t{which, true, {}, selectedZone->processorStorage[which]},
            cont);
    }
    else
    {
        serializationSendToClient(s2c_respond_single_processor_metadata_and_data,
                                  processorDataResponsePayload_t{which, false, {}, {}}, cont);
    }
}
CLIENT_SERIAL_REQUEST_RESPONSE(ProcessorMetadataAndData,
                               c2s_request_single_processor_metadata_and_data, int,
                               s2c_respond_single_processor_metadata_and_data,
                               processorDataResponsePayload_t,
                               processorDataResponse(payload, engine, cont),
                               onProcessorDataAndMetadata);

// C2S set processor type (sends back data and metadata)
typedef std::pair<int32_t, int32_t> setProcessorPayload_t;
inline void setProcessorType(const setProcessorPayload_t &whichToType, const engine::Engine &engine,
                             messaging::MessageController &cont)
{
    const auto &[w, id] = whichToType;
    auto addr = engine.getSelectionManager()->getSelectedZone();

    if (addr.has_value() && addr->part >= 0 && addr->group >= 0 && addr->zone >= 0)
    {
        cont.scheduleAudioThreadCallback(
            [a = *addr, which = w, type = id](auto &e) {
                const auto &z = e.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone);
                z->setProcessorType(which, (dsp::processor::ProcessorType)type);
            },
            [a = *addr, which = w](const auto &engine) {
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
}
CLIENT_TO_SERIAL(SetSelectedProcessorType, c2s_set_processor_type, setProcessorPayload_t,
                 setProcessorType(payload, engine, cont));

// C2S set processor storage
typedef std::pair<int32_t, dsp::processor::ProcessorStorage> setProcessorStoragePayload_t;
inline void setProcessorStorage(const setProcessorStoragePayload_t &payload,
                                const engine::Engine &engine, messaging::MessageController &cont)
{
    const auto &[w, st] = payload;
    auto addr = engine.getSelectionManager()->getSelectedZone();

    if (addr.has_value() && addr->part >= 0 && addr->group >= 0 && addr->zone >= 0)
    {
        cont.scheduleAudioThreadCallback([a = *addr, which = w, storage = st](auto &e) {
            const auto &z = e.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone);
            z->processorStorage[which] = storage;
        });
    }
}
CLIENT_TO_SERIAL(SetSelectedProcessorStorage, c2s_update_single_processor_data,
                 setProcessorStoragePayload_t, setProcessorStorage(payload, engine, cont));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_PROCESSOR_MESSAGES_H
