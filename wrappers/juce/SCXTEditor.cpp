/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

// Before diving into this code you want to read the short comment set
// at the top of src/sampler_wrapper_actiondata.h

#include "SCXTEditor.h"
#include "SCXTProcessor.h"
#include "version.h"

#include <unordered_map>

#include "components/DebugPanel.h"
#include "components/HeaderPanel.h"

#include "proxies/ZoneDataProxy.h"
#include "proxies/ZoneListDataProxy.h"
#include "proxies/BrowserDataProxy.h"
#include "proxies/SelectionStateProxy.h"
#include "proxies/VUMeterProxy.h"
#include "proxies/MultiDataProxy.h"
#include "proxies/PartDataProxy.h"
#include "proxies/ConfigDataProxy.h"

#include "pages/PageBase.h"
#include "pages/AboutPage.h"
#include "pages/ZonePage.h"
#include "pages/FXPage.h"
#include "pages/PartPage.h"

#include "widgets/CompactVUMeter.h"

#define DEBUG_UNHANDLED_MESSAGES 1

#if DEBUG_UNHANDLED_MESSAGES
#include "wrapper_msg_to_string.h"
#endif

struct SC3IdleTimer : juce::Timer
{
    SC3IdleTimer(SCXTEditor *ed) : ed(ed) {}
    ~SC3IdleTimer() = default;
    void timerCallback() override { ed->idle(); }
    SCXTEditor *ed;
};

//==============================================================================
SCXTEditor::SCXTEditor(SCXTProcessor &p) : AudioProcessorEditor(&p), audioProcessor(p)
{
    auto area = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->totalArea;
    float ws = 900.f / area.getWidth();
    float hs = 700.f / area.getHeight();
    auto x = std::max(ws, hs);

    auto sc = 1.f;
    for (const auto &q : {1.25, 1.5, 1.75, 2.})
    {
        if (q * x < 1)
            sc = q;
    }

    setTransform(juce::AffineTransform().scaled(sc));
    lookAndFeel = std::make_unique<SCXTLookAndFeel>();
    setLookAndFeel(lookAndFeel.get());

    for (int i = 0; i < max_zones; ++i)
        activeZones[i] = false;
    for (int i = 0; i < 128; ++i)
        playingMidiNotes[i] = 0;

    actiondataToUI = std::make_unique<SC3EngineToWrapperQueue<actiondata>>();
    logToUI = std::make_unique<SC3EngineToWrapperQueue<SCXTEditor::LogTransport>>();
    p.addLogDisplayListener(this);
    p.sc3->registerWrapperForEvents(this);

    // This is going to be a little pattern I'm sure
    browserDataProxy = make_proxy<scxt::proxies::BrowserDataProxy>();
    selectionStateProxy = make_proxy<scxt::proxies::SelectionStateProxy>();
    multiDataProxy = make_proxy<scxt::proxies::MultiDataProxy>();
    vuMeterProxy = make_proxy<scxt::proxies::VUMeterProxy>();
    partProxy = make_proxy<scxt::proxies::PartDataProxy>();
    configProxy = make_proxy<scxt::proxies::ConfigDataProxy>();
    zoneProxy = make_proxy<scxt::proxies::ZoneDataProxy>();
    zoneListProxy = make_proxy<scxt::proxies::ZoneListDataProxy>();

    headerPanel = std::make_unique<scxt::components::HeaderPanel>(this);
    selectionStateProxy->clients.insert(headerPanel.get());
    vuMeterProxy->clients.insert(headerPanel->vuMeter0.get());
    addAndMakeVisible(*headerPanel);

    pages[ZONE] = std::make_unique<scxt::pages::ZonePage>(this, ZONE);
    pages[PART] = std::make_unique<scxt::pages::PartPage>(this, PART);
    pages[FX] = std::make_unique<scxt::pages::FXPage>(this, FX);
    pages[CONFIG] = std::make_unique<scxt::pages::PageBase>(this, CONFIG);
    pages[ABOUT] = std::make_unique<scxt::pages::AboutPage>(this, ABOUT);

    selectionStateProxy->clients.insert(pages[ZONE].get());
    zoneListProxy->clients.insert(pages[ZONE].get());
    zoneProxy->clients.insert(pages[ZONE].get());

    multiDataProxy->clients.insert(pages[FX].get());

    partProxy->clients.insert(pages[PART].get());

    debugWindow = std::make_unique<DebugPanelWindow>();
    debugWindow->setVisible(true);
    debugWindow->setEditor(this);

    for (const auto &[p, page] : pages)
    {
        page->connectProxies();
        page->setVisible(false);
        addChildComponent(*page);
    }
    pages[ZONE]->setVisible(true);

    actiondata ad;
    ad.actiontype = vga_openeditor;
    sendActionInternal(ad);

    idleTimer = std::make_unique<SC3IdleTimer>(this);
    idleTimer->startTimer(1000 / 30);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(900, 700);
    // setResizable(true, true);
}

SCXTEditor::~SCXTEditor()
{
    setLookAndFeel(nullptr);
    uiStateProxies.clear();
    debugWindow->setEditor(nullptr);
    idleTimer->stopTimer();
    audioProcessor.removeLogDisplayListener(this);

    actiondata ad;
    ad.actiontype = vga_closeeditor;
    sendActionInternal(ad);

    audioProcessor.sc3->unregisterWrapperForEvents(this);
}

void SCXTEditor::sendActionToEngine(const actiondata &ad) { sendActionInternal(ad); }

void SCXTEditor::sendActionInternal(const actiondata &ad)
{
    audioProcessor.sc3->postEventsFromWrapper(ad);
}

void SCXTEditor::buttonClicked(juce::Button *b) {}

void SCXTEditor::buttonStateChanged(juce::Button *b) {}

//==============================================================================
void SCXTEditor::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    auto bounds = getLocalBounds();
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    auto bottomLabel = bounds.removeFromTop(bounds.getHeight()).expanded(-2, 0);
    g.drawFittedText("Shortcircuit XT", bottomLabel, juce::Justification::bottomLeft, 1);
    g.drawFittedText(scxt::build::FullVersionStr, bottomLabel, juce::Justification::bottomRight, 1);
}

void SCXTEditor::resized()
{
    auto r = getLocalBounds();
    headerPanel->setBounds(r.withHeight(25));
    r = r.withTrimmedTop(25);
    for (const auto &[p, page] : pages)
    {
        page->setBounds(r);
    }
}

bool SCXTEditor::isInterestedInFileDrag(const juce::StringArray &files) { return true; }
void SCXTEditor::filesDropped(const juce::StringArray &files, int x, int y)
{
    auto d = new DropList();

    for (auto f : files)
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

void SCXTEditor::refreshSamplerTextViewInThreadUnsafeWay()
{
    // Really this isn't thread safe and we should fix it
    debugWindow->setSamplerText(audioProcessor.sc3->generateInternalStateView());
}

void SCXTEditor::idle()
{
    int mcount = 0;
    actiondata ad;

#if DEBUG_UNHANDLED_MESSAGES
    std::map<int, std::map<int, int>> unhandled;
    uint64_t unhandledCount = 0;
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
            unhandledCount++;

            int aid = ad.id;
            if (std::holds_alternative<VAction>(ad.actiontype))
                unhandled[aid][std::get<VAction>(ad.actiontype)]++;
            else
            {
                jassertfalse;
            }
        }
#endif
    }

    for (auto &p : uiStateProxies)
    {
        p->sweepValidity();
    }
#if DEBUG_UNHANDLED_MESSAGES
    // eventually
    // jassert(unhandledCount == 0);
    // but for now
    if (unhandledCount)
        DBG("Unhandled Message Count : " << unhandledCount);
#endif

    std::ostringstream debugLog;

    LogTransport lt;
    while (logToUI->pop(lt))
    {
        std::string t = scxt::log::getShortLevelStr(lt.lev);
        // We now have an option to do something else with errors if we want
        debugLog << t << lt.txt << "\n";
        mcount++;
    }

#if DEBUG_UNHANDLED_MESSAGES
    // bit of a hack, sure.
    if (unhandled.size())
    {
        debugLog << "[EDITOR] ---- Unhandled " << unhandledCount << " Messages ---\n";
    }
    for (auto uh : unhandled)
    {
        debugLog << "[EDITOR] Message ID: " << debug_wrapper_ip_to_string(uh.first) << "\n";
        for (auto uhsub : uh.second)
        {
            debugLog << "[EDITOR]       Action=" << debug_wrapper_vga_to_string(uhsub.first)
                     << " ct=" << uhsub.second << "\n";
        }
    }
#endif

    debugWindow->appendLogText(debugLog.str(), false);

    // Obviously this is thread unsafe and wonky still
    if (mcount)
        refreshSamplerTextViewInThreadUnsafeWay();
}

void SCXTEditor::receiveActionFromProgram(const actiondata &ad) { actiondataToUI->push(ad); }
scxt::log::Level SCXTEditor::getLevel()
{
    // TODO some kind of global config read
    return scxt::log::Level::Debug;
}
void SCXTEditor::message(scxt::log::Level lev, const std::string &msg)
{
    logToUI->push(SCXTEditor::LogTransport(lev, msg));
}

void SCXTEditor::selectPart(int i)
{
    actiondata ad;
    ad.id = ip_partselect;
    ad.actiontype = vga_intval;
    ad.data.i[0] = i;
    sendActionToEngine(ad);
    selectedPart = i;
    selectedLayer = -1;
    selectedZone = -1;
}

void SCXTEditor::selectLayer(int i)
{
    actiondata ad;
    ad.id = ip_layerselect;
    ad.actiontype = vga_intval;
    ad.data.i[0] = i;
    sendActionToEngine(ad);
    selectedLayer = i;
    selectedZone = -1;
}

void SCXTEditor::showPage(const Pages &pTo)
{
    for (const auto &[p, page] : pages)
    {
        page->setVisible(p == pTo);
    }
}

void SCXTEditor::parentHierarchyChanged()
{
    juce::Component *c = this;
    while (c)
    {
        if (auto dw = dynamic_cast<juce::TopLevelWindow *>(c))
        {
            auto n = std::string("ShortCircuit XT Alpha - ");
            n += scxt::build::FullVersionStr;
            c->setName(n);
            return;
        }
        c = c->getParentComponent();
    }
}