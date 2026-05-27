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

    GroupZoneSidebarWidget(SidebarParent *sb) : sidebar(sb)
    {
        rebuild();
        setSelectionMode(jcmp::ListView::SelectionMode::MULTI_SELECTION);
        getRowCount = [this]() { return visibleRows.size() + 1; };
        getRowHeight = [this]() { return 18; };
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
                    rc->gzRow->isSelected =
                        selectedZones.find(gzData[gzIndexForRow(row)].address) !=
                        selectedZones.end();
                    rc->gzRow->complete();
                }
                rc->resized();
                rc->repaint();
            }
        };
        setRowSelection = [this](auto &c, bool isSel) {
            auto rc = dynamic_cast<rowTopComponent *>(c.get());
            if (rc && rc->gzRow)
            {
                rc->gzRow->isSelected = isSel;
                repaint();
            }
        };
    }
    void rebuild()
    {
        auto &sbo = sidebar->editor->currentLeadZoneSelection;

        auto &pgz = sidebar->partGroupSidebar->pgzStructure;
        gzData.clear();
        for (const auto &el : pgz)
            if (el.address.part == sidebar->editor->selectedPart && el.address.group >= 0)
                gzData.push_back(el);
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

    selection::SelectionManager::ZoneAddress getZoneAddress(int rowNumber)
    {
        if (rowNumber < 0 || rowNumber >= (int)visibleRows.size())
            return {};
        return gzData[gzIndexForRow(rowNumber)].address;
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

    struct rowComponent : juce::Component, juce::DragAndDropTarget, juce::TextEditor::Listener
    {
        int rowNumber{-1};
        bool isSelected{false};
        GroupZoneSidebarWidget<SidebarParent, forZone> *lbm{nullptr};
        SidebarParent *gsb{nullptr};

        std::unique_ptr<juce::TextEditor> renameEditor;
        using bdm_t =
            sst::jucegui::component_adapters::DiscreteToValueReference<jcmp::ToggleButton, bool>;
        std::unique_ptr<bdm_t> muteProvider;
        bool muteValue{false};
        bool glyphHovered{false};
        rowComponent()
        {
            renameEditor = std::make_unique<juce::TextEditor>();
            addChildComponent(*renameEditor);
            renameEditor->addListener(this);
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

        void complete()
        {
            if (!isZone())
            {
                const auto &tgl = lbm->gzData;
                const auto &sg = tgl[lbm->gzIndexForRow(rowNumber)];

                muteValue = sg.features & engine::GroupZoneFeatures::MUTED;
                muteProvider = std::make_unique<bdm_t>(muteValue);
                muteProvider->widget->setLabel("M");
                muteProvider->setup();
                muteProvider->onValueChanged = [this](bool v) {
                    auto shift = juce::ModifierKeys::getCurrentModifiers().isShiftDown();
                    setMuteTo(v, !shift);
                };
                addAndMakeVisible(*muteProvider->widget);
                resized();
            }
            else if (muteProvider && muteProvider->widget)
            {
                removeChildComponent(muteProvider->widget.get());
                muteProvider.reset();
            }
        }

        int zonePad = 16;
        int grouplabelPad = zonePad;

        enum DragOverState
        {
            NONE,
            DRAG_OVER
        } dragOverState{NONE};

        void setMuteTo(bool v, bool allSel)
        {
            assert(!isZone());
            const auto &tgl = lbm->gzData;
            const auto &sg = tgl[lbm->gzIndexForRow(rowNumber)];
            gsb->sendToSerialization(
                cmsg::MuteOrSoloGroup({sg.address.part, sg.address.group, v, false, allSel}));
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
                if (isSelected && isZone())
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
                else if (isSelected && isGroup())
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
                g.drawLine(0, getHeight(), getWidth(), getHeight());

                auto bx = getLocalBounds().withWidth(grouplabelPad);
                auto nb = getLocalBounds().withTrimmedLeft(grouplabelPad);
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
                g.setColour(textColor);
                g.drawText(sg.name, nb, juce::Justification::centredLeft);

                if (dragOverState == DRAG_OVER)
                {
                    if (isZone())
                    {
                        g.setColour(editor->themeColor(theme::ColorMap::accent_1b));
                        g.drawHorizontalLine(1, 0, getWidth());
                    }
                    else
                    {
                        g.setColour(editor->themeColor(theme::ColorMap::accent_1b).withAlpha(0.1f));
                        g.fillRect(getLocalBounds());
                        g.setColour(editor->themeColor(theme::ColorMap::accent_1b));
                        g.drawRect(getLocalBounds(), 1);
                    }
                }
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
                    g.drawText(std::to_string(lbm->selectedZones.size()) + " Selected Zones",
                               getLocalBounds().translated(zonePad + 2, 0),
                               juce::Justification::centredLeft);
                    return;
                }
                g.drawText(sg.name, getLocalBounds().translated(zonePad + 2, 0),
                           juce::Justification::centredLeft);

                if (voiceCount > 0)
                {
                    auto b = getLocalBounds()
                                 .withWidth(zonePad)
                                 // .translated(getWidth() - zonePad, 0)
                                 .reduced(2);
                    jcmp::GlyphPainter::paintGlyph(g, b, jcmp::GlyphPainter::SPEAKER, textColor);
                }

                if (dragOverState == DRAG_OVER)
                {
                    g.setColour(editor->themeColor(theme::ColorMap::accent_1b));
                    g.drawHorizontalLine(1, zonePad, getWidth());
                }
            }
        }

        bool isDragging{false}, isPopup{false}, consumedFoldClick{false};
        selection::SelectionManager::ZoneAddress getZoneAddress()
        {
            return lbm->getZoneAddress(rowNumber);
        }
        bool isZone() { return getZoneAddress().zone >= 0; }
        bool isGroup() { return getZoneAddress().zone == -1; }

        void mouseDown(const juce::MouseEvent &e) override
        {
            isPopup = false;
            consumedFoldClick = false;

            // Left-click in the group-row gutter toggles fold; right-click falls through.
            // Empty groups show a status dot instead of an arrow — no fold action there.
            if (!e.mods.isPopupMenu() && isGroup() && e.x < grouplabelPad &&
                lbm->groupHasZones(lbm->gzIndexForRow(rowNumber)))
            {
                auto za = getZoneAddress();
                auto gzIdx = lbm->gzIndexForRow(rowNumber);
                bool nowCollapsed = !lbm->isGroupCollapsed(gzIdx);
                // Optimistic: flip the local FOLDED bit and refresh both sidebars
                // so the click feels instant. The c2s round-trip will trigger a
                // structure broadcast that reaffirms the state.
                lbm->setGroupCollapsedLocal(gzIdx, nowCollapsed);
                gsb->partGroupSidebar->collapsedGroupsChanged();
                gsb->sendToSerialization(
                    cmsg::SetGroupCollapsed({za.part, za.group, nowCollapsed}));
                consumedFoldClick = true;
                return;
            }

            if (e.mods.isPopupMenu())
            {
                juce::PopupMenu p;
                if (isZone())
                {
                    auto za = getZoneAddress();
                    const auto &tgl = lbm->gzData;
                    const auto &sg = tgl[lbm->gzIndexForRow(rowNumber)];

                    shared::populateZoneRightMouseMenuForZone(gsb, this, p, za, sg.name);
                    p.addSeparator();
                    shared::populateZoneRightMouseMenuForSelectedZones(gsb, p, za.part);
                }
                else if (isGroup())
                {
                    const auto &tgl = lbm->gzData;
                    if (rowNumber < 0 || rowNumber >= (int)lbm->visibleRows.size())
                        return;

                    const auto &sg = tgl[lbm->gzIndexForRow(rowNumber)];

                    p.addSectionHeader(sg.name);
                    p.addSeparator();
                    p.addItem("Rename", [w = juce::Component::SafePointer(this)]() {
                        if (!w)
                            return;
                        w->doGroupRename();
                    });
                    p.addItem("Copy", [w = juce::Component::SafePointer(this)]() {
                        if (!w)
                            return;
                        auto za = w->getZoneAddress();
                        w->gsb->sendToSerialization(cmsg::CopyGroup(za));
                    });
                    p.addItem("Paste",
                              gsb->editor->clipboardType == engine::Clipboard::ContentType::GROUP,
                              false, [w = juce::Component::SafePointer(this)]() {
                                  if (!w)
                                      return;
                                  auto za = w->getZoneAddress();
                                  w->gsb->sendToSerialization(cmsg::PasteGroup(za));
                              });
                    p.addItem("Duplicate", [w = juce::Component::SafePointer(this)]() {
                        if (!w)
                            return;
                        auto za = w->getZoneAddress();
                        w->gsb->sendToSerialization(cmsg::DuplicateGroup(za));
                    });
                    p.addItem("Paste Zone",
                              gsb->editor->clipboardType == engine::Clipboard::ContentType::ZONE,
                              false, [w = juce::Component::SafePointer(this)]() {
                                  if (!w)
                                      return;
                                  auto za = w->getZoneAddress();
                                  w->gsb->sendToSerialization(cmsg::PasteZone(za));
                              });
                    p.addItem("Create Empty Zone", [w = juce::Component::SafePointer(this)]() {
                        if (!w)
                            return;
                        auto za = w->getZoneAddress();
                        w->gsb->sendToSerialization(
                            cmsg::AddBlankZone({za.part, za.group, 48, 72, 0, 127}));
                    });
                    p.addItem("Delete", [w = juce::Component::SafePointer(this)]() {
                        if (!w)
                            return;
                        auto za = w->getZoneAddress();
                        w->gsb->sendToSerialization(cmsg::DeleteGroup(za));
                    });
                    p.addItem("Delete Empty Groups", [w = juce::Component::SafePointer(this)]() {
                        if (!w)
                            return;
                        auto za = w->getZoneAddress();
                        w->gsb->sendToSerialization(cmsg::DeleteEmptyGroups(za.part));
                    });

                    p.addSeparator();
                    p.addItem("Expand All Groups", [w = juce::Component::SafePointer(this)]() {
                        if (!w)
                            return;
                        auto za = w->getZoneAddress();
                        w->gsb->sendToSerialization(cmsg::SetAllGroupsCollapsed({za.part, false}));
                    });
                    p.addItem("Collapse All Groups", [w = juce::Component::SafePointer(this)]() {
                        if (!w)
                            return;
                        auto za = w->getZoneAddress();
                        w->gsb->sendToSerialization(cmsg::SetAllGroupsCollapsed({za.part, true}));
                    });

                    auto za = getZoneAddress();

                    p.addSeparator();
                    shared::populateZoneRightMouseMenuForSelectedZones(gsb, p, za.part);
                }

                isPopup = true;
                p.showMenuAsync(gsb->editor->defaultPopupMenuOptions());
            }
        }

        bool isPaintSnapshot{false};
        bool isDragMulti{false};

        // big thanks to https://forum.juce.com/t/listbox-drag-to-reorder-solved/28477
        void mouseDrag(const juce::MouseEvent &e) override
        {
            if (isZone() && !isDragging && e.getDistanceFromDragStart() > 2)
            {
                if (auto *container = juce::DragAndDropContainer::findParentDragContainerFor(this))
                {
                    if (isSelected && lbm->selectedZones.size() > 1 && !e.mods.isShiftDown())
                    {
                        isDragMulti = true;
                    }
                    else
                    {
                        isDragMulti = false;
                    }
                    isPaintSnapshot = true;
                    container->startDragging("ZoneRow", this);
                    isPaintSnapshot = false;
                    isDragging = true;
                }
            }

            if (isGroup() && !isDragging && e.getDistanceFromDragStart() > 2)
            {
                if (auto *container = juce::DragAndDropContainer::findParentDragContainerFor(this))
                {
                    container->startDragging("GroupRow", this);

                    isDragging = true;
                }
            }
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

            auto za = getZoneAddress();
            gsb->onRowClicked(za, isSelected, event.mods);
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

        void itemDragEnter(const SourceDetails &dragSourceDetails) override
        {
            dragOverState = DRAG_OVER;
            repaint();
        }
        void itemDragMove(const SourceDetails &dragSourceDetails) override {}
        void itemDragExit(const SourceDetails &dragSourceDetails) override
        {
            dragOverState = NONE;
            repaint();
        }

        void itemDropped(const SourceDetails &dragSourceDetails) override
        {
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
                    // For zone rows, resolve to the parent group
                    int targetGroup = za.group;
                    int targetPart = za.part;
                    if (isZone())
                    {
                        // already have the right part/group — zone index doesn't matter
                    }
                    if (!isZone() && wsi->encompassesMultipleSampleInfos())
                    {
                        shared::executeBatchDropOnGroup(wsi, targetPart, targetGroup, gsb);
                        return;
                    }
                    auto src = shared::SampleDropSource::fromBrowserItem(wsi);
                    if (src.isSingleSample())
                    {
                        src.dropAsZoneInGroup(targetPart, targetGroup, gsb);
                        return;
                    }
                }
            }

            auto rd = dynamic_cast<rowComponent *>(sc.get());
            if (rd && rd->isZone())
            {
                auto tgt = getZoneAddress();

                if (rd->isDragMulti)
                {
                    gsb->sendToSerialization(cmsg::MoveZonesFromTo({lbm->selectedZones, tgt}));
                }
                else
                {
                    auto src = rd->getZoneAddress();

                    gsb->sendToSerialization(cmsg::MoveZonesFromTo({{src}, tgt}));
                }
            }
            else if (rd && rd->isGroup())
            {
                auto src = rd->getZoneAddress();
                auto tgt = getZoneAddress();
                gsb->sendToSerialization(cmsg::MoveGroupTo({src, tgt}));
            }
        }
        void resized() override
        {
            renameEditor->setBounds(getLocalBounds().withTrimmedLeft(zonePad));
            if (muteProvider && muteProvider->widget)
            {
                muteProvider->widget->setBounds(
                    getLocalBounds().withTrimmedLeft(getWidth() - getHeight() + 2).reduced(2));
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

        void textEditorReturnKeyPressed(juce::TextEditor &) override
        {
            auto za = getZoneAddress();
            if (isZone())
            {
                gsb->sendToSerialization(
                    cmsg::RenameZone({za, renameEditor->getText().toStdString()}));
            }
            else
            {
                gsb->sendToSerialization(
                    cmsg::RenameGroup({za, renameEditor->getText().toStdString()}));
            }
            renameEditor->setVisible(false);
        }
        void textEditorEscapeKeyPressed(juce::TextEditor &) override
        {
            renameEditor->setVisible(false);
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
            if (!fz)
                return false;
            // Only accept drags from other tree rows (zone reordering), not browser drops
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
            auto sc = dragSourceDetails.sourceComponent;
            if (!sc) // weak component
                return;

            auto rd = dynamic_cast<rowComponent *>(sc.get());
            if (rd)
            {
                if (fz)
                {
                    auto tgt =
                        selection::SelectionManager::ZoneAddress{gsb->editor->selectedPart, -1, -1};

                    if (rd->isDragMulti)
                    {
                        auto sz = lbm->selectedZones;
                        // Filter out any selected zones with zone == -1
                        std::erase_if(sz, [](const auto &za) { return za.zone == -1; });
                        // if theres still zones available (so !sz.empty) go for it
                        if (!sz.empty())
                        {
                            gsb->sendToSerialization(
                                cmsg::MoveZonesFromTo({lbm->selectedZones, tgt}));
                        }
                    }
                    else
                    {
                        auto src = rd->getZoneAddress();
                        if (src.zone > 0)
                        {
                            gsb->sendToSerialization(cmsg::MoveZonesFromTo({{src}, tgt}));
                        }
                    }
                }
                else
                {
                    // Dragging group onto a + we can happily do nothing
                }
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

    void mouseDown(const juce::MouseEvent &event) override
    {
        sidebar->editor->doSelectionAction(
            selection::SelectionManager::SelectActionContents::deselectSentinel());
    }
};

} // namespace scxt::ui::app::edit_screen

#endif // SHORTCIRCUITXT_PARTGROUPTREE_H
