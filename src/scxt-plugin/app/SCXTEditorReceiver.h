/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_SCXTEDITORRECEIVER_H
#define SCXT_SRC_SCXT_PLUGIN_APP_SCXTEDITORRECEIVER_H

// Motivation
// ----------
// Every serialization->client message is handled by a typed callback (onFoo(payload)).
// Previously these lived directly on SCXTEditor, which forced SCXTEditor.h to
// expose every payload type. That meant SCXTEditor.h had to pull the full
// `client_messages.h` umbrella — and since SCXTEditor.h is included by ~36 UI panel
// translation units, every panel paid the parse cost of all 17 message headers
// (and everything they drag in).
//
// SCXTEditorReceiver is the dispatch target instead. SCXTEditor owns one via
// unique_ptr; the dispatch site in SCXTEditor.cpp passes `receiver.get()` to
// clientThreadExecuteSerializationMessage<Client>. Only this header (plus the
// two TUs that include it: SCXTEditor.cpp and SCXTEditorResponseHandlers.cpp)
// needs to see the message payload types.
//
// The receiver holds an `editor` reference and is friended by SCXTEditor so its
// method bodies can touch private editor state. No external code calls these
// methods directly — they're only invoked by the dispatcher.
//
// Adding a new s2c message:
//   1. Define the SERIAL_TO_CLIENT(...) macro in the appropriate
//      messaging/client/<area>_messages.h with the desired callback name.
//   2. Add a matching method declaration here.
//   3. Implement the method body in SCXTEditorResponseHandlers.cpp; use
//      `editor.<member>` to touch SCXTEditor state.

#include "messaging/client/client_messages.h"
#include "messaging/client/detail/client_serial_impl.h"
#include "engine/engine.h"
#include "selection/selection_manager.h"

namespace scxt::ui::app
{
struct SCXTEditor;

struct SCXTEditorReceiver
{
    SCXTEditor &editor;
    explicit SCXTEditorReceiver(SCXTEditor &e) : editor(e) {}

    // Serialization to Client Messages
    void onErrorFromEngine(const scxt::messaging::client::s2cError_t &);
    void onEngineStatus(const engine::Engine::EngineStatusMessage &e);
    void onMappingUpdated(const scxt::messaging::client::mappingSelectedZoneViewResposne_t &);
    void onSamplesUpdated(const scxt::messaging::client::sampleSelectedZoneViewResposne_t &);
    void onStructureUpdated(const engine::Engine::pgzStructure_t &);
    void
    onGroupOrZoneEnvelopeUpdated(const scxt::messaging::client::adsrViewResponsePayload_t &payload);
    void onGroupOrZoneProcessorDataAndMetadata(
        const scxt::messaging::client::processorDataResponsePayload_t &d);

    void onZoneVoiceMatrixMetadata(const scxt::voice::modulation::voiceMatrixMetadata_t &);
    void onZoneVoiceMatrix(const scxt::voice::modulation::Matrix::RoutingTable &);

    void onGroupMatrixMetadata(const scxt::modulation::groupMatrixMetadata_t &);
    void onGroupMatrix(const scxt::modulation::GroupMatrix::RoutingTable &);

    void onGroupTriggerConditions(const scxt::engine::GroupTriggerConditions &);

    void onGroupOrZoneModulatorStorageUpdated(
        const scxt::messaging::client::indexedModulatorStorageUpdate_t &);
    void onGroupOrZoneMiscModStorageUpdated(const scxt::messaging::client::gzMiscStorageUpdate_t &);
    void
    onGroupOrZoneAudioModStorageUpdated(const scxt::messaging::client::gzAudioModStorageUpdate_t &);
    void onZoneOutputInfoUpdated(const scxt::messaging::client::zoneOutputInfoUpdate_t &p);
    void onGroupOutputInfoUpdated(const scxt::messaging::client::groupOutputInfoUpdate_t &p);

    void onGroupZoneMappingSummary(const scxt::engine::Part::zoneMappingSummary_t &);
    void onSelectionState(const scxt::messaging::client::selectedStateMessage_t &);
    void onSelectedPart(const int16_t);

    void onPartConfiguration(const scxt::messaging::client::partConfigurationPayload_t &);

    void onOtherTabSelection(const scxt::selection::SelectionManager::otherTabSelection_t &p);

    void onBusOrPartEffectFullData(const scxt::messaging::client::busEffectFullData_t &);
    void onBusOrPartSendData(const scxt::messaging::client::busSendData_t &);

    void onBrowserRefresh(const bool);
    void onBrowserQueueLengthRefresh(const std::pair<int32_t, int32_t>);

    void onDebugInfoGenerated(const scxt::messaging::client::debugResponse_t &);

    void onColormap(const std::string &);

    void onAllProcessorDescriptions(const std::vector<dsp::processor::ProcessorDescription> &v);

    void onActivityNotification(const scxt::messaging::client::activityNotificationPayload_t &);

    void onTuningStatus(const scxt::messaging::client::tuningStatusPayload_t &);

    void onMacroFullState(const scxt::messaging::client::macroFullState_t &);
    void onMacroValue(const scxt::messaging::client::macroValue_t &);

    void onMissingResolutionWorkItemList(const std::vector<engine::MissingResolutionWorkItem> &);

    void onClipboardType(const scxt::engine::Clipboard::ContentType &s);

    void onMpeTuningAwarenessFromEngine(bool);
    void onPitchBendTuningAwarenessFromEngine(bool);
    void onOmniFlavorFromEngine(std::pair<int, bool> f);
};
} // namespace scxt::ui::app

#endif // SCXT_SRC_SCXT_PLUGIN_APP_SCXTEDITORRECEIVER_H
