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

#ifndef SCXT_SRC_UI_COMPONENTS_MULTI_DETAIL_GROUPZONETREECONTROL_H
#define SCXT_SRC_UI_COMPONENTS_MULTI_DETAIL_GROUPZONETREECONTROL_H

#include <juce_graphics/juce_graphics.h>
#include <juce_core/juce_core.h>

#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/Label.h"

namespace scxt::ui::multi::detail
{

namespace jcmp = sst::jucegui::components;
namespace cmsg = scxt::messaging::client;

template <typename SidebarParent> struct GroupZoneListBoxModel : juce::ListBoxModel
{
    SidebarParent *sidebar{nullptr};
    engine::Engine::pgzStructure_t thisGroup;
    GroupZoneListBoxModel(SidebarParent *sb) : sidebar(sb) { rebuild(); }
    void rebuild()
    {
        auto &sbo = sidebar->editor->currentLeadZoneSelection;

        auto &pgz = sidebar->partGroupSidebar->pgzStructure;
        thisGroup.clear();
        for (const auto &el : pgz)
            if (el.first.part == sidebar->editor->selectedPart && el.first.group >= 0)
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
        auto &sad = tgl[rowNumber].first;
        return sad;
    }

    struct rowComponent : juce::Component, juce::DragAndDropTarget, juce::TextEditor::Listener
    {
        int rowNumber{-1};
        bool isSelected{false};
        GroupZoneListBoxModel<SidebarParent> *lbm{nullptr};
        SidebarParent *gsb{nullptr};

        std::unique_ptr<juce::TextEditor> renameEditor;
        rowComponent()
        {
            renameEditor = std::make_unique<juce::TextEditor>();
            addChildComponent(*renameEditor);
            renameEditor->addListener(this);
        }

        int zonePad = 20;

        void paint(juce::Graphics &g) override
        {
            if (!gsb)
                return;
            const auto &tgl = lbm->thisGroup;
            if (rowNumber < 0 || rowNumber >= tgl.size())
                return;

            const auto &sg = tgl[rowNumber];

            bool isLeadZone = isZone() && gsb->isLeadZone(sg.first);

            auto st = gsb->partGroupSidebar->style();
            g.setFont(st->getFont(jcmp::Label::Styles::styleClass, jcmp::Label::Styles::labelfont));

            // TODO: Style all of these
            auto borderColor = juce::Colour(0xFF, 0x90, 0x00).darker(0.4);
            auto textColor = juce::Colour(190, 190, 190);
            auto lowTextColor = textColor.darker(0.4);
            auto fillColor = juce::Colour(0, 0, 0).withAlpha(0.f);

            if (isSelected)
                fillColor = juce::Colour(0x40, 0x20, 0x00);
            if (isLeadZone)
            {
                fillColor = juce::Colour(0x15, 0x15, 0x50);
                textColor = juce::Colour(170, 170, 220);
            }
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

                if (isLeadZone)
                {
                    g.drawText("*",
                               getLocalBounds()
                                   .translated(zonePad + 2, 0)
                                   .withWidth(getWidth() - zonePad - 5),
                               juce::Justification::centredRight);
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
                    p.addItem("Rename", []() {});
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

                    p.addSectionHeader(sg.second);
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
        void resized() override
        {
            renameEditor->setBounds(getLocalBounds().withTrimmedLeft(zonePad));
        }

        void doGroupRename()
        {
            const auto &tgl = lbm->thisGroup;

            const auto &sg = tgl[rowNumber];
            assert(sg.first.zone < 0);
            auto st = gsb->partGroupSidebar->style();
            renameEditor->setFont(
                st->getFont(jcmp::Label::Styles::styleClass, jcmp::Label::Styles::labelfont));
            renameEditor->applyFontToAllText(
                st->getFont(jcmp::Label::Styles::styleClass, jcmp::Label::Styles::labelfont));
            renameEditor->setText(sg.second);
            renameEditor->setSelectAllWhenFocused(true);
            renameEditor->setVisible(true);
            renameEditor->grabKeyboardFocus();
        }

        void textEditorReturnKeyPressed(juce::TextEditor &) override
        {
            auto za = getZoneAddress();
            gsb->sendToSerialization(
                cmsg::RenameGroup({za, renameEditor->getText().toStdString()}));
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

} // namespace scxt::ui::multi::detail

#endif // SHORTCIRCUITXT_PARTGROUPTREE_H
