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

#ifndef SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_MAPPINGDISPLAY_H
#define SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_MAPPINGDISPLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/Viewport.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/ZoomContainer.h"
#include "engine/part.h"
#include "selection/selection_manager.h"
#include "connectors/PayloadDataAttachment.h"
#include "app/HasEditor.h"
#include "app/edit-screen/components/MacroMappingVariantPane.h"

namespace scxt::ui::app::edit_screen
{

struct ZoneLayoutDisplay;
struct ZoneLayoutKeyboard;

struct MappingZoneHeader : HasEditor, juce::Component
{
    std::unique_ptr<sst::jucegui::components::TextPushButton> autoMap, fixOverlap, fadeOverlap,
        zoneSolo;
    std::unique_ptr<sst::jucegui::components::GlyphButton> midiButton, midiLRButton, midiUDButton,
        lockButton;
    std::unique_ptr<sst::jucegui::components::Label> fileLabel;
    std::unique_ptr<sst::jucegui::components::MenuButton> fileMenu;

    MappingZoneHeader(SCXTEditor *ed);

    void resized() override
    {
        auto b = getLocalBounds().reduced(1);
        autoMap->setBounds(b.withWidth(85));

        midiButton->setBounds(b.withTrimmedLeft(91).withWidth(16));
        midiLRButton->setBounds(b.withTrimmedLeft(113).withWidth(28));
        midiUDButton->setBounds(b.withTrimmedLeft(147).withWidth(28));
        fixOverlap->setBounds(b.withTrimmedLeft(181).withWidth(85));
        fadeOverlap->setBounds(b.withTrimmedLeft(181 + 87).withWidth(85));
        zoneSolo->setBounds(b.withTrimmedLeft(181 + 2 * 87).withWidth(85));
        lockButton->setBounds(b.withTrimmedLeft(181 + 3 * 87).withWidth(16));
        fileLabel->setBounds(b.withTrimmedLeft(181 + 3 * 87 + 20).withWidth(40));
        fileMenu->setBounds(b.withTrimmedLeft(500));
    }
};

struct MappingDisplay : juce::Component,
                        HasEditor,
                        juce::FileDragAndDropTarget,
                        juce::DragAndDropTarget

{

    typedef connectors::PayloadDataAttachment<engine::Zone::ZoneMappingData, int16_t>
        int16Attachment_t;
    typedef connectors::PayloadDataAttachment<engine::Zone::ZoneMappingData, float>
        floatAttachment_t;

    std::unique_ptr<sst::jucegui::components::ZoomContainer> zoneLayoutViewport;

    struct LayoutAndKeyboard : juce::Component, sst::jucegui::components::ZoomContainerClient
    {
        Component *associatedComponent() override { return this; }
        bool supportsVerticalZoom() const override { return true; }
        bool supportsHorizontalZoom() const override { return true; }
        void setHorizontalZoom(float pctStart, float zoomFactor) override;
        void setVerticalZoom(float pctStart, float zoomFactor) override;
        LayoutAndKeyboard(MappingDisplay *that);
        std::unique_ptr<ZoneLayoutDisplay> mappingZones;
        std::unique_ptr<ZoneLayoutKeyboard> keyboard;

        void resized() override;
    };
    // Weak refs here since they are owned by the zoom viewport
    ZoneLayoutDisplay *mappingZones{nullptr};
    ZoneLayoutKeyboard *keyboard{nullptr};

    bool isMovingKeyboard{false};
    bool isResizingKeyboard{false};
    bool isMovingZones{false};
    bool isResizingZones{false};

    std::unique_ptr<MappingZoneHeader> zoneHeader;

    enum Ctrl
    {
        RootKey,
        KeyStart,
        KeyEnd,
        FadeStart,
        FadeEnd,
        VelStart,
        VelEnd,
        VelFadeStart,
        VelFadeEnd,
        PBDown,
        PBUp,

        VelocitySens,
        Level,
        Pan,
        Pitch
    };

    template <typename T> struct MapEls
    {
        T RootKey, KeyStart, KeyEnd, FadeStart, FadeEnd, VelStart, VelEnd, VelFadeStart, VelFadeEnd,
            PBDown, PBUp, VelocitySens, Level, Pan, Pitch;
    };

    MapEls<std::unique_ptr<int16Attachment_t>> intAttachments;
    MapEls<std::unique_ptr<floatAttachment_t>> floatAttachments;
    MapEls<std::unique_ptr<sst::jucegui::components::DraggableTextEditableValue>> textEds;
    MapEls<std::unique_ptr<sst::jucegui::components::Label>> labels;
    MapEls<std::unique_ptr<sst::jucegui::components::GlyphPainter>> glyphs;

    MappingDisplay(MacroMappingVariantPane *p);
    ~MappingDisplay();

    engine::Zone::ZoneMappingData &mappingView;
    MacroMappingVariantPane *parentPane{nullptr};

    void resized() override;

    void mappingChangedFromGUI();
    void setActive(bool b) { setVisible(b); }

    void setGroupZoneMappingSummary(const engine::Part::zoneMappingSummary_t &d);

    void setLeadSelection(const selection::SelectionManager::ZoneAddress &za);

    int voiceCountFor(const selection::SelectionManager::ZoneAddress &z);

    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
    void itemDropped(const SourceDetails &dragSourceDetails) override;
    bool isUndertakingDrop{false};
    juce::Point<int> currentDragPoint;
    void itemDragEnter(const SourceDetails &dragSourceDetails) override;

    void itemDragExit(const SourceDetails &dragSourceDetails) override;

    void itemDragMove(const SourceDetails &dragSourceDetails) override;
    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void fileDragEnter(const juce::StringArray &files, int x, int y) override;
    void fileDragMove(const juce::StringArray &files, int x, int y) override;
    void fileDragExit(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;

    engine::Part::zoneMappingSummary_t summary{};
};

} // namespace scxt::ui::app::edit_screen
#endif // SHORTCIRCUITXT_MAPPINGDISPLAY_H
