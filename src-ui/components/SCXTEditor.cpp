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

#include "sst/jucegui/style/StyleSheet.h"

#include "HeaderRegion.h"
#include "MultiScreen.h"
#include "SendFXScreen.h"
#include "connectors/SCXTStyleSheetCreator.h"
#include "AboutScreen.h"

namespace scxt::ui
{

void SCXTEditor::Tooltip::paint(juce::Graphics &g)
{
    g.setColour(juce::Colours::black.withAlpha(0.60f));
    g.fillRect(getLocalBounds());
    g.setColour(juce::Colour(0xFF, 0x90, 0x00));
    g.drawRect(getLocalBounds(), 2);

    g.setColour(juce::Colour(0xFF, 0x90, 0x00));
    g.setFont(juce::Font("Comic Sans MS", 14, juce::Font::plain));
    g.drawText(tooltipText, getLocalBounds().reduced(2), juce::Justification::centred);
}

SCXTEditor::SCXTEditor(messaging::MessageController &e, const sample::SampleManager &s)
    : msgCont(e), sampleManager(s)
{
    sst::jucegui::style::StyleSheet::initializeStyleSheets([]() {});
    setStyle(connectors::SCXTStyleSheetCreator::setup());

    idleTimer = std::make_unique<IdleTimer>(this);
    idleTimer->startTimer(1000 / 60);

    toolTip = std::make_unique<SCXTEditor::Tooltip>();
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

    sendFxScreen = std::make_unique<SendFXScreen>();
    addChildComponent(*sendFxScreen);

    aboutScreen = std::make_unique<AboutScreen>(this);
    addChildComponent(*aboutScreen);

    onStyleChanged();
}

SCXTEditor::~SCXTEditor() noexcept
{
    idleTimer->stopTimer();
    msgCont.unregisterClient();
}

void SCXTEditor::setActiveScreen(ActiveScreen s)
{
    switch (s)
    {
    case MULTI:
        multiScreen->setVisible(true);
        sendFxScreen->setVisible(false);
        aboutScreen->setVisible(false);
        resized();
        break;

    case SEND_FX:
        multiScreen->setVisible(false);
        sendFxScreen->setVisible(true);
        aboutScreen->setVisible(false);
        resized();
        break;

    case ABOUT:
        multiScreen->setVisible(false);
        sendFxScreen->setVisible(false);
        aboutScreen->setVisible(true);
        resized();
        break;
    }
}

void SCXTEditor::resized()
{
    static constexpr uint32_t headerHeight{40};
    headerRegion->setBounds(0, 0, getWidth(), headerHeight);

    if (multiScreen->isVisible())
        multiScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
    if (sendFxScreen->isVisible())
        sendFxScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
    if (aboutScreen->isVisible())
        aboutScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
}

void SCXTEditor::idle()
{
    // This drain queue should in theory not be needed
    // since the message recipient schedules a drain queue.
    // In fact it might be the case that we don't need an
    // idle at all in SCXT. Leave it here for now while I think
    // about it though.
    // drainCallbackQueue();
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
        }
    }
}

void SCXTEditor::singleSelectItem(const selection::SelectionManager::ZoneAddress &a)
{
    namespace cmsg = scxt::messaging::client;
    currentSingleSelection = a;
    SCDBGCOUT << __func__ << " " << SCD(a.part) << SCD(a.group) << SCD(a.zone) << std::endl;
    cmsg::clientSendToSerialization(cmsg::SingleSelectAddress(a), msgCont);
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
    cmsg::clientSendToSerialization(cmsg::AddSample(std::string{(const char *)(files[0].toUTF8())}),
                                    msgCont);
}

void SCXTEditor::showTooltip(const juce::Component &relativeTo)
{
    auto fb = getLocalArea(&relativeTo, relativeTo.getLocalBounds());
    toolTip->setVisible(true);
    toolTip->toFront(false);
    toolTip->setBounds(fb.getWidth() + fb.getX(), fb.getY(), 200, 30);
}

void SCXTEditor::hideTooltip() { toolTip->setVisible(false); }

void SCXTEditor::setTooltipContents(const std::string &s) { toolTip->setTooltipText(s); }

} // namespace scxt::ui