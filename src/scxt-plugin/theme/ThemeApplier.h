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

#ifndef SCXT_SRC_SCXT_PLUGIN_THEME_THEMEAPPLIER_H
#define SCXT_SRC_SCXT_PLUGIN_THEME_THEMEAPPLIER_H

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>
#include <sst/jucegui/style/StyleSheet.h>
#include <sst/jucegui/components/Label.h>

#include "ColorMap.h"

namespace scxt::ui
{
namespace app
{
struct SCXTEditor;
}
namespace theme
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
    void applyVariantLoopTheme(juce::Component *toThis);
    void applyMixerEffectTheme(juce::Component *toThis);
    void applyMixerChannelTheme(juce::Component *toThis);
    void applyHeaderTheme(juce::Component *toThis);
    void applyHeaderSCButtonTheme(sst::jucegui::style::StyleConsumer *);
    void applyChannelStripTheme(juce::Component *toThis);
    void applyAuxChannelStripTheme(juce::Component *toThis);

    // Some utilities to move single items
    void setLabelToHighlight(sst::jucegui::style::StyleConsumer *);

    juce::Font interBoldFor(int ht) const;
    juce::Font interMediumFor(int ht) const;
    juce::Font interRegularFor(int ht) const;
    juce::Font interLightFor(int ht) const;
    juce::Font anonmyousProRegularFor(int ht) const;

    friend scxt::ui::app::SCXTEditor;

  private:
    std::unique_ptr<ColorMap> colors;
};
} // namespace theme
} // namespace scxt::ui
#endif // SHORTCIRCUITXT_THEMEAPPLIER_H
