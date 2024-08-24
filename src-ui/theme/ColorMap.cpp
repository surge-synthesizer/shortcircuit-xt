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
#include <map>
#include <fmt/core.h>

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>
#include <tao/json/events/from_string.hpp>

#include "json/scxt_traits.h"

#include "connectors/JSONAssetSupport.h"

#include "utils.h"

namespace scxt::ui::theme
{

using metadata_t = std::map<std::string, std::string>;
using colors_t = std::map<std::string, std::string>;
using colormap_t = std::tuple<metadata_t, colors_t>;

std::string keyName(ColorMap::Colors c)
{
#define C(x)                                                                                       \
    case ColorMap::Colors::x:                                                                      \
        return #x;

    switch (c)
    {
        C(accent_1a);
        C(accent_1b);
        C(accent_1b_alpha_a);
        C(accent_1b_alpha_b);
        C(accent_1b_alpha_c);
        C(accent_2a);
        C(accent_2a_alpha_a);
        C(accent_2a_alpha_b);
        C(accent_2a_alpha_c);
        C(accent_2b);
        C(accent_2b_alpha_a);
        C(accent_2b_alpha_b);
        C(accent_2b_alpha_c);
        C(bg_1);
        C(bg_2);
        C(bg_3);
        C(gutter_2);
        C(gutter_3);
        C(panel_outline_2);
        C(panel_outline_3);
        C(knob_fill);
        C(generic_content_highest);
        C(generic_content_high);
        C(generic_content_medium);
        C(generic_content_low);
        C(generic_content_lowest);
        C(grid_primary);
        C(grid_secondary);
        C(warning_1a);
        C(warning_1b);
    }
#undef C
    return "error";
}

struct StdMapColormap : ColorMap
{
    colors_t colorMap;

    std::array<juce::Colour, ColorMap::lastColor + 1> resolvedMap;
    StdMapColormap(colors_t &c) { setupFromColormap(c); }
    void setupFromColormap(colors_t &c)
    {
        colorMap = c;
        std::fill(resolvedMap.begin(), resolvedMap.end(), juce::Colours::red);
        std::map<std::string, int> nameToIndex;
        for (int i = 0; i < lastColor + 1; ++i)
        {
            nameToIndex[keyName((ColorMap::Colors(i)))] = i;
        }
        for (auto &[k, m] : colorMap)
        {
            auto n2f = nameToIndex.find(k);
            if (n2f != nameToIndex.end())
            {
                auto idx = n2f->second;
                if (idx != -1)
                {
                    uint32_t cx;
                    std::stringstream ss;
                    ss << std::hex << m.substr(1);
                    ss >> cx;
                    resolvedMap[idx] = juce::Colour(cx);
                    nameToIndex[k] = -1;
                }
                else
                {
                    SCLOG("Warning: Key appears twice in colormap " << k);
                }
            }
            else
            {
                SCLOG("Error: Unable to resolve colormap key '" << k << "'gre");
            }
        }
        for (auto &[k, i] : nameToIndex)
        {
            if (i >= 0)
            {
                SCLOG("Error: Color map didn't contain key '" << k << "'");
            }
        }
    }
    juce::Colour getImpl(ColorMap::Colors c, float alpha) const override
    {
        auto ic = (size_t)c;
        auto res = resolvedMap[ic];
        if (alpha == 1.f)
            return res;

        auto alp = res.getAlpha();
        auto nalp = res.getAlpha() * 1.f / 255 * alpha;
        return res.withAlpha(nalp);
    }
};

template <template <typename...> class... Transformers, template <typename...> class Traits>
void to_pretty_stream(std::ostream &os, const tao::json::basic_value<Traits> &v)
{
    tao::json::events::transformer<tao::json::events::to_pretty_stream, Transformers...> consumer(
        os, 3);
    tao::json::events::from_value(consumer, v);
}

std::string ColorMap::toJson() const
{
    colors_t colorMap;
    for (int i = 0; i <= lastColor; ++i)
    {
        Colors c = (Colors)i;
        auto col = getImpl(c);
        auto cols = fmt::format("#{:02x}{:02x}{:02x}{:02x}", col.getAlpha(), col.getRed(),
                                col.getGreen(), col.getBlue());
        auto keys = keyName(c);
        colorMap[keys] = cols;
    }

    metadata_t meta{{"version", "1"}, {"hoverfactor", std::to_string(hoverFactor)}};
    colormap_t doc{meta, colorMap};

    tao::json::value jsonv = doc;
    std::ostringstream oss;
    to_pretty_stream(oss, jsonv);
    return oss.str();
}

std::unique_ptr<ColorMap> ColorMap::jsonToColormap(const std::string &json)
{
    colormap_t cm;
    tao::json::events::transformer<tao::json::events::to_basic_value<json::scxt_traits>> consumer;
    tao::json::events::from_string(consumer, json);
    auto jv = std::move(consumer.value);
    jv.to(cm);

    auto meta = std::get<0>(cm);

    auto res = std::make_unique<StdMapColormap>(std::get<1>(cm));
    auto hvi = meta.find("hoverfactor");
    if (hvi != meta.end())
    {
        auto hf = std::atof(hvi->second.c_str());
        if (hf == 0.f)
            hf = 0.1;
        res->hoverFactor = hf;
    }
    else
    {
        res->hoverFactor = 0.1;
    }

    return res;
}

std::unique_ptr<ColorMap> ColorMap::createColorMap(scxt::ui::theme::ColorMap::BuiltInColorMaps cm)
{
    std::unique_ptr<ColorMap> res;
    std::string json;
    switch (cm)
    {
    case TEST:
        json = connectors::JSONAssetLibrary::jsonForAsset("themes/test-colors.json");
        res = ColorMap::jsonToColormap(json);
        break;
    case WIREFRAME:
        json = connectors::JSONAssetLibrary::jsonForAsset("themes/wireframe-dark.json");
        res = ColorMap::jsonToColormap(json);
        break;
    case HICONTRAST_DARK:
        json = connectors::JSONAssetLibrary::jsonForAsset("themes/wireframe-dark-hicontrast.json");
        res = ColorMap::jsonToColormap(json);
        break;
    case LIGHT:
        json = connectors::JSONAssetLibrary::jsonForAsset("themes/wireframe-light.json");
        res = ColorMap::jsonToColormap(json);
        break;
    }

    if (!res)
    {
        json = connectors::JSONAssetLibrary::jsonForAsset("themes/wireframe-dark.json");
        res = ColorMap::jsonToColormap(json);
        res->myId = WIREFRAME;
    }
    else
    {
        res->myId = cm;
    }

    assert(res);
    return res;
}

} // namespace scxt::ui::theme