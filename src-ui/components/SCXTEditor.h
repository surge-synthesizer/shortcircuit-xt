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

#ifndef SCXT_SRC_UI_COMPONENTS_SCXTEDITOR_H
#define SCXT_SRC_UI_COMPONENTS_SCXTEDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "messaging/messaging.h"
#include "sst/jucegui/components/HSlider.h"
#include "datamodel/adsr_storage.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "messaging/client/zone_messages.h"

namespace scxt::ui
{

struct HeaderRegion;
struct MultiScreen;
struct SendFXScreen;
struct AboutScreen;

struct SCXTEditor : sst::jucegui::components::WindowPanel, juce::FileDragAndDropTarget
{
    // The message controller is needed to communicate
    messaging::MessageController &msgCont;

    /* In theory we could have a copy of all the samples in the UI but
     * their lifetime is well known and their json load is big so for now
     * lets make the simplifyign assumption that we get a const reference to
     * the sample manager for our engine.
     */
    const sample::SampleManager &sampleManager;

    struct IdleTimer : juce::Timer
    {
        SCXTEditor *editor{nullptr};
        IdleTimer(SCXTEditor *ed) : editor(ed) {}
        void timerCallback() override { editor->idle(); }
    };
    std::unique_ptr<IdleTimer> idleTimer;

    std::unique_ptr<HeaderRegion> headerRegion;
    std::unique_ptr<MultiScreen> multiScreen;
    std::unique_ptr<SendFXScreen> sendFxScreen;
    std::unique_ptr<AboutScreen> aboutScreen;

    // TODO fix me with a proper tooltip type
    struct Tooltip : juce::Component
    {
        void paint(juce::Graphics &g);
        void setTooltipText(const std::string &t)
        {
            tooltipText = t;
            repaint();
        }
        std::string tooltipText{};
    };
    std::unique_ptr<Tooltip> toolTip;

    SCXTEditor(messaging::MessageController &e, const sample::SampleManager &s);
    virtual ~SCXTEditor() noexcept;

    enum ActiveScreen
    {
        MULTI,
        SEND_FX,
        ABOUT
    };
    void setActiveScreen(ActiveScreen s);

    void resized() override;

    // File Drag and Drop Interface
    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &, int, int) override;

    // Deal with message queues.
    void idle();
    void drainCallbackQueue();

    // Serialization to Client Messages
    void onErrorFromEngine(const scxt::messaging::client::s2cError_t &);
    void onVoiceCount(const uint32_t &v);
    void onEngineStatus(const engine::Engine::EngineStatusMessage &e);
    void onEnvelopeUpdated(const scxt::messaging::client::adsrViewResponsePayload_t &);
    void onMappingUpdated(const scxt::messaging::client::mappingSelectedZoneViewResposne_t &);
    void onSamplesUpdated(const scxt::messaging::client::sampleSelectedZoneViewResposne_t &);
    void onStructureUpdated(const engine::Engine::pgzStructure_t &);
    void
    onProcessorDataAndMetadata(const scxt::messaging::client::processorDataResponsePayload_t &);
    void onZoneVoiceMatrixMetadata(const scxt::modulation::voiceModMatrixMetadata_t &);
    void onZoneVoiceMatrix(const scxt::modulation::VoiceModMatrix::routingTable_t &);
    void onZoneLfoUpdated(const scxt::messaging::client::indexedLfoUpdate_t &);

    void onGroupZoneMappingSummary(const scxt::engine::Group::zoneMappingSummary_t &);
    void onSingleSelection(const scxt::selection::SelectionManager::ZoneAddress &);

    std::vector<dsp::processor::ProcessorDescription> allProcessors;
    void onAllProcessorDescriptions(const std::vector<dsp::processor::ProcessorDescription> &v)
    {
        allProcessors = v;
    }

    // Originate client to serialization messages
    void singleSelectItem(const selection::SelectionManager::ZoneAddress &);
    selection::SelectionManager::ZoneAddress currentSingleSelection;

    void showTooltip(const juce::Component &relativeTo);
    void hideTooltip();
    void setTooltipContents(const std::string &s);

    std::mutex callbackMutex;
    std::queue<std::string> callbackQueue;
    engine::Engine::EngineStatusMessage engineStatus;
};
} // namespace scxt::ui

#endif // SHORTCIRCUIT_SCXTEDITOR_H
