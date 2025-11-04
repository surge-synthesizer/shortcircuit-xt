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

#include "MacroMappingVariantPane.h"
#include "app/SCXTEditor.h"
#include "app/edit-screen/EditScreen.h"
#include "datamodel/metadata.h"
#include "selection/selection_manager.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/TabbedComponent.h"
#include "sst/jucegui/components/Viewport.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/ZoomContainer.h"
#include "connectors/PayloadDataAttachment.h"
#include "messaging/client/client_serial.h"
#include "messaging/client/client_messages.h"

#include "engine/part.h"

#include "app/edit-screen/components/mapping-pane/VariantDisplay.h"
#include "app/edit-screen/components/mapping-pane/MacroDisplay.h"
#include "app/edit-screen/components/mapping-pane/MappingDisplay.h"
#include "app/edit-screen/components/mapping-pane/SampleWaveform.h"

namespace scxt::ui::app::edit_screen
{

namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;

struct MappingDisplay;

MacroMappingVariantPane::MacroMappingVariantPane(SCXTEditor *e)
    : sst::jucegui::components::NamedPanel(""), HasEditor(e)
{
    hasHamburger = true;
    isTabbed = true;
    tabNames = {"MACROS", "MAPPING", "SAMPLE"};
    selectTab(1);

    mappingDisplay = std::make_unique<MappingDisplay>(this);
    addAndMakeVisible(*mappingDisplay);

    sampleDisplay = std::make_unique<VariantDisplay>(this);
    addChildComponent(*sampleDisplay);

    macroDisplay = std::make_unique<MacroDisplay>(editor);
    addChildComponent(*macroDisplay);

    resetTabState();

    onTabSelected = [wt = juce::Component::SafePointer(this)](int i) {
        if (wt)
        {
            wt->setSelectedTab(i);
        }
    };
}

MacroMappingVariantPane::~MacroMappingVariantPane() {}

void MacroMappingVariantPane::setSelectedTab(int i)
{
    if (i < 0 || i > 2)
        return;
    selectedTab = i;
    sampleDisplay->setVisible(i == 2);
    mappingDisplay->setVisible(i == 1);
    macroDisplay->setVisible(i == 0);

    repaint();
    editor->setTabSelection(editor->editScreen->tabKey("multi.mapping"), std::to_string(i));
}

void MacroMappingVariantPane::resized()
{
    mappingDisplay->setBounds(getContentArea());
    sampleDisplay->setBounds(getContentArea());
    macroDisplay->setBounds(getContentArea());
}

void MacroMappingVariantPane::setMappingData(const engine::Zone::ZoneMappingData &m)
{
    mappingView = m;
    mappingDisplay->repaint();
}

void MacroMappingVariantPane::setSampleData(const engine::Zone::Variants &m)
{
    sampleView = m;
    sampleDisplay->rebuildForSelectedVariation(sampleDisplay->selectedVariation, true);
}

void MacroMappingVariantPane::setActive(bool b)
{
    mappingDisplay->setActive(b);
    mappingDisplay->setVisible(selectedTab == 1);
    sampleDisplay->setActive(b);
    sampleDisplay->setVisible(selectedTab == 2);
    macroDisplay->setVisible(selectedTab == 0);
}

void MacroMappingVariantPane::setGroupZoneMappingSummary(
    const engine::Part::zoneMappingSummary_t &d)
{
    mappingDisplay->setGroupZoneMappingSummary(d);
}

void MacroMappingVariantPane::editorSelectionChanged()
{
    if (editor->currentLeadZoneSelection.has_value())
        mappingDisplay->setLeadSelection(*(editor->currentLeadZoneSelection));

    sampleDisplay->rebuildForSelectedVariation(sampleDisplay->selectedVariation, true);
    repaint();
}

void MacroMappingVariantPane::invertScroll(bool invert)
{
    SCLOG_UNIMPL("InvertScroll todo");
    // mappingDisplay->mappingViewport->invertScroll(invert);
    for (auto &w : sampleDisplay->waveforms)
    {
        // w.waveformViewport->invertScroll(invert);
    }
}

void MacroMappingVariantPane::addSamplePlaybackPosition(size_t sampleIndex, int64_t samplePos)
{
    if (sampleDisplay->isVisible() && sampleIndex == sampleDisplay->selectedVariation)
        sampleDisplay->waveforms[sampleDisplay->selectedVariation]
            .waveform->addSamplePlaybackPosition(samplePos);
}

void MacroMappingVariantPane::clearSamplePlaybackPositions()
{
    sampleDisplay->waveforms[sampleDisplay->selectedVariation]
        .waveform->clearSamplePlaybackPositions();
}

void MacroMappingVariantPane::selectedPartChanged() { macroDisplay->selectedPartChanged(); }
void MacroMappingVariantPane::macroDataChanged(int part, int index)
{
    assert(part == editor->selectedPart);
    macroDisplay->macroDataChanged(part, index);
}

MappingZoneHeader::MappingZoneHeader(scxt::ui::app::SCXTEditor *ed) : HasEditor(ed)
{
    autoMap = std::make_unique<sst::jucegui::components::TextPushButton>();
    autoMap->setLabel("AUTO-MAP");
    autoMap->setOnCallback(editor->makeComingSoon());
    addAndMakeVisible(*autoMap);

    midiButton = std::make_unique<sst::jucegui::components::GlyphButton>(
        sst::jucegui::components::GlyphPainter::GlyphType::MIDI);
    midiButton->setOnCallback(
        [this]() { initiateMidiZoneAction(engine::Engine::MidiZoneAction::SELECT); });
    addAndMakeVisible(*midiButton);

    midiUDButton = std::make_unique<sst::jucegui::components::GlyphButton>(
        sst::jucegui::components::GlyphPainter::GlyphType::MIDI,
        sst::jucegui::components::GlyphPainter::GlyphType::UP_DOWN, 16);
    midiUDButton->setOnCallback(
        [this]() { initiateMidiZoneAction(engine::Engine::MidiZoneAction::SET_VEL_BOUNDS_START); });
    addAndMakeVisible(*midiUDButton);

    midiLRButton = std::make_unique<sst::jucegui::components::GlyphButton>(
        sst::jucegui::components::GlyphPainter::GlyphType::MIDI,
        sst::jucegui::components::GlyphPainter::GlyphType::LEFT_RIGHT, 16);
    midiLRButton->setOnCallback(
        [this]() { initiateMidiZoneAction(engine::Engine::MidiZoneAction::SET_KEY_BOUNDS_START); });
    addAndMakeVisible(*midiLRButton);

    if (scxt::hasFeature::mappingPane11Controls)
    {
        fixOverlap = std::make_unique<sst::jucegui::components::TextPushButton>();
        fixOverlap->setLabel("FIX OVERLAP");
        fixOverlap->setOnCallback(editor->makeComingSoon());
        addAndMakeVisible(*fixOverlap);

        fadeOverlap = std::make_unique<sst::jucegui::components::TextPushButton>();
        fadeOverlap->setLabel("FADE OVERLAP");
        fadeOverlap->setOnCallback(editor->makeComingSoon());
        addAndMakeVisible(*fadeOverlap);

        zoneSolo = std::make_unique<sst::jucegui::components::TextPushButton>();
        zoneSolo->setLabel("ZONE SOLO");
        zoneSolo->setOnCallback(editor->makeComingSoon());
        addAndMakeVisible(*zoneSolo);

        lockButton = std::make_unique<sst::jucegui::components::GlyphButton>(
            sst::jucegui::components::GlyphPainter::GlyphType::LOCK);
        lockButton->setOnCallback(editor->makeComingSoon());
        addAndMakeVisible(*lockButton);
    }

    fileLabel = std::make_unique<sst::jucegui::components::Label>();
    fileLabel->setText("FILE");
    addAndMakeVisible(*fileLabel);

    fileMenu = std::make_unique<sst::jucegui::components::MenuButton>();
    fileMenu->setLabel("sample-name-to-come.wav");
    fileMenu->setOnCallback(editor->makeComingSoon());
    addAndMakeVisible(*fileMenu);
}

void MappingZoneHeader::initiateMidiZoneAction(engine::Engine::MidiZoneAction a)
{
    sendToSerialization(cmsg::InitiateMidiZoneAction{(int)a});
}

} // namespace scxt::ui::app::edit_screen
