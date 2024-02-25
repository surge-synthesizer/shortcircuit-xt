//
// Created by Paul Walker on 2/25/24.
//

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
        }
        return res.withAlpha(alpha);
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