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

#if HAS_MELATONIN_INSPECTOR
#include "melatonin_inspector/melatonin_inspector.h"
#endif

#include "infrastructure/user_defaults.h"
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

#include <version.h>

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

    m.addSeparator();
    m.addItem(juce::String("Copy ") + scxt::build::FullVersionStr,
              [w = juce::Component::SafePointer(this)] {
                  if (w)
                      w->aboutScreen->copyInfo();
              });
    m.addItem("Show About Screen", [w = juce::Component::SafePointer(this)] {
        if (w)
            w->showAboutOverlay();
    });
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
    auto dp = juce::PopupMenu();
    dp.addSectionHeader("Developer");
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
    dp.addItem("Dump Colormap JSON", [this]() { SCLOG(themeApplier.colors->toJson()); });
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
    p.addItem("Twelve TET", [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->sendToSerialization(cmsg::SetTuningMode(tuning::MidikeyRetuner::TWELVE_TET));
    });
    p.addItem("MTS-ESP", [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->sendToSerialization(cmsg::SetTuningMode(tuning::MidikeyRetuner::MTS_ESP));
    });
}

void SCXTEditor::addZoomMenu(juce::PopupMenu &p, bool addTitle)
{
    if (addTitle)
    {
        p.addSectionHeader("Zoom");
        p.addSeparator();
    }
    for (auto v : {75, 100, 125, 150, 200})
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
        {theme::ColorMap::WIREFRAME, "Wireframe Colors"},
        {theme::ColorMap::LIGHT, "Wireframe Light"},
        {theme::ColorMap::HICONTRAST_DARK, "High Contrast Dark"},
        {theme::ColorMap::TEST, "Test Colors"},
    };
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
                w->defaultsProvider.updateUserDefaultValue(
                    infrastructure::DefaultKeys::colormapPathIfFile,
                    result[0].getFullPathName().toStdString());
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
    for (int i = -1; i <= 1; ++i)
    {
        p.addItem(fmt::format("C{} is MIDI 60", 4 + i),
                  [w = juce::Component::SafePointer(this), i]() {
                      if (w)
                      {
                          w->defaultsProvider.updateUserDefaultValue(infrastructure::octave0, i);
                          sst::basic_blocks::params::ParamMetaData::defaultMidiNoteOctaveOffset = i;
                      }
                  });
    }
}

void SCXTEditor::popupMenuForContinuous(sst::jucegui::components::ContinuousParamEditor *e)
{
    auto data = e->continuous();
    if (!data)
    {
        SCLOG("Continuous with no data - no popup to be had");
        return;
    }

    if (!e->isEnabled())
    {
        SCLOG("No menu on non-enabled widget");
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

    p.showMenuAsync(defaultPopupMenuOptions());
}

} // namespace scxt::ui::app
