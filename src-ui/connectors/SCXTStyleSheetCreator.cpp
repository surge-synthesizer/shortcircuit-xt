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
#include "sst/jucegui/components/VSlider.h"

namespace scxt::ui::connectors
{
using sheet_t = sst::jucegui::style::StyleSheet;
namespace comp = sst::jucegui::components;
const sheet_t::ptr_t SCXTStyleSheetCreator::setup()
{
    sheet_t::addClass(SCXTStyleSheetCreator::ModulationEditorVSlider)
        .withBaseClass(comp::VSlider::Styles::styleClass);
    const auto &base = sheet_t::getBuiltInStyleSheet(sheet_t::DARK);
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::guttercol,
                    juce::Colour(0x39, 0x39, 0x39));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::valcol,
                    juce::Colour(0x27, 0x88, 0xD6));
    base->setColour(ModulationEditorVSlider, comp::VSlider::Styles::handlecol,
                    juce::Colour(0xC4, 0xC4, 0xC4));

    auto interMed = juce::Typeface::createSystemTypefaceFor(
        scxt::ui::binary::InterMedium_ttf, scxt::ui::binary::InterMedium_ttfSize);
    base->replaceFontsWithTypeface(interMed);

    return base;
}

} // namespace scxt::ui::connectors