/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#if HAS_MELATONIN_INSPECTOR
#include "melatonin_inspector/melatonin_inspector.h"
#endif

#include "infrastructure/user_defaults.h"
#include "sst/plugininfra/version_information.h"
#include "sst/jucegui/component-adapters/ComponentTags.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/clap_juce_shim/menu_helper.h"

#include "app/SCXTEditor.h"
#include "app/edit-screen/EditScreen.h"
#include "app/edit-screen/components/AdsrPane.h"
#include "app/edit-screen/components/LFOPane.h"
#include "app/edit-screen/components/ModPane.h"
#include "app/edit-screen/components/ProcessorPane.h"
#include "app/edit-screen/components/PartGroupSidebar.h"
#include "app/shared/HeaderRegion.h"
#include "app/edit-screen/components/MacroMappingVariantPane.h"
#include "app/other-screens/AboutScreen.h"
#include "app/shared/MenuValueTypein.h"
#include "app/shared/UIHelpers.h"

namespace scxt::ui::app
{
namespace cmsg = scxt::messaging::client;

void SCXTEditor::showMainMenu()
{
    juce::PopupMenu m;

    m.addSectionHeader("ShortCircuit XT");
#if BUILD_IS_DEBUG
    m.addSeparator();
    m.addSectionHeader("(Debug Build)");
#endif

    m.addSeparator();

    juce::PopupMenu tun;
    addTuningMenu(tun);
    m.addSubMenu("Tuning", tun);
    juce::PopupMenu zoom;
    addZoomMenu(zoom);
    m.addSubMenu("Zoom", zoom);

    juce::PopupMenu skin;
    addUIThemesMenu(skin);
    m.addSubMenu("UI Behavior", skin);

    m.addItem("Show Log", [w = juce::Component::SafePointer(this)] {
        if (w)
            w->showLogOverlay();
    });

    m.addItem("Show Welcome Screen", [w = juce::Component::SafePointer(this)] {
        if (w)
            w->showWelcomeOverlay();
    });
    m.addItem("Read Source", [] {
        juce::URL("https://github.com/surge-synthesizer/shortcircuit-xt").launchInDefaultBrowser();
    });

    m.addSeparator();
    m.addItem("About Shortcircuit XT", [w = juce::Component::SafePointer(this)] {
        if (w)
            w->showAboutOverlay();
    });

    m.addSeparator();
    auto dp = juce::PopupMenu();
    dp.addSectionHeader("Developer");
    dp.addSeparator();
    dp.addItem("Resend full engine state to UI", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::ResendFullState{true});
    });
    dp.addSeparator();
    dp.addItem("Pretty JSON (DAW)", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::RequestDebugAction{cmsg::DebugActions::pretty_json_daw});
    });
    dp.addItem("Pretty JSON (MULTI)", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::RequestDebugAction{cmsg::DebugActions::pretty_json_multi});
    });
    dp.addItem("Pretty JSON (PROCESS)", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::RequestDebugAction{cmsg::DebugActions::pretty_json});
    });
    dp.addItem("Pretty JSON Selected Part", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::RequestDebugAction{cmsg::DebugActions::pretty_json_part});
    });
    // dp.addItem("Focus Debugger Toggle", []() {});
    dp.addSeparator();
    dp.addItem("Dump Colormap JSON", [this]() { SCLOG_IF(always, themeApplier.colors->toJson()); });
    dp.addSeparator();
    dp.addItem("Raise Dummy Error", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::RaiseDebugError{true});
    });
    dp.addItem("Raise Dummy OK Cancel", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->promptOKCancel(
            "Dummy OK Cancel", "This is the dummy OK Cancel Message",
            []() { SCLOG_IF(always, "OK Pressed"); }, []() { SCLOG_IF(always, "Cancel Pressed"); });
    });
    dp.addSeparator();
    dp.addItem("Toggle Focus Debugger", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->focusDebugger->setDoFocusDebug(!w->focusDebugger->doFocusDebug);
    });
    dp.addSeparator();

#if HAS_MELATONIN_INSPECTOR
    if (melatoninInspector)
    {
        dp.addItem("Close Melatonin Inspector", [w = juce::Component::SafePointer(this)]() {
            if (w && w->melatoninInspector)
            {
                w->melatoninInspector->setVisible(false);
                w->melatoninInspector.reset();
            }
        });
    }
    else
    {
        dp.addItem("Launch Melatonin Inspector", [w = juce::Component::SafePointer(this)] {
            if (!w)
                return;

            w->melatoninInspector = std::make_unique<melatonin::Inspector>(*w);
            w->melatoninInspector->onClose = [w]() { w->melatoninInspector.reset(); };

            w->melatoninInspector->setVisible(true);
        });
    }
#endif

    dp.addItem("Release all Voices", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::StopSounds{false});
    });
    dp.addItem("Stop all Sounds", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::StopSounds{true});
    });

    m.addSubMenu("Developer", dp);

    if (headerRegion && headerRegion->scMenu)
    {
        m.showMenuAsync(defaultPopupMenuOptions(headerRegion->scMenu.get()));
    }
    else
    {
        m.showMenuAsync(defaultPopupMenuOptions());
    }
}

void SCXTEditor::addTuningMenu(juce::PopupMenu &p, bool addTitle)
{
    if (addTitle)
    {
        p.addSectionHeader("Tuning");
        p.addSeparator();
    }
    auto st = editScreen->editor->tuningStatus;
    p.addItem("Twelve TET", true, st.first == engine::Engine::TuningMode::TWELVE_TET,
              [st, w = juce::Component::SafePointer(this)]() {
                  if (w)
                  {
                      auto s = st;
                      s.first = engine::Engine::TuningMode::TWELVE_TET;
                      w->sendToSerialization(cmsg::SetTuningMode(s));
                  }
              });
    p.addItem("MTS-ESP (if active)", true, st.first == engine::Engine::TuningMode::MTS_CONTINOUS,
              [st, w = juce::Component::SafePointer(this)]() {
                  if (w)
                  {
                      auto s = st;
                      s.first = engine::Engine::TuningMode::MTS_CONTINOUS;
                      w->sendToSerialization(cmsg::SetTuningMode(s));
                  }
              });
    p.addItem("MTS-ESP NoteOn (if active)", true,
              st.first == engine::Engine::TuningMode::MTS_NOTE_ON,
              [st, w = juce::Component::SafePointer(this)]() {
                  if (w)
                  {
                      auto s = st;
                      s.first = engine::Engine::TuningMode::MTS_NOTE_ON;
                      w->sendToSerialization(cmsg::SetTuningMode(s));
                  }
              });
}

void SCXTEditor::addOmniFlavorMenu(juce::PopupMenu &p)
{
    p.addSectionHeader("MIDI Channel usage");
    p.addSeparator();

    std::string n[3] = {"Omni", "MPE", "Channel/Octave"};

    for (int i = 0; i < 3; ++i)
    {
        p.addItem(n[i], true, this->currentOmniFlavor == i,
                  [w = juce::Component::SafePointer(this), i]() {
                      if (w)
                          w->setOmniFlavor(i);
                  });
    }
    p.addSeparator();

    p.addItem("Use " + n[this->currentOmniFlavor] + " as default for new instances", true, false,
              [w = juce::Component::SafePointer(this)]() {
                  if (w)
                      w->setOmniFlavorDefault(w->currentOmniFlavor);
              });

    repaint();
}

void SCXTEditor::addZoomMenu(juce::PopupMenu &p, bool addTitle)
{
    if (addTitle)
    {
        p.addSectionHeader("Zoom");
        p.addSeparator();
    }
    for (auto v : {75, 80, 90, 100, 110, 125, 150, 200})
    {
        bool checked = (std::fabs(zoomFactor * 100 - v) < 1);
        p.addItem(juce::String(v) + "%", true, checked,
                  [w = juce::Component::SafePointer(this), v]() {
                      if (w)
                          w->setZoomFactor(v * 0.01);
                  });
    }

    auto r = juce::PopupMenu();
    auto invertScroll = defaultsProvider.getUserDefaultValue(infrastructure::invertScroll, 0) == 1;
    r.addItem("Invert Mouse Scroll Direction", true, invertScroll,
              [w = juce::Component::SafePointer(this), invertScroll]() {
                  if (w)
                  {
                      auto newScrollBehavior = !invertScroll;
                      w->defaultsProvider.updateUserDefaultValue(
                          infrastructure::DefaultKeys::invertScroll, newScrollBehavior ? 1 : 0);

                      w->editScreen->mappingPane->invertScroll(newScrollBehavior);
                  }
              });

    p.addSeparator();
    p.addSubMenu("Mapping Zoom", r);
}

void SCXTEditor::addUIThemesMenu(juce::PopupMenu &p, bool addTitle)
{
    if (addTitle)
    {
        p.addSectionHeader("UI Behavior");
        p.addSeparator();
    }
    p.addSectionHeader("Themes");
    std::vector<std::pair<theme::ColorMap::BuiltInColorMaps, std::string>> maps = {
        {theme::ColorMap::WIREFRAME, "Default Colors"},

        {theme::ColorMap::CELTIC, "Celtic"},
        {theme::ColorMap::GRAYLOW, "GrayLow"},
        {theme::ColorMap::LUX2, "Lux2"},
        {theme::ColorMap::OCEANOR, "Oceanor"}};
    auto cid = themeApplier.colors->myId;
    for (const auto &[mo, d] : maps)
    {
        p.addItem(d, true, cid == mo, [m = mo, w = juce::Component::SafePointer(this)]() {
            if (w)
            {
                auto cm = theme::ColorMap::createColorMap(m);
                auto showK = w->defaultsProvider.getUserDefaultValue(
                    infrastructure::DefaultKeys::showKnobs, false);
                cm->hasKnobs = showK;
                w->defaultsProvider.updateUserDefaultValue(infrastructure::DefaultKeys::colormapId,
                                                           m);
                w->themeApplier.recolorStylesheetWith(std::move(cm), w->style());
                w->setStyle(w->style());
            }
        });
    }

    p.addSeparator();
    p.addItem("Save Theme...", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->fileChooser = std::make_unique<juce::FileChooser>(
            "Save Theme", juce::File(w->browser.themeDirectory.u8string()), "*.sctheme");
        w->fileChooser->launchAsync(juce::FileBrowserComponent::canSelectFiles |
                                        juce::FileBrowserComponent::saveMode |
                                        juce::FileBrowserComponent::warnAboutOverwriting,
                                    [w](const juce::FileChooser &c) {
                                        auto result = c.getResults();
                                        if (result.isEmpty() || result.size() > 1)
                                        {
                                            return;
                                        }
                                        // send a 'save multi' message
                                        auto json = w->themeApplier.colors->toJson();
                                        result[0].replaceWithText(json);
                                    });
    });
    p.addItem("Load Theme...", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->fileChooser = std::make_unique<juce::FileChooser>(
            "Load Theme", juce::File(w->browser.themeDirectory.u8string()), "*.sctheme");
        w->fileChooser->launchAsync(
            juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::openMode,
            [w](const juce::FileChooser &c) {
                auto result = c.getResults();
                if (result.isEmpty() || result.size() > 1)
                {
                    return;
                }
                auto json = result[0].loadFileAsString();
                auto cm = theme::ColorMap::jsonToColormap(json.toStdString());
                if (cm)
                    w->themeApplier.recolorStylesheetWith(std::move(cm), w->style());
                w->defaultsProvider.updateUserDefaultValue(infrastructure::DefaultKeys::colormapId,
                                                           theme::ColorMap::FILE_COLORMAP_ID);
                auto fp = shared::juceFileToFSPath(result[0]);
                w->defaultsProvider.updateUserDefaultValue(
                    infrastructure::DefaultKeys::colormapPathIfFile, fp.u8string());
            });
    });
    p.addSeparator();
    auto knobsOn = themeApplier.colors->hasKnobs;
    p.addItem("Use Knob Bodies", true, knobsOn, [w = juce::Component::SafePointer(this)]() {
        if (w)
        {
            w->themeApplier.colors->hasKnobs = !w->themeApplier.colors->hasKnobs;
            w->defaultsProvider.updateUserDefaultValue(infrastructure::DefaultKeys::showKnobs,
                                                       w->themeApplier.colors->hasKnobs);

            w->themeApplier.recolorStylesheet(w->style());
            w->setStyle(w->style());
        }
    });

    p.addSeparator();
    p.addSectionHeader("MIDI Octave");
    auto cv = defaultsProvider.getUserDefaultValue(infrastructure::octave0, 0);
    for (int i = -1; i <= 1; ++i)
    {
        p.addItem(fmt::format("C{} is MIDI 60", 4 + i), true, i == cv,
                  [w = juce::Component::SafePointer(this), i]() {
                      if (w)
                      {
                          w->defaultsProvider.updateUserDefaultValue(infrastructure::octave0, i);
                          sst::basic_blocks::params::ParamMetaData::defaultMidiNoteOctaveOffset = i;
                          w->repaint();
                      }
                  });
    }

#if JUCE_WINDOWS
    p.addSeparator();
    auto swr = defaultsProvider.getUserDefaultValue(
        infrastructure::DefaultKeys::useSoftwareRenderer, false);

    p.addItem("Use Software Renderer", true, swr, [w = juce::Component::SafePointer(this), swr]() {
        if (!w)
            return;
        w->defaultsProvider.updateUserDefaultValue(infrastructure::DefaultKeys::useSoftwareRenderer,
                                                   !swr);
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon, "Software Renderer Change",
            "A software renderer change is only active once you restart/reload the plugin.");
    });
#endif
}

bool SCXTEditor::supressPopupMenuForContinuous(
    sst::jucegui::components::ContinuousParamEditor *e) const
{
    // Make this a standalone function so we could add a user pref or ally hook in the future
    // This basically supresses the popup on any draggable text edit now.
    if (!e)
        return true;

    if (dynamic_cast<sst::jucegui::components::DraggableTextEditableValue *>(e))
        return true;

    return false;
}

void SCXTEditor::popupMenuForContinuous(sst::jucegui::components::ContinuousParamEditor *e)
{
    if (supressPopupMenuForContinuous(e))
        return;

    auto data = e->continuous();
    if (!data)
    {
        SCLOG_IF(warnings, "Continuous with no data - no popup to be had");
        return;
    }

    if (!e->isEnabled())
    {
        return;
    }

    auto p = juce::PopupMenu();
    p.addSectionHeader(data->getLabel());
    p.addSeparator();
    p.addCustomItem(
        -1, std::make_unique<shared::MenuValueTypein>(this, juce::Component::SafePointer(e)));
    p.addSeparator();
    p.addItem("Set to Default", [w = juce::Component::SafePointer(e)]() {
        if (!w)
            return;
        w->continuous()->setValueFromGUI(w->continuous()->getDefaultValue());
    });

    auto pid = sst::jucegui::component_adapters::getClapParamId(e);
    if (pid.has_value())
    {
        sst::clap_juce_shim::populateMenuForClapParam(p, *pid, clapHost);
    }

    p.showMenuAsync(defaultPopupMenuOptions());
}

} // namespace scxt::ui::app
