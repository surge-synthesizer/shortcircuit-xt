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

// Before diving into this code you want to read the short comment set
// at the top of src/sampler_wrapper_actiondata.h

#include "SCXTEditor.h"
#include "SCXTProcessor.h"
#include "version.h"

#include <unordered_map>

#include "components/BrowserSidebar.h"
#include "components/DebugPanel.h"
#include "components/HeaderPanel.h"

#include "proxies/ZoneDataProxy.h"
#include "proxies/ZoneListDataProxy.h"
#include "proxies/BrowserDataProxy.h"
#include "proxies/SelectionStateProxy.h"
#include "proxies/VUMeterPolyphonyProxy.h"
#include "proxies/MultiDataProxy.h"
#include "proxies/PartDataProxy.h"
#include "proxies/ConfigDataProxy.h"

#include "pages/PageBase.h"
#include "pages/AboutPage.h"
#include "pages/ZonePage.h"
#include "pages/FXPage.h"
#include "pages/PartPage.h"

#include "widgets/CompactVUMeter.h"
#include "widgets/PolyphonyDisplay.h"

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

struct SCXTTopLevel : public juce::Component, scxt::style::DOMParticipant
{
    SCXTEditor *editor{nullptr};
    SCXTTopLevel(SCXTEditor *e) : juce::Component(), editor(e), DOMParticipant("scxt")
    {
        setupJuceAccessibility();
    }
    void resized() override;
};

//==============================================================================
SCXTEditor::SCXTEditor(SCXTProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p), sheet(scxt::style::createDefaultSheet())
{
    topLevel = std::make_unique<SCXTTopLevel>(this);
    addAndMakeVisible(*topLevel);

    // this is pretty arbitrary
    auto sc = 1.f;
    auto zf = p.sc3->defaultsProvider->getUserDefaultValue(scxt::defaults::zoomLevel, -1);
    if (zf > 50)
    {
        sc = zf / 100.f;
    }
    else
    {
        sc = optimalScaleForDisplay();
    }

    // topLevel->setTransform(juce::AffineTransform().scaled(sc));
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
    vuMeterProxy = make_proxy<scxt::proxies::VUMeterPolyphonyProxy>();
    partProxy = make_proxy<scxt::proxies::PartDataProxy>();
    configProxy = make_proxy<scxt::proxies::ConfigDataProxy>();
    zoneProxy = make_proxy<scxt::proxies::ZoneDataProxy>();
    zoneListProxy = make_proxy<scxt::proxies::ZoneListDataProxy>();

    headerPanel = std::make_unique<scxt::components::HeaderPanel>(this);
    selectionStateProxy->clients.insert(headerPanel.get());
    vuMeterProxy->clients.insert(headerPanel->vuMeter0.get());
    vuMeterProxy->clients.insert(headerPanel->polyDisplay.get());
    topLevel->addAndMakeVisible(*headerPanel);

    browserSidebar = std::make_unique<scxt::components::BrowserSidebar>(this);
    topLevel->addAndMakeVisible(*browserSidebar);

    pages[ZONE] = std::make_unique<scxt::pages::ZonePage>(this, ZONE);
    pages[PART] = std::make_unique<scxt::pages::PartPage>(this, PART);
    pages[FX] = std::make_unique<scxt::pages::FXPage>(this, FX);
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
        topLevel->addChildComponent(*page);
    }
    pages[ZONE]->setVisible(true);

    actiondata ad;
    ad.actiontype = vga_openeditor;
    sendActionInternal(ad);

    idleTimer = std::make_unique<SC3IdleTimer>(this);
    idleTimer->startTimer(1000 / 30);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setScale(sc);
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

    // We do this here to clean up the UI elements before the data elements explicitly
    for (auto &p : pages)
        p.second.reset(nullptr);
}

void SCXTEditor::sendActionToEngine(const actiondata &ad) { sendActionInternal(ad); }

void SCXTEditor::sendActionInternal(const actiondata &ad)
{
    audioProcessor.sc3->postEventsFromWrapper(ad);
}

void SCXTEditor::buttonClicked(juce::Button *b) {}

void SCXTEditor::buttonStateChanged(juce::Button *b) {}

//==============================================================================
void SCXTEditor::paint(juce::Graphics &g) { g.fillAll(juce::Colours::black); }

void SCXTEditor::setScale(float sc)
{
    scale = sc;
    setSize(scWidth * sc, scHeight * sc);
    audioProcessor.sc3->defaultsProvider->updateUserDefaultValue(scxt::defaults::zoomLevel,
                                                                 (int)(round(sc * 100)));
}

float SCXTEditor::optimalScaleForDisplay()
{
    auto area = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->totalArea;
    float ws = 1.f * scWidth / area.getWidth();
    float hs = 1.f * scHeight / area.getHeight();
    auto x = std::max(ws, hs);

    float sc = 1.f;
    for (const auto &q : {1.25, 1.5, 1.75, 2.})
    {
        if (q * x < 0.85)
            sc = q;
    }
    return sc;
}

void SCXTEditor::resized()
{
    topLevel->setBounds(juce::Rectangle<int>(0, 0, scWidth, scHeight));
    topLevel->setTransform(juce::AffineTransform().scaled(scale));
}

void SCXTTopLevel::resized()
{
    auto r = getLocalBounds();
    editor->headerPanel->setBounds(r.withHeight(25));

    r = r.withTrimmedTop(25);
    auto sidebarWidth = 145;
    auto side = r.withWidth(sidebarWidth).translated(r.getWidth() - sidebarWidth, 0);
    editor->browserSidebar->setBounds(side.reduced(1, 0));
    r = r.withTrimmedRight(sidebarWidth);

    for (const auto &[p, page] : editor->pages)
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

void SCXTEditor::dumpStyles()
{
    std::function<void(juce::Component *, const std::string &)> rdump;

    rdump = [&rdump](juce::Component *c, const std::string &pfx) {
        std::cout << pfx << " " << c << " ";
        auto dp = dynamic_cast<scxt::style::DOMParticipant *>(c);
        auto recurse = true;
        if (dp)
        {
            std::cout << dp->getSelector().id;
            recurse = dp->getIsDOMContainer();
        }
        else
        {
            std::cout << "[non-dom] " << typeid(*c).name();
        }
        std::cout << "\n";
        if (recurse)
        {
            for (auto q : c->getChildren())
            {
                rdump(q, pfx + "|--");
            }
        }
    };
    std::cout << "STYLE STRUCTURE DUMP" << std::endl;
    rdump(topLevel.get(), "");
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