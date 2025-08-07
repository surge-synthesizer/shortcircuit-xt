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

#ifndef SCXT_SRC_UI_APP_SCXTEDITOR_H
#define SCXT_SRC_UI_APP_SCXTEDITOR_H

#include <clap/clap.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <type_traits>

#include "engine/engine.h"
#include "messaging/client/selection_messages.h"
#include "messaging/messaging.h"
#include "selection/selection_manager.h"
#include "sst/jucegui/components/HSlider.h"
#include "sst/jucegui/components/ToolTip.h"
#include "sst/jucegui/accessibility/FocusDebugger.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/screens/ScreenHolder.h"
#include "sst/jucegui/style/JUCELookAndFeelAdapter.h"

#include "sst/basic-blocks/dsp/RNG.h"
#include "messaging/client/zone_messages.h"
#include "browser/browser.h"

#include "SCXTEditorDataCache.h"
#include "HasEditor.h"

#include "theme/ThemeApplier.h"

#include "connectors/PayloadDataAttachment.h"

namespace melatonin
{
class Inspector;
}
namespace sst::jucegui::components
{
struct HasDiscreteParamMenuBuilder;
}
namespace scxt::ui::app
{

struct KeyBindings;

namespace shared
{
struct HeaderRegion;
}
namespace edit_screen
{
struct EditScreen;
}
namespace mixer_screen
{
struct MixerScreen;
}
namespace play_screen
{
struct PlayScreen;
}
namespace missing_resolution
{
struct MissingResolutionScreen;
}
namespace other_screens
{
struct WelcomeScreen;
struct AboutScreen;
struct LogScreen;
} // namespace other_screens

struct SCXTEditor : sst::jucegui::components::WindowPanel,
                    juce::DragAndDropContainer,
                    sst::jucegui::screens::ScreenHolder<SCXTEditor>
{
    // The message controller is needed to communicate
    messaging::MessageController &msgCont;

    /* In theory, we could have a copy of all the samples in the UI but
     * their lifetime is well known and their json load is big so for now
     * lets make the simplifying assumption that we get a const reference to
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

    /*
     * This is an object responsible for theme and color management
     */
    theme::ThemeApplier themeApplier;
    juce::Colour themeColor(scxt::ui::theme::ColorMap::Colors, float alpha = 1.f) const;
    void resetColorsFromUserPreferences();

    /*
     * See the comment in SCXTEditorDataCache for more on this class. Shared space to
     * cache the UI data types
     */
    SCXTEditorDataCache editorDataCache;

    sst::basic_blocks::dsp::RNG rng;

    static constexpr int edWidth{1186}, edHeight{816};

    std::unique_ptr<sst::jucegui::style::LookAndFeelManager> lnf;
    void onStyleChanged() override;

    struct IdleTimer : juce::Timer
    {
        SCXTEditor *editor{nullptr};
        IdleTimer(SCXTEditor *ed) : editor(ed) {}
        void timerCallback() override { editor->idle(); }
    };
    std::unique_ptr<IdleTimer> idleTimer;

    std::unique_ptr<shared::HeaderRegion> headerRegion;
    std::unique_ptr<edit_screen::EditScreen> editScreen;
    std::unique_ptr<mixer_screen::MixerScreen> mixerScreen;
    std::unique_ptr<play_screen::PlayScreen> playScreen;
    std::unique_ptr<other_screens::AboutScreen> aboutScreen;
    std::unique_ptr<other_screens::WelcomeScreen> welcomeScreen;
    std::unique_ptr<other_screens::LogScreen> logScreen;
    std::unique_ptr<missing_resolution::MissingResolutionScreen> missingResolutionScreen;
    bool hasMissingSamples{false};
    void showMissingResolutionScreen();

    std::unique_ptr<sst::jucegui::components::ToolTip> toolTip;

    std::unique_ptr<sst::jucegui::accessibility::FocusDebugger> focusDebugger;

    SCXTEditor(messaging::MessageController &e, infrastructure::DefaultsProvider &d,
               const sample::SampleManager &s, const scxt::browser::Browser &b,
               const engine::Engine::SharedUIMemoryState &state);
    virtual ~SCXTEditor() noexcept;

    enum ActiveScreen
    {
        PLAY,
        MULTI,
        MIXER,
    } activeScreen{MULTI};
    void setActiveScreen(ActiveScreen s);
    void showAboutOverlay();
    void showLogOverlay();
    void showWelcomeOverlay();
    int32_t checkWelcomeCountdown{20};

    float zoomFactor{1.f};
    void setZoomFactor(float zoomFactor); // 1.0 == 100%
    std::function<void(float)> onZoomChanged{nullptr};

    void resized() override;
    void parentHierarchyChanged() override;

    // Deal with message queues.
    void idle();
    void drainCallbackQueue();
    uint64_t inboundMessageCount{0};
    uint64_t inboundMessageBytes{0};

    // Popup Menu Options
    juce::PopupMenu::Options defaultPopupMenuOptions(juce::Component *on = nullptr)
    {
        auto r = juce::PopupMenu::Options().withParentComponent(this);
        if (on)
            r = r.withTargetComponent(on);
        return r;
    }
    void configureHasDiscreteMenuBuilder(sst::jucegui::components::HasDiscreteParamMenuBuilder *);
    void popupMenuForContinuous(sst::jucegui::components::ContinuousParamEditor *e);
    // Sometimes we don't want a popup. Se #1669
    bool supressPopupMenuForContinuous(sst::jucegui::components::ContinuousParamEditor *e) const;

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
    void onZoneOutputInfoUpdated(const scxt::messaging::client::zoneOutputInfoUpdate_t &p);
    void onGroupOutputInfoUpdated(const scxt::messaging::client::groupOutputInfoUpdate_t &p);

    void onGroupZoneMappingSummary(const scxt::engine::Part::zoneMappingSummary_t &);
    void onSelectionState(const scxt::messaging::client::selectedStateMessage_t &);
    void onSelectedPart(const int16_t);
    int16_t getSelectedPart() const;

    void onPartConfiguration(const scxt::messaging::client::partConfigurationPayload_t &);
    std::array<scxt::engine::Part::PartConfiguration, scxt::numParts> partConfigurations;

    selection::SelectionManager::otherTabSelection_t otherTabSelection;
    void onOtherTabSelection(const scxt::selection::SelectionManager::otherTabSelection_t &p);
    std::string queryTabSelection(const std::string &k);
    void setTabSelection(const std::string &k, const std::string &t);

    void onBusOrPartEffectFullData(const scxt::messaging::client::busEffectFullData_t &);
    void onBusOrPartSendData(const scxt::messaging::client::busSendData_t &);

    void onBrowserRefresh(const bool);
    void onBrowserQueueLengthRefresh(const std::pair<int32_t, int32_t>);

    void onDebugInfoGenerated(const scxt::messaging::client::debugResponse_t &);

    std::vector<dsp::processor::ProcessorDescription> allProcessors;
    void onAllProcessorDescriptions(const std::vector<dsp::processor::ProcessorDescription> &v)
    {
        allProcessors = v;
    }

    void onActivityNotification(const scxt::messaging::client::activityNotificationPayload_t &);

    std::array<std::array<scxt::engine::Macro, scxt::macrosPerPart>, scxt::numParts> macroCache;
    void onMacroFullState(const scxt::messaging::client::macroFullState_t &);
    void onMacroValue(const scxt::messaging::client::macroValue_t &);

    void onMissingResolutionWorkItemList(const std::vector<engine::MissingResolutionWorkItem> &);

    // Originate client to serialization messages
    void doSelectionAction(const selection::SelectionManager::ZoneAddress &, bool selecting,
                           bool distinct, bool asLead);
    void doSelectionAction(const selection::SelectionManager::SelectActionContents &);
    void doSelectionAction(const std::vector<selection::SelectionManager::SelectActionContents> &);
    std::optional<selection::SelectionManager::ZoneAddress> currentLeadZoneSelection;
    std::optional<selection::SelectionManager::ZoneAddress> currentLeadGroupSelection;
    selection::SelectionManager::selectedZones_t allZoneSelections;
    selection::SelectionManager::selectedZones_t allGroupSelections;
    int16_t selectedPart{0};

    std::set<int> groupsWithSelectedZones;
    bool isAnyZoneFromGroupSelected(int groupIdx)
    {
        auto lzig =
            (currentLeadZoneSelection.has_value() && currentLeadZoneSelection->group == groupIdx);
        auto ing = groupsWithSelectedZones.find(groupIdx) != groupsWithSelectedZones.end();
        return lzig || ing;
    }

    bool isSelected(const selection::SelectionManager::ZoneAddress &a)
    {
        auto islz = currentLeadZoneSelection.has_value() && *currentLeadZoneSelection == a;
        auto ing = allZoneSelections.find(a) != allZoneSelections.end();
        return islz || ing;
    }

    void showTooltip(const juce::Component &relativeTo);
    void showTooltip(const juce::Component &relativeTo, const juce::Point<int> &internalPosition);
    void repositionTooltip(const juce::Component &relativeTo,
                           const juce::Point<int> &internalPosition);
    void hideTooltip();
    void setTooltipContents(const std::string &title,
                            const std::vector<sst::jucegui::components::ToolTip::Row> &rows);
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

    std::function<void()> makeComingSoon(const std::string &feature = "This feature");

    void promptOKCancel(
        const std::string &title, const std::string &message, std::function<void()> onOK,
        std::function<void()> onCancel = []() {});
    void displayError(const std::string &title, const std::string &message);

    /*
     * Items to deal with the shared memory reads
     */
    int64_t lastVoiceDisplayWriteCounter{-1};
    float lastProcessMemoryInMegabytes{0};

    friend struct HasEditor;

    // Keyboard shortcuts
    std::unique_ptr<KeyBindings> keyBindings;
    bool keyPressed(const juce::KeyPress &key) override;

    void switchGroupOrZoneFocus();

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

#if HAS_MELATONIN_INSPECTOR
    std::unique_ptr<melatonin::Inspector> melatoninInspector;
#endif

  public:
    std::unique_ptr<juce::FileChooser> fileChooser;

    const clap_host_t *clapHost{nullptr};
};

template <typename T> inline void HasEditor::sendToSerialization(const T &msg)
{
    editor->sendToSerialization(msg);
}

template <typename T> inline void HasEditor::updateValueTooltip(const T &at)
{
    editor->setTooltipContents(at.label, at.getValueAsString());
}

template <typename W, typename A>
inline void HasEditor::setupWidgetForValueTooltip(W *w, const A &a)
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
    w->onIdleHoverEnd = [this]() { editor->hideTooltip(); };
    w->onWheelEditOccurred = [this, &slRef = *w] { slRef.immediatelyInitiateIdleAction(1000); };
    if constexpr (std::is_base_of_v<sst::jucegui::components::ContinuousParamEditor,
                                    std::remove_pointer_t<W>>)
    {
        w->onPopupMenu = [this, q = juce::Component::SafePointer(w)](auto &mods) {
            editor->hideTooltip();
            editor->popupMenuForContinuous(q);
        };
    }
}

template <typename W, typename A>
inline void HasEditor::setupIntAttachedWidgetForValueMenu(W *w, const A &a)
{
    w->onPopupMenu = [this, q = juce::Component::SafePointer(w)](auto &mods) {
        auto qc = dynamic_cast<sst::jucegui::components::ContinuousParamEditor *>(q.getComponent());
        if (qc)
            editor->popupMenuForContinuous(qc);
    };
}

template <typename P, typename A> void HasEditor::addSubscription(const P &p, A &a)
{
    auto *dc = &editor->editorDataCache;
    auto dce = dc + sizeof(SCXTEditorDataCache);
    if ((size_t)&p >= (size_t)dc && (size_t)&p < (size_t)dce)
    {
        auto *edCopy = editor;
        auto *vp = (void *)&p;

        connectors::addGuiStep(*a, [edCopy, vp, f = a.get()](auto &) {
            edCopy->editorDataCache.fireSubscription(vp, f);
        });
        editor->editorDataCache.addSubscription(vp, sizeof(typename A::element_type::value_t),
                                                a.get());
    }
}
} // namespace scxt::ui::app

#endif // SCXT_SRC_UI_COMPONENTS_SCXTEDITOR_H
