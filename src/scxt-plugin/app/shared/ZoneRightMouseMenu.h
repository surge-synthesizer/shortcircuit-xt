/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_SHARED_ZONERIGHTMOUSEMENU_H
#define SCXT_SRC_SCXT_PLUGIN_APP_SHARED_ZONERIGHTMOUSEMENU_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "selection/selection_manager.h"
#include "messaging/client/client_messages.h"

#include <app/edit-screen/components/PartGroupSidebar.h>
#include <app/edit-screen/EditScreen.h>

namespace scxt::ui::app::shared
{
inline engine::Engine::pgzStructure_t
findOtherGroups(SCXTEditor *e,
                const selection::SelectionManager::ZoneAddress &forZone = {-1, -1, -1})
{
    auto &pg = e->editScreen->partSidebar->pgzStructure;
    auto res = engine::Engine::pgzStructure_t{};

    for (const auto &[a, n, f] : pg)
    {
        if (a.part == e->selectedPart && a.zone == -1 && a.group >= 0 && a.group != forZone.group)
        {
            res.push_back({a, n, f});
        }
    }
    return res;
}
template <typename SendingComp, typename RenamingComp> // COMP is a HasEditor and a few other things
void populateZoneRightMouseMenuForZone(SendingComp *that, RenamingComp *rnThat, juce::PopupMenu &p,
                                       const selection::SelectionManager::ZoneAddress &forZone,
                                       const std::string &zoneSectionName)
{
    namespace cmsg = scxt::messaging::client;

    p.addSectionHeader(zoneSectionName);
    p.addItem("Rename", [w = juce::Component::SafePointer(rnThat), forZone]() {
        if (!w)
            return;
        w->doZoneRename(forZone);
    });
    p.addItem("Copy", [w = juce::Component::SafePointer(that), forZone]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::CopyZone(forZone));
    });
    p.addItem("Paste", that->editor->clipboardType == engine::Clipboard::ContentType::ZONE, false,
              [w = juce::Component::SafePointer(that), forZone]() {
                  if (!w)
                      return;
                  w->sendToSerialization(cmsg::PasteZone(forZone));
              });
    p.addItem("Duplicate", [w = juce::Component::SafePointer(that), forZone]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::DuplicateZone(forZone));
    });
    p.addItem("Delete", [w = juce::Component::SafePointer(that), forZone]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::DeleteZone(forZone));
    });

    auto moveMenu = juce::PopupMenu();
    moveMenu.addItem("New Group", [w = juce::Component::SafePointer(that), forZone]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::MoveZonesFromTo(
            {{forZone},
             selection::SelectionManager::ZoneAddress{w->editor->selectedPart, -1, -1}}));
    });
    moveMenu.addItem("New Duplicate Group", false, false, []() {});

    auto og = findOtherGroups(that->editor, forZone);
    if (!og.empty())
    {
        auto g = juce::PopupMenu();
        for (const auto &sg : og)
        {
            auto &n = sg.name;
            auto &a = sg.address;
            g.addItem(n, [forZone, w = juce::Component::SafePointer(that), a]() {
                if (!w)
                    return;
                w->sendToSerialization(cmsg::MoveZonesFromTo({{forZone}, a}));
            });
        }
        moveMenu.addSubMenu("Group", g);
    }
    p.addSubMenu("Move to", moveMenu);
}

template <typename SendingComp>
void populateZoneRightMouseMenuForSelectedZones(SendingComp *that, juce::PopupMenu &p, int part,
                                                const std::string &name = "")
{
    namespace cmsg = scxt::messaging::client;

    if (name.empty())
        p.addSectionHeader("Selected Zones");
    else
        p.addSectionHeader(name);

    p.addItem("Delete All", [w = juce::Component::SafePointer(that)]() {
        if (!w)
            return;

        w->sendToSerialization(cmsg::DeleteAllSelectedZones(true));
    });

    auto moveMenu = juce::PopupMenu();
    moveMenu.addItem("New Group", [w = juce::Component::SafePointer(that)]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::MoveZonesFromTo(
            {{}, selection::SelectionManager::ZoneAddress{w->editor->selectedPart, -1, -1}}));
    });
    moveMenu.addItem("New Duplicate Group", false, false, []() {});
    auto og = findOtherGroups(that->editor);
    if (!og.empty())
    {
        auto g = juce::PopupMenu();
        for (const auto &sg : og)
        {
            auto &n = sg.name;
            auto &a = sg.address;
            g.addItem(n, [w = juce::Component::SafePointer(that), a]() {
                if (!w)
                    return;
                w->sendToSerialization(cmsg::MoveZonesFromTo({{}, a}));
            });
        }
        moveMenu.addSubMenu("Group", g);
    }
    p.addSubMenu("Move All to", moveMenu);

    p.addSeparator();
    p.addSectionHeader("Current Part");
    p.addItem("Delete All Zones and Groups", [w = juce::Component::SafePointer(that), part]() {
        if (!w)
            return;

        w->sendToSerialization(cmsg::ClearPart(part));
    });

    if (that->editor->hasMissingSamples)
    {
        p.addSeparator();
        p.addItem("Resolve Missing Samples", [w = juce::Component::SafePointer(that)]() {
            if (!w)
                return;
            w->editor->showMissingResolutionScreen();
        });
    }
}
} // namespace scxt::ui::app::shared

#endif // ZONERIGHTMOUSEMENU_H
