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
#include "components/WaveDisplay.h"

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

    actiondataToUI = std::make_unique<SC3EngineToWrapperQueue<actiondata>>();
    logToUI = std::make_unique<SC3EngineToWrapperQueue<SC3AudioProcessorEditor::LogTransport>>();
    p.addLogDisplayListener(this);
    p.sc3->registerWrapperForEvents(this);

    // This is going to be a little pattern I'm sure
    zoneStateProxy = std::make_unique<ZoneStateProxy>();
    zoneKeyboardDisplay = std::make_unique<ZoneKeyboardDisplay>(zoneStateProxy.get(), this);
    waveDisplay = std::make_unique<WaveDisplay>(this, this);
    uiStateProxies.insert(zoneStateProxy.get());
    uiStateProxies.insert(waveDisplay.get());
    zoneStateProxy->clients.insert(zoneKeyboardDisplay.get());

    addAndMakeVisible(zoneKeyboardDisplay.get());
    addAndMakeVisible(waveDisplay.get());

    debugWindow = std::make_unique<DebugPanelWindow>();
    debugWindow->setVisible(true);
    debugWindow->setEditor(this);

    actiondata ad;
    ad.actiontype = vga_openeditor;
    sendActionInternal(ad);


    idleTimer = std::make_unique<SC3IdleTimer>(this);
    idleTimer->startTimer(1000 / 30);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(900, 600);
}


SC3AudioProcessorEditor::~SC3AudioProcessorEditor() {
    uiStateProxies.clear();
    debugWindow->setEditor(nullptr);
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
    g.drawFittedText("Shortcircuit XT", bottomLabel, juce::Justification::bottomLeft, 1);
    g.drawFittedText(SC3::Build::FullVersionStr, bottomLabel, juce::Justification::bottomRight, 1);
}

void SC3AudioProcessorEditor::resized()
{
    Rectangle<int> r = getLocalBounds();
    r.reduce(2,2);
    zoneKeyboardDisplay->setBounds(r.removeFromTop(200));
    r.removeFromBottom(10);
    waveDisplay->setBounds(r);
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
    debugWindow->setSamplerText(audioProcessor.sc3->generateInternalStateView());
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
        if (std::holds_alternative<VAction>(ad.actiontype))
        {
            auto id = ad.id;
            if (id > 0)
            {
                auto inter = ip_data[id];
                std::cout << ad << " " << inter << std::endl;
            }
        }
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
            if (std::holds_alternative<VAction>(ad.actiontype))
                unhandled[std::get<VAction>(ad.actiontype)][aid]++;
            else
                jassert(false);
        }
#endif
    }

    LogTransport lt;
    while (logToUI->pop(lt))
    {
        std::string t = SC3::Log::getShortLevelStr(lt.lev);
        // We now have an option to do something else with errors if we want
        debugWindow->appendLogText(t + lt.txt);
        mcount++;
    }

#if DEBUG_UNHANDLED_MESSAGES
    // bit of a hack, sure.
    if (unhandled.size())
    {
        debugWindow->appendLogText("[EDITOR] -------- MESSAGE BLOCK -------");
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
            debugWindow->appendLogText(oss.str());
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
SC3::Log::Level SC3AudioProcessorEditor::getLevel()
{
    // TODO some kind of global config read
    return SC3::Log::Level::Debug;
}
void SC3AudioProcessorEditor::message(SC3::Log::Level lev, const std::string &msg) {
    logToUI->push(SC3AudioProcessorEditor::LogTransport(lev, msg));
}
