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

#include "SCXTEditor.h"

#include "PlayScreen.h"
#include "engine/engine.h"
#include "messaging/client/selection_messages.h"
#include "selection/selection_manager.h"
#include "sst/jucegui/style/StyleSheet.h"

#include "infrastructure/user_defaults.h"

#include "HeaderRegion.h"
#include "MultiScreen.h"
#include "MixerScreen.h"
#include "connectors/SCXTStyleSheetCreator.h"
#include "AboutScreen.h"
#include "LogScreen.h"
#include "SCXTJuceLookAndFeel.h"
#include "widgets/Tooltip.h"

namespace scxt::ui
{

SCXTEditor::SCXTEditor(messaging::MessageController &e, infrastructure::DefaultsProvider &d,
                       const sample::SampleManager &s,
                       const engine::Engine::SharedUIMemoryState &st)
    : msgCont(e), sampleManager(s), defaultsProvider(d), sharedUiMemoryState(st)
{
    sst::jucegui::style::StyleSheet::initializeStyleSheets([]() {});

    // TODO: Obviously expand this
    auto sn = defaultsProvider.getUserDefaultValue(infrastructure::skinName, "builtin.DARK");
    if (sn == "builtin.LIGHT")
    {
        setStyle(connectors::SCXTStyleSheetCreator::setup(sst::jucegui::style::StyleSheet::LIGHT));
    }
    else
    {
        setStyle(connectors::SCXTStyleSheetCreator::setup());
    }

    // TODO what happens with two windows open when I go away?
    lnf = std::make_unique<SCXTJuceLookAndFeel>();
    juce::LookAndFeel::setDefaultLookAndFeel(lnf.get());

    idleTimer = std::make_unique<IdleTimer>(this);
    idleTimer->startTimer(1000 / 60);

    toolTip = std::make_unique<widgets::Tooltip>();
    addChildComponent(*toolTip);

    namespace cmsg = scxt::messaging::client;
    msgCont.registerClient("SCXTEditor", [this](auto &s) {
        {
            // Remember this runs on the serialization thread so needs to be thread safe
            std::lock_guard<std::mutex> g(callbackMutex);
            callbackQueue.push(s);
        }
        juce::MessageManager::callAsync([this]() { drainCallbackQueue(); });
    });

    headerRegion = std::make_unique<HeaderRegion>(this);
    addAndMakeVisible(*headerRegion);

    multiScreen = std::make_unique<MultiScreen>(this);
    addAndMakeVisible(*multiScreen);

    mixerScreen = std::make_unique<MixerScreen>(this);
    addChildComponent(*mixerScreen);

    playScreen = std::make_unique<PlayScreen>();
    addChildComponent(*playScreen);

    aboutScreen = std::make_unique<AboutScreen>(this);
    addChildComponent(*aboutScreen);

    logScreen = std::make_unique<LogScreen>(this);
    addChildComponent(*logScreen);

    setStyle(style());

    auto zfi = defaultsProvider.getUserDefaultValue(infrastructure::DefaultKeys::zoomLevel, 100);
    setZoomFactor(zfi * 0.01);
}

SCXTEditor::~SCXTEditor() noexcept
{
    idleTimer->stopTimer();
    msgCont.unregisterClient();
}

void SCXTEditor::setActiveScreen(ActiveScreen s)
{
    activeScreen = s;
    aboutScreen->setVisible(false);
    logScreen->setVisible(false);
    switch (s)
    {
    case MULTI:
        multiScreen->setVisible(true);
        mixerScreen->setVisible(false);
        playScreen->setVisible(false);
        resized();
        break;

    case MIXER:
        multiScreen->setVisible(false);
        mixerScreen->setVisible(true);
        playScreen->setVisible(false);
        resized();
        break;

    case PLAY:
        multiScreen->setVisible(false);
        mixerScreen->setVisible(false);
        playScreen->setVisible(true);
        resized();
        break;
    }
    repaint();
}

void SCXTEditor::showAboutOverlay()
{
    aboutScreen->toFront(true);
    aboutScreen->setVisible(true);
    logScreen->setVisible(false);
    resized();
}

void SCXTEditor::showLogOverlay()
{
    logScreen->toFront(true);
    logScreen->setVisible(true);
    aboutScreen->setVisible(false);
    resized();
}

void SCXTEditor::resized()
{
    static constexpr uint32_t headerHeight{40};
    headerRegion->setBounds(0, 0, getWidth(), headerHeight);

    if (multiScreen->isVisible())
        multiScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
    if (mixerScreen->isVisible())
        mixerScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
    if (playScreen->isVisible())
        playScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
    if (aboutScreen->isVisible())
        aboutScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
    if (logScreen->isVisible())
        logScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
}

void SCXTEditor::idle()
{
    /*
     * The SCXT idle is just used for polling the shared memory items,
     * not for handling events which the message controller does
     * immediately
     */
    if (sharedUiMemoryState.voiceDisplayStateWriteCounter != lastVoiceDisplayWriteCounter)
    {
        lastVoiceDisplayWriteCounter = sharedUiMemoryState.voiceDisplayStateWriteCounter;
        headerRegion->setVoiceCount(sharedUiMemoryState.voiceCount);

        if (multiScreen->isVisible())
            multiScreen->onVoiceInfoChanged();
    }

    headerRegion->setVULevel(sharedUiMemoryState.busVULevels[0][0],
                             sharedUiMemoryState.busVULevels[0][1]);

    if (mixerScreen->isVisible())
    {
        mixerScreen->setVULevelForBusses(sharedUiMemoryState.busVULevels);
    }
}

void SCXTEditor::drainCallbackQueue()
{
    namespace cmsg = scxt::messaging::client;

    bool itemsToDrain{true};
    while (itemsToDrain)
    {
        itemsToDrain = false;
        std::string qmsg;
        {
            std::lock_guard<std::mutex> g(callbackMutex);
            if (!callbackQueue.empty())
            {
                qmsg = callbackQueue.front();
                itemsToDrain = true;
                callbackQueue.pop();
            }
        }
        if (itemsToDrain)
        {
            assert(msgCont.threadingChecker.isClientThread());
            cmsg::clientThreadExecuteSerializationMessage(qmsg, this);
            inboundMessageCount++;
            if (inboundMessageCount % 100 == 0)
            {
                SCLOG("Serial -> Client Message Count: " << inboundMessageCount);
            }
        }
    }
}

void SCXTEditor::doSelectionAction(const selection::SelectionManager::ZoneAddress &a,
                                   bool selecting, bool distinct, bool asLead)
{
    namespace cmsg = scxt::messaging::client;
    currentLeadSelection = a;
    sendToSerialization(cmsg::DoSelectAction(selection::SelectionManager::SelectActionContents{
        a.part, a.group, a.zone, selecting, distinct, asLead}));
    repaint();
}

void SCXTEditor::doMultiSelectionAction(
    const std::vector<selection::SelectionManager::SelectActionContents> &p)
{
    namespace cmsg = scxt::messaging::client;
    sendToSerialization(cmsg::DoMultiSelectAction(p));
    repaint();
}

bool SCXTEditor::isInterestedInFileDrag(const juce::StringArray &files)
{
    // TODO be more parsimonious
    return files.size() == 1;
}

void SCXTEditor::filesDropped(const juce::StringArray &files, int, int)
{
    assert(files.size() == 1);
    namespace cmsg = scxt::messaging::client;
    sendToSerialization(cmsg::AddSample(std::string{(const char *)(files[0].toUTF8())}));
}

void SCXTEditor::showTooltip(const juce::Component &relativeTo)
{
    auto fb = getLocalArea(&relativeTo, relativeTo.getLocalBounds());
    toolTip->resetSizeFromData();
    toolTip->setVisible(true);
    toolTip->toFront(false);
    toolTip->setBounds(fb.getWidth() + fb.getX(), fb.getY(), toolTip->getWidth(),
                       toolTip->getHeight());
}

void SCXTEditor::hideTooltip() { toolTip->setVisible(false); }

void SCXTEditor::setTooltipContents(const std::string &title, const std::vector<std::string> &data)
{
    toolTip->setTooltipTitleAndData(title, data);
}

void SCXTEditor::parentHierarchyChanged() { setZoomFactor(zoomFactor); }

void SCXTEditor::setZoomFactor(float zf)
{
    zoomFactor = zf;
    setTransform(juce::AffineTransform().scaled(zoomFactor));
    defaultsProvider.updateUserDefaultValue(infrastructure::DefaultKeys::zoomLevel,
                                            zoomFactor * 100);
    if (onZoomChanged)
        onZoomChanged(zoomFactor);
}

} // namespace scxt::ui