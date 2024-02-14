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

#include "SCXTStyleSheetCreator.h"
#include "sst/jucegui/components/BaseStyles.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/ToolTip.h"
#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/components/JogUpDownButton.h"
#include "connectors/SCXTResources.h"

namespace scxt::ui::connectors
{
using sheet_t = sst::jucegui::style::StyleSheet;
namespace comp = sst::jucegui::components;

void SCXTStyleSheetCreator::makeDarkColors(const sheet_t::ptr_t &base)
{
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::gutter,
                    juce::Colour(0x39, 0x39, 0x39));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::gutter_hover,
                    juce::Colour(0x49, 0x49, 0x59));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::value,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::value_hover,
                    juce::Colour(0x37, 0x98, 0xFF));

    base->setColour(ModulationTabs, comp::NamedPanel::Styles::selectedtab,
                    juce::Colour(0x27, 0x88, 0xD6));

    base->setColour(ModulationEditorKnob, comp::Knob::Styles::value,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationEditorKnob, comp::VSlider::Styles::value_hover,
                    juce::Colour(0x37, 0x98, 0xFF));

    base->setColour(ModulationMultiSwitch, comp::MultiSwitch::Styles::value,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationMultiSwitch, comp::MultiSwitch::Styles::value_hover,
                    juce::Colour(0x27, 0x88, 0xD6).brighter(0.4));

    base->setColour(ModulationJogButon, comp::JogUpDownButton::Styles::jogbutton_hover,
                    juce::Colour(0x27, 0x88, 0xD6));

    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::value,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::value_hover,
                    juce::Colour(0x27, 0x88, 0xD6).brighter(0.4));

    // TODO - do hvoer
    base->setColour(InformationLabel, comp::base_styles::BaseLabel::labelcolor,
                    juce::Colour(0x88, 0x88, 0x88));

    base->setColour(GroupMultiNamedPanel, comp::NamedPanel::Styles::background,
                    juce::Colour(0x15, 0x30, 0x15));
    base->setColour(GroupMultiNamedPanel, comp::NamedPanel::Styles::brightoutline,
                    juce::Colour(0x25, 0x80, 0x25));
}

void SCXTStyleSheetCreator::makeLightColors(const sheet_t::ptr_t &base)
{
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::gutter,
                    juce::Colour(200, 200, 210));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::gutter_hover,
                    juce::Colour(220, 220, 240));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::value,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::value_hover,
                    juce::Colour(0x37, 0x98, 0xFF));

    base->setColour(ModulationTabs, comp::NamedPanel::Styles::selectedtab,
                    juce::Colour(0x27, 0x88, 0xD6));

    base->setColour(ModulationEditorKnob, comp::Knob::Styles::gutter, juce::Colour(200, 200, 210));
    base->setColour(ModulationEditorKnob, comp::Knob::Styles::gutter_hover,
                    juce::Colour(220, 220, 240));
    base->setColour(ModulationEditorKnob, comp::Knob::Styles::value,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationEditorKnob, comp::VSlider::Styles::value_hover,
                    juce::Colour(0x37, 0x98, 0xFF));

    base->setColour(InformationLabel, comp::base_styles::BaseLabel::labelcolor,
                    juce::Colour(0x88, 0x88, 0x88));

    base->setColour(GroupMultiNamedPanel, comp::NamedPanel::Styles::background,
                    juce::Colour(220, 230, 220));
    base->setColour(GroupMultiNamedPanel, comp::NamedPanel::Styles::brightoutline,
                    juce::Colour(85, 150, 85));
}

const sheet_t::ptr_t
SCXTStyleSheetCreator::setup(sst::jucegui::style::StyleSheet::BuiltInTypes baseType)
{
    sheet_t::addClass(SCXTStyleSheetCreator::ModulationEditorVSlider)
        .withBaseClass(comp::VSlider::Styles::styleClass);
    sheet_t::addClass(SCXTStyleSheetCreator::ModulationEditorKnob)
        .withBaseClass(comp::Knob::Styles::styleClass);
    sheet_t::addClass(SCXTStyleSheetCreator::ModulationTabs)
        .withBaseClass(comp::NamedPanel::Styles::styleClass);
    sheet_t::addClass(SCXTStyleSheetCreator::ModulationMatrixToggle)
        .withBaseClass(comp::ToggleButton::Styles::styleClass);
    sheet_t::addClass(SCXTStyleSheetCreator::ModulationMultiSwitch)
        .withBaseClass(comp::MultiSwitch::Styles::styleClass);
    sheet_t::addClass(SCXTStyleSheetCreator::ModulationJogButon)
        .withBaseClass(comp::JogUpDownButton::Styles::styleClass);
    sheet_t::addClass(SCXTStyleSheetCreator::ModulationMatrixMenu)
        .withBaseClass(SCXTStyleSheetCreator::ModulationMatrixToggle);
    sheet_t::addClass(SCXTStyleSheetCreator::InformationLabel)
        .withBaseClass(comp::base_styles::BaseLabel::styleClass);
    sheet_t::addClass(SCXTStyleSheetCreator::GroupMultiNamedPanel)
        .withBaseClass(comp::NamedPanel::Styles::styleClass);

    const auto &base = sheet_t::getBuiltInStyleSheet(baseType);
    if (baseType == sst::jucegui::style::StyleSheet::DARK)
    {
        makeDarkColors(base);
    }
    else
    {
        makeLightColors(base);
    }

    auto interMed = connectors::resources::loadTypeface("fonts/Inter/static/Inter-Medium.ttf");
    base->replaceFontsWithTypeface(interMed);

    auto fixedWidth =
        connectors::resources::loadTypeface("fonts/Anonymous_Pro/AnonymousPro-Regular.ttf");

    auto fw = juce::Font(fixedWidth).withHeight(11);
    base->setFont(sst::jucegui::components::ToolTip::Styles::styleClass,
                  sst::jucegui::components::ToolTip::Styles::datafont, fw);

    return base;
}

juce::Font SCXTStyleSheetCreator::interMediumFor(int ht)
{
    static auto interMed =
        connectors::resources::loadTypeface("fonts/Inter/static/Inter-Medium.ttf");
    return juce::Font(interMed).withHeight(ht);
}

} // namespace scxt::ui::connectors