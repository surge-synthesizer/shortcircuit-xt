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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_SHARED_ZONERIGHTMOUSEMENU_H
#define SCXT_SRC_SCXT_PLUGIN_APP_SHARED_ZONERIGHTMOUSEMENU_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "selection/selection_manager.h"
#include "messaging/client/client_messages.h"

#include <app/edit-screen/components/PartGroupSidebar.h>
#include <app/edit-screen/EditScreen.h>
#include "app/KeyCommands.h"
#include "app/editor-impl/KeyBindings.h"
#include "app/shared/UIHelpers.h"
#include "browser/browser.h"
#include "engine/feature_enums.h"

namespace scxt::ui::app::shared
{
using za_t = selection::SelectionManager::ZoneAddress;

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

inline void addShortcutItem(SCXTEditor *ed, juce::PopupMenu &p, const std::string &label,
                            KeyCommands command, bool enabled, std::function<void()> action)
{
    juce::PopupMenu::Item item(label);
    item.shortcutKeyDescription = ed->keyBindings->shortcutDescription(command);
    item.setEnabled(enabled);
    item.setAction(std::move(action));
    p.addItem(std::move(item));
}

inline std::string revealLabel()
{
#if MAC
    return "Reveal Sample in Finder";
#elif WINDOWS
    return "Reveal Sample in Explorer";
#else
    return "Reveal Sample in File Manager";
#endif
}

// a click inside the selection means all of it; no click means the selection, else the lead
struct MenuTargets
{
    std::vector<za_t> items;
    bool isSelection{false};
};

inline MenuTargets menuTargets(const selection::SelectionManager::selectedZones_t &all,
                               const std::optional<za_t> &lead, const std::optional<za_t> &clicked)
{
    MenuTargets res;
    if (clicked.has_value() && !(all.count(*clicked) > 0 && all.size() > 1))
    {
        res.items.push_back(*clicked);
        return res;
    }
    res.items.assign(all.begin(), all.end());
    std::sort(res.items.begin(), res.items.end());
    res.isSelection = !res.items.empty();
    if (res.items.empty() && lead.has_value() && lead->group >= 0)
        res.items.push_back(*lead);
    return res;
}

// the ZONES menu: tree rows, the zones hamburger and the mapping pane all build it here
template <typename SendingComp>
void populateZoneMenu(SendingComp *that, juce::PopupMenu &p, const std::optional<za_t> &clicked,
                      const std::string &clickedName, std::function<void()> onRename = {})
{
    namespace cmsg = scxt::messaging::client;
    using w_t = juce::Component::SafePointer<SendingComp>;

    auto *ed = that->editor;
    auto part = (int16_t)ed->selectedPart;
    const auto &lead = ed->currentLeadZoneSelection;
    auto leadSet = lead.has_value() && lead->group >= 0 && lead->zone >= 0;
    auto t = menuTargets(ed->allZoneSelections, leadSet ? lead : std::nullopt, clicked);
    auto targets = t.items;
    auto any = !targets.empty();
    auto single = targets.size() == 1;

    if (t.isSelection && targets.size() > 1)
        p.addSectionHeader(std::to_string(targets.size()) + " Selected Zones");
    else if (clicked.has_value())
        p.addSectionHeader(clickedName);
    else
        p.addSectionHeader("Zones");

    if (onRename)
    {
        p.addItem("Rename", std::move(onRename));
        p.addSeparator();
    }

    // a lone clicked zone deletes by address, anything else is the selection
    auto deleteTargets = [isSel = t.isSelection || !clicked.has_value(), targets](auto *w) {
        if (isSel)
            w->sendToSerialization(cmsg::DeleteAllSelectedZones(true));
        else if (!targets.empty())
            w->sendToSerialization(cmsg::DeleteZone(targets.front()));
    };

    addShortcutItem(ed, p, "Cut", CUT, any, [w = w_t(that), targets, deleteTargets]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::CopyZones(targets));
        deleteTargets(w.getComponent());
    });
    addShortcutItem(ed, p, "Copy", COPY, any, [w = w_t(that), targets]() {
        if (w)
            w->sendToSerialization(cmsg::CopyZones(targets));
    });
    addShortcutItem(ed, p, "Duplicate", DUPLICATE, any, [w = w_t(that), targets]() {
        if (w)
            w->sendToSerialization(cmsg::DuplicateZones(targets));
    });

    // after the clicked or lead zone, else the end of the lead group, else wherever a new zone goes
    auto pasteInto = za_t{part, -1, -1};
    const auto &leadGroup = ed->currentLeadGroupSelection;
    if (clicked.has_value())
        pasteInto = *clicked;
    else if (leadSet)
        pasteInto = *lead;
    else if (leadGroup.has_value() && leadGroup->group >= 0)
        pasteInto = *leadGroup;
    addShortcutItem(ed, p, "Paste", PASTE,
                    ed->clipboardType == engine::Clipboard::ContentType::ZONE,
                    [w = w_t(that), pasteInto]() {
                        if (w)
                            w->sendToSerialization(cmsg::PasteZone(pasteInto));
                    });
    addShortcutItem(ed, p, "Delete", DELETE_SELECTED, any, [w = w_t(that), deleteTargets]() {
        if (w)
            deleteTargets(w.getComponent());
    });

    bool anyMissing{false};
    for (const auto &r : ed->editScreen->partSidebar->pgzStructure)
        if (r.address.part == part && r.address.zone >= 0 &&
            (r.features & engine::GroupZoneFeatures::MISSING_SAMPLE))
            anyMissing = true;
    p.addItem("Delete All Zones with Missing Samples", anyMissing, false, [w = w_t(that), part]() {
        if (w)
            w->sendToSerialization(cmsg::DeleteZonesWithMissingSamples(part));
    });

    p.addSeparator();
    auto one = single ? targets.front() : za_t{};
    p.addItem("Exchange Sample...", single, false, [w = w_t(that), one]() {
        if (!w)
            return;
        std::string pattern;
        for (const auto &s : scxt::browser::Browser::LoadableFile::singleSample)
            pattern += (pattern.empty() ? "*" : ",*") + s;
        auto *e = w->editor;
        e->fileChooser =
            std::make_unique<juce::FileChooser>("Exchange Sample", juce::File(), pattern);
        e->fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [w, one](const juce::FileChooser &fc) {
                if (!w || fc.getResults().isEmpty())
                    return;
                auto path = juceFileToFSPath(fc.getResult()).u8string();
                // the engine loads into the lead zone, so make the target lead first
                w->editor->doSelectionAction(one, true, true, true);
                w->sendToSerialization(
                    cmsg::AddSampleInZone({path, one.part, one.group, one.zone, 0}));
            });
    });
    p.addItem(revealLabel(), single, false, [w = w_t(that), one]() {
        if (w)
            w->sendToSerialization(cmsg::RequestRevealZoneSample({one, 0}));
    });

    p.addSeparator();
    addShortcutItem(ed, p, "Select All Zones", SELECT_ALL, true, [w = w_t(that)]() {
        if (w)
            w->editor->editScreen->selectAllInPart(true);
    });
    p.addItem("Deselect All Zones", !ed->allZoneSelections.empty(), false, [w = w_t(that)]() {
        if (w)
            w->editor->doSelectionAction(
                selection::SelectionManager::SelectActionContents::deselectSentinel());
    });

    p.addSeparator();
    // none means the selection, which the engine reads for itself
    auto sources = t.isSelection ? std::vector<za_t>{} : targets;
    auto newGroup = juce::PopupMenu();
    auto addNew = [&](const std::string &label, bool eachOwn, bool clone) {
        newGroup.addItem(label, any, false, [w = w_t(that), sources, eachOwn, clone]() {
            if (w)
                w->sendToSerialization(cmsg::MoveZonesToNewGroups({sources, eachOwn, clone}));
        });
    };
    addNew("Empty Group", false, false);
    addNew("Clone of Selected Group", false, true);
    newGroup.addSeparator();
    addNew("Each Zone to Its Own Empty Group", true, false);
    addNew("Each Zone to Its Own Clone of Selected Group", true, true);
    p.addSubMenu("Move to New Group", newGroup, any);

    auto existing = juce::PopupMenu();
    std::map<int32_t, int> zoneCounts;
    for (const auto &r : ed->editScreen->partSidebar->pgzStructure)
        if (r.address.part == part && r.address.zone >= 0)
            zoneCounts[r.address.group]++;
    for (const auto &g : findOtherGroups(ed))
    {
        auto gi = g.address.group;
        auto allThere = std::all_of(targets.begin(), targets.end(),
                                    [gi](const auto &z) { return z.group == gi; });
        auto n = zoneCounts[gi];
        auto label = "#" + std::to_string(gi + 1) + ": " + g.name + " (" + std::to_string(n) +
                     (n == 1 ? " zone)" : " zones)");
        existing.addItem(label, any && !allThere, false, [w = w_t(that), targets, a = g.address]() {
            if (w)
                w->sendToSerialization(cmsg::MoveZonesTo({targets, a, false}));
        });
    }
    p.addSubMenu("Move to Existing Group", existing, any && existing.getNumItems() > 0);

    p.addSeparator();
    auto batch = juce::PopupMenu();
    auto addBatch = [&](const std::string &label, engine::Engine::ZoneBatchOp op) {
        batch.addItem(label, [w = w_t(that), targets, op]() {
            if (w)
                w->sendToSerialization(cmsg::ApplyZoneBatchOp({targets, (int32_t)op}));
        });
    };
    addBatch("Move Root Keys to First Zone Key", engine::Engine::ROOT_TO_FIRST_KEY);
    addBatch("Move Root Keys to Zone Center", engine::Engine::ROOT_TO_CENTER_KEY);
    addBatch("Move Root Keys to Last Zone Key", engine::Engine::ROOT_TO_LAST_KEY);
    batch.addSeparator();
    addBatch("Auto-Apply Key Crossfades", engine::Engine::AUTO_KEY_CROSSFADES);
    addBatch("Auto-Apply Velocity Crossfades", engine::Engine::AUTO_VELOCITY_CROSSFADES);
    batch.addSeparator();
    addBatch("Remove Key Crossfades", engine::Engine::REMOVE_KEY_CROSSFADES);
    addBatch("Remove Velocity Crossfades", engine::Engine::REMOVE_VELOCITY_CROSSFADES);
    p.addSubMenu("Batch Functions", batch, any);
}

inline bool allGroupsLinked(SCXTEditor *ed, const std::vector<za_t> &groups)
{
    if (groups.empty())
        return false;
    for (const auto &g : groups)
    {
        bool linked{false};
        for (const auto &r : ed->editScreen->partSidebar->pgzStructure)
            if (r.address == za_t{g.part, g.group, -1})
                linked = r.features & engine::GroupZoneFeatures::LINKED_SELECTION;
        if (!linked)
            return false;
    }
    return true;
}

template <typename SendingComp>
void addLinkZoneSelectionItem(SendingComp *that, juce::PopupMenu &p,
                              const std::vector<za_t> &groups, const std::string &label)
{
    namespace cmsg = scxt::messaging::client;
    auto linked = allGroupsLinked(that->editor, groups);
    std::vector<za_t> gs;
    for (const auto &g : groups)
        gs.push_back({g.part, g.group, -1});
    p.addItem(label, !gs.empty(), linked, [w = juce::Component::SafePointer(that), gs, linked]() {
        if (w)
            w->sendToSerialization(cmsg::SetLinkZoneSelection({gs, !linked}));
    });
}

// the GROUPS menu, for group rows and the groups hamburger
template <typename SendingComp>
void populateGroupMenu(SendingComp *that, juce::PopupMenu &p, const std::optional<za_t> &clicked,
                       const std::string &clickedName, std::function<void()> onRename = {})
{
    namespace cmsg = scxt::messaging::client;
    using w_t = juce::Component::SafePointer<SendingComp>;

    auto *ed = that->editor;
    auto part = (int16_t)ed->selectedPart;
    const auto &lead = ed->currentLeadGroupSelection;
    auto leadSet = lead.has_value() && lead->group >= 0;
    auto t = menuTargets(ed->allGroupSelections, leadSet ? lead : std::nullopt, clicked);
    auto targets = t.items;
    auto any = !targets.empty();

    if (t.isSelection && targets.size() > 1)
        p.addSectionHeader(std::to_string(targets.size()) + " Selected Groups");
    else if (clicked.has_value())
        p.addSectionHeader(clickedName);
    else
        p.addSectionHeader("Groups");

    if (onRename)
    {
        p.addItem("Rename", std::move(onRename));
        p.addSeparator();
    }

    auto deleteTargets = [isSel = t.isSelection || !clicked.has_value(), targets](auto *w) {
        if (isSel)
            w->sendToSerialization(cmsg::DeleteAllSelectedGroups(true));
        else if (!targets.empty())
            w->sendToSerialization(cmsg::DeleteGroup(targets.front()));
    };

    addShortcutItem(ed, p, "Copy", COPY, any, [w = w_t(that), targets]() {
        if (w)
            w->sendToSerialization(cmsg::CopyGroups(targets));
    });
    addShortcutItem(ed, p, "Cut", CUT, any, [w = w_t(that), targets, deleteTargets]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::CopyGroups(targets));
        deleteTargets(w.getComponent());
    });
    addShortcutItem(ed, p, "Duplicate", DUPLICATE, any, [w = w_t(that), targets]() {
        if (w)
            w->sendToSerialization(cmsg::DuplicateGroups(targets));
    });

    // after the clicked or lead group, else the end of the part
    auto pasteInto = clicked.has_value() ? *clicked : (leadSet ? *lead : za_t{part, -1, -1});
    auto hasGroup = ed->clipboardType == engine::Clipboard::ContentType::GROUP;
    addShortcutItem(ed, p, "Paste with Zones", PASTE, hasGroup, [w = w_t(that), pasteInto]() {
        if (w)
            w->sendToSerialization(cmsg::PasteGroup(pasteInto));
    });
    p.addItem("Paste without Zones", hasGroup, false, [w = w_t(that), pasteInto]() {
        if (w)
            w->sendToSerialization(cmsg::PasteGroupWithoutZones(pasteInto));
    });
    if (clicked.has_value())
    {
        p.addItem("Paste Zone", ed->clipboardType == engine::Clipboard::ContentType::ZONE, false,
                  [w = w_t(that), za = *clicked]() {
                      if (w)
                          w->sendToSerialization(cmsg::PasteZone(za));
                  });
    }
    addShortcutItem(ed, p, "Delete", DELETE_SELECTED, any, [w = w_t(that), deleteTargets]() {
        if (w)
            deleteTargets(w.getComponent());
    });
    p.addItem("Delete Empty Groups", [w = w_t(that), part]() {
        if (w)
            w->sendToSerialization(cmsg::DeleteEmptyGroups(part));
    });

    p.addSeparator();
    p.addItem("Add New Group", [w = w_t(that), part]() {
        if (w)
            w->sendToSerialization(cmsg::CreateGroup(part));
    });
    if (clicked.has_value())
    {
        p.addItem("Create Empty Zone", [w = w_t(that), za = *clicked]() {
            if (w)
                w->sendToSerialization(cmsg::AddBlankZone({za.part, za.group, 48, 72, 0, 127}));
        });
    }

    p.addSeparator();
    addLinkZoneSelectionItem(that, p, targets, "Link Zone Selection");

    p.addSeparator();
    addShortcutItem(ed, p, "Select All Groups", SELECT_ALL, true, [w = w_t(that)]() {
        if (w)
            w->editor->editScreen->selectAllInPart(false);
    });
    p.addItem("Deselect All Groups", !ed->allGroupSelections.empty(), false, [w = w_t(that)]() {
        if (w)
            w->editor->doSelectionAction(
                selection::SelectionManager::SelectActionContents::deselectSentinel());
    });
}

template <typename SendingComp>
void populatePartRightMouseMenu(SendingComp *that, juce::PopupMenu &p, int part)
{
    namespace cmsg = scxt::messaging::client;

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
