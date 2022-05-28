/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "SCXTProcessor.h"
#include "sample.h"
#include "sampler.h"

#include "vt_gui/browserdata.h"

#include "components/DebugPanel.h"
#include "data/SCXTData.h"
#include "SCXTLookAndFeel.h"
#include "style/StyleSheet.h"

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

namespace scxt
{
namespace pages
{
struct PageBase;
}
namespace components
{
struct HeaderPanel;
struct BrowserSidebar;
} // namespace components

namespace proxies
{
class ZoneDataProxy;
class ZoneListDataProxy;
class WaveDisplayProxy;
struct BrowserDataProxy;
struct SelectionStateProxy;
struct VUMeterPolyphonyProxy;
struct MultiDataProxy;
struct PartDataProxy;
struct ConfigDataProxy;
} // namespace proxies
} // namespace scxt

//==============================================================================
/**
 */
struct SCXTTopLevel;
class SCXTEditor : public juce::AudioProcessorEditor,
                   public juce::Button::Listener,
                   public juce::FileDragAndDropTarget,
                   public sampler::WrapperListener,
                   public scxt::data::ActionSender,
                   public scxt::log::LoggingCallback
{
  public:
    SCXTEditor(SCXTProcessor &);
    ~SCXTEditor();

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;
    void parentHierarchyChanged() override;

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
    SCXTProcessor &audioProcessor;

    enum Pages
    {
        ZONE,
        PART,
        FX,
        ABOUT
    };

    static std::string pageName(const Pages &p)
    {
        switch (p)
        {
        case ZONE:
            return "Zone";
        case PART:
            return "Part";
        case FX:
            return "FX";
        case ABOUT:
            return "About";
        }
        return "ERROR";
    }

    void showPage(const Pages &p);

    static constexpr int scWidth = 970, scHeight = 700;
    float scale = 1.0;
    void setScale(float sc);
    float optimalScaleForDisplay();
    void dumpStyles();

  private:
    void sendActionInternal(const actiondata &ad);

    std::unique_ptr<DebugPanelWindow> debugWindow;
    std::unique_ptr<SC3EngineToWrapperQueue<actiondata>> actiondataToUI;
    struct LogTransport
    {
        scxt::log::Level lev = scxt::log::Level::None;
        std::string txt = "";
        LogTransport() = default;
        LogTransport(scxt::log::Level l, const std::string &t) : lev(l), txt(t) {}
    };
    std::unique_ptr<SC3EngineToWrapperQueue<LogTransport>> logToUI;
    std::unique_ptr<SC3IdleTimer> idleTimer;
    std::unique_ptr<SCXTTopLevel> topLevel;

  public:
    std::unique_ptr<scxt::style::Sheet> sheet;

    std::set<scxt::data::UIStateProxy *> uiStateProxies;
    std::unique_ptr<scxt::proxies::SelectionStateProxy> selectionStateProxy;
    std::unique_ptr<scxt::proxies::VUMeterPolyphonyProxy> vuMeterProxy;
    std::unique_ptr<scxt::proxies::BrowserDataProxy> browserDataProxy;
    std::unique_ptr<scxt::proxies::ZoneDataProxy> zoneProxy;
    std::unique_ptr<scxt::proxies::ZoneListDataProxy> zoneListProxy;
    std::unique_ptr<scxt::proxies::MultiDataProxy> multiDataProxy;
    std::unique_ptr<scxt::proxies::PartDataProxy> partProxy;
    std::unique_ptr<scxt::proxies::ConfigDataProxy> configProxy;

    template <typename T> std::unique_ptr<T> make_proxy()
    {
        auto r = std::make_unique<T>(this);
        uiStateProxies.insert(r.get());
        return (std::move(r));
    }

    std::map<Pages, std::unique_ptr<scxt::pages::PageBase>> pages;

    std::unique_ptr<scxt::components::HeaderPanel> headerPanel;
    std::unique_ptr<scxt::components::BrowserSidebar> browserSidebar;

    std::unique_ptr<SCXTLookAndFeel> lookAndFeel;

  public:
    std::array<int, 128> playingMidiNotes;

    struct VUData
    {
        uint8_t ch1, ch2;
        bool clip1, clip2;
    };
    std::array<VUData, MAX_OUTPUTS> vuData;
    int polyphony{0};

    int selectedPart{0};
    int selectedLayer{0};
    int selectedZone{-1};
    int zoneListMode{0};
    void selectPart(int i);
    void selectLayer(int i);

    bool activeZones[MAX_ZONES];
    bool selectedZones[MAX_ZONES];

    int freeParamsC[n_ip_free_items][16];

    // scxt::data::PartData parts[N_SAMPLER_PARTS];
    scxt::data::MultiData multi;
    scxt::data::ConfigData config;
    scxt::data::ZoneData currentZone;
    scxt::data::PartData currentPart;

    sample_zone zonesCopy[MAX_ZONES];
    std::string currentSampleName, currentSampleMetadata;

    /*
     * Configuration Data
     */
    scxt::data::NameList multiFilterTypeNames;
    scxt::data::NameList multiFilterOutputNames;

    scxt::data::NameList partFilterTypeNames;
    scxt::data::NameList partAuxOutputNames;
    scxt::data::NameList partMMSrc, partMMSrc2, partMMDst, partMMCurve, partNCSrc;

    scxt::data::NameList zonePlaymode, zoneAuxOutput, zoneFilterType;
    scxt::data::NameList zoneMMSrc, zoneMMSrc2, zoneMMDst, zoneMMCurve, zoneNCSrc, zoneLFOPresets;

    std::array<database_samplelist, MAX_SAMPLES> samplesCopy;
    uint32_t samplesCopyActiveCount{0};

    // implement logging interface for logs generated on ui side
    scxt::log::Level getLevel() override;
    void message(scxt::log::Level lev, const std::string &msg) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SCXTEditor)
};
