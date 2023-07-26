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

#include "SCXTBinary.h"
#include "SCXTStyleSheetCreator.h"
#include "sst/jucegui/components/BaseStyles.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/components/NamedPanel.h"

#include "components/widgets/Tooltip.h"

namespace scxt::ui::connectors
{
using sheet_t = sst::jucegui::style::StyleSheet;
namespace comp = sst::jucegui::components;

void SCXTStyleSheetCreator::makeDarkColors(const sheet_t::ptr_t &base)
{
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::guttercol,
                    juce::Colour(0x39, 0x39, 0x39));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::gutterhovcol,
                    juce::Colour(0x49, 0x49, 0x59));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::valcol,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::handlecol,
                    juce::Colour(0xC4, 0xC4, 0xC4));

    base->setColour(ModulationTabs, comp::NamedPanel::Styles::selectedtabcol,
                    juce::Colour(0x27, 0x88, 0xD6));

    base->setColour(ModulationEditorKnob, comp::Knob::Styles::guttercol,
                    juce::Colour(0x39, 0x39, 0x39));
    base->setColour(ModulationEditorKnob, comp::Knob::Styles::gutterhovcol,
                    juce::Colour(0x49, 0x49, 0x59));
    base->setColour(ModulationEditorKnob, comp::Knob::Styles::valcol,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationEditorKnob, comp::Knob::Styles::handlecol,
                    juce::Colour(0xC4, 0xC4, 0xC4));
    base->setColour(ModulationEditorKnob, comp::Knob::Styles::gradientcenter,
                    juce::Colour(0x27, 0x68, 0xA6));

    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::onbgcol,
                    juce::Colour(0x15, 0x15, 0x15));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::hoveronbgcol,
                    juce::Colour(0x15, 0x15, 0x15));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::offbgcol,
                    juce::Colour(0x15, 0x15, 0x15));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::hoveroffbgcol,
                    juce::Colour(0x15, 0x15, 0x15));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::textoncol,
                    juce::Colour(0xFF, 0x90, 0x00));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::textoffcol,
                    juce::Colour(0x88, 0x88, 0x88));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::borderoncol,
                    juce::Colour(0xFF, 0x90, 0x00));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::bordercol,
                    juce::Colour(0x35, 0x35, 0x35));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::texthoveroncol,
                    juce::Colour(0xD6, 0xD6, 0xD6));

    base->setColour(comp::ToggleButton::Styles::styleClass, comp::ToggleButton::Styles::onbgcol,
                    juce::Colour(0x15, 0x15, 0x15));
    base->setColour(comp::ToggleButton::Styles::styleClass,
                    comp::ToggleButton::Styles::hoveronbgcol, juce::Colour(0x15, 0x15, 0x15));
    base->setColour(comp::ToggleButton::Styles::styleClass, comp::ToggleButton::Styles::offbgcol,
                    juce::Colour(0x15, 0x15, 0x15));
    base->setColour(comp::ToggleButton::Styles::styleClass,
                    comp::ToggleButton::Styles::hoveroffbgcol, juce::Colour(0x15, 0x15, 0x15));
    base->setColour(comp::ToggleButton::Styles::styleClass, comp::ToggleButton::Styles::textoncol,
                    juce::Colour(0xFF, 0x90, 0x00));
    base->setColour(comp::ToggleButton::Styles::styleClass, comp::ToggleButton::Styles::textoffcol,
                    juce::Colour(0x88, 0x88, 0x88));
    base->setColour(comp::ToggleButton::Styles::styleClass, comp::ToggleButton::Styles::borderoncol,
                    juce::Colour(0xFF, 0x90, 0x00));
    base->setColour(comp::ToggleButton::Styles::styleClass, comp::ToggleButton::Styles::bordercol,
                    juce::Colour(0x35, 0x35, 0x35));
    base->setColour(comp::ToggleButton::Styles::styleClass,
                    comp::ToggleButton::Styles::texthoveroncol, juce::Colour(0xD6, 0xD6, 0xD6));

    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::onbgcol,
                    juce::Colour(0x15, 0x15, 0x15));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::hoveronbgcol,
                    juce::Colour(0x15, 0x15, 0x15));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::offbgcol,
                    juce::Colour(0x15, 0x15, 0x15));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::hoveroffbgcol,
                    juce::Colour(0x15, 0x15, 0x15));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::textoncol,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::textoffcol,
                    juce::Colour(0x88, 0x88, 0x88));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::borderoncol,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::bordercol,
                    juce::Colour(0x15, 0x15, 0x15));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::texthoveroncol,
                    juce::Colour(0xD6, 0xD6, 0xD6));

    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::textoncol,
                    juce::Colour(0xD6, 0xD6, 0xD6));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::texthoveroncol,
                    juce::Colour(0xE6, 0xE6, 0xF6));

    base->setColour(InformationLabel, comp::ControlStyles::controlLabelCol,
                    juce::Colour(0x88, 0x88, 0x88));

    base->setColour(GroupMultiNamedPanel, comp::NamedPanel::Styles::regionBG,
                    juce::Colour(0x15, 0x30, 0x15));
    base->setColour(GroupMultiNamedPanel, comp::NamedPanel::Styles::regionBorder,
                    juce::Colour(0x25, 0x80, 0x25));
}

void SCXTStyleSheetCreator::makeLightColors(const sheet_t::ptr_t &base)
{
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::guttercol,
                    juce::Colour(200, 200, 210));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::gutterhovcol,
                    juce::Colour(220, 220, 240));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::valcol,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::handlecol,
                    juce::Colour(0xC4, 0xC4, 0xC4));

    base->setColour(ModulationTabs, comp::NamedPanel::Styles::selectedtabcol,
                    juce::Colour(0x27, 0x88, 0xD6));

    base->setColour(ModulationEditorKnob, comp::Knob::Styles::guttercol,
                    juce::Colour(200, 200, 210));
    base->setColour(ModulationEditorKnob, comp::Knob::Styles::gutterhovcol,
                    juce::Colour(220, 220, 240));
    base->setColour(ModulationEditorKnob, comp::Knob::Styles::valcol,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationEditorKnob, comp::Knob::Styles::handlecol,
                    juce::Colour(0xC4, 0xC4, 0xC4));
    base->setColour(ModulationEditorKnob, comp::Knob::Styles::gradientcenter,
                    juce::Colour(0x27, 0x68, 0xA6));

    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::onbgcol,
                    juce::Colour(220, 220, 220));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::hoveronbgcol,
                    juce::Colour(220, 220, 240));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::offbgcol,
                    juce::Colour(200, 200, 210));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::hoveroffbgcol,
                    juce::Colour(200, 200, 210));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::textoncol,
                    juce::Colour(0x00, 0x00, 0x40));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::textoffcol,
                    juce::Colour(0x88, 0x88, 0x88));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::borderoncol,
                    juce::Colour(0xFF, 0x90, 0x00));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::bordercol,
                    juce::Colour(0x35, 0x35, 0x35));
    base->setColour(comp::MenuButton::Styles::styleClass, comp::MenuButton::Styles::texthoveroncol,
                    juce::Colour(0x30, 0x30, 0x90));

    base->setColour(comp::ToggleButton::Styles::styleClass, comp::ToggleButton::Styles::onbgcol,
                    juce::Colour(200, 200, 210));
    base->setColour(comp::ToggleButton::Styles::styleClass,
                    comp::ToggleButton::Styles::hoveronbgcol, juce::Colour(220, 220, 240));
    base->setColour(comp::ToggleButton::Styles::styleClass, comp::ToggleButton::Styles::offbgcol,
                    juce::Colour(0xA0, 0xA0, 0xA0));
    base->setColour(comp::ToggleButton::Styles::styleClass,
                    comp::ToggleButton::Styles::hoveroffbgcol, juce::Colour(0x15, 0x15, 0x15));
    base->setColour(comp::ToggleButton::Styles::styleClass, comp::ToggleButton::Styles::textoncol,
                    juce::Colour(0x00, 0x00, 0x40));
    base->setColour(comp::ToggleButton::Styles::styleClass, comp::ToggleButton::Styles::textoffcol,
                    juce::Colour(0x40, 0x40, 0x40));
    base->setColour(comp::ToggleButton::Styles::styleClass, comp::ToggleButton::Styles::borderoncol,
                    juce::Colour(0xFF, 0x90, 0x00));
    base->setColour(comp::ToggleButton::Styles::styleClass, comp::ToggleButton::Styles::bordercol,
                    juce::Colour(0x35, 0x35, 0x35));
    base->setColour(comp::ToggleButton::Styles::styleClass,
                    comp::ToggleButton::Styles::texthoveroncol, juce::Colour(0x30, 0x30, 0x50));

    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::onbgcol,
                    juce::Colour(200, 200, 210));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::hoveronbgcol,
                    juce::Colour(220, 220, 240));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::offbgcol,
                    juce::Colour(0xA0, 0xA0, 0xA0));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::hoveroffbgcol,
                    juce::Colour(0xA0, 0xA0, 0xC0));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::textoncol,
                    juce::Colour(0x00, 0x00, 0x40));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::textoffcol,
                    juce::Colour(0x30, 0x30, 0x30));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::borderoncol,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::bordercol,
                    juce::Colour(0x15, 0x15, 0x15));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::texthoveroncol,
                    juce::Colour(0x00, 0x00, 0x40));

    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::textoncol,
                    juce::Colour(0x00, 0x00, 0x00));
    base->setColour(ModulationMatrixToggle, comp::ToggleButton::Styles::texthoveroncol,
                    juce::Colour(0x00, 0x00, 0x40));

    base->setColour(InformationLabel, comp::ControlStyles::controlLabelCol,
                    juce::Colour(0x88, 0x88, 0x88));

    base->setColour(GroupMultiNamedPanel, comp::NamedPanel::Styles::regionBG,
                    juce::Colour(220, 230, 220));
    base->setColour(GroupMultiNamedPanel, comp::NamedPanel::Styles::regionBorder,
                    juce::Colour(85, 150, 85));
}

const sheet_t::ptr_t
SCXTStyleSheetCreator::setup(sst::jucegui::style::StyleSheet::BuiltInTypes baseType)
{
    widgets::Tooltip::Styles::initialize();

    sheet_t::addClass(SCXTStyleSheetCreator::ModulationEditorVSlider)
        .withBaseClass(comp::VSlider::Styles::styleClass);
    sheet_t::addClass(SCXTStyleSheetCreator::ModulationEditorKnob)
        .withBaseClass(comp::Knob::Styles::styleClass);
    sheet_t::addClass(SCXTStyleSheetCreator::ModulationTabs)
        .withBaseClass(comp::NamedPanel::Styles::styleClass);
    sheet_t::addClass(SCXTStyleSheetCreator::ModulationMatrixToggle)
        .withBaseClass(comp::ToggleButton::Styles::styleClass);
    sheet_t::addClass(SCXTStyleSheetCreator::ModulationMatrixMenu)
        .withBaseClass(SCXTStyleSheetCreator::ModulationMatrixToggle);
    sheet_t::addClass(SCXTStyleSheetCreator::InformationLabel)
        .withBaseClass(comp::ControlStyles::styleClass);
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

    base->setColour(widgets::Tooltip::Styles::styleClass, widgets::Tooltip::Styles::bordercol,
                    base->getColour(sst::jucegui::components::NamedPanel::Styles::styleClass,
                                    sst::jucegui::components::NamedPanel::Styles::labelrulecol));

    base->setFont(comp::NamedPanel::Styles::styleClass, comp::NamedPanel::Styles::regionLabelFont,
                  juce::Font(11));

    auto interMed = juce::Typeface::createSystemTypefaceFor(scxt::ui::binary::InterMedium_ttf,
                                                            scxt::ui::binary::InterMedium_ttfSize);
    base->replaceFontsWithTypeface(interMed);

    auto fixedWidth = juce::Typeface::createSystemTypefaceFor(
        scxt::ui::binary::AnonymousProRegular_ttf, scxt::ui::binary::AnonymousProRegular_ttfSize);

    auto fw = juce::Font(fixedWidth).withHeight(11);
    base->setFont(widgets::Tooltip::Styles::styleClass, widgets::Tooltip::Styles::datafont, fw);

    return base;
}

juce::Font SCXTStyleSheetCreator::interMediumFor(int ht)
{
    static auto interMed = juce::Typeface::createSystemTypefaceFor(
        scxt::ui::binary::InterMedium_ttf, scxt::ui::binary::InterMedium_ttfSize);
    return juce::Font(interMed).withHeight(ht);
}

} // namespace scxt::ui::connectors