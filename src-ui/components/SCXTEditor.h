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

#ifndef SCXT_UI_COMPONENTS_SCXTEDITOR_H
#define SCXT_UI_COMPONENTS_SCXTEDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "messaging/messaging.h"
#include "sst/jucegui/components/HSlider.h"
#include "datamodel/adsr_storage.h"
#include "sst/jucegui/components/WindowPanel.h"

namespace scxt::ui
{

struct HeaderRegion;
struct MultiScreen;
struct SendFXScreen;

struct SCXTEditor : sst::jucegui::components::WindowPanel, juce::FileDragAndDropTarget
{
    messaging::MessageController &msgCont;

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

    // TODO fix me with a proper tooltip type
    std::unique_ptr<juce::Component> toolTip;

    SCXTEditor(messaging::MessageController &e);
    virtual ~SCXTEditor() noexcept;

    enum ActiveScreen
    {
        MULTI,
        SEND_FX
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
    void onVoiceCount(const uint32_t &v);
    void onEnvelopeUpdated(const int &which, const bool &active, const datamodel::AdsrStorage &);
    void onStructureUpdated(const engine::Engine::pgzStructure_t &);

    // Originate client to serialization messages
    void singleSelectItem(const selection::SelectionManager::ZoneAddress &);

    void showTooltip(const juce::Component &relativeTo);
    void hideTooltip();
    void setTooltipContents(const std::string &s);

    std::mutex callbackMutex;
    std::queue<std::string> callbackQueue;
};
} // namespace scxt::ui

#endif // SHORTCIRCUIT_SCXTEDITOR_H
