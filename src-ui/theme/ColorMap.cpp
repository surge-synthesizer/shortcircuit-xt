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

#include "ColorMap.h"

namespace scxt::ui::theme
{
struct WireframeColors : ColorMap
{
    juce::Colour getImpl(ColorMap::Colors c, float alpha) const override
    {
        auto res = juce::Colours::red;

        switch (c)
        {
        case accent_1a:
            res = juce::Colour(0xFFFFB949);
            break;
        case accent_1b:
            res = juce::Colour(0xFFD09030);
            break;
        case accent_2a:
            res = juce::Colour(0xFF2788D6);
            break;
        case accent_2b:
            res = juce::Colour(0xFF004f8A);
            break;
        case bg_1:
            res = juce::Colour(0xFF1B1D20);
            break;
        case bg_2:
            res = juce::Colour(0xFF262a2f);
            break;
        case bg_3:
            res = juce::Colour(0xFF333333);
            break;

        case knob_fill:
            res = juce::Colour(82, 82, 82);
            break;

        case generic_content_highest:
            res = juce::Colour(0xFFFFFFFF);
            break;
        case generic_content_high:
            res = juce::Colour(0xFFDFDFDF);
            break;
        case generic_content_medium:
            res = juce::Colour(0xFFAFAFA);
            break;
        case generic_content_low:
            res = juce::Colour(0xFF777777);
            break;
        case generic_content_lowest:
            res = juce::Colour(0xFF000000);
            break;

        case grid_primary:
            res = juce::Colour(0xA8393939);
            break;
        case grid_secondary:
            res = juce::Colour(0x14A6A6A6);
            break;

        case warning_1a:
            res = juce::Colour(0xFFD67272);
            break;
        case warning_1b:
            res = juce::Colour(0xFF8A0000);
            break;

        case gutter_2:
            res = getImpl(bg_3, alpha);
            break;
        case gutter_3:
            res = getImpl(bg_2, alpha);
            break;
        case panel_outline_2:
        case panel_outline_3:
            res = juce::Colour(0x00000000);
            break;
        }
        return res.withAlpha(std::clamp(res.getAlpha() * alpha, 0.f, 1.f));
    }
};

struct TestColors : ColorMap
{
    juce::Colour getImpl(ColorMap::Colors c, float alpha) const override
    {
        auto res = juce::Colours::red;

        switch (c)
        {
        case accent_1a:
            res = juce::Colours::orchid;
            break;
        case accent_1b:
            res = juce::Colours::pink;
            break;
        case accent_2a:
            res = juce::Colours::aquamarine.darker(0.3);
            break;
        case accent_2b:
            res = juce::Colours::cyan;
            break;
        case bg_1:
            res = juce::Colours::greenyellow;
            break;
        case bg_2:
            res = juce::Colours::darkgoldenrod;
            break;
        case bg_3:
            res = juce::Colours::pink;
            break;

        case knob_fill:
            res = juce::Colours::greenyellow;
            break;

        case generic_content_highest:
            res = juce::Colour(0xFF0000FF);
            break;
        case generic_content_high:
            res = juce::Colour(0xFF0000DA);
            break;
        case generic_content_medium:
            res = juce::Colour(0xFF0000AF);
            break;
        case generic_content_low:
            res = juce::Colour(0xFF000077);
            break;
        case generic_content_lowest:
            res = juce::Colour(0xFF000000);
            break;

        case grid_primary:
            res = juce::Colour(0xA8393939);
            break;
        case grid_secondary:
            res = juce::Colour(0x14A6A6A6);
            break;

        case warning_1a:
            res = juce::Colour(0xFFD67272);
            break;
        case warning_1b:
            res = juce::Colour(0xFF8A0000);
            break;

        case gutter_2:
            res = getImpl(bg_3, alpha);
            break;
        case gutter_3:
            res = getImpl(bg_2, alpha);
            break;
        case panel_outline_2:
        case panel_outline_3:
            res = juce::Colour(0x00000000);
            break;
        }
        return res.withAlpha(alpha);
    }
};

struct HiContrastDark : ColorMap
{
    WireframeColors under;
    juce::Colour getImpl(ColorMap::Colors c, float alpha) const override
    {
        auto res = juce::Colours::red;

        switch (c)
        {
        case accent_1a:
            res = juce::Colour(0xFFFFA904);
            break;

        case accent_2a:
            res = juce::Colour(0xFF0788F6);
            break;

        case bg_1:
            res = juce::Colour(0xFF000000);
            break;
        case bg_2:
            res = juce::Colour(0xFF22222A);
            break;
        case bg_3:
            res = juce::Colour(0xFF222A22);
            break;

        case knob_fill:
            res = juce::Colour(82, 82, 82);
            break;

        case generic_content_highest:
            res = juce::Colour(0xFFFFFFFF);
            break;
        case generic_content_high:
            res = juce::Colour(0xFFDFDFDF);
            break;
        case generic_content_medium:
            res = juce::Colour(0xFFCFCFC);
            break;
        case generic_content_low:
            res = juce::Colour(0xFF999999);
            break;
        case generic_content_lowest:
            res = juce::Colour(0xFF000000);
            break;

        case gutter_2:
            res = juce::Colour(0xFF888899);
            break;
        case gutter_3:
            res = juce::Colour(0xFF889988);
            break;
        case panel_outline_2:
            res = juce::Colour(0xFFAAAABB);
            break;
        case panel_outline_3:
            res = juce::Colour(0xFFAABBAA);
            break;
        default:
            res = under.get(c, alpha);
            break;
        }
        return res.withAlpha(std::clamp(res.getAlpha() * alpha, 0.f, 1.f));
    }

    virtual juce::Colour getHover(Colors c, float alpha = 1.f) const override
    {
        auto cres = get(c, alpha);
        return cres.brighter(0.25);
    }
};

std::unique_ptr<ColorMap> ColorMap::createColorMap(scxt::ui::theme::ColorMap::BuiltInColorMaps cm)
{
    std::unique_ptr<ColorMap> res;
    switch (cm)
    {
    case TEST:
        res = std::make_unique<TestColors>();
        break;
    case WIREFRAME:
        res = std::make_unique<WireframeColors>();
        break;
    case HICONTRAST_DARK:
        res = std::make_unique<HiContrastDark>();
        break;
    }

    if (!res)
    {
        res = std::make_unique<WireframeColors>();
        res->myId = WIREFRAME;
    }
    else
    {
        res->myId = cm;
    }

    return res;
}
} // namespace scxt::ui::theme