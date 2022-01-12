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

#include "proxies/ZoneStateProxy.h"
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
    zoneStateProxy = make_proxy<scxt::proxies::ZoneStateProxy>();
    browserDataProxy = make_proxy<scxt::proxies::BrowserDataProxy>();
    selectionStateProxy = make_proxy<scxt::proxies::SelectionStateProxy>();
    multiDataProxy = make_proxy<scxt::proxies::MultiDataProxy>();
    vuMeterProxy = make_proxy<scxt::proxies::VUMeterProxy>();
    partProxy = make_proxy<scxt::proxies::PartDataProxy>();
    configProxy = make_proxy<scxt::proxies::ConfigDataProxy>();

    headerPanel = std::make_unique<scxt::components::HeaderPanel>(this);
    selectionStateProxy->clients.insert(headerPanel.get());
    vuMeterProxy->clients.insert(headerPanel->vuMeter0.get());
    addAndMakeVisible(*headerPanel);

    pages[ZONE] = std::make_unique<scxt::pages::ZonePage>(this, ZONE);
    pages[PART] = std::make_unique<scxt::pages::PartPage>(this, PART);
    pages[FX] = std::make_unique<scxt::pages::FXPage>(this, FX);
    pages[CONFIG] = std::make_unique<scxt::pages::PageBase>(this, CONFIG);
    pages[ABOUT] = std::make_unique<scxt::pages::AboutPage>(this, ABOUT);

    multiDataProxy->clients.insert(pages[FX].get());

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
        for (auto &p : uiStateProxies)
        {
            p->sweepValidity();
        }

#if DEBUG_UNHANDLED_MESSAGES
        if (!handled)
        {
            if (ad.id >= ip_multi_params_begin && ad.id <= ip_multi_params_end)
            {
                if (std::holds_alternative<VAction>(ad.actiontype))
                {
                    auto at = std::get<VAction>(ad.actiontype);
                    if (at == vga_datamode)
                    {
                        auto ipd = ip_data[ad.id];
                        std::cout << (char *)&(ad.data.str[0]);
                    }
                }
                std::cout << std::endl;
            }
            unhandledCount++;
            if (std::holds_alternative<VAction>(ad.actiontype))
            {
                auto id = ad.id;
                auto at = std::get<VAction>(ad.actiontype);

                if (at == vga_database_samplelist)
                {
                    std::cout << "DBSL " << ad.data.i[2] << " at " << ad.data.ptr[0] << " " << id
                              << std::endl;
                }

                if (id >= 0)
                {
                    auto inter = ip_data[id];
                    auto itarget = targetForInteractionId((InteractionId)id);

                    if (at == vga_zonelist_populate)
                    {
                        std::cout << "ZLP " << std::endl;
                    }
                    switch (itarget)
                    {
                    case Zone:
                    {
                        switch (at)
                        {
                        case vga_floatval:
                            // std::cout << ad << " " << inter << " float=" << ad.data.f[0]
                            //           << std::endl;
                            break;
                        case vga_intval:
                            // std::cout << ad << " " << inter << " int=" << ad.data.i[0] <<
                            // std::endl;
                            break;
                        case vga_text:
                            // std::cout << debug_wrapper_ip_to_string(ad.id) << std::endl;
                            // std::cout << ad << " " << inter << " str=" << ad.data.str <<
                            // std::endl;
                            break;
                        case vga_disable_state:
                            // fixme
                            break;
                        default:
                            // std::cout << "UNHZONE " << ad << " " << inter << std::endl;
                            break;
                        }

                        break;
                    }
                    case Multi:
                        // std::cout << "MULTI " << ad << " " << inter << std::endl;
                        break;
                    case Part:
                    { /*
                         switch (at)
                         {
                         case vga_entry_add_ival_from_self_with_id:
                         {
                             std::cout << "VGA ENTRY WITH ID IN PART " << ad.id << " " << ad.subid
                                       << " " << ad.data.i[0] << " " << (char *)&ad.data.str[4]
                                       << " " << inter << std::endl;
                             break;
                         }
                         case vga_entry_add_ival_from_self:
                         {
                             std::cout << "VGA ENTRY NO ID PART " << ad.id << " " << ad.subid << " "
                                       << ad.data.str << " " << inter << std::endl;
                             break;
                         }
                         default:
                             std::cout << "PART " << ad << " " << inter << std::endl;
                             break;
                         }*/
                        break;
                    }
                    default:
                        // std::cout << "DEFTARGET " << ad << " " << inter << " " << itarget
                        //           << std::endl;
                        break;
                    }
                }
            }
            else
            {
                // std::cout << "NOT AN AT " << ad << std::endl;
            }

            // if (unhandled.find(ad.actiontype) == unhandled.end())
            //    unhandled[ad.actiontype];
            int aid = ad.id;
            if (std::holds_alternative<VAction>(ad.actiontype))
                unhandled[aid][std::get<VAction>(ad.actiontype)]++;
            else
            {
                // jassert(false);
            }
        }
#endif
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
}

void SCXTEditor::showPage(const Pages &pTo)
{
    for (const auto &[p, page] : pages)
    {
        page->setVisible(p == pTo);
    }
}