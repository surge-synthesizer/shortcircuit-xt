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

#ifndef SCXT_SRC_UI_THEME_THEMEAPPLIER_H
#define SCXT_SRC_UI_THEME_THEMEAPPLIER_H

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>
#include <sst/jucegui/style/StyleSheet.h>

#include "ColorMap.h"

namespace scxt::ui::theme
{
struct ThemeApplier
{
    ThemeApplier();

    void recolorStylesheet(const sst::jucegui::style::StyleSheet::ptr_t &);
    void recolorStylesheetWith(std::unique_ptr<ColorMap> &&,
                               const sst::jucegui::style::StyleSheet::ptr_t &);

    void applyMultiScreenSidebarTheme(juce::Component *toThis);
    void applyZoneMultiScreenTheme(juce::Component *toThis);
    void applyZoneMultiScreenModulationTheme(juce::Component *toThis);
    void applyGroupMultiScreenTheme(juce::Component *toThis);
    void applyGroupMultiScreenModulationTheme(juce::Component *toThis);
    void applyMixerEffectTheme(juce::Component *toThis);
    void applyMixerChannelTheme(juce::Component *toThis);

    const std::unique_ptr<ColorMap> &colorMap() { return colors; }

    juce::Font interMediumFor(int ht);

  protected:
    std::unique_ptr<ColorMap> colors;
};
} // namespace scxt::ui::theme
#endif // SHORTCIRCUITXT_THEMEAPPLIER_H
