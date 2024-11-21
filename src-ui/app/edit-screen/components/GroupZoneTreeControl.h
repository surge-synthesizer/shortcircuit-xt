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
#include "engine/feature_enums.h"

namespace scxt::ui::app::edit_screen
{

namespace jcmp = sst::jucegui::components;
namespace cmsg = scxt::messaging::client;

template <typename SidebarParent, bool fz> struct GroupZoneListBoxModel : juce::ListBoxModel
{
    static constexpr bool forZone{fz};
    SidebarParent *sidebar{nullptr};
    engine::Engine::pgzStructure_t thisGroup;
    GroupZoneListBoxModel(SidebarParent *sb) : sidebar(sb) { rebuild(); }
    void rebuild()
    {
        auto &sbo = sidebar->editor->currentLeadZoneSelection;

        auto &pgz = sidebar->partGroupSidebar->pgzStructure;
        thisGroup.clear();
        for (const auto &el : pgz)
            if (el.address.part == sidebar->editor->selectedPart && el.address.group >= 0)
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
            sidebar->processRowsChanged();
        }
    }

    selection::SelectionManager::ZoneAddress getZoneAddress(int rowNumber)
    {
        auto &tgl = thisGroup;
        if (rowNumber < 0 || rowNumber >= tgl.size())
            return {};
        auto &sad = tgl[rowNumber].address;
        return sad;
    }

    struct rowComponent : juce::Component, juce::DragAndDropTarget, juce::TextEditor::Listener
    {
        int rowNumber{-1};
        bool isSelected{false};
        GroupZoneListBoxModel<SidebarParent, forZone> *lbm{nullptr};
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
            const auto &tgl = lbm->thisGroup;
            if (rowNumber < 0 || rowNumber >= tgl.size())
                return;

            const auto &sg = tgl[rowNumber];

            bool isLeadZone = isZone() && gsb->isLeadZone(sg.address);

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
                if (isLeadZone && isZone())
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
                if (isSelected && isGroup())
                {
                    fillColor = editor->themeColor(theme::ColorMap::accent_1b, 0.4);
                    textColor = editor->themeColor(theme::ColorMap::generic_content_highest);
                    lowTextColor = editor->themeColor(theme::ColorMap::accent_1a);
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
                    p.addSectionHeader("Zone");
                    p.addSeparator();
                    p.addItem("Rename", [w = juce::Component::SafePointer(this)]() {
                        if (!w)
                            return;
                        w->doZoneRename();
                    });
                    p.addItem("Delete", [w = juce::Component::SafePointer(this)]() {
                        if (!w)
                            return;
                        auto za = w->getZoneAddress();
                        w->gsb->sendToSerialization(cmsg::DeleteZone(za));
                    });
                    p.addItem("Delete All Selected Zones",
                              [w = juce::Component::SafePointer(this)]() {
                                  if (!w)
                                      return;
                                  auto za = w->getZoneAddress();
                                  w->gsb->sendToSerialization(cmsg::DeleteAllSelectedZones(true));
                              });
                }
                else if (isGroup())
                {
                    const auto &tgl = lbm->thisGroup;
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
                    p.addItem("Delete", [w = juce::Component::SafePointer(this)]() {
                        if (!w)
                            return;
                        auto za = w->getZoneAddress();
                        w->gsb->sendToSerialization(cmsg::DeleteGroup(za));
                    });
                }

                p.addItem("Delete All Zones and Groups",
                          [w = juce::Component::SafePointer(this)]() {
                              if (!w)
                                  return;
                              auto za = w->getZoneAddress();

                              w->gsb->sendToSerialization(cmsg::ClearPart(za.part));
                          });

                if (gsb->editor->hasMissingSamples)
                {
                    p.addSeparator();
                    p.addItem("Resolve Missing Samples",
                              [w = juce::Component::SafePointer(this)]() {
                                  if (!w)
                                      return;
                                  w->gsb->editor->showMissingResolutionScreen();
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

            if (!isDragging && e.getDistanceFromDragStart() > 2)
            {
                if (auto *container = juce::DragAndDropContainer::findParentDragContainerFor(this))
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
            gsb->onRowClicked(getZoneAddress(), isSelected, event.mods);
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
            if (rd)
            {
                auto tgt = getZoneAddress();
                auto src = rd->getZoneAddress();

                gsb->sendToSerialization(cmsg::MoveZoneFromTo({src, tgt}));
            }
        }
        void resized() override
        {
            renameEditor->setBounds(getLocalBounds().withTrimmedLeft(zonePad));
        }

        void doGroupRename()
        {
            const auto &tgl = lbm->thisGroup;

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

        void doZoneRename()
        {
            const auto &tgl = lbm->thisGroup;

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
    struct rowAddComponent : juce::Component
    {
        SidebarParent *gsb{nullptr};
        std::unique_ptr<jcmp::GlyphButton> gBut;
        rowAddComponent()
        {
            gBut = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::GlyphType::PLUS);
            addAndMakeVisible(*gBut);
            gBut->glyphButtonPad = 3;
            gBut->setOnCallback([this]() { gsb->addGroup(); });
        }

        void resized()
        {
            auto b = getLocalBounds().withSizeKeepingCentre(getHeight(), getHeight()).reduced(1);
            gBut->setBounds(b);
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(rowAddComponent);
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

} // namespace scxt::ui::app::edit_screen

#endif // SHORTCIRCUITXT_PARTGROUPTREE_H
