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

#ifndef SCXT_SRC_UI_COMPONENTS_SCXTEDITOR_H
#define SCXT_SRC_UI_COMPONENTS_SCXTEDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <type_traits>

#include "SCXTJuceLookAndFeel.h"
#include "engine/engine.h"
#include "engine/patch.h"
#include "messaging/client/selection_messages.h"
#include "messaging/messaging.h"
#include "selection/selection_manager.h"
#include "sst/jucegui/components/HSlider.h"
#include "sst/jucegui/components/ToolTip.h"
#include "sst/jucegui/accessibility/FocusDebugger.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "messaging/client/zone_messages.h"
#include "infrastructure/rng_gen.h"
#include "browser/browser.h"

#include "HasEditor.h"

namespace sst::jucegui::components
{
struct HasDiscreteParamMenuBuilder;
}
namespace scxt::ui
{

struct HeaderRegion;
struct MultiScreen;
struct MixerScreen;
struct AboutScreen;
struct PlayScreen;
struct LogScreen;
struct SCXTJuceLookAndFeel;

struct SCXTEditor : sst::jucegui::components::WindowPanel,
                    juce::FileDragAndDropTarget,
                    juce::DragAndDropContainer
{
    // The message controller is needed to communicate
    messaging::MessageController &msgCont;

    /* In theory we could have a copy of all the samples in the UI but
     * their lifetime is well known and their json load is big so for now
     * lets make the simplifyign assumption that we get a const reference to
     * the sample manager for our engine.
     */
    const sample::SampleManager &sampleManager;

    /*
     * We have a reference to the defaults provider from the engine
     */
    infrastructure::DefaultsProvider &defaultsProvider;

    /*
     * And the browser for read only functions. (Modifications go back by
     * the message controller)
     */
    const scxt::browser::Browser &browser;

    /* And finally we have shared state with the engine for display
     * things we don't want to stream. See the comment in engine.h
     * about this structure. Note this is a const & containing atomics,
     * and is not writable
     */
    const engine::Engine::SharedUIMemoryState &sharedUiMemoryState;

    infrastructure::RNGGen rngGen;

    static constexpr int edWidth{1186}, edHeight{816};

    std::unique_ptr<SCXTJuceLookAndFeel> lnf;

    struct IdleTimer : juce::Timer
    {
        SCXTEditor *editor{nullptr};
        IdleTimer(SCXTEditor *ed) : editor(ed) {}
        void timerCallback() override { editor->idle(); }
    };
    std::unique_ptr<IdleTimer> idleTimer;

    std::unique_ptr<HeaderRegion> headerRegion;
    std::unique_ptr<MultiScreen> multiScreen;
    std::unique_ptr<MixerScreen> mixerScreen;
    std::unique_ptr<PlayScreen> playScreen;
    std::unique_ptr<AboutScreen> aboutScreen;
    std::unique_ptr<LogScreen> logScreen;

    std::unique_ptr<sst::jucegui::components::ToolTip> toolTip;

    std::unique_ptr<sst::jucegui::accessibility::FocusDebugger> focusDebugger;

    SCXTEditor(messaging::MessageController &e, infrastructure::DefaultsProvider &d,
               const sample::SampleManager &s, const scxt::browser::Browser &b,
               const engine::Engine::SharedUIMemoryState &state);
    virtual ~SCXTEditor() noexcept;

    enum ActiveScreen
    {
        MULTI,
        MIXER,
        PLAY
    } activeScreen{MULTI};
    void setActiveScreen(ActiveScreen s);
    void showAboutOverlay();
    void showLogOverlay();

    float zoomFactor{1.f};
    void setZoomFactor(float zoomFactor); // 1.0 == 100%
    std::function<void(float)> onZoomChanged{nullptr};

    void resized() override;
    void parentHierarchyChanged() override;

    // File Drag and Drop Interface
    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &, int, int) override;

    // Deal with message queues.
    void idle();
    void drainCallbackQueue();
    uint64_t inboundMessageCount{0};
    uint64_t inboundMessageBytes{0};

    // Popup Menu Options
    juce::PopupMenu::Options defaultPopupMenuOptions()
    {
        auto r = juce::PopupMenu::Options().withParentComponent(this);
        return r;
    }
    void configureHasDiscreteMenuBuilder(sst::jucegui::components::HasDiscreteParamMenuBuilder *);

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
    void onZoneProcessorDataMismatch(const scxt::messaging::client::processorMismatchPayload_t &);

    void onZoneVoiceMatrixMetadata(const scxt::voice::modulation::voiceMatrixMetadata_t &);
    void onZoneVoiceMatrix(const scxt::voice::modulation::Matrix::RoutingTable &);

    void onGroupMatrixMetadata(const scxt::modulation::groupMatrixMetadata_t &);
    void onGroupMatrix(const scxt::modulation::GroupMatrix::RoutingTable &);

    void onGroupOrZoneModulatorStorageUpdated(
        const scxt::messaging::client::indexedModulatorStorageUpdate_t &);
    void onZoneOutputInfoUpdated(const scxt::messaging::client::zoneOutputInfoUpdate_t &p);
    void onGroupOutputInfoUpdated(const scxt::messaging::client::groupOutputInfoUpdate_t &p);

    void onGroupZoneMappingSummary(const scxt::engine::Part::zoneMappingSummary_t &);
    void onSelectionState(const scxt::messaging::client::selectedStateMessage_t &);

    void onMixerBusEffectFullData(const scxt::messaging::client::busEffectFullData_t &);
    void onMixerBusSendData(const scxt::messaging::client::busSendData_t &);

    void onBrowserRefresh(const bool);

    void onDebugInfoGenerated(const scxt::messaging::client::debugResponse_t &);

    std::vector<dsp::processor::ProcessorDescription> allProcessors;
    void onAllProcessorDescriptions(const std::vector<dsp::processor::ProcessorDescription> &v)
    {
        allProcessors = v;
    }

    // Originate client to serialization messages
    void doSelectionAction(const selection::SelectionManager::ZoneAddress &, bool selecting,
                           bool distinct, bool asLead);
    void doSelectionAction(const selection::SelectionManager::SelectActionContents &);
    void
    doMultiSelectionAction(const std::vector<selection::SelectionManager::SelectActionContents> &);
    std::optional<selection::SelectionManager::ZoneAddress> currentLeadZoneSelection;
    selection::SelectionManager::selectedZones_t allZoneSelections;
    selection::SelectionManager::selectedZones_t allGroupSelections;

    // TODO: Do we allow part multi-select? I think we don't
    std::set<int> groupsWithSelectedZones;
    bool isAnyZoneFromGroupSelected(int gidx)
    {
        return groupsWithSelectedZones.find(gidx) != groupsWithSelectedZones.end();
    }

    bool isSelected(const selection::SelectionManager::ZoneAddress &a)
    {
        return allZoneSelections.find(a) != allZoneSelections.end();
    }

    void showTooltip(const juce::Component &relativeTo);
    void hideTooltip();
    void setTooltipContents(const std::string &title, const std::vector<std::string> &display);
    void setTooltipContents(const std::string &title, const std::string &display)
    {
        setTooltipContents(title, std::vector<std::string>{display});
    }

    void showMainMenu();
    void addTuningMenu(juce::PopupMenu &into, bool addTitle = true);
    void addZoomMenu(juce::PopupMenu &into, bool addTitle = true);
    void addUIThemesMenu(juce::PopupMenu &p, bool addTitle = true);

    std::mutex callbackMutex;
    std::queue<std::string> callbackQueue;
    engine::Engine::EngineStatusMessage engineStatus;

    /*
     * Items to deal with the shared memory reads
     */
    int64_t lastVoiceDisplayWriteCounter{-1};
    float lastProcessMemoryInMegabytes{0};

    friend struct HasEditor;

  protected:
    template <typename T> void sendToSerialization(const T &msg)
    {
        scxt::messaging::client::clientSendToSerialization(msg, msgCont);
    }

    template <typename A> void beginEditNotifyEngine(const A &a)
    {
        sendToSerialization(scxt::messaging::client::BeginEdit{true});
    }
    template <typename A> void endEditNotifyEngine(const A &a)
    {
        sendToSerialization(scxt::messaging::client::EndEdit{true});
    }
};

template <typename T> inline void HasEditor::sendToSerialization(const T &msg)
{
    editor->sendToSerialization(msg);
}

template <typename T> inline void HasEditor::updateValueTooltip(const T &at)
{
    editor->setTooltipContents(at.label, at.description.valueToString(at.value).value_or("Error"));
}

template <typename W, typename A>
inline void HasEditor::setupWidgetForValueTooltip(const W &w, const A &a)
{
    w->onBeginEdit = [this, &slRef = *w, &atRef = *a]() {
        editor->showTooltip(slRef);
        updateValueTooltip(atRef);
        editor->beginEditNotifyEngine(atRef);
    };
    w->onEndEdit = [this, &atRef = *a]() {
        editor->hideTooltip();
        editor->endEditNotifyEngine(atRef);
    };
    w->onIdleHover = [this, &slRef = *w, &atRef = *a]() {
        editor->showTooltip(slRef);
        updateValueTooltip(atRef);
    };
    w->onIdleHoverEnd = [this, &atRef = *a]() { editor->hideTooltip(); };
}
} // namespace scxt::ui

#endif // SHORTCIRCUIT_SCXTEDITOR_H
