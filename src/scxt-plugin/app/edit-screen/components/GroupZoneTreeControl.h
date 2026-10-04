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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_GROUPZONETREECONTROL_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_GROUPZONETREECONTROL_H

#include <juce_graphics/juce_graphics.h>
#include <juce_core/juce_core.h>

#include <vector>

#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/ListView.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/component-adapters/DiscreteToReference.h"
#include "engine/feature_enums.h"

#include "app/shared/ZoneRightMouseMenu.h"
#include "app/browser-ui/BrowserPaneInterfaces.h"
#include "app/shared/SampleDropHandler.h"

namespace scxt::ui::app::edit_screen
{

namespace jcmp = sst::jucegui::components;
namespace cmsg = scxt::messaging::client;

template <typename SidebarParent, bool fz> struct GroupZoneSidebarWidget : jcmp::ListView
{
    static constexpr bool forZone{fz};
    SidebarParent *sidebar{nullptr};
    engine::Engine::pgzStructure_t gzData;
    std::set<selection::SelectionManager::ZoneAddress> selectedZones;

    // Indices into gzData of the rows the ListView should actually show.
    std::vector<size_t> visibleRows;
    bool anyGroupSoloed{false};

    // Map a ListView row index to its gzData index.
    size_t gzIndexForRow(int row) const { return visibleRows[row]; }

    // True if the group at gzData[gzIdx] has at least one zone (next entry is a zone).
    bool groupHasZones(size_t gzIdx) const
    {
        return gzIdx + 1 < gzData.size() && gzData[gzIdx + 1].address.zone >= 0;
    }

    // Fold state lives on the wire: the FOLDED feature bit on group rows in
    // gzData. The server stamps it; the widget just reads it.
    bool isGroupCollapsed(size_t gzIdx) const
    {
        if (gzIdx >= gzData.size())
            return false;
        return (gzData[gzIdx].features & engine::GroupZoneFeatures::FOLDED) != 0;
    }
    void setGroupCollapsedLocal(size_t gzIdx, bool collapsed)
    {
        if (gzIdx >= gzData.size())
            return;
        if (collapsed)
            gzData[gzIdx].features |= engine::GroupZoneFeatures::FOLDED;
        else
            gzData[gzIdx].features &= ~engine::GroupZoneFeatures::FOLDED;
    }

    // flips the local bit so the fold is instant; the structure broadcast that follows confirms it
    bool setGroupFolded(const selection::SelectionManager::ZoneAddress &group, bool collapsed)
    {
        for (size_t i = 0; i < gzData.size(); ++i)
        {
            const auto &a = gzData[i].address;
            if (a.zone >= 0 || a.part != group.part || a.group != group.group)
                continue;
            if (!groupHasZones(i) || isGroupCollapsed(i) == collapsed)
                return false;

            setGroupCollapsedLocal(i, collapsed);
            sidebar->partGroupSidebar->collapsedGroupsChanged();
            sidebar->sendToSerialization(cmsg::SetGroupCollapsed({a.part, a.group, collapsed}));
            return true;
        }
        return false;
    }

    GroupZoneSidebarWidget(SidebarParent *sb) : sidebar(sb)
    {
        rebuild();
        setSelectionMode(jcmp::ListView::SelectionMode::MULTI_SELECTION);
        getRowCount = [this]() { return visibleRows.size() + 1; };
        getRowHeight = [this]() { return 15; };
        makeRowComponent = [this]() { return std::make_unique<rowTopComponent>(); };
        assignComponentToRow = [this](const std::unique_ptr<juce::Component> &c, uint32_t row) {
            auto rc = dynamic_cast<rowTopComponent *>(c.get());
            if (rc)
            {
                if (row == visibleRows.size())
                {
                    rc->enableAdd();
                    rc->addRow->gsb = sidebar;
                    rc->addRow->lbm = this;
                }
                else
                {
                    rc->enableGZ();
                    rc->gzRow->rowNumber = row;
                    rc->gzRow->lbm = this;
                    rc->gzRow->gsb = sidebar;
                    rc->gzRow->complete();
                }
                rc->resized();
                rc->repaint();
            }
        };
        // rows read selection from selectedZones as they paint
        setRowSelection = [this](auto &, bool) { repaint(); };
    }
    void rebuild()
    {
        auto &sbo = sidebar->editor->currentLeadZoneSelection;

        auto &pgz = sidebar->partGroupSidebar->pgzStructure;
        gzData.clear();
        anyGroupSoloed = false;
        for (const auto &el : pgz)
        {
            if (el.address.part == sidebar->editor->selectedPart && el.address.group >= 0)
            {
                gzData.push_back(el);
                anyGroupSoloed =
                    anyGroupSoloed || (el.features & engine::GroupZoneFeatures::SOLOED);
            }
        }
        rebuildVisible();
    }

    // Recompute visibleRows from gzData. Fold state lives on each group row's
    // features bitfield (FOLDED) — set server-side and stamped on the wire.
    void rebuildVisible()
    {
        visibleRows.clear();
        visibleRows.reserve(gzData.size());
        bool skipping{false};
        for (size_t i = 0; i < gzData.size(); ++i)
        {
            const auto &addr = gzData[i].address;
            if (addr.zone < 0)
            {
                visibleRows.push_back(i); // group row: always visible
                skipping = (gzData[i].features & engine::GroupZoneFeatures::FOLDED) != 0;
            }
            else if (!skipping)
            {
                visibleRows.push_back(i);
            }
        }
    }

    int groupCount() const
    {
        int res{0};
        for (const auto &r : gzData)
            if (r.address.zone < 0)
                res++;
        return res;
    }

    selection::SelectionManager::ZoneAddress getZoneAddress(int rowNumber)
    {
        if (rowNumber < 0 || rowNumber >= (int)visibleRows.size())
            return {};
        return gzData[gzIndexForRow(rowNumber)].address;
    }

    // a folded-away zone resolves to its group's row
    std::optional<int> rowForAddress(const selection::SelectionManager::ZoneAddress &a) const
    {
        std::optional<int> groupRow;
        for (int r = 0; r < (int)visibleRows.size(); ++r)
        {
            const auto &addr = gzData[visibleRows[r]].address;
            if (addr == a)
                return r;
            if (addr.zone < 0 && addr.part == a.part && addr.group == a.group)
                groupRow = r;
        }
        return groupRow;
    }

    std::optional<selection::SelectionManager::ZoneAddress> revealedLead;

    // only a changed lead scrolls, so a manual scroll away from it is left alone
    void revealLead()
    {
        const auto &lead = forZone ? sidebar->editor->currentLeadZoneSelection
                                   : sidebar->editor->currentLeadGroupSelection;
        if (lead == revealedLead)
            return;

        auto vh = viewPort->getViewHeight();
        auto row = lead.has_value() ? rowForAddress(*lead) : std::nullopt;

        // structure or layout not here yet, so retry on a later pass
        if (lead.has_value() && (!row.has_value() || vh <= 0))
            return;

        revealedLead = lead;
        if (!row.has_value())
            return;

        auto rh = (int)getRowHeight();
        auto top = *row * rh;
        auto vy = viewPort->getViewPositionY();
        if (top + rh > vy && top < vy + vh)
            return;

        viewPort->setViewPosition(viewPort->getViewPositionX(), top - (vh - rh) / 2);
    }

    // Returns the group ZoneAddress (zone==-1) for the row at position (x,y) in this widget's
    // local coordinate space, accounting for scrolling. If the position falls on a zone row the
    // parent group address is returned. Returns an empty optional if out of bounds.
    std::optional<selection::SelectionManager::ZoneAddress> groupAddressForDropPosition(int x,
                                                                                        int y)
    {
        if (!viewPort || !getRowHeight)
            return std::nullopt;
        int rh = (int)getRowHeight();
        if (rh <= 0)
            return std::nullopt;
        int contentY = y - viewPort->getY() + viewPort->getViewPositionY();
        if (contentY < 0)
            return std::nullopt;
        int rowIdx = contentY / rh;
        if (rowIdx < 0 || rowIdx >= (int)visibleRows.size())
            return std::nullopt;
        auto addr = gzData[gzIndexForRow(rowIdx)].address;
        // If this is a zone row, return the parent group address
        if (addr.zone >= 0)
            addr.zone = -1;
        return addr;
    }

    struct RenameEditor : juce::TextEditor
    {
        std::function<void(int)> onTab;
        bool keyPressed(const juce::KeyPress &key) override
        {
            if (key.getKeyCode() == juce::KeyPress::tabKey && onTab)
            {
                onTab(key.getModifiers().isShiftDown() ? -1 : 1);
                return true;
            }
            return juce::TextEditor::keyPressed(key);
        }
    };

    struct rowComponent : juce::Component, juce::DragAndDropTarget, juce::TextEditor::Listener
    {
        int rowNumber{-1};
        GroupZoneSidebarWidget<SidebarParent, forZone> *lbm{nullptr};
        SidebarParent *gsb{nullptr};

        std::unique_ptr<RenameEditor> renameEditor;
        using bdm_t =
            sst::jucegui::component_adapters::DiscreteToValueReference<jcmp::ToggleButton, bool>;
        std::unique_ptr<bdm_t> muteProvider, soloProvider;
        bool muteValue{false}, soloValue{false};
        bool glyphHovered{false};
        rowComponent()
        {
            renameEditor = std::make_unique<RenameEditor>();
            addChildComponent(*renameEditor);
            renameEditor->addListener(this);
            renameEditor->onTab = [this](int dir) {
                auto from = getZoneAddress();
                commitRename();
                if (!gsb->renameNeighbourOf(from, dir))
                    gsb->grabKeyboardFocus();
            };
        }

        // Hover-on-glyph: highlight the fold arrow when the mouse is over the gutter
        // of a foldable group row (empty groups show a status dot, no hover).
        bool computeGlyphHovered(int x) const
        {
            if (!lbm || rowNumber < 0 || rowNumber >= (int)lbm->visibleRows.size())
                return false;
            const auto &addr = lbm->gzData[lbm->gzIndexForRow(rowNumber)].address;
            if (addr.zone >= 0)
                return false;
            if (!lbm->groupHasZones(lbm->gzIndexForRow(rowNumber)))
                return false;
            return x < grouplabelPad;
        }

        void mouseMove(const juce::MouseEvent &e) override
        {
            bool h = computeGlyphHovered(e.x);
            if (h != glyphHovered)
            {
                glyphHovered = h;
                repaint();
            }
        }

        void mouseExit(const juce::MouseEvent &) override
        {
            if (glyphHovered)
            {
                glyphHovered = false;
                repaint();
            }
        }

        // Rows are recycled across refreshes, so build the mute and solo widgets once and
        // retarget them. Rebuilding them here allocates on every row assignment.
        void complete()
        {
            if (!isZone())
            {
                const auto &tgl = lbm->gzData;
                const auto &sg = tgl[lbm->gzIndexForRow(rowNumber)];

                if (!muteProvider)
                {
                    muteProvider = makeMuteOrSoloToggle(muteValue, "M", false);
                    soloProvider = makeMuteOrSoloToggle(soloValue, "S", true);
                    resized();
                }
                muteProvider->setValueFromModel(sg.features & engine::GroupZoneFeatures::MUTED);
                soloProvider->setValueFromModel(sg.features & engine::GroupZoneFeatures::SOLOED);
                muteProvider->widget->setVisible(true);
                soloProvider->widget->setVisible(true);
                // a solo in the part decides what sounds, as on the mixer
                muteProvider->widget->setEnabled(!lbm->anyGroupSoloed);
            }
            else if (muteProvider)
            {
                muteProvider->widget->setVisible(false);
                soloProvider->widget->setVisible(false);
            }
        }

        std::unique_ptr<bdm_t> makeMuteOrSoloToggle(bool &value, const std::string &label,
                                                    bool isSolo)
        {
            auto res = std::make_unique<bdm_t>(value);
            res->widget->setLabel(label);
            res->onValueChanged = [this, isSolo](bool v) { setMuteOrSoloTo(isSolo, v); };
            addAndMakeVisible(*res->widget);
            return res;
        }

        int zonePad = 16;
        int grouplabelPad = zonePad;

        enum DragOverState
        {
            NONE,
            BEFORE,
            AFTER,
            INTO
        } dragOverState{NONE};

        void paintDropIndicator(juce::Graphics &g, int left)
        {
            auto col = gsb->editor->themeColor(theme::ColorMap::accent_1b);
            switch (dragOverState)
            {
            case BEFORE:
                g.setColour(col);
                g.fillRect(left, 0, getWidth() - left, 2);
                break;
            case AFTER:
                g.setColour(col);
                g.fillRect(left, getHeight() - 2, getWidth() - left, 2);
                break;
            case INTO:
                g.setColour(col.withAlpha(0.1f));
                g.fillRect(getLocalBounds());
                g.setColour(col);
                g.drawRect(getLocalBounds(), 1);
                break;
            case NONE:
                break;
            }
        }

        // alt is exclusive, shift sweeps a range, command skips the selection
        void setMuteOrSoloTo(bool isSolo, bool v)
        {
            assert(!isZone());
            const auto &tgl = lbm->gzData;
            const auto &sg = tgl[lbm->gzIndexForRow(rowNumber)];

            auto mods = juce::ModifierKeys::getCurrentModifiers();
            auto gesture = cmsg::MS_SELECTED_GROUPS;
            if (mods.isAltDown())
                gesture = cmsg::MS_EXCLUSIVE;
            else if (mods.isShiftDown())
                gesture = cmsg::MS_RANGE;
            else if (mods.isCommandDown())
                gesture = cmsg::MS_THIS_GROUP;

            gsb->sendToSerialization(cmsg::MuteOrSoloGroup(
                {sg.address.part, sg.address.group, isSolo, v, (int32_t)gesture}));
        }

        void paint(juce::Graphics &g) override
        {
            if (!gsb)
                return;

            const auto &tgl = lbm->gzData;
            if (rowNumber < 0 || rowNumber >= (int)lbm->visibleRows.size())
                return;

            const auto &sg = tgl[lbm->gzIndexForRow(rowNumber)];

            bool isLeadZone = isZone() && gsb->isLeadZone(sg.address);
            bool isLeadGroup = isGroup() && gsb->isLeadGroup(sg.address);
            bool rowSelected = isSelected();

            auto editor = gsb->partGroupSidebar->editor;

            auto st = gsb->partGroupSidebar->style();
            auto zoneFont = editor->themeApplier.interLightFor(11);
            auto groupFont = editor->themeApplier.interRegularFor(11);

            if (isZone())
                g.setFont(isLeadZone ? editor->themeApplier.interBoldFor(11) : zoneFont);
            else
                g.setFont(groupFont);

            auto borderColor = editor->themeColor(theme::ColorMap::accent_1b, 0.4);
            auto textColor = editor->themeColor(theme::ColorMap::generic_content_medium);
            auto lowTextColor = editor->themeColor(theme::ColorMap::generic_content_low);
            auto fillColor = editor->themeColor(theme::ColorMap::bg_2);
            if (!forZone || isGroup())
                fillColor = editor->themeColor(theme::ColorMap::bg_3);

            if (forZone)
            {
                if (rowSelected && isZone())
                {
                    fillColor = editor->themeColor(theme::ColorMap::accent_1b, 0.2);
                    textColor = editor->themeColor(theme::ColorMap::generic_content_high);
                }
                if ((isLeadZone || (isPaintSnapshot && isDragMulti)) && isZone())
                {
                    fillColor = editor->themeColor(theme::ColorMap::accent_1b, 0.3);
                    textColor = editor->themeColor(theme::ColorMap::generic_content_highest);
                }
                if (isGroup())
                {
                    textColor = editor->themeColor(theme::ColorMap::generic_content_high);
                }
            }
            else
            {
                if (isLeadGroup && isGroup())
                {
                    fillColor = editor->themeColor(theme::ColorMap::accent_1b, 0.4);
                    textColor = editor->themeColor(theme::ColorMap::generic_content_highest);
                    lowTextColor = editor->themeColor(theme::ColorMap::accent_1a);
                }
                else if (rowSelected && isGroup())
                {
                    fillColor = editor->themeColor(theme::ColorMap::accent_1b, 0.2);
                    textColor = editor->themeColor(theme::ColorMap::generic_content_medium);
                    lowTextColor = editor->themeColor(theme::ColorMap::accent_1b);
                }
            }

            if (sg.address.zone < 0)
            {
                g.setColour(fillColor);
                g.fillRect(getLocalBounds());

                g.setColour(borderColor);
                g.drawHorizontalLine(getHeight() - 1, 0, getWidth());

                auto bx = getLocalBounds().withWidth(grouplabelPad);
                auto nb = getLocalBounds()
                              .withTrimmedLeft(grouplabelPad)
                              .withTrimmedRight(2 * (getHeight() - 2))
                              .withTrimmedBottom(1);
                auto glyphColor = lowTextColor;
                bool groupIsSelected =
                    editor->allGroupSelections.find(sg.address) != editor->allGroupSelections.end();
                if (isLeadGroup)
                    glyphColor = editor->themeColor(theme::ColorMap::generic_content_highest);
                else if (groupIsSelected)
                    glyphColor = editor->themeColor(theme::ColorMap::generic_content_high);

                bool hasZones = lbm->groupHasZones(lbm->gzIndexForRow(rowNumber));
                bool collapsed = lbm->isGroupCollapsed(lbm->gzIndexForRow(rowNumber));
                auto gb = bx.reduced(2);
                auto glyph = !hasZones ? jcmp::GlyphPainter::MINUS
                                       : (collapsed ? jcmp::GlyphPainter::JOG_RIGHT
                                                    : jcmp::GlyphPainter::JOG_DOWN);
                if (hasZones && glyphHovered)
                    glyphColor = editor->themeColor(theme::ColorMap::generic_content_high);
                jcmp::GlyphPainter::paintGlyph(g, gb, glyph, glyphColor);

                if (editor->sharedUiMemoryState.isGroupSounding(sg.address.part, sg.address.group))
                {
                    auto sb = nb.removeFromRight(nb.getHeight()).reduced(1);
                    jcmp::GlyphPainter::paintGlyph(g, sb, jcmp::GlyphPainter::SPEAKER, textColor);
                }

                // every keyswitch group shows the glyph, lit while it is the live articulation
                if (sg.features & engine::GroupZoneFeatures::KEYSWITCHED)
                {
                    auto off = (sg.features & engine::GroupZoneFeatures::MUTED_BY_KEYSWITCH) != 0;
                    auto kb = nb.removeFromRight(nb.getHeight()).reduced(1);
                    jcmp::GlyphPainter::paintGlyph(
                        g, kb, jcmp::GlyphPainter::KEYBOARD,
                        editor->themeColor(off ? theme::ColorMap::generic_content_low
                                               : theme::ColorMap::accent_1b));
                }
                // its zones select as one
                if (sg.features & engine::GroupZoneFeatures::LINKED_SELECTION)
                {
                    auto lk = nb.removeFromRight(nb.getHeight()).reduced(1);
                    jcmp::GlyphPainter::paintGlyph(g, lk, jcmp::GlyphPainter::LINK,
                                                   editor->themeColor(theme::ColorMap::accent_1b));
                }
                g.setColour(textColor);
                if (isPaintSnapshot && isDragMulti)
                    g.drawText(std::to_string(dragSources.size()) + " Selected Groups", nb,
                               juce::Justification::centredLeft);
                else
                    g.drawText(sg.name, nb, juce::Justification::centredLeft);

                paintDropIndicator(g, 0);
            }
            else
            {
                auto bx = getLocalBounds().withTrimmedLeft(zonePad);
                g.setColour(fillColor);
                g.fillRect(bx);
                g.setColour(borderColor);
                g.drawVerticalLine(zonePad, 0, getHeight());
                g.drawHorizontalLine(getHeight() - 1, zonePad, getWidth());

                g.setColour(textColor);
                if (sg.features & engine::GroupZoneFeatures::MISSING_SAMPLE)
                {
                    g.setColour(editor->themeColor(theme::ColorMap::warning_1a));
                }

                size_t voiceCount{0};
                auto p = editor->editScreen->voiceCountByZoneAddress.find(sg.address);
                if (p != editor->editScreen->voiceCountByZoneAddress.end())
                {
                    voiceCount = p->second;
                }

                if (isPaintSnapshot && isDragMulti)
                {
                    g.drawText(std::to_string(dragSources.size()) + " Selected Zones",
                               getLocalBounds().translated(zonePad + 2, 0),
                               juce::Justification::centredLeft);
                    return;
                }
                g.drawText(sg.name,
                           getLocalBounds().translated(zonePad + 2, 0).withTrimmedBottom(1),
                           juce::Justification::centredLeft);

                if (voiceCount > 0)
                {
                    auto b = getLocalBounds()
                                 .withWidth(zonePad)
                                 // .translated(getWidth() - zonePad, 0)
                                 .reduced(2);
                    jcmp::GlyphPainter::paintGlyph(g, b, jcmp::GlyphPainter::SPEAKER, textColor);
                }

                paintDropIndicator(g, zonePad);
            }
        }

        bool isDragging{false}, isPopup{false}, consumedFoldClick{false};
        selection::SelectionManager::ZoneAddress getZoneAddress()
        {
            return lbm->getZoneAddress(rowNumber);
        }
        bool isZone() { return getZoneAddress().zone >= 0; }
        bool isGroup() { return getZoneAddress().zone == -1; }
        bool isSelected() { return lbm && lbm->selectedZones.count(getZoneAddress()) > 0; }

        void mouseDown(const juce::MouseEvent &e) override
        {
            isPopup = false;
            consumedFoldClick = false;

            // Left-click in the group-row gutter toggles fold; right-click falls through.
            // Empty groups show a status dot instead of an arrow — no fold action there.
            if (!e.mods.isPopupMenu() && isGroup() && e.x < grouplabelPad &&
                lbm->groupHasZones(lbm->gzIndexForRow(rowNumber)))
            {
                auto gzIdx = lbm->gzIndexForRow(rowNumber);
                lbm->setGroupFolded(getZoneAddress(), !lbm->isGroupCollapsed(gzIdx));
                consumedFoldClick = true;
                return;
            }

            if (e.mods.isPopupMenu())
            {
                if (rowNumber < 0 || rowNumber >= (int)lbm->visibleRows.size())
                    return;
                juce::PopupMenu p;
                auto za = getZoneAddress();
                const auto &sg = lbm->gzData[lbm->gzIndexForRow(rowNumber)];
                auto rename = [w = juce::Component::SafePointer(this), za]() {
                    if (!w)
                        return;
                    if (w->isZone())
                        w->doZoneRename(za);
                    else
                        w->doGroupRename();
                };
                if (isZone())
                    shared::populateZoneMenu(gsb, p, za, sg.name, rename);
                else
                    shared::populateGroupMenu(gsb, p, za, sg.name, rename);

                p.addSeparator();
                shared::populatePartRightMouseMenu(gsb, p, za.part);

                isPopup = true;
                p.showMenuAsync(gsb->editor->defaultPopupMenuOptions());
            }
        }

        bool isPaintSnapshot{false};
        bool isDragMulti{false};
        // taken when the drag starts, as rows are recycled under it
        std::vector<selection::SelectionManager::ZoneAddress> dragSources;

        // a selected row drags the whole selection unless shift singles it out
        void beginRowDrag(const juce::MouseEvent &e)
        {
            auto *container = juce::DragAndDropContainer::findParentDragContainerFor(this);
            if (!container)
                return;

            auto za = getZoneAddress();
            const auto &ed = gsb->editor;
            dragSources.clear();
            if (isZone())
            {
                isDragMulti =
                    ed->isSelected(za) && ed->allZoneSelections.size() > 1 && !e.mods.isShiftDown();
                if (isDragMulti)
                    dragSources.assign(ed->allZoneSelections.begin(), ed->allZoneSelections.end());
            }
            else
            {
                isDragMulti = ed->allGroupSelections.count(za) > 0 &&
                              ed->allGroupSelections.size() > 1 && !e.mods.isShiftDown();
                if (isDragMulti)
                    dragSources.assign(ed->allGroupSelections.begin(),
                                       ed->allGroupSelections.end());
            }
            if (!isDragMulti)
                dragSources.push_back(za);
            std::erase_if(dragSources, [&za](const auto &a) { return a.part != za.part; });
            std::sort(dragSources.begin(), dragSources.end());

            isPaintSnapshot = true;
            container->startDragging(isZone() ? "ZoneRow" : "GroupRow", this);
            isPaintSnapshot = false;
            isDragging = true;
        }

        // big thanks to https://forum.juce.com/t/listbox-drag-to-reorder-solved/28477
        void mouseDrag(const juce::MouseEvent &e) override
        {
            if (consumedFoldClick || isPopup || isDragging || e.getDistanceFromDragStart() <= 2)
                return;
            if (isZone() || isGroup())
                beginRowDrag(e);
        }

        void mouseDoubleClick(const juce::MouseEvent &e) override
        {
            if (consumedFoldClick || e.mods.isPopupMenu())
                return;
            if (isGroup() && e.x < grouplabelPad)
                return;
            if (isZone())
                doZoneRename(getZoneAddress());
            else if (isGroup())
                doGroupRename();
        }

        void mouseUp(const juce::MouseEvent &event) override
        {
            if (consumedFoldClick)
            {
                consumedFoldClick = false;
                return;
            }
            if (isDragging || isPopup)
            {
                isDragging = false;
                isPopup = false;
                return;
            }

            // keep focus on the list rather than a child juce might pick, like a mute toggle
            gsb->grabKeyboardFocus();

            auto za = getZoneAddress();
            gsb->onRowClicked(za, isSelected(), event.mods);
        }

        bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override
        {
            // Always accept drags from other tree rows (zone/group reordering)
            if (dynamic_cast<rowComponent *>(dragSourceDetails.sourceComponent.get()) != nullptr)
                return true;
            // Also accept browser sample drops (single or batch) onto group or zone rows
            {
                auto wsi = browser_ui::asSampleInfo(dragSourceDetails.sourceComponent);
                if (wsi)
                {
                    if (wsi->encompassesMultipleSampleInfos())
                        return !isZone(); // batch only onto group rows
                    return shared::SampleDropSource::fromBrowserItem(wsi).isSingleSample();
                }
            }
            return false;
        }

        // the last row showing for this row's group, where a group lands after it
        bool isLastRowOfGroup()
        {
            auto next = rowNumber + 1;
            if (next >= (int)lbm->visibleRows.size())
                return true;
            return lbm->getZoneAddress(next).group != getZoneAddress().group;
        }

        DragOverState dropStateFor(const SourceDetails &sd)
        {
            auto rd = dynamic_cast<rowComponent *>(sd.sourceComponent.get());
            if (!rd)
                return INTO;
            auto upper = sd.localPosition.y < getHeight() / 2;
            if (rd->isZone())
            {
                if (isGroup())
                    return INTO;
                return upper ? BEFORE : AFTER;
            }
            // a group lands between groups; any zone row means after that zone's group
            if (isGroup())
                return (upper || !isLastRowOfGroup()) ? BEFORE : AFTER;
            return isLastRowOfGroup() ? AFTER : INTO;
        }

        void itemDragEnter(const SourceDetails &sd) override
        {
            dragOverState = dropStateFor(sd);
            repaint();
        }
        void itemDragMove(const SourceDetails &sd) override
        {
            auto ns = dropStateFor(sd);
            if (ns != dragOverState)
            {
                dragOverState = ns;
                repaint();
            }
        }
        void itemDragExit(const SourceDetails &dragSourceDetails) override
        {
            dragOverState = NONE;
            repaint();
        }

        void itemDropped(const SourceDetails &dragSourceDetails) override
        {
            auto where = dropStateFor(dragSourceDetails);
            dragOverState = NONE;
            repaint();
            auto sc = dragSourceDetails.sourceComponent;
            if (!sc) // weak component
                return;

            // Handle browser sample drop onto a group or zone row
            {
                auto wsi = browser_ui::asSampleInfo(dragSourceDetails.sourceComponent);
                if (wsi)
                {
                    auto za = getZoneAddress();
                    if (!isZone() && wsi->encompassesMultipleSampleInfos())
                    {
                        shared::executeBatchDropOnGroup(wsi, za.part, za.group, gsb);
                        return;
                    }
                    auto src = shared::SampleDropSource::fromBrowserItem(wsi);
                    if (src.isSingleSample())
                    {
                        src.dropAsZoneInGroup(za.part, za.group, gsb);
                        return;
                    }
                }
            }

            auto rd = dynamic_cast<rowComponent *>(sc.get());
            if (!rd || rd->dragSources.empty())
                return;

            // command or alt at the drop copies rather than moves
            auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
            auto copy = mods.isCommandDown() || mods.isAltDown();
            auto tgt = getZoneAddress();

            if (rd->isZone())
            {
                auto at = tgt;
                if (where == INTO)
                    at.zone = -1;
                else if (where == AFTER)
                    at.zone = tgt.zone + 1;
                gsb->sendToSerialization(cmsg::MoveZonesTo({rd->dragSources, at, copy}));
            }
            else
            {
                std::vector<int32_t> groups;
                for (const auto &a : rd->dragSources)
                    groups.push_back(a.group);
                auto before = where == BEFORE ? tgt.group : tgt.group + 1;
                if (before >= lbm->groupCount())
                    before = -1;
                gsb->sendToSerialization(
                    cmsg::MoveGroupsTo({(int16_t)tgt.part, groups, before, copy}));
            }
        }
        void resized() override
        {
            renameEditor->setBounds(getLocalBounds().withTrimmedLeft(zonePad));
            if (muteProvider)
            {
                auto bx = getLocalBounds().withTrimmedLeft(getWidth() - getHeight() + 2);
                muteProvider->widget->setBounds(bx.reduced(1));
                soloProvider->widget->setBounds(bx.translated(-bx.getWidth(), 0).reduced(1));
            }
        }

        void doGroupRename()
        {
            const auto &tgl = lbm->gzData;

            const auto &sg = tgl[lbm->gzIndexForRow(rowNumber)];
            assert(sg.address.zone < 0);
            auto st = gsb->partGroupSidebar->style();
            auto groupFont = gsb->editor->themeApplier.interRegularFor(11);
            renameEditor->setFont(groupFont);
            renameEditor->applyFontToAllText(groupFont);
            renameEditor->setText(sg.name);
            renameEditor->setSelectAllWhenFocused(true);
            renameEditor->setIndents(2, 1);
            renameEditor->setVisible(true);
            renameEditor->grabKeyboardFocus();
        }

        void doZoneRename(const selection::SelectionManager::ZoneAddress &za)
        {
            const auto &tgl = lbm->gzData;

            const auto &sg = tgl[lbm->gzIndexForRow(rowNumber)];
            auto st = gsb->partGroupSidebar->style();
            auto zoneFont = gsb->editor->themeApplier.interLightFor(11);
            renameEditor->setFont(zoneFont);
            renameEditor->applyFontToAllText(zoneFont);
            renameEditor->setText(sg.name);
            renameEditor->setSelectAllWhenFocused(true);
            renameEditor->setIndents(2, 1);
            renameEditor->setVisible(true);
            renameEditor->grabKeyboardFocus();
        }

        void commitRename()
        {
            renameEditor->setVisible(false);
            if (rowNumber < 0 || rowNumber >= (int)lbm->visibleRows.size())
                return;

            // tabbing past a row leaves its name alone rather than adding an undo step
            auto name = renameEditor->getText().toStdString();
            if (name == lbm->gzData[lbm->gzIndexForRow(rowNumber)].name)
                return;

            auto za = getZoneAddress();
            if (isZone())
                gsb->sendToSerialization(cmsg::RenameZone({za, name}));
            else
                gsb->sendToSerialization(cmsg::RenameGroup({za, name}));
        }

        void textEditorReturnKeyPressed(juce::TextEditor &) override
        {
            commitRename();
            gsb->grabKeyboardFocus();
        }
        void textEditorEscapeKeyPressed(juce::TextEditor &) override
        {
            renameEditor->setVisible(false);
            gsb->grabKeyboardFocus();
        }
        void textEditorFocusLost(juce::TextEditor &) override { renameEditor->setVisible(false); }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(rowComponent);
    };
    struct rowAddComponent : juce::Component, juce::DragAndDropTarget
    {
        SidebarParent *gsb{nullptr};
        GroupZoneSidebarWidget<SidebarParent, forZone> *lbm{nullptr};

        std::unique_ptr<jcmp::GlyphButton> gBut;
        rowAddComponent()
        {
            gBut = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::GlyphType::PLUS);
            addAndMakeVisible(*gBut);
            gBut->glyphButtonPad = 3;
            gBut->setOnCallback([this]() { gsb->addGroup(); });
        }

        void resized() override
        {
            auto b = getLocalBounds().withSizeKeepingCentre(getHeight(), getHeight()).reduced(1);
            gBut->setBounds(b);
        }

        bool isDroppingOn{false};
        void paint(juce::Graphics &g) override
        {
            if (isDroppingOn)
                g.fillAll(
                    gsb->editor->themeColor(theme::ColorMap::generic_content_low).withAlpha(0.5f));
        }

        bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override
        {
            // tree rows only; zones go to a new group, groups go to the end
            return dynamic_cast<rowComponent *>(dragSourceDetails.sourceComponent.get()) != nullptr;
        }

        void itemDragEnter(const SourceDetails &dragSourceDetails) override
        {
            isDroppingOn = true;
            repaint();
        }
        void itemDragMove(const SourceDetails &dragSourceDetails) override {}
        void itemDragExit(const SourceDetails &dragSourceDetails) override
        {
            isDroppingOn = false;
            repaint();
        }

        void itemDropped(const SourceDetails &dragSourceDetails) override
        {
            isDroppingOn = false;
            repaint();
            auto rd = dynamic_cast<rowComponent *>(dragSourceDetails.sourceComponent.get());
            if (!rd || rd->dragSources.empty())
                return;

            auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
            auto copy = mods.isCommandDown() || mods.isAltDown();
            auto part = (int16_t)rd->dragSources.front().part;
            if (rd->isZone())
            {
                gsb->sendToSerialization(
                    cmsg::MoveZonesTo({rd->dragSources, {part, -1, -1}, copy}));
            }
            else
            {
                std::vector<int32_t> groups;
                for (const auto &a : rd->dragSources)
                    groups.push_back(a.group);
                gsb->sendToSerialization(cmsg::MoveGroupsTo({part, groups, -1, copy}));
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(rowAddComponent);
    };

    struct rowTopComponent : juce::Component
    {
        std::unique_ptr<rowComponent> gzRow;
        std::unique_ptr<rowAddComponent> addRow;
        rowTopComponent() {}

        void enableAdd()
        {
            if (!addRow)
            {
                removeAllChildren();
                gzRow.reset();
                addRow = std::make_unique<rowAddComponent>();
                addAndMakeVisible(*addRow);
                resized();
            }
        }
        void enableGZ()
        {
            if (!gzRow)
            {
                removeAllChildren();
                addRow.reset();
                gzRow = std::make_unique<rowComponent>();
                addAndMakeVisible(*gzRow);
                resized();
            }
        }

        void resized()
        {
            assert(!(addRow && gzRow));
            if (addRow)
                addRow->setBounds(getLocalBounds());
            if (gzRow)
                gzRow->setBounds(getLocalBounds());
        }
    };

    rowComponent *rowComponentForAddress(const selection::SelectionManager::ZoneAddress &a)
    {
        auto *content = viewPort->getViewedComponent();
        if (!content)
            return nullptr;
        for (auto *c : content->getChildren())
        {
            auto *rt = dynamic_cast<rowTopComponent *>(c);
            if (rt && rt->gzRow && rt->gzRow->lbm && rt->gzRow->getZoneAddress() == a)
                return rt->gzRow.get();
        }
        return nullptr;
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        sidebar->grabKeyboardFocus();
        sidebar->editor->doSelectionAction(
            selection::SelectionManager::SelectActionContents::deselectSentinel());
    }
};

} // namespace scxt::ui::app::edit_screen

#endif // SHORTCIRCUITXT_PARTGROUPTREE_H
