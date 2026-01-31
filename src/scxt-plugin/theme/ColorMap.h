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

#ifndef SCXT_SRC_SCXT_PLUGIN_THEME_COLORMAP_H
#define SCXT_SRC_SCXT_PLUGIN_THEME_COLORMAP_H

#include <juce_gui_basics/juce_gui_basics.h>

namespace scxt::ui::theme
{
struct ColorMap
{
    enum BuiltInColorMaps : uint32_t
    {
        WIREFRAME = 'wire',

        CELTIC = 'cltc',
        LUX2 = 'lux2',
        GRAYLOW = 'grlw',
        OCEANOR = 'ocen'

    } myId{WIREFRAME};
    static constexpr uint32_t FILE_COLORMAP_ID{'fcmp'};
    static std::unique_ptr<ColorMap> createColorMap(BuiltInColorMaps cm);

    virtual ~ColorMap() = default;
    /*
     * the order of these doesn't matter - we stream as strings - but they must
     * be continguous and you have to update lastColor if you change the last color
     */
    enum Colors : size_t
    {
        accent_1a,
        accent_1b,
        accent_1b_alpha_a,
        accent_1b_alpha_b,
        accent_1b_alpha_c,
        accent_2a,
        accent_2a_alpha_a,
        accent_2a_alpha_b,
        accent_2a_alpha_c,
        accent_2b,
        accent_2b_alpha_a,
        accent_2b_alpha_b,
        accent_2b_alpha_c,
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
        warning_1b, // update lastColor if you add after this please!
    };
    static constexpr int lastColor{(int)Colors::warning_1b};

    bool hasKnobs{false};
    float hoverFactor{0.1f};

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
        if (hoverFactor > 0)
            return cres.brighter(hoverFactor);
        else
            return cres.darker(-hoverFactor);
    }
    virtual juce::Colour getImpl(Colors c, float alpha = 1.f) const = 0;

    // https://www.youtube.com/watch?v=e0RWtQOeuRo
    std::string toJson() const;
    static std::unique_ptr<ColorMap> jsonToColormap(const std::string &json);
};

} // namespace scxt::ui::theme
#endif // SHORTCIRCUITXT_COLORMAP_H
