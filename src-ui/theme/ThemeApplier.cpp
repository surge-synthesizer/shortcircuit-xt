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
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/components/GlyphButton.h"
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

namespace edit
{
static constexpr sheet_t::Class ModulationJogButon{"multi.modulation.jogbutton"};
static constexpr sheet_t::Class ModulationToggle{"multi.modulation.toggle"};
static constexpr sheet_t::Class ModulationMenu{"multi.modulation.menu"};

void applyColors(const sheet_t::ptr_t &, const ColorMap &);
void init()
{
    sheet_t::addClass(ModulationJogButon).withBaseClass(jcmp::JogUpDownButton::Styles::styleClass);
    sheet_t::addClass(ModulationToggle).withBaseClass(jcmp::ToggleButton::Styles::styleClass);
    sheet_t::addClass(ModulationMenu).withBaseClass(jcmp::MenuButton::Styles::styleClass);
}

namespace zone
{
static constexpr sheet_t::Class ModulationMultiSwitch{"multi.zone.modulation.multiswitch"};
static constexpr sheet_t::Class ModulationNamedPanel{"multi.zone.modulation.namedpanel"};
static constexpr sheet_t::Class ModulationVSlider{"multi.zone.modulation.vslider"};
static constexpr sheet_t::Class ModulationKnob{"multi.zone.modulation.knob"};
static constexpr sheet_t::Class ModulationDraggableTextEditableValue{
    "multi.zone.modulation.draggabletexteditablevalue"};
static constexpr sheet_t::Class ModulationHSliderFilled{"multi.zone.modulation.hsliderfilled"};

void applyColors(const sheet_t::ptr_t &, const ColorMap &);
void init()
{
    sheet_t::addClass(ModulationMultiSwitch).withBaseClass(jcmp::MultiSwitch::Styles::styleClass);
    sheet_t::addClass(ModulationNamedPanel).withBaseClass(jcmp::NamedPanel::Styles::styleClass);
    sheet_t::addClass(ModulationVSlider).withBaseClass(jcmp::VSlider::Styles::styleClass);
    sheet_t::addClass(ModulationKnob).withBaseClass(jcmp::Knob::Styles::styleClass);
    sheet_t::addClass(ModulationDraggableTextEditableValue)
        .withBaseClass(jcmp::DraggableTextEditableValue::Styles::styleClass);
    sheet_t::addClass(ModulationHSliderFilled)
        .withBaseClass(jcmp::HSliderFilled::Styles::styleClass);
}

} // namespace zone
namespace group
{
static constexpr sheet_t::Class MultiSwitch{"multi.group.multiswitch"};
static constexpr sheet_t::Class ModulationMultiSwitch{"multi.group.modulation.multiswitch"};

static constexpr sheet_t::Class NamedPanel{"multi.group.namedpanel"};
static constexpr sheet_t::Class ModulationNamedPanel{"multi.group.modulation.namedpanel"};
static constexpr sheet_t::Class Knob{"multi.group.knob"};
static constexpr sheet_t::Class VSlider{"multi.group.vslider"};
static constexpr sheet_t::Class DraggableTextEditableValue{
    "multi.group.draggabletexteditablevalue"};
static constexpr sheet_t::Class ModulationKnob{"multi.group.modulation.knob"};
static constexpr sheet_t::Class ModulationVSlider{"multi.group.modulation.vslider"};
static constexpr sheet_t::Class ModulationDraggableTextEditableValue{
    "multi.group.modulation.draggabletexteditablevalue"};
static constexpr sheet_t::Class ModulationHSliderFilled{"multi.group.modulation.hsliderfilled"};

void applyColors(const sheet_t::ptr_t &, const ColorMap &);
void init()
{
    sheet_t::addClass(NamedPanel).withBaseClass(jcmp::NamedPanel::Styles::styleClass);
    sheet_t::addClass(ModulationNamedPanel).withBaseClass(jcmp::NamedPanel::Styles::styleClass);
    sheet_t::addClass(MultiSwitch).withBaseClass(jcmp::MultiSwitch::Styles::styleClass);
    sheet_t::addClass(ModulationMultiSwitch).withBaseClass(MultiSwitch);

    sheet_t::addClass(Knob).withBaseClass(jcmp::Knob::Styles::styleClass);
    sheet_t::addClass(VSlider).withBaseClass(jcmp::VSlider::Styles::styleClass);
    sheet_t::addClass(DraggableTextEditableValue)
        .withBaseClass(jcmp::DraggableTextEditableValue::Styles::styleClass);
    sheet_t::addClass(ModulationKnob).withBaseClass(Knob);
    sheet_t::addClass(ModulationVSlider).withBaseClass(VSlider);
    sheet_t::addClass(ModulationDraggableTextEditableValue)
        .withBaseClass(DraggableTextEditableValue);
    sheet_t::addClass(ModulationHSliderFilled)
        .withBaseClass(jcmp::HSliderFilled::Styles::styleClass);
}

} // namespace group
} // namespace edit
namespace header
{
static constexpr sheet_t::Class TextPushButton{"header.textbutton"};
static constexpr sheet_t::Class ToggleButton{"header.togglebutton"};
static constexpr sheet_t::Class MenuButton{"header.menubutton"};
static constexpr sheet_t::Class GlyphButton{"header.menubutton"};
static constexpr sheet_t::Class GlyphButtonAccent{"header.menubutton.accent"};
void applyColorsAndFonts(const sheet_t::ptr_t &, const ColorMap &, const ThemeApplier &);
void init()
{
    sheet_t::addClass(TextPushButton).withBaseClass(jcmp::TextPushButton::Styles::styleClass);
    sheet_t::addClass(ToggleButton).withBaseClass(jcmp::ToggleButton::Styles::styleClass);
    sheet_t::addClass(MenuButton).withBaseClass(jcmp::MenuButton::Styles::styleClass);
    sheet_t::addClass(GlyphButton).withBaseClass(jcmp::GlyphButton::Styles::styleClass);
    sheet_t::addClass(GlyphButtonAccent).withBaseClass(GlyphButton);
}
} // namespace header

namespace mix
{
namespace channelstrip
{
static constexpr sheet_t::Class CSPanel{"channelstrip.namedpanel"};
static constexpr sheet_t::Class VUMeter{"channelstrip.vumeter"};
void applyColorsAndFonts(const sheet_t::ptr_t &, const ColorMap &, const ThemeApplier &);
void init()
{
    sheet_t::addClass(CSPanel).withBaseClass(jcmp::NamedPanel::Styles::styleClass);
    sheet_t::addClass(VUMeter).withBaseClass(jcmp::VUMeter::Styles::styleClass);
}
} // namespace channelstrip
namespace auxchannelstrip
{
static constexpr sheet_t::Class AuxPanel{"auxchannelstrip.namedpanel"};
static constexpr sheet_t::Class VUMeter{"auzchannelstrip.vumeter"};
void applyColorsAndFonts(const sheet_t::ptr_t &, const ColorMap &, const ThemeApplier &);
void init()
{
    sheet_t::addClass(AuxPanel).withBaseClass(jcmp::NamedPanel::Styles::styleClass);
    sheet_t::addClass(VUMeter).withBaseClass(jcmp::VUMeter::Styles::styleClass);
}
} // namespace auxchannelstrip
} // namespace mix
namespace util
{
static constexpr sheet_t::Class LabelHighlight("util.label.highlight");
void applyColors(const sheet_t::ptr_t &, const ColorMap &);

void init() { sheet_t::addClass(LabelHighlight).withBaseClass(jcmp::Label::Styles::styleClass); }
} // namespace util
} // namespace detail

ThemeApplier::ThemeApplier()
{
    static bool detailInitialized{false};
    if (!detailInitialized)
    {
        detail::edit::init();
        detail::edit::zone::init();
        detail::edit::group::init();
        detail::header::init();
        detail::util::init();
        detail::mix::channelstrip::init();
        detail::mix::auxchannelstrip::init();
        detailInitialized = true;
    }
    colors = ColorMap::createColorMap(ColorMap::WIREFRAME);
}

void ThemeApplier::recolorStylesheet(const sst::jucegui::style::StyleSheet::ptr_t &s)
{
    detail::global::applyColors(s, *colors);
    detail::edit::applyColors(s, *colors);
    detail::edit::zone::applyColors(s, *colors);
    detail::edit::group::applyColors(s, *colors);
    detail::header::applyColorsAndFonts(s, *colors, *this);
    detail::mix::channelstrip::applyColorsAndFonts(s, *colors, *this);
    detail::mix::auxchannelstrip::applyColorsAndFonts(s, *colors, *this);
    detail::util::applyColors(s, *colors);
}

void ThemeApplier::recolorStylesheetWith(std::unique_ptr<ColorMap> &&c, const sheet_t::ptr_t &s)
{
    colors = std::move(c);
    recolorStylesheet(s);
}

void populateSharedGroupZoneMultiModulation(jstl::CustomTypeMap &map)
{
    map.addCustomClass<jcmp::JogUpDownButton>(detail::edit::ModulationJogButon);
    map.addCustomClass<jcmp::ToggleButton>(detail::edit::ModulationToggle);
    map.addCustomClass<jcmp::MenuButton>(detail::edit::ModulationMenu);
    map.addCustomClass<jcmp::MenuButtonDiscreteEditor>(detail::edit::ModulationMenu);
}
void ThemeApplier::applyZoneMultiScreenModulationTheme(juce::Component *toThis)
{
    jstl::CustomTypeMap map;
    map.addCustomClass<jcmp::NamedPanel>(detail::edit::zone::ModulationNamedPanel);
    map.addCustomClass<jcmp::MultiSwitch>(detail::edit::zone::ModulationMultiSwitch);
    map.addCustomClass<jcmp::VSlider>(detail::edit::zone::ModulationVSlider);
    map.addCustomClass<jcmp::Knob>(detail::edit::zone::ModulationKnob);
    map.addCustomClass<jcmp::HSliderFilled>(detail::edit::zone::ModulationHSliderFilled);
    map.addCustomClass<jcmp::DraggableTextEditableValue>(
        detail::edit::zone::ModulationDraggableTextEditableValue);
    populateSharedGroupZoneMultiModulation(map);
    map.applyMapTo(toThis);
}
void ThemeApplier::applyZoneMultiScreenTheme(juce::Component *toThis)
{
    jstl::CustomTypeMap map;
    map.addCustomClass<jcmp::NamedPanel>(jcmp::NamedPanel::Styles::styleClass);
    map.addCustomClass<jcmp::MultiSwitch>(jcmp::MultiSwitch::Styles::styleClass);
    map.applyMapTo(toThis);
}

void ThemeApplier::applyGroupMultiScreenModulationTheme(juce::Component *toThis)
{
    jstl::CustomTypeMap map;
    map.addCustomClass<jcmp::NamedPanel>(detail::edit::group::ModulationNamedPanel);
    map.addCustomClass<jcmp::MultiSwitch>(detail::edit::group::ModulationMultiSwitch);
    map.addCustomClass<jcmp::NamedPanel>(detail::edit::group::ModulationNamedPanel);
    map.addCustomClass<jcmp::VSlider>(detail::edit::group::ModulationVSlider);
    map.addCustomClass<jcmp::Knob>(detail::edit::group::ModulationKnob);
    map.addCustomClass<jcmp::DraggableTextEditableValue>(
        detail::edit::group::ModulationDraggableTextEditableValue);
    map.addCustomClass<jcmp::HSliderFilled>(detail::edit::group::ModulationHSliderFilled);
    populateSharedGroupZoneMultiModulation(map);
    map.applyMapTo(toThis);
}
void ThemeApplier::applyGroupMultiScreenTheme(juce::Component *toThis)
{
    jstl::CustomTypeMap map;
    map.addCustomClass<jcmp::MultiSwitch>(detail::edit::group::MultiSwitch);
    map.addCustomClass<jcmp::NamedPanel>(detail::edit::group::NamedPanel);
    map.addCustomClass<jcmp::Knob>(detail::edit::group::Knob);
    map.addCustomClass<jcmp::DraggableTextEditableValue>(
        detail::edit::group::DraggableTextEditableValue);
    map.applyMapTo(toThis);
}

void ThemeApplier::applyHeaderTheme(juce::Component *toThis)
{
    jstl::CustomTypeMap map;
    map.addCustomClass<jcmp::TextPushButton>(detail::header::TextPushButton);
    map.addCustomClass<jcmp::ToggleButton>(detail::header::ToggleButton);
    map.addCustomClass<jcmp::MenuButton>(detail::header::MenuButton);
    map.addCustomClass<jcmp::GlyphButton>(detail::header::GlyphButton);
    map.applyMapTo(toThis);
}

void ThemeApplier::setLabelToHighlight(sst::jucegui::style::StyleConsumer *s)
{
    s->setCustomClass(detail::util::LabelHighlight);
}

void ThemeApplier::applyHeaderSCButtonTheme(sst::jucegui::style::StyleConsumer *s)
{
    s->setCustomClass(detail::header::GlyphButtonAccent);
}

void ThemeApplier::applyChannelStripTheme(juce::Component *toThis)
{
    jstl::CustomTypeMap map;
    map.addCustomClass<jcmp::NamedPanel>(detail::mix::channelstrip::CSPanel);
    map.addCustomClass<jcmp::VUMeter>(detail::mix::channelstrip::VUMeter);
    map.applyMapTo(toThis);
}

void ThemeApplier::applyAuxChannelStripTheme(juce::Component *toThis)
{
    jstl::CustomTypeMap map;
    map.addCustomClass<jcmp::NamedPanel>(detail::mix::auxchannelstrip::AuxPanel);
    map.addCustomClass<jcmp::VUMeter>(detail::mix::auxchannelstrip::VUMeter);
    map.applyMapTo(toThis);
}

juce::Font ThemeApplier::interBoldFor(int ht) const
{
    static auto interMed = connectors::resources::loadTypeface("fonts/Inter/static/Inter-Bold.ttf");
    return juce::Font(interMed).withHeight(ht);
}

juce::Font ThemeApplier::interMediumFor(int ht) const
{
    static auto interMed =
        connectors::resources::loadTypeface("fonts/Inter/static/Inter-Medium.ttf");
    return juce::Font(interMed).withHeight(ht);
}

juce::Font ThemeApplier::interRegularFor(int ht) const
{
    static auto interMed =
        connectors::resources::loadTypeface("fonts/Inter/static/Inter-Regular.ttf");
    return juce::Font(interMed).withHeight(ht);
}

juce::Font ThemeApplier::interLightFor(int ht) const
{
    static auto interMed =
        connectors::resources::loadTypeface("fonts/Inter/static/Inter-Light.ttf");
    return juce::Font(interMed).withHeight(ht);
}

namespace detail
{
namespace global
{
void applyColors(const sheet_t::ptr_t &base, const ColorMap &cols)
{
    using namespace jcmp::base_styles;
    /*
     * Set up base classes
     */
    base->setColour(Base::styleClass, Base::background, cols.get(ColorMap::bg_2));
    base->setColour(Base::styleClass, Base::background_hover, cols.getHover(ColorMap::bg_2));

    base->setFont(BaseLabel::styleClass, BaseLabel::labelfont,
                  juce::Font(juce::Font::plain).withPointHeight(11));
    base->setColour(BaseLabel::styleClass, jcmp::NamedPanel::Styles::labelcolor,
                    cols.get(ColorMap::generic_content_medium));
    base->setColour(BaseLabel::styleClass, jcmp::NamedPanel::Styles::labelcolor_hover,
                    cols.getHover(ColorMap::generic_content_medium));

    base->setColour(Outlined::styleClass, Outlined::outline, cols.get(ColorMap::bg_1));
    base->setColour(Outlined::styleClass, Outlined::brightoutline,
                    cols.get(ColorMap::generic_content_low));

    base->setColour(PushButton::styleClass, PushButton::fill, cols.get(ColorMap::bg_1));
    base->setColour(PushButton::styleClass, PushButton::fill_hover, cols.getHover(ColorMap::bg_1));
    base->setColour(PushButton::styleClass, PushButton::fill_pressed,
                    cols.getHover(ColorMap::bg_1));

    base->setColour(ValueBearing::styleClass, ValueBearing::value, cols.get(ColorMap::accent_1b));
    base->setColour(ValueBearing::styleClass, ValueBearing::value_hover,
                    cols.getHover(ColorMap::accent_1b));
    base->setColour(ValueBearing::styleClass, ValueBearing::valuelabel,
                    cols.get(ColorMap::generic_content_medium));
    base->setColour(ValueBearing::styleClass, ValueBearing::valuelabel_hover,
                    cols.getHover(ColorMap::generic_content_medium));
    base->setColour(ValueBearing::styleClass, ValueBearing::valuebg,
                    cols.get(ColorMap::accent_1b_alpha_a));

    base->setColour(ValueGutter::styleClass, ValueGutter::gutter, cols.get(ColorMap::gutter_2));
    base->setColour(ValueGutter::styleClass, ValueGutter::gutter_hover,
                    cols.getHover(ColorMap::gutter_2));

    base->setColour(jcmp::Knob::Styles::styleClass, jcmp::Knob::Styles::knobbase,
                    cols.get(ColorMap::knob_fill));
    base->setColour(jcmp::Knob::Styles::styleClass, jcmp::Knob::Styles::handle,
                    cols.get(ColorMap::generic_content_medium));
    base->setColour(jcmp::Knob::Styles::styleClass, jcmp::Knob::Styles::handle_hover,
                    cols.getHover(ColorMap::generic_content_medium));

    base->setColour(jcmp::ScrollBar::Styles::styleClass, jcmp::ScrollBar::Styles::outline,
                    cols.get(ColorMap::generic_content_low));

    base->setColour(GraphicalHandle::styleClass, GraphicalHandle::handle,
                    cols.get(ColorMap::generic_content_high));
    base->setColour(GraphicalHandle::styleClass, GraphicalHandle::handle_hover,
                    cols.getHover(ColorMap::generic_content_high));
    base->setColour(GraphicalHandle::styleClass, GraphicalHandle::handle_outline,
                    cols.get(ColorMap::generic_content_high).brighter(0.1));

    base->setColour(jcmp::VUMeter::Styles::styleClass, jcmp::VUMeter::Styles::vu_gutter,
                    cols.get(ColorMap::gutter_2));
    base->setColour(jcmp::VUMeter::Styles::styleClass, jcmp::VUMeter::Styles::vu_gradstart,
                    cols.get(ColorMap::accent_1b));
    base->setColour(jcmp::VUMeter::Styles::styleClass, jcmp::VUMeter::Styles::vu_gradend,
                    cols.get(ColorMap::accent_1a));
    base->setColour(jcmp::VUMeter::Styles::styleClass, jcmp::VUMeter::Styles::vu_overload,
                    cols.get(ColorMap::warning_1a));

    base->setColour(jcmp::DraggableTextEditableValue::Styles::styleClass,
                    jcmp::DraggableTextEditableValue::Styles::background,
                    cols.get(ColorMap::accent_1b_alpha_a));
    base->setColour(jcmp::DraggableTextEditableValue::Styles::styleClass,
                    jcmp::DraggableTextEditableValue::Styles::background_hover,
                    cols.get(ColorMap::accent_1b_alpha_b));

    base->setColour(jcmp::MultiSwitch::Styles::styleClass, jcmp::MultiSwitch::Styles::background,
                    cols.get(ColorMap::bg_2));
    base->setColour(jcmp::MultiSwitch::Styles::styleClass,
                    jcmp::MultiSwitch::Styles::unselected_hover, cols.getHover(ColorMap::bg_2));

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
                    cols.get(ColorMap::generic_content_medium));
    auto npol = cols.get(ColorMap::panel_outline_2);
    if (npol.getAlpha() == 0)
        npol = cols.get(ColorMap::bg_2).brighter(0.05);
    base->setColour(jcmp::NamedPanel::Styles::styleClass, jcmp::NamedPanel::Styles::brightoutline,
                    npol);
    base->setColour(jcmp::NamedPanel::Styles::styleClass, jcmp::NamedPanel::Styles::selectedtab,
                    cols.get(ColorMap::accent_1a));
    base->setColour(jcmp::NamedPanel::Styles::styleClass, jcmp::NamedPanel::Styles::accentedPanel,
                    cols.get(ColorMap::accent_1b));

    base->setColour(jcmp::MenuButton::Styles::styleClass, jcmp::MenuButton::Styles::menuarrow_hover,
                    cols.get(ColorMap::accent_1a));

    base->setColour(jcmp::HSliderFilled::Styles::styleClass, jcmp::HSliderFilled::Styles::value,
                    cols.get(ColorMap::accent_1b));
    base->setColour(jcmp::HSliderFilled::Styles::styleClass,
                    jcmp::HSliderFilled::Styles::value_hover, cols.getHover(ColorMap::accent_1b));
    base->setColour(jcmp::HSliderFilled::Styles::styleClass, jcmp::HSliderFilled::Styles::handle,
                    cols.get(ColorMap::accent_1a));
    base->setColour(jcmp::HSliderFilled::Styles::styleClass,
                    jcmp::HSliderFilled::Styles::handle_hover, cols.getHover(ColorMap::accent_1a));

    // auto interMed = connectors::resources::loadTypeface("fonts/Inter/static/Inter-Medium.ttf");
    auto interReg = connectors::resources::loadTypeface("fonts/Inter/static/Inter-Regular.ttf");
    base->replaceFontsWithTypeface(interReg);

    auto fixedWidth =
        connectors::resources::loadTypeface("fonts/Anonymous_Pro/AnonymousPro-Regular.ttf");

    auto fw = juce::Font(fixedWidth).withHeight(11);
    base->setFont(sst::jucegui::components::ToolTip::Styles::styleClass,
                  sst::jucegui::components::ToolTip::Styles::datafont, fw);
}
} // namespace global
namespace edit
{
void applyColors(const sheet_t::ptr_t &base, const ColorMap &cols)
{
    base->setColour(ModulationJogButon, jcmp::JogUpDownButton::Styles::jogbutton_hover,
                    cols.get(ColorMap::accent_2a));

    base->setColour(ModulationMenu, jcmp::MenuButton::Styles::menuarrow_hover,
                    cols.get(ColorMap::accent_2a));

    base->setColour(ModulationToggle, jcmp::ToggleButton::Styles::value,
                    cols.get(ColorMap::accent_2a));
    base->setColour(ModulationToggle, jcmp::ToggleButton::Styles::value_hover,
                    cols.getHover(ColorMap::accent_2a));
}
namespace zone
{
void applyColors(const sheet_t::ptr_t &base, const ColorMap &cols)
{
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::value,
                    cols.get(ColorMap::accent_2a));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::valuebg,
                    cols.get(ColorMap::accent_2a_alpha_a));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::valuelabel,
                    cols.get(ColorMap::generic_content_high));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::value_hover,
                    cols.getHover(ColorMap::accent_2a));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::valuelabel_hover,
                    cols.getHover(ColorMap::generic_content_high));

    base->setColour(ModulationHSliderFilled, jcmp::HSliderFilled::Styles::value,
                    cols.get(ColorMap::accent_2b));
    base->setColour(ModulationHSliderFilled, jcmp::HSliderFilled::Styles::value_hover,
                    cols.getHover(ColorMap::accent_2b));
    base->setColour(ModulationHSliderFilled, jcmp::HSliderFilled::Styles::handle,
                    cols.get(ColorMap::accent_2a));
    base->setColour(ModulationHSliderFilled, jcmp::HSliderFilled::Styles::handle_hover,
                    cols.getHover(ColorMap::accent_2a));

    base->setColour(ModulationVSlider, jcmp::VSlider::Styles::value, cols.get(ColorMap::accent_2a));
    base->setColour(ModulationVSlider, jcmp::VSlider::Styles::value_hover,
                    cols.getHover(ColorMap::accent_2a));

    base->setColour(ModulationKnob, jcmp::Knob::Styles::value, cols.get(ColorMap::accent_2a));
    base->setColour(ModulationKnob, jcmp::Knob::Styles::value_hover,
                    cols.getHover(ColorMap::accent_2a));

    base->setColour(ModulationNamedPanel, jcmp::NamedPanel::Styles::selectedtab,
                    cols.get(ColorMap::accent_2a));

    base->setColour(ModulationDraggableTextEditableValue,
                    jcmp::DraggableTextEditableValue::Styles::value, cols.get(ColorMap::accent_2a));
    base->setColour(ModulationDraggableTextEditableValue,
                    jcmp::DraggableTextEditableValue::Styles::value_hover,
                    cols.getHover(ColorMap::accent_2a));
    base->setColour(ModulationDraggableTextEditableValue,
                    jcmp::DraggableTextEditableValue::Styles::background,
                    cols.get(ColorMap::accent_2a_alpha_a));
    base->setColour(ModulationDraggableTextEditableValue,
                    jcmp::DraggableTextEditableValue::Styles::background_hover,
                    cols.get(ColorMap::accent_2a_alpha_b));

    base->setColour(jcmp::MultiSwitch::Styles::styleClass, jcmp::MultiSwitch::Styles::valuebg,
                    cols.get(ColorMap::accent_1b_alpha_a));
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

    base->setColour(MultiSwitch, jcmp::MultiSwitch::Styles::background,
                    cols.get(ColorMap::gutter_3));
    base->setColour(MultiSwitch, jcmp::MultiSwitch::Styles::unselected_hover,
                    cols.getHover(ColorMap::gutter_3));
    base->setColour(MultiSwitch, jcmp::MultiSwitch::Styles::valuebg, cols.get(ColorMap::bg_1));

    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::value,
                    cols.get(ColorMap::accent_2a));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::valuebg,
                    cols.get(ColorMap::accent_2a_alpha_a));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::valuelabel,
                    cols.get(ColorMap::generic_content_high));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::value_hover,
                    cols.getHover(ColorMap::accent_2a));
    base->setColour(ModulationMultiSwitch, jcmp::MultiSwitch::Styles::valuelabel_hover,
                    cols.getHover(ColorMap::generic_content_high));

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
                    cols.getHover(ColorMap::accent_2a));

    base->setColour(ModulationHSliderFilled, jcmp::HSliderFilled::Styles::value,
                    cols.get(ColorMap::accent_2b));
    base->setColour(ModulationHSliderFilled, jcmp::HSliderFilled::Styles::value_hover,
                    cols.getHover(ColorMap::accent_2b));
    base->setColour(ModulationHSliderFilled, jcmp::HSliderFilled::Styles::handle,
                    cols.get(ColorMap::accent_2a));
    base->setColour(ModulationHSliderFilled, jcmp::HSliderFilled::Styles::handle_hover,
                    cols.getHover(ColorMap::accent_2a));
    base->setColour(ModulationHSliderFilled, jcmp::HSliderFilled::Styles::gutter,
                    cols.get(ColorMap::bg_2));
    base->setColour(ModulationHSliderFilled, jcmp::HSliderFilled::Styles::gutter_hover,
                    cols.getHover(ColorMap::bg_2));

    base->setColour(ModulationKnob, jcmp::Knob::Styles::value, cols.get(ColorMap::accent_2a));
    base->setColour(ModulationKnob, jcmp::Knob::Styles::value_hover,
                    cols.getHover(ColorMap::accent_2a));

    base->setColour(ModulationDraggableTextEditableValue,
                    jcmp::DraggableTextEditableValue::Styles::value, cols.get(ColorMap::accent_2a));
    base->setColour(ModulationDraggableTextEditableValue,
                    jcmp::DraggableTextEditableValue::Styles::value_hover,
                    cols.getHover(ColorMap::accent_2a));
    base->setColour(ModulationDraggableTextEditableValue,
                    jcmp::DraggableTextEditableValue::Styles::background,
                    cols.get(ColorMap::accent_2a_alpha_a));
    base->setColour(ModulationDraggableTextEditableValue,
                    jcmp::DraggableTextEditableValue::Styles::background_hover,
                    cols.get(ColorMap::accent_2a_alpha_b));
}
} // namespace group
} // namespace edit
namespace header
{
void applyColorsAndFonts(const sheet_t::ptr_t &base, const ColorMap &cols, const ThemeApplier &t)
{
    base->setColour(TextPushButton, jcmp::TextPushButton::Styles::fill, cols.get(ColorMap::bg_2));
    base->setColour(TextPushButton, jcmp::TextPushButton::Styles::fill_hover,
                    cols.get(ColorMap::bg_3));
    base->setFont(TextPushButton, jcmp::TextPushButton::Styles::labelfont, t.interMediumFor(14));
    base->setColour(ToggleButton, jcmp::ToggleButton::Styles::fill, cols.get(ColorMap::bg_2));
    base->setColour(ToggleButton, jcmp::ToggleButton::Styles::fill_hover,
                    cols.getHover(ColorMap::bg_2));
    base->setFont(ToggleButton, jcmp::ToggleButton::Styles::labelfont, t.interMediumFor(14));
    base->setColour(MenuButton, jcmp::MenuButton::Styles::fill, cols.get(ColorMap::bg_2));
    base->setFont(MenuButton, jcmp::MenuButton::Styles::labelfont, t.interMediumFor(14));
    base->setColour(GlyphButton, jcmp::GlyphButton::Styles::fill, cols.get(ColorMap::bg_2));
    base->setColour(GlyphButton, jcmp::GlyphButton::Styles::fill_hover, cols.get(ColorMap::bg_3));

    base->setColour(GlyphButtonAccent, jcmp::GlyphButton::Styles::labelcolor,
                    cols.get(ColorMap::accent_1a));
    base->setColour(GlyphButtonAccent, jcmp::GlyphButton::Styles::labelcolor_hover,
                    cols.get(ColorMap::accent_1a));
}
} // namespace header

namespace mix
{
namespace channelstrip
{
void applyColorsAndFonts(const sheet_t::ptr_t &base, const ColorMap &cols, const ThemeApplier &t)
{
    base->setColour(VUMeter, jcmp::VUMeter::Styles::vu_gutter, cols.get(ColorMap::bg_1));
}
} // namespace channelstrip
namespace auxchannelstrip
{
void applyColorsAndFonts(const sheet_t::ptr_t &base, const ColorMap &cols, const ThemeApplier &t)
{
    base->setColour(VUMeter, jcmp::VUMeter::Styles::vu_gutter, cols.get(ColorMap::bg_1));
}
} // namespace auxchannelstrip
} // namespace mix
namespace util
{
void applyColors(const sheet_t::ptr_t &base, const ColorMap &cols)
{
    base->setColour(LabelHighlight, jcmp::Label::Styles::labelcolor,
                    cols.get(ColorMap::generic_content_highest));
}
} // namespace util
} // namespace detail
} // namespace scxt::ui::theme