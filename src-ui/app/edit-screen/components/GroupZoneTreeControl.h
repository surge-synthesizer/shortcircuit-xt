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

#ifndef SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_GROUPZONETREECONTROL_H
#define SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_GROUPZONETREECONTROL_H

#include <juce_graphics/juce_graphics.h>
#include <juce_core/juce_core.h>

#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/ListView.h"
#include "engine/feature_enums.h"

#include "app/shared/ZoneRightMouseMenu.h"

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

    GroupZoneSidebarWidget(SidebarParent *sb) : sidebar(sb)
    {
        rebuild();
        setSelectionMode(jcmp::ListView::SelectionMode::MULTI_SELECTION);
        getRowCount = [this]() { return gzData.size() + 1; };
        getRowHeight = [this]() { return 18; };
        makeRowComponent = [this]() { return std::make_unique<rowTopComponent>(); };
        assignComponentToRow = [this](const std::unique_ptr<juce::Component> &c, uint32_t row) {
            auto rc = dynamic_cast<rowTopComponent *>(c.get());
            if (rc)
            {
                if (row == gzData.size())
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
                        selectedZones.find(gzData[row].address) != selectedZones.end();
                }

                rc->repaint();
            }
        };
        setRowSelection = [this](auto &c, bool isSel) {
            auto rc = dynamic_cast<rowTopComponent *>(c.get());
            if (rc && rc->gzRow)
            {
                SCLOG("SELECTION STATE CHANGED " << rc->gzRow->rowNumber << " to " << isSel);
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
    }

    selection::SelectionManager::ZoneAddress getZoneAddress(int rowNumber)
    {
        auto &tgl = gzData;
        if (rowNumber < 0 || rowNumber >= tgl.size())
            return {};
        auto &sad = tgl[rowNumber].address;
        return sad;
    }

    struct rowComponent : juce::Component, juce::DragAndDropTarget, juce::TextEditor::Listener
    {
        int rowNumber{-1};
        bool isSelected{false};
        GroupZoneSidebarWidget<SidebarParent, forZone> *lbm{nullptr};
        SidebarParent *gsb{nullptr};

        std::unique_ptr<juce::TextEditor> renameEditor;
        rowComponent()
        {
            renameEditor = std::make_unique<juce::TextEditor>();
            addChildComponent(*renameEditor);
            renameEditor->addListener(this);
        }

        int zonePad = 16;
        int grouplabelPad = zonePad;

        enum DragOverState
        {
            NONE,
            DRAG_OVER
        } dragOverState{NONE};

        void paint(juce::Graphics &g) override
        {
            if (!gsb)
                return;

            // SCLOG("paint " << this << " " << isDragging << " " << rowNumber);
            const auto &tgl = lbm->gzData;
            if (rowNumber < 0 || rowNumber >= tgl.size())
                return;

            const auto &sg = tgl[rowNumber];

            bool isLeadZone = isZone() && gsb->isLeadZone(sg.address);
            bool isLeadGroup = isGroup() && gsb->isLeadGroup(sg.address);

            auto editor = gsb->partGroupSidebar->editor;

            auto st = gsb->partGroupSidebar->style();
            auto zoneFont = editor->themeApplier.interLightFor(11);
            auto groupFont = editor->themeApplier.interRegularFor(11);

            if (isZone())
                g.setFont(zoneFont);
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
                g.setColour(lowTextColor);
                g.drawText(std::to_string(sg.address.group + 1), bx,
                           juce::Justification::centredLeft);
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
                if (sg.features & engine::ZoneFeatures::MISSING_SAMPLE)
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

                if (isLeadZone)
                {
                    auto b = getLocalBounds().withWidth(zonePad).withTrimmedLeft(2);
                    auto q = b.getHeight() - b.getWidth();
                    b = b.withTrimmedTop(q / 2).withTrimmedBottom(q / 2);
                    jcmp::GlyphPainter::paintGlyph(g, b, jcmp::GlyphPainter::JOG_RIGHT,
                                                   textColor.withAlpha(0.5f));
                }

                if (voiceCount > 0)
                {
                    auto b = getLocalBounds()
                                 .withWidth(zonePad)
                                 .translated(getWidth() - zonePad, 0)
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
                    auto za = getZoneAddress();
                    const auto &tgl = lbm->gzData;
                    const auto &sg = tgl[rowNumber];

                    shared::populateZoneRightMouseMenuForZone(gsb, this, p, za, sg.name);
                    p.addSeparator();
                    shared::populateZoneRightMouseMenuForSelectedZones(gsb, p, za.part);
                }
                else if (isGroup())
                {
                    const auto &tgl = lbm->gzData;
                    if (rowNumber < 0 || rowNumber >= tgl.size())
                        return;

                    const auto &sg = tgl[rowNumber];

                    p.addSectionHeader(sg.name);
                    p.addSeparator();
                    p.addItem("Rename", [w = juce::Component::SafePointer(this)]() {
                        if (!w)
                            return;
                        w->doGroupRename();
                    });
                    p.addItem("Paste Zone",
                              gsb->editor->clipboardType == engine::Clipboard::ContentType::ZONE,
                              false, [w = juce::Component::SafePointer(this)]() {
                                  if (!w)
                                      return;
                                  auto za = w->getZoneAddress();
                                  w->gsb->sendToSerialization(cmsg::PasteZone(za));
                              });
                    p.addItem("Delete", [w = juce::Component::SafePointer(this)]() {
                        if (!w)
                            return;
                        auto za = w->getZoneAddress();
                        w->gsb->sendToSerialization(cmsg::DeleteGroup(za));
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
            return true; // !isZone();
        }

        void itemDragEnter(const SourceDetails &dragSourceDetails) override
        {
            dragOverState = DRAG_OVER;
            repaint();
        }
        void itemDragMove(const SourceDetails &dragSourceDetails) override
        {
            // SCLOG("Item Drag Move");
        }
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
                gsb->sendToSerialization(cmsg::MoveGroupToAfter({src, tgt}));
            }
        }
        void resized() override
        {
            renameEditor->setBounds(getLocalBounds().withTrimmedLeft(zonePad));
        }

        void doGroupRename()
        {
            const auto &tgl = lbm->gzData;

            const auto &sg = tgl[rowNumber];
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

            const auto &sg = tgl[rowNumber];
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
            return fz; // !irsZone();
        }

        void itemDragEnter(const SourceDetails &dragSourceDetails) override
        {
            isDroppingOn = true;
            repaint();
        }
        void itemDragMove(const SourceDetails &dragSourceDetails) override
        {
            // SCLOG("Item Drag Move");
        }
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
                        gsb->sendToSerialization(cmsg::MoveZonesFromTo({lbm->selectedZones, tgt}));
                    }
                    else
                    {
                        auto src = rd->getZoneAddress();

                        gsb->sendToSerialization(cmsg::MoveZonesFromTo({{src}, tgt}));
                    }
                }
                else
                {
                    // SCLOG("Dropping group onto +");
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
};

} // namespace scxt::ui::app::edit_screen

#endif // SHORTCIRCUITXT_PARTGROUPTREE_H
