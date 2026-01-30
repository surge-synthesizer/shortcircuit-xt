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

#include "PartGroupSidebar.h"
#include "app/SCXTEditor.h"
#include "selection/selection_manager.h"
#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/Viewport.h"
#include "sst/jucegui/components/NamedPanelDivider.h"
#include "sst/jucegui/util/VisibilityParentWatcher.h"
#include "messaging/messaging.h"
#include "infrastructure/user_defaults.h"
#include "app/edit-screen/EditScreen.h"
#include "connectors/PayloadDataAttachment.h"
#include "GroupZoneTreeControl.h"
#include "GroupSettingsCard.h"
#include "GroupTriggersCard.h"
#include "app/shared/PartSidebarCard.h"

namespace scxt::ui::app::edit_screen
{
namespace jcmp = sst::jucegui::components;
namespace cmsg = scxt::messaging::client;

struct PartSidebar : juce::Component, HasEditor
{
    PartGroupSidebar *partGroupSidebar{nullptr};
    std::unique_ptr<jcmp::Viewport> viewport;
    std::unique_ptr<juce::Component> viewportContents;
    std::array<std::unique_ptr<shared::PartSidebarCard>, scxt::numParts> parts;
    std::unique_ptr<jcmp::TextPushButton> addPartButton;
    bool tallMode{false};

    std::unique_ptr<sst::jucegui::util::VisibilityParentWatcher> psbw;

    PartSidebar(PartGroupSidebar *p) : partGroupSidebar(p), HasEditor(p->editor)
    {
        psbw = std::make_unique<sst::jucegui::util::VisibilityParentWatcher>(this);
        viewport = std::make_unique<jcmp::Viewport>("Parts");
        tallMode = (bool)editor->defaultsProvider.getUserDefaultValue(
            infrastructure::DefaultKeys::partSidebarPartExpanded, false);
        viewportContents = std::make_unique<juce::Component>();
        for (int i = 0; i < scxt::numParts; ++i)
        {
            parts[i] = std::make_unique<shared::PartSidebarCard>(i, editor);
            parts[i]->setTallMode(tallMode);
            viewportContents->addChildComponent(*parts[i]);
        }
        addPartButton = std::make_unique<jcmp::TextPushButton>();
        addPartButton->setLabel("Add Part");
        addPartButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
            w->sendToSerialization(cmsg::ActivateNextPart(true));
        });
        viewportContents->addChildComponent(*addPartButton);
        viewport->setViewedComponent(viewportContents.get(), false);
        addAndMakeVisible(*viewport);
    }
    void resetParts()
    {
        for (int i = 0; i < scxt::numParts; ++i)
        {
            parts[i]->setTallMode(tallMode);
            parts[i]->resetFromEditorCache();
        }
        resized();
    }

    void visibilityChanged() override
    {
        if (isVisible())
        {
            tallMode = (bool)editor->defaultsProvider.getUserDefaultValue(
                infrastructure::DefaultKeys::partSidebarPartExpanded, false);

            resetParts();
        }
    }
    void resized() override
    {
        auto w = shared::PartSidebarCard::width + viewport->getScrollBarThickness() + 2;
        viewport->setBounds(getLocalBounds().withWidth(w).translated(3, 0));
        auto nDispParts{0};
        for (int i = 0; i < scxt::numParts; ++i)
        {
            if (parts[i]->isVisible())
            {
                nDispParts++;
            }
        }
        int extraSpace{0};
        if (nDispParts < scxt::numParts)
            extraSpace = 30;
        auto partHeight = shared::PartSidebarCard::height;
        if (tallMode)
            partHeight = shared::PartSidebarCard::tallHeight;
        viewportContents->setBounds(0, 0, shared::PartSidebarCard::width,
                                    partHeight * nDispParts + extraSpace);
        auto ct{0};
        for (int i = 0; i < scxt::numParts; ++i)
        {
            if (parts[i]->isVisible())
            {
                parts[i]->setBounds(0, ct * partHeight, shared::PartSidebarCard::width, partHeight);
                ct++;
            }
        }

        if (nDispParts < scxt::numParts)
        {
            addPartButton->setBounds(10, ct * partHeight + 4, shared::PartSidebarCard::width - 20,
                                     22);
        }
    }

    void restackForActive()
    {
        int nDispParts{0};
        for (int i = 0; i < scxt::numParts; ++i)
        {
            if (editor->partConfigurations[i].active)
            {
                nDispParts++;
            }
            parts[i]->setVisible(editor->partConfigurations[i].active);
        }
        addPartButton->setVisible(nDispParts < scxt::numParts);
        resized();
    }
};

template <typename T, bool forZone>
struct GroupZoneSidebarBase : juce::Component, HasEditor, juce::DragAndDropContainer
{
    PartGroupSidebar *partGroupSidebar{nullptr};
    std::unique_ptr<GroupZoneSidebarWidget<T, forZone>> gzTreeControl;
    std::unique_ptr<jcmp::MenuButton> partSelector;

    T *asT() { return static_cast<T *>(this); }

    GroupZoneSidebarBase(PartGroupSidebar *p) : partGroupSidebar(p), HasEditor(p->editor) {}
    ~GroupZoneSidebarBase() {}

    void postInit()
    {
        gzTreeControl = std::make_unique<GroupZoneSidebarWidget<T, forZone>>(asT());
        addAndMakeVisible(*gzTreeControl);

        partSelector = std::make_unique<jcmp::MenuButton>();
        partSelector->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showPartSelectorMenu();
        });
        partSelector->setLabel("Part 1");
        addAndMakeVisible(*partSelector);
    }

    void showPartSelectorMenu()
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Part");
        p.addSeparator();
        for (int i = 0; i < scxt::numParts; ++i)
        {
            if (editor->partConfigurations[i].active)
            {
                p.addItem("Part " + std::to_string(i + 1), true, i == editor->selectedPart,
                          [w = juce::Component::SafePointer(this), index = i]() {
                              w->sendToSerialization(cmsg::SelectPart(index));
                          });
            }
        }
        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }

    void showSelectedPart(int part) { partSelector->setLabel("Part " + std::to_string(part + 1)); }

    juce::Rectangle<int> baseResize()
    {
        auto r = getLocalBounds();
        auto pg = r.withHeight(22);
        auto res = r.withTrimmedTop(24);
        partSelector->setBounds(pg);
        return res;
    }

    void addGroup()
    {
        auto &mc = partGroupSidebar->editor->msgCont;
        partGroupSidebar->sendToSerialization(cmsg::CreateGroup(editor->selectedPart));
    }
    void updateSelectionFrom(const selection::SelectionManager::selectedZones_t &sel)
    {
        gzTreeControl->selectedZones =
            std::set<selection::SelectionManager::ZoneAddress>(sel.begin(), sel.end());
        gzTreeControl->reassignAllComponents();
    }

    bool isLeadZone(const selection::SelectionManager::ZoneAddress &za)
    {
        return editor->currentLeadZoneSelection.has_value() &&
               za == *(editor->currentLeadZoneSelection);
    }

    bool isLeadGroup(const selection::SelectionManager::ZoneAddress &za)
    {
        return editor->currentLeadGroupSelection.has_value() &&
               za == *(editor->currentLeadGroupSelection);
    }
};

struct GroupSidebar : GroupZoneSidebarBase<GroupSidebar, false>
{
    GroupSidebar(PartGroupSidebar *p) : GroupZoneSidebarBase<GroupSidebar, false>(p)
    {
        groupSettings = std::make_unique<GroupSettingsCard>(this->editor);
        addAndMakeVisible(*groupSettings);

        groupTriggers = std::make_unique<GroupTriggersCard>(this->editor);
        addAndMakeVisible(*groupTriggers);

        settingsDivider = std::make_unique<jcmp::NamedPanelDivider>();
        addAndMakeVisible(*settingsDivider);
        triggersDivider = std::make_unique<jcmp::NamedPanelDivider>();
        addAndMakeVisible(*triggersDivider);
    }
    ~GroupSidebar() = default;

    void updateSelection()
    {
        bool anySel = !partGroupSidebar->editor->allGroupSelections.empty();
        updateSelectionFrom(partGroupSidebar->editor->allGroupSelections);
        groupSettings->setVisible(anySel);
        groupTriggers->setVisible(anySel);
    }

    void resized() override
    {
        auto b = baseResize();
        auto trigHeight = 116;
        auto settingsHeight = 146;
        auto dividerHeight = 8;

        auto lb = b.withTrimmedBottom(trigHeight + settingsHeight + 2 * dividerHeight);
        gzTreeControl->setBounds(lb);
        auto tb = b.withY(lb.getBottom()).withHeight(dividerHeight);
        triggersDivider->setBounds(tb);
        tb = tb.translated(0, dividerHeight).withHeight(trigHeight);
        groupTriggers->setBounds(tb.reduced(4, 2));
        tb = tb.translated(0, trigHeight).withHeight(dividerHeight);
        settingsDivider->setBounds(tb);
        tb = tb.translated(0, dividerHeight).withHeight(settingsHeight);
        groupSettings->setBounds(tb.reduced(4, 2));
    }

    void onRowClicked(const selection::SelectionManager::ZoneAddress &rowZone, bool isSelected,
                      const juce::ModifierKeys &mods)
    {
        // For now just force it to select the group
        auto se = selection::SelectionManager::SelectActionContents(rowZone);

        if (rowZone.zone >= 0)
        {
            /*
            se.selecting = !isSelected;
            se.distinct = !mods.isCommandDown();
            se.selectingAsLead = true;
            se.forZone = true;
            editor->doSelectionAction({se});
            */
            SCLOG_ONCE_IF(selection, "Supressing zone selection in group sidebar");
        }
        else
        {
            se.distinct = !(mods.isCommandDown() || mods.isShiftDown()); // for now
            se.selecting = !se.distinct || !isSelected;
            se.selectingAsLead = se.selecting;
            se.forZone = false;
            editor->doSelectionAction({se});
        }
    }
    std::unique_ptr<GroupSettingsCard> groupSettings;
    std::unique_ptr<GroupTriggersCard> groupTriggers;
    std::unique_ptr<jcmp::NamedPanelDivider> settingsDivider, triggersDivider;
};

struct ZoneSidebar : GroupZoneSidebarBase<ZoneSidebar, true>
{
    ZoneSidebar(PartGroupSidebar *p) : GroupZoneSidebarBase<ZoneSidebar, true>(p) {}
    ~ZoneSidebar() = default;

    void updateSelection()
    {
        updateSelectionFrom(partGroupSidebar->editor->allZoneSelections);
        if (partGroupSidebar->editor->currentLeadZoneSelection.has_value())
            lastZoneClicked = *(partGroupSidebar->editor->currentLeadZoneSelection);
    }
    void resized() override { gzTreeControl->setBounds(baseResize()); }

    selection::SelectionManager::ZoneAddress lastZoneClicked{0, 0, 0};
    void onRowClicked(const selection::SelectionManager::ZoneAddress &rowZone, bool isSelected,
                      const juce::ModifierKeys &mods)
    {
        /*
          Zone Mode Sidebar
            - click zone is select distinct as lead everywhere
            - click group is select entire groups zones
            - shift click is contiguous select
            - cmd/ctrl click is non-contiguous toggle select zone
            - alt-click is move lead or add and make lead
         */
        if (mods.isShiftDown() && lastZoneClicked != rowZone && rowZone.zone >= 0)
        {
            std::vector<selection::SelectionManager::SelectActionContents> actions;
            SCLOG_IF(selection, "Contiguous from " << lastZoneClicked << " to " << rowZone);

            bool doPush{false};
            for (auto &r : partGroupSidebar->pgzStructure)
            {
                bool firstDoPush{false};
                if (r.address == lastZoneClicked || r.address == rowZone)
                {
                    if (!doPush)
                    {
                        doPush = true;
                        firstDoPush = true;
                    }
                }
                if (doPush && r.address.zone >= 0)
                {
                    SCLOG_IF(selection, "Including zone in selection " << r.address)
                    auto se = selection::SelectionManager::SelectActionContents(r.address);
                    se.selecting = true;
                    se.distinct = false;
                    se.selectingAsLead =
                        (r.address == editor->currentLeadZoneSelection.value_or(
                                          selection::SelectionManager::ZoneAddress()));
                    se.forZone = true;
                    actions.push_back(se);
                }
                if (r.address == lastZoneClicked || r.address == rowZone)
                {
                    if (!firstDoPush)
                    {
                        doPush = false;
                        break;
                    }
                }
            }
            editor->doSelectionAction(actions);
        }
        else if (rowZone.zone >= 0)
        {
            auto se = selection::SelectionManager::SelectActionContents(rowZone);

            if (mods.isAltDown())
            {
                se.selecting = true;
                se.distinct = false;
                se.selectingAsLead = true;
                se.forZone = true;
                editor->doSelectionAction(se);
            }
            else
            {
                se.selecting = mods.isCommandDown() ? !isSelected : true;
                se.distinct = !mods.isCommandDown();
                se.selectingAsLead = se.selecting;
                se.forZone = true;
                editor->doSelectionAction(se);
            }
        }
        else if (rowZone.group >= 0)
        {
            // Group Selection on the server side selects all in the group
            auto se = selection::SelectionManager::SelectActionContents(rowZone);

            se.selecting = true;
            se.distinct = !mods.isAltDown();
            se.selectingAsLead = false;
            se.forZone = true;
            editor->doSelectionAction(se);
        }
        else
        {
            SCLOG_IF(warnings, "Selected row zone inconsistent " << rowZone);
        }
        lastZoneClicked = rowZone;
    }
};

PartGroupSidebar::PartGroupSidebar(SCXTEditor *e)
    : HasEditor(e), sst::jucegui::components::NamedPanel("Parts n Groups")
{
    isTabbed = true;
    hasHamburger = true;
    tabNames = {"PARTS", "GROUPS", "ZONES"};
    selectedTab = 2;
    onHamburger = [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showHamburgerMenu();
    };
    onTabSelected = [w = juce::Component::SafePointer(this)](int t) {
        if (!w)
            return;

        if (!w->partSidebar || !w->groupSidebar || !w->zoneSidebar)
            return;

        w->setSelectedTab(t);
    };
    resetTabState();

    selectGroups();

    zoneSidebar = std::make_unique<ZoneSidebar>(this);
    zoneSidebar->postInit();
    addAndMakeVisible(*zoneSidebar);
    groupSidebar = std::make_unique<GroupSidebar>(this);
    groupSidebar->postInit();
    addChildComponent(*groupSidebar);
    partSidebar = std::make_unique<PartSidebar>(this);
    addChildComponent(*partSidebar);
}

PartGroupSidebar::~PartGroupSidebar() {}

void PartGroupSidebar::resized()
{
    if (partSidebar)
        partSidebar->setBounds(getContentArea());
    if (groupSidebar)
        groupSidebar->setBounds(getContentArea());
    if (zoneSidebar)
        zoneSidebar->setBounds(getContentArea());
}

void PartGroupSidebar::setPartGroupZoneStructure(const engine::Engine::pgzStructure_t &p)
{
    pgzStructure = p;
#if LOG_PART_GROUP_SIDEBAR
    SCLOG_IF(debug, "PartGroupZone in Sidebar: Showing Part0 Entries");
    for (const auto &a : pgzStructure)
    {
        if (a.first.part == 0)
        {
            std::string pad{"|--|--|"};
            if (a.first.group == -1)
                pad = "|";
            else if (a.first.zone == -1)
                pad = "|--|";
            SCLOG_IF(debug, "  " << pad << " " << a.second << " -> " << a.first);
        }
    }
#endif

    groupSidebar->gzTreeControl->rebuild();
    groupSidebar->gzTreeControl->refresh();
    zoneSidebar->gzTreeControl->rebuild();
    zoneSidebar->gzTreeControl->refresh();

    editorSelectionChanged();
    repaint();
}

void PartGroupSidebar::selectedPartChanged()
{
    groupSidebar->showSelectedPart(editor->selectedPart);
    groupSidebar->gzTreeControl->rebuild();
    groupSidebar->gzTreeControl->refresh();

    zoneSidebar->showSelectedPart(editor->selectedPart);
    zoneSidebar->gzTreeControl->rebuild();
    zoneSidebar->gzTreeControl->refresh();

    editorSelectionChanged();
    repaint();
}

void PartGroupSidebar::editorSelectionChanged()
{
    if (groupSidebar)
    {
        groupSidebar->updateSelection();
    }
    if (zoneSidebar)
    {
        zoneSidebar->updateSelection();
    }
    repaint();
}

void PartGroupSidebar::setSelectedTab(int t)
{
    selectedTab = t;
    bool p = (t == 0), g = (t == 1), z = (t == 2);
    partSidebar->setVisible(p);
    groupSidebar->setVisible(g);
    zoneSidebar->setVisible(z);

    editor->editScreen->setSelectionMode(
        p ? EditScreen::SelectionMode::PART
          : (g ? EditScreen::SelectionMode::GROUP : EditScreen::SelectionMode::ZONE));

    if (g)
        editor->themeApplier.applyGroupMultiScreenTheme(this);
    else
        editor->themeApplier.applyZoneMultiScreenTheme(this);

    editor->setTabSelection(editor->editScreen->tabKey("multi.pgz"),
                            (t == 0 ? "part" : (t == 1 ? "group" : "zone")));
    repaint();
}

void PartGroupSidebar::partConfigurationChanged(int i)
{
    partSidebar->parts[i]->resetFromEditorCache();
    partSidebar->restackForActive();
}

void PartGroupSidebar::groupTriggerConditionChanged(const scxt::engine::GroupTriggerConditions &c)
{
    groupSidebar->groupTriggers->setGroupTriggerConditions(c);
}

void PartGroupSidebar::showHamburgerMenu()
{
    if (selectedTab == 0)
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Part Options");
        p.addSeparator();
        p.addItem("Expanded Display", true, partSidebar->tallMode,
                  [w = juce::Component::SafePointer(this)]() {
                      if (w)
                      {
                          w->editor->defaultsProvider.updateUserDefaultValue(
                              infrastructure::DefaultKeys::partSidebarPartExpanded, true);
                          w->partSidebar->tallMode = true;
                          w->partSidebar->resetParts();
                      }
                  });
        p.addItem("Compressed Display", true, !partSidebar->tallMode,
                  [w = juce::Component::SafePointer(this)]() {
                      if (w)
                      {
                          w->editor->defaultsProvider.updateUserDefaultValue(
                              infrastructure::DefaultKeys::partSidebarPartExpanded, false);
                          w->partSidebar->tallMode = false;
                          w->partSidebar->resetParts();
                      }
                  });
        p.addSeparator();
        p.addItem("Set All to Omni", [w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            auto &psb = w->partSidebar;
            for (int i = 0; i < scxt::numParts; ++i)
            {
                psb->parts[i]->setMidiChannel(engine::Part::PartConfiguration::omniChannel);
            }
        });
        p.addItem("Set All to MPE", [w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            auto &psb = w->partSidebar;
            for (int i = 0; i < scxt::numParts; ++i)
            {
                psb->parts[i]->setMidiChannel(engine::Part::PartConfiguration::mpeChannel);
            }
        });
        p.addItem("Set All to Ch/Oct", [w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            auto &psb = w->partSidebar;
            for (int i = 0; i < scxt::numParts; ++i)
            {
                psb->parts[i]->setMidiChannel(engine::Part::PartConfiguration::chPerOctaveChannel);
            }
        });
        p.addItem("Set to Incremental MIDI Channels", [w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            auto &psb = w->partSidebar;
            for (int i = 0; i < scxt::numParts; ++i)
            {
                psb->parts[i]->setMidiChannel(i);
            }
        });
        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }
    else
    {
        // No hamburger on this tab!
    }
}

} // namespace scxt::ui::app::edit_screen