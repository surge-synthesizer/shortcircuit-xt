//
// Created by Paul Walker on 10/22/25.
//

#ifndef SHORTCIRCUITXT_JSONLAYOUTENGINESUPPORT_H
#define SHORTCIRCUITXT_JSONLAYOUTENGINESUPPORT_H

#include <string>
#include "utils.h"
#include <sst/jucegui/components/ContinuousParamEditor.h>
#include <sst/jucegui/components/DiscreteParamEditor.h>
#include <sst/jucegui/components/Knob.h>
#include <sst/jucegui/components/Label.h>
#include <sst/jucegui/components/ToggleButton.h>
#include <sst/jucegui/components/MultiSwitch.h>
#include <sst/jucegui/components/JogUpDownButton.h>
#include <sst/jucegui/layouts/JsonLayoutEngine.h>
#include <concepts>

namespace scxt::ui::connectors::jsonlayout
{
std::string resolveJsonFile(const std::string &path);

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
    if (ctrl.enabledIf->type != "int")
    {
        onError("Enabled If index " + std::to_string(iidx) + " is not an int");
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
    if (ctrl.label.has_value() && cls.allowsLabel)
    {
        auto lab = std::make_unique<jcmp::Label>();
        // Always position below for now
        SCLOG_ONCE("Assuming labels always below controls");
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
    if (cls.controlType == "knob")
    {
        auto kb = std::make_unique<sst::jucegui::components::Knob>();
        kb->setDrawLabel(false);
        return kb;
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
            kb->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
            kb->setGlyph(jcmp::GlyphPainter::SMALL_POWER_LIGHT);

            if (cls.extraKVs.at("glyph") != "small-power-light")
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
