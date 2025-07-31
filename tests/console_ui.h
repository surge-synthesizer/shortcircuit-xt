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

#ifndef CONSOLE_UI_H
#define CONSOLE_UI_H

#include <memory>
#include <thread>
#include <atomic>

#include "utils.h"
#include "engine/engine.h"
#include "messaging/client/selection_messages.h"
#include "messaging/messaging.h"
#include "selection/selection_manager.h"

namespace scxt::tests
{
struct ConsoleUI
{
    bool logMessages{false};

    messaging::MessageController &msgCont;
    const sample::SampleManager &sampleManager;
    infrastructure::DefaultsProvider &defaultsProvider;
    const scxt::browser::Browser &browser;
    const engine::Engine::SharedUIMemoryState &sharedUiMemoryState;

    ConsoleUI(messaging::MessageController &e, infrastructure::DefaultsProvider &d,
              const sample::SampleManager &s, const scxt::browser::Browser &b,
              const engine::Engine::SharedUIMemoryState &state);
    ~ConsoleUI();

#define ON_STUB                                                                                    \
    {                                                                                              \
        if (logMessages)                                                                           \
            SCLOG_WFUNC("Message");                                                                \
    }

    // Serialization to Client Messages
    void onErrorFromEngine(const scxt::messaging::client::s2cError_t &) ON_STUB;
    void onEngineStatus(const engine::Engine::EngineStatusMessage &e) ON_STUB;
    void
    onMappingUpdated(const scxt::messaging::client::mappingSelectedZoneViewResposne_t &) ON_STUB;
    void
    onSamplesUpdated(const scxt::messaging::client::sampleSelectedZoneViewResposne_t &) ON_STUB;
    void onStructureUpdated(const engine::Engine::pgzStructure_t &) ON_STUB;
    void onGroupOrZoneEnvelopeUpdated(
        const scxt::messaging::client::adsrViewResponsePayload_t &payload) ON_STUB;
    void onGroupOrZoneProcessorDataAndMetadata(
        const scxt::messaging::client::processorDataResponsePayload_t &d) ON_STUB;
    void onZoneProcessorDataMismatch(
        const scxt::messaging::client::processorMismatchPayload_t &) ON_STUB;

    void onZoneVoiceMatrixMetadata(const scxt::voice::modulation::voiceMatrixMetadata_t &) ON_STUB;
    void onZoneVoiceMatrix(const scxt::voice::modulation::Matrix::RoutingTable &) ON_STUB;

    void onGroupMatrixMetadata(const scxt::modulation::groupMatrixMetadata_t &) ON_STUB;
    void onGroupMatrix(const scxt::modulation::GroupMatrix::RoutingTable &) ON_STUB;

    void onGroupTriggerConditions(const scxt::engine::GroupTriggerConditions &) ON_STUB;

    void onGroupOrZoneModulatorStorageUpdated(
        const scxt::messaging::client::indexedModulatorStorageUpdate_t &) ON_STUB;
    void onGroupOrZoneMiscModStorageUpdated(
        const scxt::messaging::client::gzMiscStorageUpdate_t &) ON_STUB;
    void onZoneOutputInfoUpdated(const scxt::messaging::client::zoneOutputInfoUpdate_t &p) ON_STUB;
    void
    onGroupOutputInfoUpdated(const scxt::messaging::client::groupOutputInfoUpdate_t &p) ON_STUB;

    void onGroupZoneMappingSummary(const scxt::engine::Part::zoneMappingSummary_t &) ON_STUB;
    void onSelectionState(const scxt::messaging::client::selectedStateMessage_t &) ON_STUB;
    void onSelectedPart(const int16_t) ON_STUB;

    void onPartConfiguration(const scxt::messaging::client::partConfigurationPayload_t &) ON_STUB;

    void
    onOtherTabSelection(const scxt::selection::SelectionManager::otherTabSelection_t &p) ON_STUB;

    void onBusOrPartEffectFullData(const scxt::messaging::client::busEffectFullData_t &) ON_STUB;
    void onBusOrPartSendData(const scxt::messaging::client::busSendData_t &) ON_STUB;

    void onBrowserRefresh(const bool) ON_STUB;
    void onBrowserQueueLengthRefresh(const std::pair<int32_t, int32_t>) ON_STUB;

    void onDebugInfoGenerated(const scxt::messaging::client::debugResponse_t &) ON_STUB;

    void
    onAllProcessorDescriptions(const std::vector<dsp::processor::ProcessorDescription> &v) ON_STUB;

    void
    onActivityNotification(const scxt::messaging::client::activityNotificationPayload_t &) ON_STUB;

    void onMacroFullState(const scxt::messaging::client::macroFullState_t &) ON_STUB;
    void onMacroValue(const scxt::messaging::client::macroValue_t &) ON_STUB;

    void
    onMissingResolutionWorkItemList(const std::vector<engine::MissingResolutionWorkItem> &) ON_STUB;

    void stepUI();

  private:
    std::mutex callbackMutex;
    std::queue<std::string> callbackQueue;
    void drainQueue();
}; // namespace scxt::tests

} // namespace scxt::tests

#endif // CONSOLE_UI_H
