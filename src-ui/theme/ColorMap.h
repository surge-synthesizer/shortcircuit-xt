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

#ifndef SCXT_SRC_UI_THEME_COLORMAP_H
#define SCXT_SRC_UI_THEME_COLORMAP_H

#include <juce_gui_basics/juce_gui_basics.h>

namespace scxt::ui::theme
{
struct ColorMap
{
    enum BuiltInColorMaps : uint32_t
    {
        WIREFRAME = 'wire',
        TEST = 'test',
        HICONTRAST_DARK = 'hidk'
    } myId{WIREFRAME};
    static std::unique_ptr<ColorMap> createColorMap(BuiltInColorMaps cm);

    virtual ~ColorMap() = default;
    enum Colors
    {
        accent_1a,
        accent_1b,
        accent_2a,
        accent_2a_alpha_a,
        accent_2a_alpha_b,
        accent_2a_alpha_c,
        accent_2b,
        bg_1,
        bg_2,
        bg_3,

        gutter_2, // defaults to bg_3
        gutter_3, // defaults to bg_2

        panel_outline_2, // defaults to 0x00000000
        panel_outline_3, // defaults to 0x00000000

        knob_fill,

        generic_content_highest,
        generic_content_high,
        generic_content_medium,
        generic_content_low,
        generic_content_lowest,

        grid_primary,
        grid_secondary,

        warning_1a,
        warning_1b
    };
    bool hasKnobs{false};

    juce::Colour get(Colors c, float alpha = 1.f) const
    {
        auto res = getImpl(c, alpha);
        if (c == knob_fill && !hasKnobs)
            return juce::Colour(0x00000000);
        return res;
    }
    virtual juce::Colour getHover(Colors c, float alpha = 1.f) const
    {
        auto cres = get(c, alpha);
        return cres.brighter(0.1);
    }
    virtual juce::Colour getImpl(Colors c, float alpha = 1.f) const = 0;
};

} // namespace scxt::ui::theme
#endif // SHORTCIRCUITXT_COLORMAP_H
