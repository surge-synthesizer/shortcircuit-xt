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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_MAPPINGDISPLAY_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_MAPPINGDISPLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/Viewport.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/jucegui/components/DraggableTextEditableDiscreteValue.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/TickSeparatorLabel.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/ZoomContainer.h"
#include "sst/jucegui/component-adapters/DiscreteToReference.h"
#include "engine/part.h"
#include "engine/engine.h"
#include "selection/selection_manager.h"
#include "connectors/PayloadDataAttachment.h"
#include "app/HasEditor.h"
#include "app/KeyCommands.h"
#include "app/edit-screen/components/MacroMappingVariantPane.h"
#include "app/shared/SampleDropHandler.h"

namespace scxt::ui::app::edit_screen
{

struct ZoneLayoutDisplay;
struct ZoneLayoutKeyboard;
struct MappingDisplay;

// a toggle drawn like a glyph button, with an optional second glyph beside the first
struct GlyphToggleButton : sst::jucegui::components::ToggleButton
{
    sst::jucegui::components::GlyphPainter::GlyphType glyph;
    std::optional<sst::jucegui::components::GlyphPainter::GlyphType> secondGlyph;
    int glyphZeroWidth{0};

    GlyphToggleButton() : glyph(sst::jucegui::components::GlyphPainter::GlyphType::MIDI) {}
    void paint(juce::Graphics &g) override;
};

struct MappingZoneHeader : HasEditor, juce::Component
{
    template <typename T>
    using toggle_t = sst::jucegui::component_adapters::DiscreteToValueReference<T, bool>;

    MappingDisplay *display{nullptr};

    std::unique_ptr<sst::jucegui::components::TextPushButton> autoMap, fixOverlap, fadeOverlap;
    // the engine owns these states; the bools mirror its last word
    bool midiSelectOn{false}, keyLearnOn{false}, velLearnOn{false}, zoneSoloOn{false};
    std::unique_ptr<toggle_t<GlyphToggleButton>> midiButton, midiLRButton, midiUDButton;
    std::unique_ptr<toggle_t<sst::jucegui::components::ToggleButton>> zoneSolo;
    std::unique_ptr<toggle_t<sst::jucegui::components::ToggleButton>> lockButton;
    std::unique_ptr<sst::jucegui::components::Label> fileLabel;
    std::unique_ptr<sst::jucegui::components::GlyphButton> prevFileButton, nextFileButton;
    std::unique_ptr<sst::jucegui::components::MenuButton> fileMenu;

    MappingZoneHeader(MappingDisplay *d);

    void initiateMidiZoneAction(engine::Engine::MidiZoneAction);
    void setStateFromModel(int32_t midiZoneAction, bool zoneSolo);

    // the variant the file arrows and menu act on, in the lead zone
    int fileVariant{0};
    std::optional<selection::SelectionManager::ZoneAddress> fileNavigatorZone;
    std::string fileFullPath;
    void refreshFileNavigator();
    void stepFile(bool forward);
    void showFileMenu();
    void loadFileIntoVariant(const fs::path &, int variant);

    void resized() override
    {
        auto b = getLocalBounds().reduced(1);
        autoMap->setBounds(b.withWidth(85));

        midiButton->widget->setBounds(b.withTrimmedLeft(91).withWidth(16));
        midiLRButton->widget->setBounds(b.withTrimmedLeft(113).withWidth(28));
        midiUDButton->widget->setBounds(b.withTrimmedLeft(147).withWidth(28));
        fixOverlap->setBounds(b.withTrimmedLeft(181).withWidth(85));
        fadeOverlap->setBounds(b.withTrimmedLeft(181 + 87).withWidth(85));
        zoneSolo->widget->setBounds(b.withTrimmedLeft(181 + 2 * 87).withWidth(85));
        lockButton->widget->setBounds(b.withTrimmedLeft(181 + 3 * 87).withWidth(16));
        fileLabel->setBounds(b.withTrimmedLeft(181 + 3 * 87 + 20).withWidth(30));
        auto fb = b.withTrimmedLeft(181 + 3 * 87 + 52);
        prevFileButton->setBounds(fb.withWidth(16));
        nextFileButton->setBounds(fb.withTrimmedLeft(fb.getWidth() - 16));
        fileMenu->setBounds(fb.reduced(18, 0));
    }
};

struct MappingDisplay : juce::Component,
                        HasEditor,
                        KeyCommandTarget,
                        juce::FileDragAndDropTarget,
                        juce::DragAndDropTarget

{

    typedef connectors::DiscretePayloadDataAttachment<engine::Zone::ZoneMappingData, int16_t>
        int16Attachment_t;
    typedef connectors::PayloadDataAttachment<engine::Zone::ZoneMappingData, float>
        floatAttachment_t;
    typedef connectors::BooleanPayloadDataAttachment<engine::Zone::ZoneMappingData>
        boolAttachment_t;

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

    bool mayBeAboutToMutate{false};

    // When set, drags in the zone grid select only; they don't move or resize zones
    bool mappingLocked{false};

    // key into SCXTEditor::otherTabSelection, so the choice outlives zone changes and sessions
    static constexpr const char *showZoneNamesTabKey{"mapping.zonenames"};
    bool showZoneNames{true};
    void setShowZoneNames(bool b);
    void showHamburgerMenu();

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
            PBDown, PBUp, VelocitySens, Level, Pan, Pitch, Tracking;
    };

    // Same membership as MapEls; used for the integer-backed controls, which are
    // sparse here (only the key and velocity slots are populated).
    template <typename T> struct MapDiscreteEls
    {
        T RootKey, KeyStart, KeyEnd, FadeStart, FadeEnd, VelStart, VelEnd, VelFadeStart, VelFadeEnd,
            PBDown, PBUp, VelocitySens, Level, Pan, Pitch, Tracking;
    };

    MapDiscreteEls<std::unique_ptr<int16Attachment_t>> intAttachments;
    MapEls<std::unique_ptr<floatAttachment_t>> floatAttachments;
    // continuous (float) text editors; sparse — Level/Pan/Pitch/Tracking/VelocitySens/PB
    MapEls<std::unique_ptr<sst::jucegui::components::DraggableTextEditableValue>> textEds;
    // discrete (integer) text editors; sparse — the key and velocity slots
    MapDiscreteEls<std::unique_ptr<sst::jucegui::components::DraggableTextEditableDiscreteValue>>
        discreteTextEds;
    MapEls<std::unique_ptr<sst::jucegui::components::Label>> labels;

    static constexpr float pitchBendPixelsPerSemitone{8.f};
    std::unique_ptr<boolAttachment_t> pbUseGroupAttachment;
    void updatePitchBendEnablement();
    void showPitchBendMenu();
    void mouseDown(const juce::MouseEvent &e) override;
    MapEls<std::unique_ptr<sst::jucegui::components::GlyphPainter>> glyphs;

    enum RuleIdx
    {
        KeyHSep,
        VelHSep,
        PBHSep,
        KeyVSepLo,
        KeyVSepHi,
        VelVSepLo,
        VelVSepHi,
        NumRules
    };
    std::array<std::unique_ptr<sst::jucegui::components::TickSeparatorLabel>, NumRules> rules;

    MappingDisplay(MacroMappingVariantPane *p);
    ~MappingDisplay();

    engine::Zone::ZoneMappingData &mappingView;
    MacroMappingVariantPane *parentPane{nullptr};

    void resized() override;

    bool active{true};
    void setActive(bool b);

    void setGroupZoneMappingSummary(const engine::Part::zoneMappingSummary_t &d);

    void setLeadSelection(const selection::SelectionManager::ZoneAddress &za);

    int voiceCountFor(const selection::SelectionManager::ZoneAddress &z);

    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
    void itemDropped(const SourceDetails &dragSourceDetails) override;
    bool isUndertakingDrop{false};
    size_t dropElementCount{7};
    juce::Point<int> currentDragPoint;
    // where the pull was when ctrl went down, which freezes the span; -1 when it is up
    float ctrlLatchFromTop{-1.f};
    void itemDragEnter(const SourceDetails &dragSourceDetails) override;

    void itemDragExit(const SourceDetails &dragSourceDetails) override;

    void itemDragMove(const SourceDetails &dragSourceDetails) override;
    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void fileDragEnter(const juce::StringArray &files, int x, int y) override;
    void fileDragMove(const juce::StringArray &files, int x, int y) override;
    void fileDragExit(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;

    // Drag overlay state — used by ZoneLayoutDisplay paint and MappingDisplay::paintOverChildren
    shared::SampleDropSource currentDragSource;
    bool dragIsOnReplaceSide{false}; // true = left (replace) half, for Instrument kind

    void paintOverChildren(juce::Graphics &g) override;
    void promptForMultiInstrument(const std::vector<sample::compound::CompoundElement> &);

    void doZoneRename(const selection::SelectionManager::ZoneAddress &z);

    bool handleKeyCommand(KeyCommands command) override;

    engine::Part::zoneMappingSummary_t summary{};

    enum DeltaResult
    {
        NO_CHANGE = 0,
        KEYRANGE_CHANGED = 1,
        VELOCITY_CHANGED = 2,
        BOTH_CHANGED = KEYRANGE_CHANGED | VELOCITY_CHANGED
    };

    // Adjusts every selected zone; returns what we did to the lead zone
    DeltaResult applyDeltaToSelectedZones(engine::Zone::ChangeDimension dim, int &deltaX,
                                          int &deltaY, bool updateLead = true);

    bool applyAbsoluteToSelectedZones(engine::Zone::ChangeDimension dim, int newX, int newY,
                                      bool updateLead = true);

    bool evaluateCanApplyUISide(engine::Zone::ChangeDimension dim, int deltaX, int deltaY) const;
    void adjustDeltasToMakeSureTheyWillFit(engine::Zone::ChangeDimension dim, int &deltaX,
                                           int &deltaY, const engine::KeyboardRange &kr,
                                           const engine::VelocityRange &vr) const;
};

} // namespace scxt::ui::app::edit_screen
#endif // SHORTCIRCUITXT_MAPPINGDISPLAY_H
