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
                                           jcmp::Label::Styles::controlLabelFont);
        g.setFont(ft.withHeight(20));
        g.setColour(juce::Colours::white);
        g.drawText("Parts", getLocalBounds().withTrimmedBottom(20), juce::Justification::centred);

        g.setFont(ft.withHeight(14));
        g.setColour(juce::Colours::white);
        g.drawText("Coming Soon", getLocalBounds().withTrimmedTop(20),
                   juce::Justification::centred);
    }
};

struct GroupSidebar : juce::Component, HasEditor, juce::DragAndDropContainer
{
    struct GroupListBoxModel : juce::ListBoxModel
    {
        GroupSidebar *sidebar{nullptr};
        engine::Engine::pgzStructure_t thisGroup;
        GroupListBoxModel(GroupSidebar *sb) : sidebar(sb) { rebuild(); }
        void rebuild()
        {
            auto &sbo = sidebar->editor->currentLeadSelection;
            sidebar->part = 0;
            if (sbo.has_value() && sbo->part >= 0)
                sidebar->part = sbo->part;
            auto &pgz = sidebar->partGroupSidebar->pgzStructure;
            thisGroup.clear();
            for (const auto &el : pgz)
                if (el.first.part == sidebar->part && el.first.group >= 0)
                    thisGroup.push_back(el);
        }
        int getNumRows() override { return thisGroup.size() + 1; /* for the plus */ }

        // Selection can come from server update or from juce keypress etc
        // for keypress etc... we need to push back to the server
        bool selectionFromServer{false};
        void selectedRowsChanged(int lastRowSelected) override
        {
            if (!selectionFromServer)
            {
                // We want a modified verion of the old PGZ::selectedRowsChanged
                const auto &r = sidebar->listBox->getSelectedRows();
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
                    auto se = selection::SelectionManager::SelectActionContents(getZoneAddress(rs));
                    se.selecting = true;
                    se.distinct = true;
                    se.selectingAsLead = true;
                    sidebar->editor->doMultiSelectionAction({se});
                }
                else
                {
                    SCLOG("TODO: Multi-ui driven sidebar selection");
                    return;
                }
            }
        }

        selection::SelectionManager::ZoneAddress getZoneAddress(int rowNumber)
        {
            auto &tgl = thisGroup;
            if (rowNumber < 0 || rowNumber >= tgl.size())
                return {};
            auto &sad = tgl[rowNumber].first;
            return sad;
        }

        struct rowComponent : juce::Component, juce::DragAndDropTarget
        {
            int rowNumber{-1};
            bool isSelected{false};
            GroupListBoxModel *lbm{nullptr};
            GroupSidebar *gsb{nullptr};

            void paint(juce::Graphics &g) override
            {
                if (!gsb)
                    return;
                auto &tgl = lbm->thisGroup;
                if (rowNumber < 0 || rowNumber >= tgl.size())
                    return;

                auto &sg = tgl[rowNumber];

                auto st = gsb->partGroupSidebar->style();
                g.setFont(st->getFont(jcmp::Label::Styles::styleClass,
                                      jcmp::Label::Styles::controlLabelFont));

                // TODO: Style all of these
                auto borderColor = juce::Colour(0xFF, 0x90, 0x00).darker(0.4);
                auto textColor = juce::Colour(190, 190, 190);
                auto lowTextColor = textColor.darker(0.4);
                auto fillColor = juce::Colour(0, 0, 0).withAlpha(0.f);

                if (isSelected)
                    fillColor = juce::Colour(0x40, 0x20, 0x00);
                int zonePad = 20;
                if (sg.first.zone < 0)
                {
                    g.setColour(fillColor);
                    g.fillRect(getLocalBounds());

                    g.setColour(borderColor);
                    g.drawLine(0, getHeight(), getWidth(), getHeight());

                    auto bx = getLocalBounds().withWidth(zonePad);
                    auto nb = getLocalBounds().withTrimmedLeft(zonePad);
                    g.setColour(lowTextColor);
                    g.drawText(std::to_string(sg.first.group + 1), bx,
                               juce::Justification::centredLeft);
                    g.setColour(textColor);
                    g.drawText(sg.second, nb, juce::Justification::centredLeft);
                }
                else
                {
                    auto bx = getLocalBounds().withTrimmedLeft(zonePad);
                    g.setColour(fillColor);
                    g.fillRect(bx);
                    g.setColour(borderColor);
                    g.drawLine(zonePad, 0, zonePad, getHeight());
                    g.drawLine(zonePad, getHeight(), getWidth(), getHeight());

                    g.setColour(textColor);
                    g.drawText(sg.second, getLocalBounds().translated(zonePad + 2, 0),
                               juce::Justification::centredLeft);
                }
            }

            bool isDragging{false}, isPopup{false};
            selection::SelectionManager::ZoneAddress getZoneAddress()
            {
                return lbm->getZoneAddress(rowNumber);
            }
            bool isZone() { return getZoneAddress().zone >= 0; }
            bool isGroup() { return getZoneAddress().zone == -1; }

            void mouseDown(const juce::MouseEvent &e) override
            {
                isPopup = false;
                if (e.mods.isPopupMenu())
                {
                    juce::PopupMenu p;
                    if (isZone())
                    {
                        p.addSectionHeader("Zone");
                        p.addSeparator();
                        p.addItem("Rename", []() {});
                        p.addItem("Delete", [w = juce::Component::SafePointer(this)]() {
                            if (!w)
                                return;
                            auto za = w->getZoneAddress();
                            w->gsb->sendToSerialization(cmsg::DeleteZone(za));
                        });
                    }
                    else if (isGroup())
                    {
                        p.addSectionHeader("Group");
                        p.addSeparator();
                        p.addItem("Rename", []() {});
                        p.addItem("Delete", [w = juce::Component::SafePointer(this)]() {
                            if (!w)
                                return;
                            auto za = w->getZoneAddress();
                            w->gsb->sendToSerialization(cmsg::DeleteGroup(za));
                        });
                    }
                    isPopup = true;
                    p.showMenuAsync(gsb->editor->defaultPopupMenuOptions());
                }
            }

            // big thanks to https://forum.juce.com/t/listbox-drag-to-reorder-solved/28477
            void mouseDrag(const juce::MouseEvent &e) override
            {
                if (!isZone())
                    return;

                if (!isDragging && e.getDistanceFromDragStart() > 1.5f)
                {
                    if (auto *container =
                            juce::DragAndDropContainer::findParentDragContainerFor(this))
                    {
                        container->startDragging("ZoneRow", this);
                        isDragging = true;
                    }
                }
            }

            void mouseUp(const juce::MouseEvent &event) override
            {
                if (isDragging || isPopup)
                {
                    isDragging = false;
                    isPopup = false;
                    return;
                }

                auto se = selection::SelectionManager::SelectActionContents(getZoneAddress());
                se.selecting = !isSelected;
                se.distinct = !event.mods.isCommandDown();
                se.selectingAsLead = se.selecting;
                gsb->editor->doMultiSelectionAction({se});
            }

            bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override
            {
                return !isZone();
            }

            void itemDropped(const SourceDetails &dragSourceDetails) override
            {
                auto sc = dragSourceDetails.sourceComponent;
                if (!sc) // weak component
                    return;

                auto rd = dynamic_cast<rowComponent *>(sc.get());
                if (rd)
                {
                    auto tgt = getZoneAddress();
                    auto src = rd->getZoneAddress();

                    gsb->sendToSerialization(cmsg::MoveZoneFromTo({src, tgt}));
                }
            }
        };
        struct rowAddComponent : juce::Component
        {
            GroupSidebar *gsb{nullptr};
            std::unique_ptr<jcmp::GlyphButton> gBut;
            rowAddComponent()
            {
                gBut = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::GlyphType::BIG_PLUS);
                addAndMakeVisible(*gBut);
                gBut->glyphButtonPad = 3;
                gBut->setOnCallback([this]() { gsb->addGroup(); });
            }

            void resized()
            {
                auto b =
                    getLocalBounds().withSizeKeepingCentre(getHeight(), getHeight()).reduced(1);
                gBut->setBounds(b);
            }
        };

        juce::Component *refreshComponentForRow(int rowNumber, bool isRowSelected,
                                                juce::Component *existingComponentToUpdate) override
        {
            if (rowNumber < thisGroup.size())
            {
                auto rc = dynamic_cast<rowComponent *>(existingComponentToUpdate);

                if (!rc)
                {
                    // Should never happen but
                    if (existingComponentToUpdate)
                    {
                        delete existingComponentToUpdate;
                        existingComponentToUpdate = nullptr;
                    }

                    rc = new rowComponent();
                    rc->lbm = this;
                    rc->gsb = sidebar;
                }

                rc->isSelected = isRowSelected;
                rc->rowNumber = rowNumber;
                return rc;
            }
            else
            {
                auto rc = dynamic_cast<rowAddComponent *>(existingComponentToUpdate);

                if (!rc)
                {
                    // Should never happen but
                    if (existingComponentToUpdate)
                    {
                        delete existingComponentToUpdate;
                        existingComponentToUpdate = nullptr;
                    }

                    rc = new rowAddComponent();
                    rc->gsb = sidebar;
                }

                return rc;
            }
        }
        void paintListBoxItem(int rowNumber, juce::Graphics &g, int width, int height,
                              bool rowIsSelected) override
        {
        }
    };
    PartGroupSidebar *partGroupSidebar{nullptr};
    int part{0};
    std::unique_ptr<juce::ListBox> listBox;
    std::unique_ptr<GroupListBoxModel> listBoxModel;
    GroupSidebar(PartGroupSidebar *p) : partGroupSidebar(p), HasEditor(p->editor)
    {
        listBoxModel = std::make_unique<GroupListBoxModel>(this);
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
    void resized() override { listBox->setBounds(getLocalBounds().withTrimmedBottom(200)); }

    void updateSelection()
    {
        juce::SparseSet<int> rows;

        for (const auto &a : partGroupSidebar->editor->allSelections)
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
};

struct PGZListBox : public juce::ListBox
{
};

struct PGZListBoxModel : juce::ListBoxModel
{
    PartGroupSidebar *partGroupSidebar{nullptr};
    PGZListBoxModel(PartGroupSidebar *pgs) : partGroupSidebar(pgs) {}

    int getNumRows() override { return partGroupSidebar->pgzStructure.size(); }

    void paintListBoxItem(int rowNumber, juce::Graphics &g, int width, int height,
                          bool rowIsSelected) override
    {
        const auto &s = partGroupSidebar->pgzStructure;

        assert(rowNumber >= 0 && rowNumber < s.size());
        if (rowNumber < 0 || rowNumber >= s.size())
            return;

        const auto &[addr, name] = s[rowNumber];
        const auto &[pt, gr, zn] = addr;
        auto col = juce::Colours::black;
        auto off = 20;
        if (gr == -1)
        {
            off = 2;
            col = juce::Colours::darkred;
        }
        else if (zn == -1)
        {
            off = 11;
            col = juce::Colours::darkgreen;
        }

        if (rowIsSelected)
        {
            col = col.brighter();
        }

        g.setColour(col);
        g.fillRect(0, 0, width, height);

        g.setColour(col.contrasting());
        g.setFont(juce::Font("Comic Sans MS", 14, juce::Font::plain));
        g.drawText(name, off, 0, width - off, height, juce::Justification::centredLeft);
    }

    void selectedRowsChanged(int rowNumber) override
    {
        const auto &m = partGroupSidebar->pgzList;
        const auto &s = partGroupSidebar->pgzStructure;
        std::unordered_set<selection::SelectionManager::ZoneAddress,
                           selection::SelectionManager::ZoneAddress::Hash>
            newSelections;
        selection::SelectionManager::ZoneAddress lead;
        bool setLead{false};
        std::unordered_set<selection::SelectionManager::ZoneAddress,
                           selection::SelectionManager::ZoneAddress::Hash>
            removeSelections;
        if (m)
        {
            const auto &r = m->getSelectedRows();
            // Rather than iterating over the ranges, since we need to make
            // the both-way list we iterate over the entire structure
            for (const auto &[idx, dat] : sst::cpputils::enumerate(s))
            {
                bool selectedUI = r.contains(idx);
                bool selectedModel = partGroupSidebar->editor->isSelected(dat.first);

                if (selectedUI && !selectedModel)
                {
                    if (idx == rowNumber)
                    {
                        lead = dat.first;
                        setLead = true;
                    }
                    else
                        newSelections.insert(dat.first);
                }
                if (!selectedUI && selectedModel)
                {
                    removeSelections.insert(dat.first);
                }
            }
        }

        auto dist = m->getSelectedRows().size() == 1;
        std::vector<selection::SelectionManager::SelectActionContents> res;
        for (auto &rm : newSelections)
        {
            res.emplace_back(rm, true, dist, false);
        }

        for (auto &rm : removeSelections)
        {
            res.emplace_back(rm, false, false, false);
        }

        if (setLead)
        {
            res.emplace_back(lead, true, false, true);
        }

        partGroupSidebar->editor->doMultiSelectionAction(res);
    }
};

PartGroupSidebar::PartGroupSidebar(SCXTEditor *e)
    : HasEditor(e), sst::jucegui::components::NamedPanel("Parts n Groups")
{
    isTabbed = true;
    hasHamburger = true;
    tabNames = {"PARTS", "GROUPS"};
    selectedTab = 1;
    onTabSelected = [w = juce::Component::SafePointer(this)](int t) {
        if (!w)
            return;

        if (!w->partSidebar || !w->groupSidebar)
            return;

        bool p = (t == 0), g = (t == 1);
        w->partSidebar->setVisible(p);
        w->groupSidebar->setVisible(g);
    };
    resetTabState();

    selectGroups();

    groupSidebar = std::make_unique<GroupSidebar>(this);
    addAndMakeVisible(*groupSidebar);
    partSidebar = std::make_unique<PartSidebar>(this);
    addChildComponent(*partSidebar);

    // TODO kill all this
    pgzList = std::make_unique<PGZListBox>();
    pgzListModel = std::make_unique<PGZListBoxModel>(this);
    pgzList->setModel(pgzListModel.get());
    pgzList->setMultipleSelectionEnabled(true);
    // addAndMakeVisible(*pgzList);
    addChildComponent(*pgzList);
}

PartGroupSidebar::~PartGroupSidebar()
{
    // Just be sure it destroys in the right order
    if (pgzList)
        pgzList->setModel(nullptr);
}

void PartGroupSidebar::resized()
{
    if (pgzList)
        pgzList->setBounds(getContentArea());
    if (partSidebar)
        partSidebar->setBounds(getContentArea());
    if (groupSidebar)
        groupSidebar->setBounds(getContentArea());
}

void PartGroupSidebar::setPartGroupZoneStructure(const engine::Engine::pgzStructure_t &p)
{
    pgzStructure = p;
    SCLOG("PartGroupZone in Sidebar: Showing Part0 Entries");
    for (const auto &a : pgzStructure)
    {
        if (a.first.part == 0)
            SCLOG("  | " << a.second << " -> " << a.first);
    }
    groupSidebar->listBoxModel->rebuild();
    groupSidebar->listBox->updateContent();
    editorSelectionChanged();
    repaint();
}

void PartGroupSidebar::editorSelectionChanged()
{
    if (groupSidebar)
    {
        groupSidebar->updateSelection();
    }
    repaint();
}
} // namespace scxt::ui::multi