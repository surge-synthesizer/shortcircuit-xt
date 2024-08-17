/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#include "melatonin_inspector/melatonin_inspector.h"
#include "SCXTEditor.h"

#include "PlayScreen.h"
#include "sst/jucegui/style/StyleSheet.h"

#include "infrastructure/user_defaults.h"

#include "HeaderRegion.h"
#include "MultiScreen.h"
#include "MixerScreen.h"
#include "AboutScreen.h"
#include "LogScreen.h"
#include "WelcomeScreen.h"
#include "SCXTJuceLookAndFeel.h"
#include "sst/jucegui/components/ToolTip.h"
#include <sst/jucegui/components/DiscreteParamMenuBuilder.h>

#if MAC
#include <mach/mach.h>
#endif

namespace scxt::ui
{

SCXTEditor::SCXTEditor(messaging::MessageController &e, infrastructure::DefaultsProvider &d,
                       const sample::SampleManager &s, const scxt::browser::Browser &b,
                       const engine::Engine::SharedUIMemoryState &st)
    : msgCont(e), sampleManager(s), defaultsProvider(d), browser(b), sharedUiMemoryState(st)
{
    sst::jucegui::style::StyleSheet::initializeStyleSheets([]() {});

    sst::basic_blocks::params::ParamMetaData::defaultMidiNoteOctaveOffset =
        defaultsProvider.getUserDefaultValue(infrastructure::octave0, 0);

    setStyle(sst::jucegui::style::StyleSheet::getBuiltInStyleSheet(
        sst::jucegui::style::StyleSheet::EMPTY));

    resetColorsFromUserPreferences();

    // TODO what happens with two windows open when I go away?
    lnf = std::make_unique<SCXTJuceLookAndFeel>();
    juce::LookAndFeel::setDefaultLookAndFeel(lnf.get());

    idleTimer = std::make_unique<IdleTimer>(this);
    idleTimer->startTimer(1000 / 60);

    toolTip = std::make_unique<sst::jucegui::components::ToolTip>();
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

    mixerScreen = std::make_unique<MixerScreen>(this);
    addChildComponent(*mixerScreen);

    playScreen = std::make_unique<PlayScreen>(this);
    addChildComponent(*playScreen);

    aboutScreen = std::make_unique<AboutScreen>(this);
    addChildComponent(*aboutScreen);

    welcomeScreen = std::make_unique<WelcomeScreen>(this);
    addChildComponent(*welcomeScreen);

    logScreen = std::make_unique<LogScreen>(this);
    addChildComponent(*logScreen);

    setStyle(style());

    auto zfi = defaultsProvider.getUserDefaultValue(infrastructure::DefaultKeys::zoomLevel, 100);
    setZoomFactor(zfi * 0.01);

    setAccessible(true);
    setFocusContainerType(juce::Component::FocusContainerType::keyboardFocusContainer);

    focusDebugger = std::make_unique<sst::jucegui::accessibility::FocusDebugger>(*this);
    focusDebugger->setDoFocusDebug(false);
}

SCXTEditor::~SCXTEditor() noexcept
{
    idleTimer->stopTimer();
    msgCont.unregisterClient();
}

void SCXTEditor::setActiveScreen(ActiveScreen s)
{
    activeScreen = s;
    aboutScreen->setVisible(false);
    logScreen->setVisible(false);
    std::string val{};
    switch (s)
    {
    case MULTI:
        multiScreen->setVisible(true);
        mixerScreen->setVisible(false);
        playScreen->setVisible(false);
        val = "multi";
        resized();
        break;

    case MIXER:
        multiScreen->setVisible(false);
        mixerScreen->setVisible(true);
        playScreen->setVisible(false);
        val = "mixer";
        resized();
        break;

    case PLAY:
        multiScreen->setVisible(false);
        mixerScreen->setVisible(false);
        playScreen->setVisible(true);
        val = "play";
        resized();
        break;
    }

    setTabSelection("main_screen", val);
    repaint();
}

void SCXTEditor::showAboutOverlay()
{
    aboutScreen->toFront(true);
    aboutScreen->setVisible(true);
    logScreen->setVisible(false);
    welcomeScreen->setVisible(false);
    resized();
}

void SCXTEditor::showLogOverlay()
{
    logScreen->toFront(true);
    logScreen->setVisible(true);
    aboutScreen->setVisible(false);
    welcomeScreen->setVisible(false);
    resized();
}

void SCXTEditor::showWelcomeOverlay()
{
    welcomeScreen->toFront(true);
    welcomeScreen->setVisible(true);
    aboutScreen->setVisible(false);
    logScreen->setVisible(false);
    resized();
}

void SCXTEditor::resized()
{
    static constexpr int32_t headerHeight{40};
    headerRegion->setBounds(0, 0, getWidth(), headerHeight);

    if (multiScreen->isVisible())
        multiScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
    if (mixerScreen->isVisible())
        mixerScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
    if (playScreen->isVisible())
        playScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
    if (aboutScreen->isVisible())
        aboutScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
    if (logScreen->isVisible())
        logScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
    if (welcomeScreen->isVisible())
        welcomeScreen->setBounds(0, 0, getWidth(), getHeight());
}

void SCXTEditor::idle()
{
    /*
     * The SCXT idle is just used for polling the shared memory items,
     * not for handling events which the message controller does
     * immediately
     */
    if (sharedUiMemoryState.voiceDisplayStateWriteCounter != lastVoiceDisplayWriteCounter)
    {
        lastVoiceDisplayWriteCounter = sharedUiMemoryState.voiceDisplayStateWriteCounter;
        if (headerRegion->voiceCount != sharedUiMemoryState.voiceCount)
        {
            headerRegion->setVoiceCount(sharedUiMemoryState.voiceCount);

            if (multiScreen->isVisible())
            {
                multiScreen->onVoiceInfoChanged();
            }
        }
    }

    if (multiScreen->isVisible())
    {
        if (currentLeadZoneSelection.has_value())
        {
            for (const auto &v : sharedUiMemoryState.voiceDisplayItems)
            {

                if (v.active && v.group == currentLeadZoneSelection->group &&
                    v.part == currentLeadZoneSelection->part &&
                    v.zone == currentLeadZoneSelection->zone)
                {
                    multiScreen->updateSamplePlaybackPosition(v.sample, v.samplePos);
                }
            }
        }
    }

    headerRegion->setVULevel(sharedUiMemoryState.busVULevels[0][0],
                             sharedUiMemoryState.busVULevels[0][1]);
    headerRegion->setCPULevel((double)sharedUiMemoryState.cpuLevel);

    if (mixerScreen->isVisible())
    {
        mixerScreen->setVULevelForBusses(sharedUiMemoryState.busVULevels);
    }

    headerRegion->setMemUsage(sampleManager.sampleMemoryInBytes);

    if (checkWelcomeCountdown == 0)
    {
        auto sc = defaultsProvider.getUserDefaultValue(scxt::infrastructure::welcomeScreenSeen, -1);
        if (sc != WelcomeScreen::welcomeVersion)
        {
            showWelcomeOverlay();
        }
        checkWelcomeCountdown = -1;
    }
    else if (checkWelcomeCountdown > 0)
    {
        checkWelcomeCountdown--;
    }
}

void SCXTEditor::drainCallbackQueue()
{
    namespace cmsg = scxt::messaging::client;

    bool itemsToDrain{true};
    while (itemsToDrain)
    {
        itemsToDrain = false;
        std::string queueMsg;
        {
            std::lock_guard<std::mutex> g(callbackMutex);
            if (!callbackQueue.empty())
            {
                queueMsg = callbackQueue.front();
                itemsToDrain = true;
                callbackQueue.pop();
            }
        }
        if (itemsToDrain)
        {
            assert(msgCont.threadingChecker.isClientThread());
            cmsg::clientThreadExecuteSerializationMessage(queueMsg, this);
#if BUILD_IS_DEBUG
            inboundMessageCount++;
            inboundMessageBytes += queueMsg.size();
            if (inboundMessageCount % 500 == 0)
            {
                SCLOG("Serial -> Client Message Count: "
                      << inboundMessageCount << " size: " << inboundMessageBytes
                      << " avg msg: " << inboundMessageBytes / inboundMessageCount);
            }
#endif
        }
    }
}

void SCXTEditor::doSelectionAction(const selection::SelectionManager::ZoneAddress &a,
                                   bool selecting, bool distinct, bool asLead)
{
    namespace cmsg = scxt::messaging::client;
    currentLeadZoneSelection = a;
    sendToSerialization(cmsg::DoSelectAction(selection::SelectionManager::SelectActionContents{
        a.part, a.group, a.zone, selecting, distinct, asLead}));
    repaint();
}

void SCXTEditor::doSelectionAction(const selection::SelectionManager::SelectActionContents &p)
{
    namespace cmsg = scxt::messaging::client;
    sendToSerialization(cmsg::DoSelectAction(p));
    repaint();
}
void SCXTEditor::doMultiSelectionAction(
    const std::vector<selection::SelectionManager::SelectActionContents> &p)
{
    namespace cmsg = scxt::messaging::client;
    sendToSerialization(cmsg::DoMultiSelectAction(p));
    repaint();
}

void SCXTEditor::showTooltip(const juce::Component &relativeTo)
{
    auto fb = getLocalArea(&relativeTo, relativeTo.getLocalBounds());
    toolTip->resetSizeFromData();
    toolTip->setVisible(true);
    toolTip->toFront(false);
    toolTip->setBounds(fb.getWidth() + fb.getX(), fb.getY(), toolTip->getWidth(),
                       toolTip->getHeight());
}

void SCXTEditor::hideTooltip() { toolTip->setVisible(false); }

void SCXTEditor::setTooltipContents(const std::string &title, const std::vector<std::string> &data)
{
    toolTip->setTooltipTitleAndData(title, data);
}

void SCXTEditor::parentHierarchyChanged() { setZoomFactor(zoomFactor); }

void SCXTEditor::setZoomFactor(float zf)
{
    // SCLOG("Setting zoom factor to " << zf);
    zoomFactor = zf;
    setTransform(juce::AffineTransform().scaled(zoomFactor));
    defaultsProvider.updateUserDefaultValue(infrastructure::DefaultKeys::zoomLevel,
                                            zoomFactor * 100);
    if (onZoomChanged)
        onZoomChanged(zoomFactor);
}

void SCXTEditor::configureHasDiscreteMenuBuilder(
    sst::jucegui::components::HasDiscreteParamMenuBuilder *wss)
{
    wss->guaranteeMenuBuilder();
    wss->popupMenuBuilder->createMenuOptions = [w = juce::Component::SafePointer(this)]() {
        if (w)
            return w->defaultPopupMenuOptions();
        return juce::PopupMenu::Options();
    };
}

int16_t SCXTEditor::getSelectedPart() const { return selectedPart; }

juce::Colour SCXTEditor::themeColor(scxt::ui::theme::ColorMap::Colors c, float alpha) const
{
    return themeApplier.colors->get(c, alpha);
}

void SCXTEditor::resetColorsFromUserPreferences()
{
    auto cmid = defaultsProvider.getUserDefaultValue(infrastructure::DefaultKeys::colormapId,
                                                     theme::ColorMap::WIREFRAME);
    std::unique_ptr<theme::ColorMap> cm;
    if (cmid == theme::ColorMap::FILE_COLORMAP_ID)
    {
        auto pt = defaultsProvider.getUserDefaultValue(
            infrastructure::DefaultKeys::colormapPathIfFile, "");
        try
        {
            auto path = fs::path{pt};
            if (fs::exists(path))
            {
                std::ifstream t(path);
                std::stringstream buffer;
                buffer << t.rdbuf();
                cm = theme::ColorMap::jsonToColormap(buffer.str());
            }
        }
        catch (fs::filesystem_error &e)
        {
        }
        if (!cm)
        {
            SCLOG("Attempted to load colormap from missing colormap file " << pt);
            cm = theme::ColorMap::createColorMap(theme::ColorMap::WIREFRAME);
        }
    }
    else
    {
        cm = theme::ColorMap::createColorMap((theme::ColorMap::BuiltInColorMaps)cmid);
    }

    assert(cm);
    auto showK =
        defaultsProvider.getUserDefaultValue(infrastructure::DefaultKeys::showKnobs, false);
    cm->hasKnobs = showK;

    themeApplier.recolorStylesheetWith(std::move(cm), style());
}

std::string SCXTEditor::queryTabSelection(const std::string &k)
{
    auto p = otherTabSelection.find(k);
    if (p != otherTabSelection.end())
        return p->second;
    return {};
}

void SCXTEditor::setTabSelection(const std::string &k, const std::string &t)
{
    sendToSerialization(messaging::client::UpdateOtherTabSelection({k, t}));
}
} // namespace scxt::ui
