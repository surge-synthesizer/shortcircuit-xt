/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "SC3Processor.h"
#include "sample.h"
#include "sampler.h"

#include "vt_gui/browserdata.h"

#include "components/DebugPanel.h"
#include "DataInterfaces.h"
#include "SCXTLookAndFeel.h"

/*
 * This is basically a lock free thread safe FIFO queue of Ts with size qSize
 */
template <typename T, int qSize = 4096 * 16> class SC3EngineToWrapperQueue
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
struct SC3IdleTimer;
class SC3Editor;

// Forward decls of proxies and their componetns
class ZoneStateProxy;
class WaveDisplayProxy;
struct BrowserDataProxy;
struct SelectionStateProxy;
struct VUMeterProxy;

class ZoneKeyboardDisplay;
struct ZoneEditor;
class WaveDisplay;
struct HeaderPanel;

//==============================================================================
/**
 */
class SC3Editor : public juce::AudioProcessorEditor,
                  public juce::Button::Listener,
                  public juce::FileDragAndDropTarget,
                  public sampler::WrapperListener,
                  public ActionSender,
                  public SC3::Log::LoggingCallback
{
  public:
    SC3Editor(SC3AudioProcessor &);
    ~SC3Editor();

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

    // juce::Button::Listener interface
    virtual void buttonClicked(juce::Button *) override;
    virtual void buttonStateChanged(juce::Button *) override;

    // juce::FileDragAndDrop interface
    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;

    // Fixme - obviously this is done with no thought of threading or anything else
    void refreshSamplerTextViewInThreadUnsafeWay();

    void receiveActionFromProgram(const actiondata &ad) override;
    void sendActionToEngine(const actiondata &ad) override;

    void idle();

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SC3AudioProcessor &audioProcessor;

  private:
    void sendActionInternal(const actiondata &ad);

    std::unique_ptr<DebugPanelWindow> debugWindow;
    std::unique_ptr<SC3EngineToWrapperQueue<actiondata>> actiondataToUI;
    struct LogTransport
    {
        SC3::Log::Level lev = SC3::Log::Level::None;
        std::string txt = "";
        LogTransport() = default;
        LogTransport(SC3::Log::Level l, const std::string &t) : lev(l), txt(t) {}
    };
    std::unique_ptr<SC3EngineToWrapperQueue<LogTransport>> logToUI;
    std::unique_ptr<SC3IdleTimer> idleTimer;

    std::set<UIStateProxy *> uiStateProxies;

    std::unique_ptr<ZoneStateProxy> zoneStateProxy;
    std::unique_ptr<BrowserDataProxy> browserDataProxy;
    std::unique_ptr<SelectionStateProxy> selectionStateProxy;
    std::unique_ptr<VUMeterProxy> vuMeterProxy;

    template <typename T> std::unique_ptr<T> make_proxy()
    {
        auto r = std::make_unique<T>(this);
        uiStateProxies.insert(r.get());
        return (std::move(r));
    }

    std::unique_ptr<ZoneKeyboardDisplay> zoneKeyboardDisplay;
    std::unique_ptr<WaveDisplay> waveDisplay;
    std::unique_ptr<ZoneEditor> zoneEditor;
    std::unique_ptr<HeaderPanel> headerPanel;

    std::unique_ptr<SCXTLookAndFeel> lookAndFeel;

  public:
    std::array<int, 128> playingMidiNotes;

    struct VUData
    {
        uint8_t ch1, ch2;
        bool clip1, clip2;
    };
    std::array<VUData, max_outputs> vuData;

    int selectedPart{0};
    void selectPart(int i);

    sample_zone zonesCopy[max_zones];
    bool activeZones[max_zones];

    sample_part partsC[n_sampler_parts];
    sample_multi multiC;
    int freeParamsC[n_ip_free_items][16];

    std::array<database_samplelist, max_samples> samplesCopy;
    uint32_t samplesCopyActiveCount{0};

    // implement logging interface for logs generated on ui side
    SC3::Log::Level getLevel() override;
    void message(SC3::Log::Level lev, const std::string &msg) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SC3Editor)
};
