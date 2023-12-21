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

#include "PartGroupSidebar.h"
#include "components/SCXTEditor.h"
#include "selection/selection_manager.h"
#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/Label.h"
#include "messaging/messaging.h"
#include "components/MultiScreen.h"
#include "connectors/PayloadDataAttachment.h"
#include "detail/GroupZoneTreeControl.h"

namespace scxt::ui::multi
{
namespace jcmp = sst::jucegui::components;
namespace cmsg = scxt::messaging::client;

struct PartSidebar : juce::Component, HasEditor
{
    PartGroupSidebar *partGroupSidebar{nullptr};
    PartSidebar(PartGroupSidebar *p) : partGroupSidebar(p), HasEditor(p->editor) {}
    void paint(juce::Graphics &g) override
    {
        auto ft = editor->style()->getFont(jcmp::Label::Styles::styleClass,
                                           jcmp::Label::Styles::labelfont);
        g.setFont(ft.withHeight(20));
        g.setColour(juce::Colours::white);
        g.drawText("Parts", getLocalBounds().withTrimmedBottom(20), juce::Justification::centred);

        g.setFont(ft.withHeight(14));
        g.setColour(juce::Colours::white);
        g.drawText("Coming Soon", getLocalBounds().withTrimmedTop(20),
                   juce::Justification::centred);
    }
};

template <typename T>
struct GroupZoneSidebarBase : juce::Component, HasEditor, juce::DragAndDropContainer
{
    PartGroupSidebar *partGroupSidebar{nullptr};
    int part{0};
    std::unique_ptr<juce::ListBox> listBox;
    std::unique_ptr<detail::GroupZoneListBoxModel<T>> listBoxModel;

    T *asT() { return static_cast<T *>(this); }

    GroupZoneSidebarBase(PartGroupSidebar *p) : partGroupSidebar(p), HasEditor(p->editor) {}

    void postInit()
    {
        listBoxModel = std::make_unique<detail::GroupZoneListBoxModel<T>>(asT());
        listBoxModel->rebuild();
        listBox = std::make_unique<juce::ListBox>();
        listBox->setModel(listBoxModel.get());
        listBox->setRowHeight(18);
        listBox->setColour(juce::ListBox::backgroundColourId, juce::Colour(0, 0, 0).withAlpha(0.f));

        addAndMakeVisible(*listBox);
    }
    void addGroup()
    {
        auto &mc = partGroupSidebar->editor->msgCont;
        partGroupSidebar->sendToSerialization(cmsg::CreateGroup(part));
    }
    void updateSelectionFrom(const selection::SelectionManager::selectedZones_t &sel)
    {
        juce::SparseSet<int> rows;

        for (const auto &a : sel)
        {
            int selR = -1;
            for (const auto &[i, r] : sst::cpputils::enumerate(listBoxModel->thisGroup))
            {
                if (r.first == a)
                {
                    rows.addRange({(int)i, (int)(i + 1)});
                }
            }
        }
        listBoxModel->selectionFromServer = true;
        listBox->setSelectedRows(rows);
        listBoxModel->selectionFromServer = false;
    }

    bool isLeadZone(const selection::SelectionManager::ZoneAddress &za)
    {
        return editor->currentLeadZoneSelection.has_value() &&
               za == *(editor->currentLeadZoneSelection);
    }
};

struct GroupControls : juce::Component, HasEditor
{
    GroupControls(HasEditor *p) : HasEditor(p->editor) {}
    void paint(juce::Graphics &g) override
    {
        auto ft = editor->style()->getFont(jcmp::Label::Styles::styleClass,
                                           jcmp::Label::Styles::labelfont);
        g.setFont(ft.withHeight(20));
        g.setColour(juce::Colours::white);
        g.drawText("Group Controls", getLocalBounds().withTrimmedBottom(20),
                   juce::Justification::centred);

        g.setFont(ft.withHeight(14));
        g.setColour(juce::Colours::white);
        g.drawText("Coming Soon", getLocalBounds().withTrimmedTop(20),
                   juce::Justification::centred);

        g.drawRect(getLocalBounds().reduced(1));
    }
};

struct GroupSidebar : GroupZoneSidebarBase<GroupSidebar>
{

    GroupSidebar(PartGroupSidebar *p) : GroupZoneSidebarBase<GroupSidebar>(p)
    {
        groupControls = std::make_unique<GroupControls>(this);
        addAndMakeVisible(*groupControls);
    }
    ~GroupSidebar() = default;

    void updateSelection() { updateSelectionFrom(partGroupSidebar->editor->allGroupSelections); }

    void resized() override
    {
        auto ht = 200;
        auto lb = getLocalBounds().withTrimmedBottom(ht);
        auto cb = getLocalBounds().withY(lb.getBottom()).withHeight(ht);
        listBox->setBounds(lb);
        groupControls->setBounds(cb);
    }

    void processRowsChanged()
    {
        const auto &r = listBox->getSelectedRows();

        SCLOG("GroupSidebar::processRowsChanged Called - Keyboard Event");
        SCLOG("Selected Row Size is " << r.size());
    }

    void onRowClicked(const selection::SelectionManager::ZoneAddress &rowZone, bool isSelected,
                      const juce::ModifierKeys &mods)
    {
        // For now just force it to select the group
        auto se = selection::SelectionManager::SelectActionContents(rowZone);

        if (rowZone.zone >= 0)
        {
            se.selecting = true;
            se.distinct = false;
            se.selectingAsLead = true;
            se.forZone = true;
            editor->doMultiSelectionAction({se});
        }
        else
        {
            se.selecting = !isSelected;
            se.distinct = !mods.isCommandDown();
            se.selectingAsLead = se.selecting;
            se.forZone = false;
            editor->doMultiSelectionAction({se});
        }
    }
    std::unique_ptr<GroupControls> groupControls;
};

struct ZoneSidebar : GroupZoneSidebarBase<ZoneSidebar>
{
    ZoneSidebar(PartGroupSidebar *p) : GroupZoneSidebarBase<ZoneSidebar>(p) {}
    ~ZoneSidebar() = default;

    void updateSelection()
    {
        updateSelectionFrom(partGroupSidebar->editor->allZoneSelections);
        if (partGroupSidebar->editor->currentLeadZoneSelection.has_value())
            lastZoneClicked = *(partGroupSidebar->editor->currentLeadZoneSelection);
    }
    void resized() override { listBox->setBounds(getLocalBounds()); }

    void processRowsChanged()
    {
        SCLOG("ZoneSidebar::processRowsChanged Called. Surprising!!!");
        // We want a modified verion of the old PGZ::selectedRowsChanged
        const auto &r = listBox->getSelectedRows();
        if (r.size() == 1)
        {
            auto rg = r.getRange(0);
            auto rs = rg.getStart();
            auto re = rg.getEnd();
            if (re != rs + 1)
            {
                SCLOG("TODO: Multi-ui driven sidevar selection");
                return;
            }
            auto se =
                selection::SelectionManager::SelectActionContents(listBoxModel->getZoneAddress(rs));
            se.selecting = true;
            se.distinct = true;
            se.selectingAsLead = true;
            editor->doMultiSelectionAction({se});
        }
        else
        {
            SCLOG_UNIMPL("TODO: Multi-ui driven sidebar selection");
            return;
        }
    }

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
        if (mods.isShiftDown())
        {
            auto se = selection::SelectionManager::SelectActionContents(rowZone);
            se.selecting = true;
            se.distinct = false;
            se.selectingAsLead = false;
            se.forZone = true;
            se.isContiguous = true;
            se.contiguousFrom = lastZoneClicked;
            editor->doSelectionAction(se);
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
            auto se = selection::SelectionManager::SelectActionContents(rowZone);

            se.selecting = true;
            se.distinct = false;
            se.selectingAsLead = false;
            se.forZone = true;
            editor->doSelectionAction(se);
        }
        else
        {
            SCLOG("Selected row zone inconsistent " << rowZone);
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
    onTabSelected = [w = juce::Component::SafePointer(this)](int t) {
        if (!w)
            return;

        if (!w->partSidebar || !w->groupSidebar || !w->zoneSidebar)
            return;

        bool p = (t == 0), g = (t == 1), z = (t == 2);
        w->partSidebar->setVisible(p);
        w->groupSidebar->setVisible(g);
        w->zoneSidebar->setVisible(z);

        w->editor->multiScreen->setSelectionMode(g ? MultiScreen::SelectionMode::GROUP
                                                   : MultiScreen::SelectionMode::ZONE);
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
    SCLOG("PartGroupZone in Sidebar: Showing Part0 Entries");
    for (const auto &a : pgzStructure)
    {
        if (a.first.part == 0)
        {
            std::string pad{"|--|--|"};
            if (a.first.group == -1)
                pad = "|";
            else if (a.first.zone == -1)
                pad = "|--|";
            SCLOG("  " << pad << " " << a.second << " -> " << a.first);
        }
    }
#endif
    groupSidebar->listBoxModel->rebuild();
    groupSidebar->listBox->updateContent();
    zoneSidebar->listBoxModel->rebuild();
    zoneSidebar->listBox->updateContent();
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
} // namespace scxt::ui::multi