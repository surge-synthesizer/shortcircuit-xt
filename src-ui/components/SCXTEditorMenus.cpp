/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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
#include "version.h"
#include "connectors/SCXTStyleSheetCreator.h"
#include "infrastructure/user_defaults.h"

namespace scxt::ui
{
namespace cmsg = scxt::messaging::client;

void SCXTEditor::showMainMenu()
{
    juce::PopupMenu m;

    m.addSectionHeader("ShortCircuit XT");
#if BUILD_IS_DEBUG
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

    m.showMenuAsync(defaultPopupMenuOptions());
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
        p.addSectionHeader("Tuning");
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
    r.addSectionHeader("Hacky Mapping Zoom");
    r.addSeparator();
    r.addItem("Full Midi", [w = juce::Component::SafePointer(this)] {
        if (w)
            w->multiScreen->sample->temporarySetKeyboardCenter(-1);
    });
    for (auto v : {24, 36, 48, 60, 72, 84})
    {
        r.addItem("2 Oct Around " + std::to_string(v), [v, w = juce::Component::SafePointer(this)] {
            if (w)
                w->multiScreen->sample->temporarySetKeyboardCenter(v);
        });
    }
    p.addSeparator();
    p.addSubMenu("Mapping Zoom Hack", r);
}

void SCXTEditor::addUIThemesMenu(juce::PopupMenu &p, bool addTitle)
{
    if (addTitle)
    {
        p.addSectionHeader("UI Behavior");
        p.addSeparator();
    }
    p.addSectionHeader("Themes");
    p.addItem("Dark", [w = juce::Component::SafePointer(this)]() {
        if (w)
        {
            w->setStyle(scxt::ui::connectors::SCXTStyleSheetCreator::setup(
                sst::jucegui::style::StyleSheet::DARK));
            w->defaultsProvider.updateUserDefaultPath(infrastructure::skinName, "builtin.DARK");
        }
    });

    p.addItem("Light", [w = juce::Component::SafePointer(this)]() {
        if (w)
        {
            w->setStyle(scxt::ui::connectors::SCXTStyleSheetCreator::setup(
                sst::jucegui::style::StyleSheet::LIGHT));
            w->defaultsProvider.updateUserDefaultValue(infrastructure::skinName, "builtin.LIGHT");
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
                      }
                  });
    }
}

} // namespace scxt::ui