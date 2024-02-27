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

#include "sst/jucegui/style/CustomizationTools.h"

#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/components/HSliderFilled.h"
#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/jucegui/components/ToolTip.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/JogUpDownButton.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/VUMeter.h"
#include "sst/jucegui/components/ScrollBar.h"

#include "connectors/SCXTResources.h"

#include "utils.h"

#include "ThemeApplier.h"

namespace scxt::ui::theme
{
namespace jcmp = sst::jucegui::components;
namespace jstl = sst::jucegui::style;
using sheet_t = jstl::StyleSheet;

namespace detail
{
namespace global
{
void applyColors(const sheet_t::ptr_t &base, const ColorMap &cols);
} // namespace global

namespace multi
{
static constexpr sheet_t::Class ModulationMultiSwitch{"multi.modulation.multiswitch"};
static constexpr sheet_t::Class ModulationJogButon{"multi.modulation.jogbutton"};
static constexpr sheet_t::Class ModulationToggle{"multi.modulation.toggle"};
static constexpr sheet_t::Class ModulationMenu{"multi.modulation.menu"};
static constexpr sheet_t::Class ModulationHSliderFilled{"multi.modulation.hsliderfilled"};

void applyColors(const sheet_t::ptr_t &, const ColorMap &);
void init()
{
    SCLOG("Initializing Group and Zone Multi Styles");
    sheet_t::addClass(ModulationMultiSwitch).withBaseClass(jcmp::MultiSwitch::Styles::styleClass);
    sheet_t::addClass(ModulationJogButon).withBaseClass(jcmp::JogUpDownButton::Styles::styleClass);
    sheet_t::addClass(ModulationToggle).withBaseClass(jcmp::ToggleButton::Styles::styleClass);
    sheet_t::addClass(ModulationMenu).withBaseClass(jcmp::MenuButton::Styles::styleClass);
    sheet_t::addClass(ModulationHSliderFilled)
        .withBaseClass(jcmp::HSliderFilled::Styles::styleClass);
}

namespace zone
{
static constexpr sheet_t::Class ModulationNamedPanel{"multi.zone.modulation.namedpanel"};
static constexpr sheet_t::Class ModulationVSlider{"multi.zone.modulation.vslider"};
static constexpr sheet_t::Class ModulationKnob{"multi.zone.modulation.knob"};

void applyColors(const sheet_t::ptr_t &, const ColorMap &);
void init()
{
    SCLOG("Initializing Zone Multi Styles");
    sheet_t::addClass(ModulationNamedPanel).withBaseClass(jcmp::NamedPanel::Styles::styleClass);
    sheet_t::addClass(ModulationVSlider).withBaseClass(jcmp::VSlider::Styles::styleClass);
    sheet_t::addClass(ModulationKnob).withBaseClass(jcmp::Knob::Styles::styleClass);
}

} // namespace zone
namespace group
{
static constexpr sheet_t::Class NamedPanel{"multi.group.namedpanel"};
static constexpr sheet_t::Class ModulationNamedPanel{"multi.group.modulation.namedpanel"};
static constexpr sheet_t::Class Knob{"multi.group.knob"};
static constexpr sheet_t::Class VSlider{"multi.group.vslider"};
static constexpr sheet_t::Class ModulationKnob{"multi.group.modulation.knob"};
static constexpr sheet_t::Class ModulationVSlider{"multi.group.modulation.vslider"};

void applyColors(const sheet_t::ptr_t &, const ColorMap &);
void init()
{
    SCLOG("Initializing Zone Multi Styles");
    sheet_t::addClass(NamedPanel).withBaseClass(jcmp::NamedPanel::Styles::styleClass);
    sheet_t::addClass(ModulationNamedPanel).withBaseClass(jcmp::NamedPanel::Styles::styleClass);
    sheet_t::addClass(Knob).withBaseClass(jcmp::Knob::Styles::styleClass);
    sheet_t::addClass(VSlider).withBaseClass(jcmp::VSlider::Styles::styleClass);
    sheet_t::addClass(ModulationKnob).withBaseClass(Knob);
    sheet_t::addClass(ModulationVSlider).withBaseClass(VSlider);
}

} // namespace group
} // namespace multi
} // namespace detail

ThemeApplier::ThemeApplier()
{
    static bool detailInitialized{false};
    if (!detailInitialized)
    {
        SCLOG("Initializing All Theme Stylesheets");
        detail::multi::init();
        detail::multi::zone::init();
        detail::multi::group::init();
        detailInitialized = true;
    }
    colors = ColorMap::createColorMap(ColorMap::WIREFRAME);
}

void ThemeApplier::recolorStylesheet(const sst::jucegui::style::StyleSheet::ptr_t &s)
{
    detail::global::applyColors(s, *colors);
    detail::multi::applyColors(s, *colors);
    detail::multi::zone::applyColors(s, *colors);
    detail::multi::group::applyColors(s, *colors);
}

void ThemeApplier::recolorStylesheetWith(std::unique_ptr<ColorMap> &&c, const sheet_t::ptr_t &s)
{
    colors = std::move(c);
    recolorStylesheet(s);
}

void populateSharedGroupZoneMultiModulation(jstl::CustomTypeMap &map)
{
    map.addCustomClass<jcmp::MultiSwitch>(detail::multi::ModulationMultiSwitch);
    map.addCustomClass<jcmp::JogUpDownButton>(detail::multi::ModulationJogButon);
    map.addCustomClass<jcmp::ToggleButton>(detail::multi::ModulationToggle);
    map.addCustomClass<jcmp::MenuButton>(detail::multi::ModulationMenu);
    map.addCustomClass<jcmp::HSliderFilled>(detail::multi::ModulationHSliderFilled);
}
void ThemeApplier::applyZoneMultiScreenModulationTheme(juce::Component *toThis)
{
    jstl::CustomTypeMap map;
    map.addCustomClass<jcmp::VSlider>(detail::multi::zone::ModulationVSlider);
    map.addCustomClass<jcmp::Knob>(detail::multi::zone::ModulationKnob);
    populateSharedGroupZoneMultiModulation(map);
    map.applyMapTo(toThis);
}
void ThemeApplier::applyZoneMultiScreenTheme(juce::Component *toThis) {}

void ThemeApplier::applyGroupMultiScreenModulationTheme(juce::Component *toThis)
{
    jstl::CustomTypeMap map;
    map.addCustomClass<jcmp::NamedPanel>(detail::multi::group::NamedPanel);
    map.addCustomClass<jcmp::VSlider>(detail::multi::group::ModulationVSlider);
    map.addCustomClass<jcmp::Knob>(detail::multi::group::ModulationKnob);
    populateSharedGroupZoneMultiModulation(map);
    map.applyMapTo(toThis);
}
void ThemeApplier::applyGroupMultiScreenTheme(juce::Component *toThis)
{
    jstl::CustomTypeMap map;
    map.addCustomClass<jcmp::NamedPanel>(detail::multi::group::NamedPanel);
    map.addCustomClass<jcmp::Knob>(detail::multi::group::Knob);
    map.applyMapTo(toThis);
}

juce::Font ThemeApplier::interMediumFor(int ht)
{
    static auto interMed =
        connectors::resources::loadTypeface("fonts/Inter/static/Inter-Medium.ttf");
    return juce::Font(interMed).withHeight(ht);
}

namespace detail
{
namespace global
{
void applyColors(const sheet_t::ptr_t &base, const ColorMap &cols)
{
    /*
     * Set up base classes
     */
    base->setFont(jcmp::base_styles::BaseLabel::styleClass, jcmp::base_styles::BaseLabel::labelfont,
                  juce::Font(11, juce::Font::plain));
    base->setColour(jcmp::base_styles::BaseLabel::styleClass, jcmp::NamedPanel::Styles::labelcolor,
                    cols.get(ColorMap::generic_content_medium));
    base->setColour(jcmp::base_styles::BaseLabel::styleClass,
                    jcmp::NamedPanel::Styles::labelcolor_hover,
                    cols.get(ColorMap::generic_content_medium).brighter(0.1));

    base->setColour(jcmp::base_styles::Outlined::styleClass, jcmp::base_styles::Outlined::outline,
                    cols.get(ColorMap::bg_2).brighter(0.1));
    base->setColour(jcmp::base_styles::Outlined::styleClass,
                    jcmp::base_styles::Outlined::brightoutline,
                    cols.get(ColorMap::generic_content_low));

    base->setColour(jcmp::base_styles::ValueBearing::styleClass,
                    jcmp::base_styles::ValueBearing::value, cols.get(ColorMap::accent_1a));
    base->setColour(jcmp::base_styles::ValueBearing::styleClass,
                    jcmp::base_styles::ValueBearing::value_hover,
                    cols.getHover(ColorMap::accent_1a));

    base->setColour(jcmp::base_styles::PushButton::styleClass, jcmp::base_styles::PushButton::fill,
                    cols.get(ColorMap::generic_content_low));
    base->setColour(jcmp::base_styles::PushButton::styleClass,
                    jcmp::base_styles::PushButton::fill_hover,
                    cols.getHover(ColorMap::generic_content_low));
    base->setColour(jcmp::base_styles::PushButton::styleClass,
                    jcmp::base_styles::PushButton::fill_pressed,
                    cols.getHover(ColorMap::generic_content_low));

    base->setColour(jcmp::base_styles::ValueBearing::styleClass,
                    jcmp::base_styles::ValueBearing::value, cols.get(ColorMap::accent_1a));
    base->setColour(jcmp::base_styles::ValueBearing::styleClass,
                    jcmp::base_styles::ValueBearing::value_hover,
                    cols.getHover(ColorMap::accent_1a));
    base->setColour(jcmp::base_styles::ValueBearing::styleClass,
                    jcmp::base_styles::ValueBearing::valuelabel,
                    cols.get(ColorMap::generic_content_medium));
    base->setColour(jcmp::base_styles::ValueBearing::styleClass,
                    jcmp::base_styles::ValueBearing::valuelabel_hover,
                    cols.getHover(ColorMap::generic_content_medium));

    base->setColour(jcmp::base_styles::ValueGutter::styleClass,
                    jcmp::base_styles::ValueGutter::gutter, cols.get(ColorMap::gutter_2));
    base->setColour(jcmp::base_styles::ValueGutter::styleClass,
                    jcmp::base_styles::ValueGutter::gutter_hover,
                    cols.getHover(ColorMap::gutter_2));

    base->setColour(jcmp::Knob::Styles::styleClass, jcmp::Knob::Styles::knobbase,
                    cols.get(ColorMap::knob_fill));
    base->setColour(jcmp::Knob::Styles::styleClass, jcmp::Knob::Styles::handle,
                    cols.get(ColorMap::generic_content_medium));
    base->setColour(jcmp::Knob::Styles::styleClass, jcmp::Knob::Styles::handle_hover,
                    cols.getHover(ColorMap::generic_content_medium));

    base->setColour(jcmp::ScrollBar::Styles::styleClass, jcmp::ScrollBar::Styles::outline,
                    cols.get(ColorMap::generic_content_low));

    base->setColour(jcmp::base_styles::GraphicalHandle::styleClass,
                    jcmp::base_styles::GraphicalHandle::handle,
                    cols.get(ColorMap::generic_content_high));
    base->setColour(jcmp::base_styles::GraphicalHandle::styleClass,
                    jcmp::base_styles::GraphicalHandle::handle_hover,
                    cols.get(ColorMap::generic_content_high).brighter(0.1));
    base->setColour(jcmp::base_styles::GraphicalHandle::styleClass,
                    jcmp::base_styles::GraphicalHandle::handle_outline,
                    cols.get(ColorMap::generic_content_high).brighter(0.1));

    base->setColour(jcmp::VUMeter::Styles::styleClass, jcmp::VUMeter::Styles::vu_gutter,
                    cols.get(ColorMap::gutter_2));
    base->setColour(jcmp::VUMeter::Styles::styleClass, jcmp::VUMeter::Styles::vu_gradstart,
                    cols.get(ColorMap::accent_1b));
    base->setColour(jcmp::VUMeter::Styles::styleClass, jcmp::VUMeter::Styles::vu_gradend,
                    cols.get(ColorMap::accent_1a));
    base->setColour(jcmp::VUMeter::Styles::styleClass, jcmp::VUMeter::Styles::vu_overload,
                    cols.get(ColorMap::warning_1b));

    base->setColour(jcmp::DraggableTextEditableValue::Styles::styleClass,
                    jcmp::DraggableTextEditableValue::Styles::background_editing,
                    cols.get(ColorMap::bg_3));

    base->setColour(jcmp::JogUpDownButton::Styles::styleClass,
                    jcmp::JogUpDownButton::Styles::jogbutton_hover, cols.get(ColorMap::accent_1a));

    base->setColour(jcmp::ToolTip::Styles::styleClass, jcmp::ToolTip::Styles::background,
                    cols.get(ColorMap::bg_1));

    base->setColour(jcmp::WindowPanel::Styles::styleClass, jcmp::WindowPanel::Styles::bgstart,
                    cols.get(ColorMap::bg_1));
    base->setColour(jcmp::WindowPanel::Styles::styleClass, jcmp::WindowPanel::Styles::bgend,
                    cols.get(ColorMap::bg_1).darker(0.05));

    base->setColour(jcmp::NamedPanel::Styles::styleClass, jcmp::NamedPanel::Styles::background,
                    cols.get(ColorMap::bg_2));
    base->setColour(jcmp::NamedPanel::Styles::styleClass,
                    jcmp::NamedPanel::Styles::backgroundSelected, cols.get(ColorMap::bg_3));
    base->setColour(jcmp::NamedPanel::Styles::styleClass, jcmp::NamedPanel::Styles::labelrule,
                    cols.get(ColorMap::generic_content_low));
    auto npol = cols.get(ColorMap::panel_outline_2);
    if (npol.getAlpha() == 0)
        npol = cols.get(ColorMap::bg_2).brighter(0.05);
    base->setColour(jcmp::NamedPanel::Styles::styleClass, jcmp::NamedPanel::Styles::brightoutline,
                    npol);
    base->setColour(jcmp::NamedPanel::Styles::styleClass, jcmp::NamedPanel::Styles::selectedtab,
                    cols.get(ColorMap::accent_1a));

    base->setColour(jcmp::MenuButton::Styles::styleClass, jcmp::MenuButton::Styles::menuarrow_hover,
                    cols.get(ColorMap::accent_1a));

    auto interMed = connectors::resources::loadTypeface("fonts/Inter/static/Inter-Medium.ttf");
    base->replaceFontsWithTypeface(interMed);

    auto fixedWidth =
        connectors::resources::loadTypeface("fonts/Anonymous_Pro/AnonymousPro-Regular.ttf");

    auto fw = juce::Font(fixedWidth).withHeight(11);
    base->setFont(sst::jucegui::components::ToolTip::Styles::styleClass,
                  sst::jucegui::components::ToolTip::Styles::datafont, fw);
}
} // namespace global
namespace multi
{
void applyColors(const sheet_t::ptr_t &base, const ColorMap &cols)
{
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::background,
                    cols.get(ColorMap::bg_2));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::value,
                    cols.get(ColorMap::accent_2a));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::valuelabel,
                    cols.get(ColorMap::generic_content_high));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::value_hover,
                    cols.get(ColorMap::accent_2a).brighter(0.1));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::valuelabel_hover,
                    cols.get(ColorMap::generic_content_high).brighter(0.1));

    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::unselected_hover,
                    cols.get(ColorMap::generic_content_low));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::labelcolor,
                    cols.get(ColorMap::generic_content_medium).brighter(0.1));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::labelcolor_hover,
                    cols.get(ColorMap::generic_content_high).brighter(0.1));

    base->setColour(ModulationJogButon, jcmp::JogUpDownButton::Styles::jogbutton_hover,
                    cols.get(ColorMap::accent_2a));

    base->setColour(ModulationMenu, jcmp::MenuButton::Styles::menuarrow_hover,
                    cols.get(ColorMap::accent_2a));

    base->setColour(ModulationHSliderFilled, jcmp::HSliderFilled::Styles::value,
                    cols.get(ColorMap::accent_2a));
    base->setColour(ModulationHSliderFilled, jcmp::HSliderFilled::Styles::value_hover,
                    cols.get(ColorMap::accent_2a));

    base->setColour(ModulationToggle, jcmp::ToggleButton::Styles::value,
                    cols.get(ColorMap::accent_2a));
    base->setColour(ModulationToggle, jcmp::ToggleButton::Styles::value_hover,
                    cols.get(ColorMap::accent_2a).brighter(0.1));
}
namespace zone
{
void applyColors(const sheet_t::ptr_t &base, const ColorMap &cols)
{
    base->setColour(ModulationVSlider, jcmp::VSlider::Styles::value, cols.get(ColorMap::accent_2a));
    base->setColour(ModulationVSlider, jcmp::VSlider::Styles::value_hover,
                    cols.get(ColorMap::accent_2a).brighter(0.1));

    base->setColour(ModulationKnob, jcmp::Knob::Styles::value, cols.get(ColorMap::accent_2a));
    base->setColour(ModulationKnob, jcmp::Knob::Styles::value_hover,
                    cols.get(ColorMap::accent_2a).brighter(0.1));

    base->setColour(ModulationNamedPanel, jcmp::NamedPanel::Styles::selectedtab,
                    cols.get(ColorMap::accent_2a));
}
} // namespace zone
namespace group
{
void applyColors(const sheet_t::ptr_t &base, const ColorMap &cols)
{
    base->setColour(NamedPanel, jcmp::NamedPanel::Styles::background, cols.get(ColorMap::bg_3));
    auto npol = cols.get(ColorMap::panel_outline_3);
    if (npol.getAlpha() == 0)
        npol = cols.get(ColorMap::bg_3).brighter(0.05);
    base->setColour(NamedPanel, jcmp::NamedPanel::Styles::brightoutline, npol);

    base->setColour(ModulationNamedPanel, jcmp::NamedPanel::Styles::background,
                    cols.get(ColorMap::bg_3));
    base->setColour(ModulationNamedPanel, jcmp::NamedPanel::Styles::brightoutline,
                    cols.get(ColorMap::bg_3).brighter(0.05));
    base->setColour(ModulationNamedPanel, jcmp::NamedPanel::Styles::selectedtab,
                    cols.get(ColorMap::accent_2a));

    base->setColour(Knob, jcmp::Knob::Styles::gutter, cols.get(ColorMap::gutter_3));
    base->setColour(Knob, jcmp::Knob::Styles::gutter_hover, cols.getHover(ColorMap::gutter_3));

    base->setColour(VSlider, jcmp::VSlider::Styles::gutter, cols.get(ColorMap::gutter_3));
    base->setColour(VSlider, jcmp::VSlider::Styles::gutter_hover,
                    cols.getHover(ColorMap::gutter_3));

    base->setColour(ModulationVSlider, jcmp::VSlider::Styles::value, cols.get(ColorMap::accent_2a));
    base->setColour(ModulationVSlider, jcmp::VSlider::Styles::value_hover,
                    cols.get(ColorMap::accent_2a).brighter(0.1));

    base->setColour(ModulationKnob, jcmp::Knob::Styles::value, cols.get(ColorMap::accent_2a));
    base->setColour(ModulationKnob, jcmp::Knob::Styles::value_hover,
                    cols.get(ColorMap::accent_2a).brighter(0.1));
}
} // namespace group
} // namespace multi
} // namespace detail
} // namespace scxt::ui::theme