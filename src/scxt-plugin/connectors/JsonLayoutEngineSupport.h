/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_PLUGIN_CONNECTORS_JSONLAYOUTENGINESUPPORT_H
#define SCXT_SRC_SCXT_PLUGIN_CONNECTORS_JSONLAYOUTENGINESUPPORT_H

#include <string>
#include "utils.h"
#include "configuration.h"
#include <sst/jucegui/components/ContinuousParamEditor.h>
#include <sst/jucegui/components/DiscreteParamEditor.h>
#include <sst/jucegui/components/Knob.h>
#include <sst/jucegui/components/HSlider.h>
#include <sst/jucegui/components/VSlider.h>
#include <sst/jucegui/components/Label.h>
#include <sst/jucegui/components/ToggleButton.h>
#include <sst/jucegui/components/MultiSwitch.h>
#include <sst/jucegui/components/JogUpDownButton.h>
#include <sst/jucegui/components/RuledLabel.h>
#include <sst/jucegui/components/LineSegment.h>
#include <sst/jucegui/layouts/JsonLayoutEngine.h>
#include <concepts>
#include <optional>

namespace scxt::ui::connectors::jsonlayout
{
std::optional<std::string> resolveJsonFile(const std::string &path);

inline bool isVisible(const sst::jucegui::layouts::json_document::Control &ctrl,
                      std::function<int(int)> indexValue, std::function<void(std::string)> onError)
{
    if (!ctrl.visibleIf.has_value())
    {
        return true;
    }
    auto iidx = ctrl.visibleIf->index;
    if (ctrl.visibleIf->type != "int")
    {
        onError("Enabled If index " + std::to_string(iidx) + " is not an int");
        return true;
    }
    auto iav = indexValue(iidx);
    bool vis = false;
    for (auto &val : ctrl.visibleIf->values)
    {
        if (val == iav)
            vis = true;
    }
    return vis;
}

inline bool isEnabled(const sst::jucegui::layouts::json_document::Control &ctrl,
                      std::function<int(int)> indexValue, std::function<void(std::string)> onError)
{
    if (!ctrl.enabledIf.has_value())
    {
        return true;
    }
    auto iidx = ctrl.enabledIf->index;
    if (ctrl.enabledIf->type != "int" && ctrl.enabledIf->type != "float-activation")
    {
        onError("Enabled If index " + std::to_string(iidx) + " is not an int or float activation");
        return true;
    }
    auto iav = indexValue(iidx);
    bool vis = false;
    for (auto &val : ctrl.enabledIf->values)
    {
        if (val == iav)
            vis = true;
    }
    return vis;
}

inline std::unique_ptr<sst::jucegui::components::Label>
createControlLabel(const sst::jucegui::layouts::json_document::Control &ctrl,
                   const sst::jucegui::layouts::json_document::Class &cls,
                   sst::jucegui::style::StyleConsumer &sc, juce::Point<int> zeroPoint = {0, 0})
{
    namespace jcmp = sst::jucegui::components;
    if (ctrl.label.has_value())
    {
        auto lab = std::make_unique<jcmp::Label>();
        // Always position below for now
        SCLOG_ONCE_IF(debug || scxt::log::jsonUI, "Assuming labels always below controls");
        auto ft =
            sc.style()->getFont(jcmp::Label::Styles::styleClass, jcmp::Label::Styles::labelfont);
        auto wid = SST_STRING_WIDTH_FLOAT(ft, *ctrl.label);
        wid = wid + 5;
        auto bx = juce::Rectangle<int>(ctrl.position.x + zeroPoint.x + ctrl.position.w / 2,
                                       ctrl.position.y + zeroPoint.y + ctrl.position.h, 0,
                                       ft.getHeight() + 3);
        bx = bx.expanded(wid / 2, 0);
        lab->setBounds(bx);
        lab->setText(*ctrl.label);
        lab->setJustification(juce::Justification::centred);
        return lab;
    }
    return nullptr;
}

inline std::unique_ptr<sst::jucegui::components::ContinuousParamEditor>
createContinuousWidget(const sst::jucegui::layouts::json_document::Control &ctrl,
                       const sst::jucegui::layouts::json_document::Class &cls)
{
    namespace jcmp = sst::jucegui::components;
    if (cls.controlType == "knob")
    {
        auto kb = std::make_unique<jcmp::Knob>();
        kb->setDrawLabel(false);
        return kb;
    }
    if (cls.controlType == "hslider")
    {
        auto sl = std::make_unique<jcmp::HSlider>();
        sl->setShowLabel(false);
        return sl;
    }
    if (cls.controlType == "vslider")
    {
        auto sl = std::make_unique<jcmp::VSlider>();
        return sl;
    }

    return nullptr;
}

inline std::unique_ptr<sst::jucegui::components::DiscreteParamEditor>
createDiscreteWidget(const sst::jucegui::layouts::json_document::Control &ctrl,
                     const sst::jucegui::layouts::json_document::Class &cls,
                     std::function<void(std::string)> onError)
{

    namespace jcmp = sst::jucegui::components;
    if (cls.controlType == "toggle")
    {
        auto kb = std::make_unique<jcmp::ToggleButton>();

        std::string mode = "toggle";
        if (cls.extraKVs.find("toggle-mode") != cls.extraKVs.end())
        {
            mode = cls.extraKVs.at("toggle-mode");
        }
        if (mode == "toggle")
        {
            // default
        }
        else if (mode == "glyph")
        {
            if (cls.extraKVs.find("glyph") == cls.extraKVs.end())
            {
                onError("Missing glyph for toggle");
                return nullptr;
            }

            if (cls.extraKVs.at("glyph") == "small-power-light")
            {
                kb->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
                kb->setGlyph(jcmp::GlyphPainter::SMALL_POWER_LIGHT);
            }
            else if (cls.extraKVs.at("glyph") == "plus-minus")
            {
                kb->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
                kb->setGlyph(jcmp::GlyphPainter::MINUS);
                kb->setOffGlyph(jcmp::GlyphPainter::PLUS);
            }
            else
            {
                onError("Unimplemented glyph: " + cls.extraKVs.at("glyph"));
                // but keep going for now
            }
        }
        return kb;
    }
    else if (cls.controlType == "multiswitch")
    {
        auto ms = std::make_unique<jcmp::MultiSwitch>();
        std::string ori = "vertical";
        if (cls.extraKVs.find("orientation") != cls.extraKVs.end())
        {
            ori = cls.extraKVs.at("orientation");
        }
        if (ori == "vertical")
        {
            ms->direction = jcmp::MultiSwitch::Direction::VERTICAL;
        }
        else
        {
            ms->direction = jcmp::MultiSwitch::Direction::HORIZONTAL;
        }
        return ms;
    }
    else if (cls.controlType == "jog-updown")
    {
        return std::make_unique<jcmp::JogUpDownButton>();
    }
    return nullptr;
}

inline std::unique_ptr<juce::Component>
createAndPositionNonDataWidget(const sst::jucegui::layouts::json_document::Control &ctrl,
                               const sst::jucegui::layouts::json_document::Class &cls,
                               std::function<void(std::string)> onError,
                               juce::Point<int> zeroPoint = {0, 0})
{
    namespace jcmp = sst::jucegui::components;
    if (cls.controlType == "label")
    {
        if (!ctrl.label.has_value())
        {
            onError("Label has no 'label' member " + ctrl.name);
            return nullptr;
        }
        auto lab = std::make_unique<jcmp::Label>();
        lab->setText(*ctrl.label);
        lab->setJustification(juce::Justification::centred);
        lab->setBounds(ctrl.position.x + zeroPoint.x, ctrl.position.y + zeroPoint.y,
                       ctrl.position.w, ctrl.position.h);
        return lab;
    }
    if (cls.controlType == "ruled-label")
    {
        if (!ctrl.label.has_value())
        {
            onError("Label has no 'label' member " + ctrl.name);
            return nullptr;
        }
        auto lab = std::make_unique<jcmp::RuledLabel>();
        lab->setText(*ctrl.label);
        lab->setBounds(ctrl.position.x + zeroPoint.x, ctrl.position.y + zeroPoint.y,
                       ctrl.position.w, ctrl.position.h);
        return lab;
    }
    if (cls.controlType == "line-segment")
    {
        if (!ctrl.lineSegment.has_value())
        {
            onError("Line segment has no 'line-segment' member " + ctrl.name);
            return nullptr;
        }
        auto lab = std::make_unique<jcmp::LineSegment>();
        lab->setEndpointsAndBounds(
            ctrl.lineSegment->x0 + zeroPoint.x, ctrl.lineSegment->y0 + zeroPoint.y,
            ctrl.lineSegment->x1 + zeroPoint.x, ctrl.lineSegment->y1 + zeroPoint.y);
        return lab;
    }
    return nullptr;
}

template <typename P, typename W, typename A>
void attachAndPosition(P *parent, const std::unique_ptr<W> &ed, const std::unique_ptr<A> &att,
                       const sst::jucegui::layouts::json_document::Control &ctrl,
                       juce::Point<int> zeroPoint = {0, 0})
{
    if (!ctrl.binding.has_value())
        return;
    if (ctrl.binding->type == "float")
    {
        parent->setupWidgetForValueTooltip(ed.get(), att);
    }
    ed->setSource(att.get());
    ed->setBounds(ctrl.position.x + zeroPoint.x, ctrl.position.y + zeroPoint.y, ctrl.position.w,
                  ctrl.position.h);
    ed->setTitle(att->getLabel());
    ed->setDescription(att->getLabel());
}

} // namespace scxt::ui::connectors::jsonlayout
#endif // SHORTCIRCUITXT_JSONLAYOUTENGINESUPPORT_H
