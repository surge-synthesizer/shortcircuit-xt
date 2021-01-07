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
#include "interaction_parameters.h"

#include "components/StubRegion.h"
#include "components/DebugPanel.h"
#include "components/ZoneKeyboardDisplay.h"

#include "proxies/ZoneStateProxy.h"

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
    logToUI = std::make_unique<SC3EngineToWrapperQueue<std::string>>();
    p.mNotify=this;
    p.sc3->registerWrapperForEvents(this);

    // This is going to be a little pattern I'm sure
    zoneStateProxy = std::make_unique<ZoneStateProxy>();
    zoneKeyboardDisplay = std::make_unique<ZoneKeyboardDisplay>(zoneStateProxy.get());

    uiStateProxies.insert(zoneStateProxy.get());
    zoneStateProxy->clients.insert(zoneKeyboardDisplay.get());

    zoneKeyboardDisplay->setBounds(0, 0, getWidth(), 200);
    addAndMakeVisible(zoneKeyboardDisplay.get());

    debugWindow = std::make_unique<DebugPanelWindow>();
    debugWindow->setVisible(true);
    debugWindow->panel->setEditor(this);

    actiondata ad;
    ad.actiontype = vga_openeditor;
    audioProcessor.sc3->postEventsFromWrapper(ad);

    idleTimer = std::make_unique<SC3IdleTimer>(this);
    idleTimer->startTimer(1000 / 30);
}

SC3AudioProcessorEditor::~SC3AudioProcessorEditor() {
    uiStateProxies.clear();
    debugWindow->panel->setEditor(nullptr);
    idleTimer->stopTimer();
    audioProcessor.mNotify=0;

    actiondata ad;
    ad.actiontype = vga_closeeditor;
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
    audioProcessor.sc3->postEventsFromWrapper(ad);
}

void SC3AudioProcessorEditor::refreshSamplerTextViewInThreadUnsafeWay()
{
    debugWindow->panel->setSamplerText(audioProcessor.sc3->generateInternalStateView());
}

void SC3AudioProcessorEditor::setLogText(const std::string &txt) { logToUI->push(txt); }

void SC3AudioProcessorEditor::idle()
{
    int mcount = 0;
    actiondata ad;
    while (actiondataToUI->pop(ad))
    {
        mcount++;
        for (auto &p : uiStateProxies)
        {
            p->processActionData(ad);
        }
    }

    std::string s;
    while (logToUI->pop(s))
    {
        debugWindow->panel->appendLogText(s);
        mcount++;
    }

    // Obviously this is thread unsafe and wonky still
    if (mcount)
        refreshSamplerTextViewInThreadUnsafeWay();
}

void SC3AudioProcessorEditor::receiveActionFromProgram(actiondata ad) { actiondataToUI->push(ad); }
