/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

// Before diving into this code you want to read the short comment set
// at the top of src/sampler_wrapper_actiondata.h

#include "SC3Editor.h"
#include "SC3Processor.h"
#include "version.h"

#include <unordered_map>

#include "components/StubRegion.h"
#include "components/DebugPanel.h"
#include "components/ZoneKeyboardDisplay.h"

#include "proxies/ZoneStateProxy.h"

#define DEBUG_UNHANDLED_MESSAGES 1

#if DEBUG_UNHANDLED_MESSAGES
#include "wrapper_msg_to_string.h"
#endif

struct SC3IdleTimer : juce::Timer
{
    SC3IdleTimer(SC3AudioProcessorEditor *ed) : ed(ed) {}
    ~SC3IdleTimer() = default;
    void timerCallback() override { ed->idle(); }
    SC3AudioProcessorEditor *ed;
};

//==============================================================================
SC3AudioProcessorEditor::SC3AudioProcessorEditor(SC3AudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(900, 600);
    actiondataToUI = std::make_unique<SC3EngineToWrapperQueue<actiondata>>();
    logToUI = std::make_unique<SC3EngineToWrapperQueue<SC3AudioProcessorEditor::LogTransport>>();
    p.addLogDisplayListener(this);
    p.sc3->registerWrapperForEvents(this);

    // This is going to be a little pattern I'm sure
    zoneStateProxy = std::make_unique<ZoneStateProxy>();
    zoneKeyboardDisplay = std::make_unique<ZoneKeyboardDisplay>(zoneStateProxy.get(), this);

    uiStateProxies.insert(zoneStateProxy.get());
    zoneStateProxy->clients.insert(zoneKeyboardDisplay.get());

    zoneKeyboardDisplay->setBounds(0, 0, getWidth(), 200);
    addAndMakeVisible(zoneKeyboardDisplay.get());

    debugWindow = std::make_unique<DebugPanelWindow>();
    debugWindow->setVisible(true);
    debugWindow->panel->setEditor(this);

    actiondata ad;
    ad.actiontype = vga_openeditor;
    sendActionInternal(ad);


    idleTimer = std::make_unique<SC3IdleTimer>(this);
    idleTimer->startTimer(1000 / 30);
}


SC3AudioProcessorEditor::~SC3AudioProcessorEditor() {
    uiStateProxies.clear();
    debugWindow->panel->setEditor(nullptr);
    idleTimer->stopTimer();
    audioProcessor.removeLogDisplayListener(this);

    actiondata ad;
    ad.actiontype = vga_closeeditor;
    sendActionInternal(ad);

    audioProcessor.sc3->unregisterWrapperForEvents(this);
}

void SC3AudioProcessorEditor::sendActionToEngine(const actiondata &ad) { sendActionInternal(ad); }

void SC3AudioProcessorEditor::sendActionInternal(const actiondata &ad)
{
    audioProcessor.sc3->postEventsFromWrapper(ad);
}

void SC3AudioProcessorEditor::buttonClicked(Button *b)
{
}

void SC3AudioProcessorEditor::buttonStateChanged(Button *b)
{
}

//==============================================================================
void SC3AudioProcessorEditor::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    auto bounds = getLocalBounds();
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    auto bottomLabel = bounds.removeFromTop(bounds.getHeight()).expanded(-2,0);
    g.drawFittedText("ShortCircuit3", bottomLabel, juce::Justification::bottomLeft, 1);
    g.drawFittedText(SC3::Build::FullVersionStr, bottomLabel, juce::Justification::bottomRight, 1);
}

void SC3AudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    std::cout << "RESIZED" << std::endl;
}

bool SC3AudioProcessorEditor::isInterestedInFileDrag(const StringArray &files) {
    return true;
}
void SC3AudioProcessorEditor::filesDropped(const StringArray &files, int x, int y) {
    auto d = new DropList();

    for( auto f : files )
    {
        auto fd = DropList::File();
        fd.p = string_to_path(f.toStdString().c_str());
        d->files.push_back(fd);
    }

    actiondata ad;
    ad.actiontype = vga_load_dropfiles;
    ad.data.dropList = d;
    sendActionToEngine(ad);
}

void SC3AudioProcessorEditor::refreshSamplerTextViewInThreadUnsafeWay()
{
    // Really this isn't thread safe and we should fix it
    debugWindow->panel->setSamplerText(audioProcessor.sc3->generateInternalStateView());
}

void SC3AudioProcessorEditor::handleLogMessage(SC3::Log::Level lev, const std::string &txt)
{
    logToUI->push(SC3AudioProcessorEditor::LogTransport(lev, txt));
}

void SC3AudioProcessorEditor::idle()
{
    int mcount = 0;
    actiondata ad;

#if DEBUG_UNHANDLED_MESSAGES
    std::map<int, std::map<int, int>> unhandled;
    bool collapseSubtypes = true;
#endif
    while (actiondataToUI->pop(ad))
    {
        mcount++;
        auto handled = false;
        for (auto &p : uiStateProxies)
        {
            handled |= p->processActionData(ad);
        }
#if DEBUG_UNHANDLED_MESSAGES
        if (!handled)
        {
            // if (unhandled.find(ad.actiontype) == unhandled.end())
            //    unhandled[ad.actiontype];
            int aid = ad.id;
            if (collapseSubtypes)
                aid = -1;
            unhandled[ad.actiontype][aid]++;
        }
#endif
    }

    LogTransport lt;
    while (logToUI->pop(lt))
    {
        std::string t = SC3::Log::getShortLevelStr(lt.lev);
        // We now have an option to do something else with errors if we want
        debugWindow->panel->appendLogText(t + lt.txt);
        mcount++;
    }

#if DEBUG_UNHANDLED_MESSAGES
    // bit of a hack, sure.
    if (unhandled.size())
    {
        debugWindow->panel->appendLogText("[EDITOR] -------- MESSAGE BLOCK -------");
    }
    for (auto uh : unhandled)
    {
        for (auto uhsub : uh.second)
        {
            std::ostringstream oss;
            oss << "[EDITOR] ignored UI msg=" << debug_wrapper_vga_to_string(uh.first);
            if (!collapseSubtypes)
                oss << "/" << debug_wrapper_ip_to_string(uhsub.first);
            oss << " ct=" << uhsub.second;
            debugWindow->panel->appendLogText(oss.str());
        }
    }
#endif

    // Obviously this is thread unsafe and wonky still
    if (mcount)
        refreshSamplerTextViewInThreadUnsafeWay();
}

void SC3AudioProcessorEditor::receiveActionFromProgram(const actiondata &ad)
{
    actiondataToUI->push(ad);
}
