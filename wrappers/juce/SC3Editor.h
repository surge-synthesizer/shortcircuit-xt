/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "SC3Processor.h"
#include <JuceHeader.h>
#include "sample.h"
#include "sampler.h"
#include "components/DebugPanel.h"

/*
 * This is basically a lock free thread safe FIFO queue of Ts with size qSize
 */
template <typename T, int qSize = 4096> class SC3EngineToWrapperQueue
{
  public:
    SC3EngineToWrapperQueue() : af(qSize) {}
    bool push(const T &ad)
    {
        auto ret = false;
        int start1, size1, start2, size2;
        af.prepareToWrite(1, start1, size1, start2, size2);
        if (size1 > 0)
        {
            dq[start1] = ad;
            ret = true;
        }
        af.finishedWrite(size1 + size2);
        return ret;
    }
    bool pop(T &ad)
    {
        bool ret = false;
        int start1, size1, start2, size2;
        af.prepareToRead(1, start1, size1, start2, size2);
        if (size1 > 0)
        {
            ad = dq[start1];
            ret = true;
        }
        af.finishedRead(size1 + size2);
        return ret;
    }
    juce::AbstractFifo af;
    T dq[qSize];
};
class SC3IdleTimer;

/*
 * The UIStateProxy is a class which handles messages and keeps an appropriate state.
 * Components can render it for bulk action. For instance there woudl be a ZoneMapProxy
 * and so on. Each UIStateProxy registerred with the editor gets all the messages.
 */
class UIStateProxy
{
  public:
    virtual ~UIStateProxy() = default;
    virtual void processActionData(actiondata &d) = 0;
    std::unordered_set<juce::Component *> clients;
    void repaintClients()
    {
        for (auto c : clients)
        {
            c->repaint();
        }
    }
};

// Forward decls of proxies and their componetns
class ZoneStateProxy;
class ZoneKeyboardDisplay;

//==============================================================================
/**
 */
class SC3AudioProcessorEditor : public juce::AudioProcessorEditor,
                                public juce::Button::Listener,
                                public juce::FileDragAndDropTarget,
                                public EditorNotify,
                                public sampler::WrapperListener
{
  public:
    SC3AudioProcessorEditor(SC3AudioProcessor &);
    ~SC3AudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

    // juce::Button::Listener interface
    virtual void buttonClicked(Button *) override;
    virtual void buttonStateChanged(Button *) override;

    // juce::FileDragAndDrop interface
    bool isInterestedInFileDrag(const StringArray &files) override;
    void filesDropped(const StringArray &files, int x, int y) override;

    // Fixme - obviously this is done with no thought of threading or anything else
    void refreshSamplerTextViewInThreadUnsafeWay();

    void setLogText(const std::string &txt) override;

    void receiveActionFromProgram(actiondata ad) override;

    void idle();

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SC3AudioProcessor &audioProcessor;

  private:
    std::unique_ptr<DebugPanelWindow> debugWindow;
    std::unique_ptr<SC3EngineToWrapperQueue<actiondata>> actiondataToUI;
    std::unique_ptr<SC3EngineToWrapperQueue<std::string>> logToUI;
    std::unique_ptr<SC3IdleTimer> idleTimer;

    std::set<UIStateProxy *> uiStateProxies;

    std::unique_ptr<ZoneStateProxy> zoneStateProxy;
    std::unique_ptr<ZoneKeyboardDisplay> zoneKeyboardDisplay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SC3AudioProcessorEditor)
};
