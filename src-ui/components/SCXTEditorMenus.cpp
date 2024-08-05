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
#include "MultiScreen.h"
#include "multi/AdsrPane.h"
#include "multi/LFOPane.h"
#include "multi/ModPane.h"
#include "multi/ProcessorPane.h"
#include "multi/PartGroupSidebar.h"
#include "HeaderRegion.h"
#include "components/multi/MappingPane.h"
#include "components/AboutScreen.h"
#include "infrastructure/user_defaults.h"

#include <version.h>

namespace scxt::ui
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
    m.addItem("About", [w = juce::Component::SafePointer(this)] {
        if (w)
            w->showAboutOverlay();
    });
    m.addItem("Log", [w = juce::Component::SafePointer(this)] {
        if (w)
            w->showLogOverlay();
    });
    m.addItem("Source", [] {
        juce::URL("https://github.com/surge-synthesizer/shortcircuit-xt").launchInDefaultBrowser();
    });

    m.addSeparator();
    auto dp = juce::PopupMenu();
    dp.addSectionHeader("Developer");
    dp.addSeparator();
    dp.addItem("Pretty JSON", [w = juce::Component::SafePointer(this)]() {
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
        p.addItem(juce::String(v) + "%", [w = juce::Component::SafePointer(this), v]() {
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

                      w->multiScreen->sample->invertScroll(newScrollBehavior);
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

} // namespace scxt::ui
