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

#include "RoutingPane.h"
#include "ProcessorPane.h"
#include "app/SCXTEditor.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/HSliderFilled.h"
#include "sst/jucegui/components/MenuButton.h"
#include "connectors/PayloadDataAttachment.h"
#include "datamodel/metadata.h"
#include "app/edit-screen/EditScreen.h"
#include "connectors/JsonLayoutEngineSupport.h"

namespace scxt::ui::app::edit_screen
{

namespace jcmp = sst::jucegui::components;
namespace cmsg = scxt::messaging::client;

engine::Zone::ZoneOutputInfo &RoutingPaneZoneTraits::outputInfo(SCXTEditor *e)
{
    return e->editorDataCache.zoneOutputInfo;
}

engine::Group::GroupOutputInfo &RoutingPaneGroupTraits::outputInfo(SCXTEditor *e)
{
    return e->editorDataCache.groupOutputInfo;
}

template <typename RPTraits>
struct RoutingPaneContents : juce::Component, HasEditor, sst::jucegui::layouts::JsonLayoutHost
{
    typedef connectors::DiscretePayloadDataAttachment<typename RPTraits::info_t,
                                                      typename RPTraits::route_t>
        int_attachment_t;

    RoutingPane<RPTraits> *outputPane{nullptr};
    std::unique_ptr<jcmp::TextPushButton> consistentButton;
    std::unique_ptr<jcmp::Label> consistentLabel;

    std::unique_ptr<int_attachment_t> routingA;
    std::unique_ptr<jcmp::MultiSwitch> multiW;

    typedef connectors::PayloadDataAttachment<typename RPTraits::info_t> attachment_t;
    typedef connectors::BooleanPayloadDataAttachment<typename RPTraits::info_t> bool_attachment_t;

    std::unique_ptr<attachment_t> outputAttachment, panAttachment, velocitySensitivityAttachment;
    std::unique_ptr<jcmp::MenuButton> outputRouting;
    std::unique_ptr<jcmp::Knob> outputKnob, panKnob;
    std::unique_ptr<jcmp::HSliderFilled> velocitySensitivitySlider;
    std::unique_ptr<jcmp::Label> velocitySensitivityLabel;

    std::unique_ptr<bool_attachment_t> oversampleAttachment;
    std::unique_ptr<jcmp::ToggleButton> oversampleButton;

    std::unique_ptr<juce::Component> routingLayoutComp;
    struct RC : juce::Component
    {
    };

    RoutingPaneContents(SCXTEditor *e, RoutingPane<RPTraits> *pane)
        : HasEditor(e), outputPane(pane), info(RPTraits::outputInfo(e))
    {
        using fac = connectors::SingleValueFactory<int_attachment_t, typename RPTraits::int16Msg_t>;
        fac::attachAndAdd(info, info.procRouting, this, routingA, multiW);
        multiW->direction = jcmp::MultiSwitch::Direction::HORIZONTAL;
        addAndMakeVisible(*multiW);
        connectors::addGuiStep(*routingA, [this](auto &a) { updateProcRoutingFromInfo(); });

        routingLayoutComp = std::make_unique<RC>();
        addAndMakeVisible(*routingLayoutComp);

        consistentLabel = std::make_unique<jcmp::Label>();
        consistentLabel->setText("Multiple Routings Selected");
        addChildComponent(*consistentLabel);
        consistentButton = std::make_unique<jcmp::TextPushButton>();
        consistentButton->setLabel("Make Consistent");
        consistentButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
            {
                w->template sendSingleToSerialization<typename RPTraits::int16RefreshMsg_t>(
                    w->info, w->info.procRouting);
            }
        });
        addChildComponent(*consistentButton);

        using ofac = connectors::SingleValueFactory<attachment_t, typename RPTraits::floatMsg_t>;

        ofac::attachAndAdd(info, info.amplitude, this, outputAttachment, outputKnob);
        ofac::attachAndAdd(info, info.pan, this, panAttachment, panKnob);

        if constexpr (!RPTraits::forZone)
        {
            ofac::attachAndAdd(info, info.velocitySensitivity, this, velocitySensitivityAttachment,
                               velocitySensitivitySlider);
            velocitySensitivityLabel = std::make_unique<jcmp::Label>();
            velocitySensitivityLabel->setText("Vel");
            addAndMakeVisible(*velocitySensitivityLabel);
            using bfac = connectors::BooleanSingleValueFactory<
                bool_attachment_t, scxt::messaging::client::UpdateGroupOutputBoolValue>;
            bfac::attachAndAdd(info, info.oversample, this, oversampleAttachment, oversampleButton);
            oversampleButton->setLabel("o/s");
        }

        outputRouting = std::make_unique<jcmp::MenuButton>();
        outputRouting->setLabel(RPTraits::defaultRoutingLocationName);
        updateRoutingLabel();
        outputRouting->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->selectNewRouting();
        });
        addAndMakeVisible(*outputRouting);

        svgPaths = std::make_unique<SvgPaths>(this);
        addAndMakeVisible(*svgPaths);
        svgPaths->toBack();
    }

    std::string resolveJsonPath(const std::string &path) const override
    {
        auto res = scxt::ui::connectors::jsonlayout::resolveJsonFile(path);
        if (res.has_value())
            return *res;
        editor->displayError("JSON Path Error", "Could not resolve path '" + path + "'");
        return {};
    }

    std::string getRoutingLabel(scxt::engine::BusAddress rt)
    {
        return engine::getBusAddressLabel(rt, RPTraits::defaultRoutingLocationName);
    }

    void selectNewRouting()
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Zone Routing");
        p.addSeparator();
        for (int rt = scxt::engine::BusAddress::DEFAULT_BUS;
             rt < scxt::engine::BusAddress::AUX_0 + numAux; ++rt)
        {
            p.addItem(getRoutingLabel((scxt::engine::BusAddress)rt),
                      [w = juce::Component::SafePointer(this), rt]() {
                          if (w)
                          {
                              w->info.routeTo = (scxt::engine::BusAddress)rt;
                              w->template sendSingleToSerialization<typename RPTraits::int16Msg_t>(
                                  w->info, w->info.routeTo);
                              w->updateRoutingLabel();
                          }
                      });
        }
        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }

    void createBindAndPosition(const sst::jucegui::layouts::json_document::Control &ctrl,
                               const sst::jucegui::layouts::json_document::Class &cls) override
    {
        auto onError = [this](const std::string &msg) {
            editor->displayError("JSON Proc Routing Error", msg);
        };
        if (ctrl.binding.has_value() && ctrl.binding->type == "float")
        {
            auto i = ctrl.binding->index;
            if (i < 0 || i >= scxt::processorsPerZoneAndGroup)
            {
                onError("Binding index " + std::to_string(i) + " out of range for " + ctrl.name);
                return;
            }
            auto w = outputPane->procWeakRefs[i];
            if (!w)
            {
                SCLOG("Software Error - why is there no w here?");
                return;
            }

            auto at = std::make_unique<ProcessorPane::attachment_t>(
                datamodel::describeValue(w->processorView, w->processorView.outputCubAmp),
                w->processorView.outputCubAmp);
            connectors::configureUpdater<cmsg::UpdateZoneOrGroupProcessorFloatValue,
                                         ProcessorPane::attachment_t>(*at, w->processorView, w,
                                                                      RPTraits::forZone, i);
            levelA[i] = std::move(at);

            auto ed = connectors::jsonlayout::createContinuousWidget(ctrl, cls);
            if (!ed && cls.controlType == "routing-box")
            {
                auto rb = std::make_unique<RoutingBox>(i, editor, this);
                if (ctrl.label.has_value())
                    rb->setBoxLabel(*ctrl.label);
                ed = std::move(rb);
            }
            if (!ed)
            {
                onError("Could not create widget for " + ctrl.name + " with unknown control type " +
                        cls.controlType);
                return;
            }

            connectors::jsonlayout::attachAndPosition(this, ed, levelA[i], ctrl);
            routingLayoutComp->addAndMakeVisible(*ed);
            auto isNone = w->processorView.type == dsp::processor::proct_none;
            auto isOn = w->processorView.isActive;
            ed->setEnabled(isOn && !isNone);

            jsonComponents.push_back(std::move(ed));

            if (outputPane->style() && cls.controlType != "routing-box")
            {
                if (auto lab = connectors::jsonlayout::createControlLabel(ctrl, cls, *outputPane))
                {
                    routingLayoutComp->addAndMakeVisible(*lab);
                    jsonComponents.push_back(std::move(lab));
                }
            }
        }
        else if (auto nw =
                     connectors::jsonlayout::createAndPositionNonDataWidget(ctrl, cls, onError))
        {
            routingLayoutComp->addAndMakeVisible(*nw);
            jsonComponents.push_back(std::move(nw));
        }
        else
        {
            onError("Unknown control type " + cls.controlType + " for " + ctrl.name);
        }
    }

    void resized() override
    {
        multiW->setBounds(getLocalBounds().reduced(0, 2).withHeight(22).withTrimmedRight(140));
        svgPaths->setBounds(getLocalBounds().reduced(0, 2).withTrimmedRight(140));

        consistentLabel->setBounds(multiW->getBounds());
        consistentButton->setBounds(multiW->getBounds().translated(0, 30));
        routingLayoutComp->setBounds(getLocalBounds().withTrimmedTop(25));

        auto rside = getLocalBounds().reduced(0, 2).withTrimmedLeft(getWidth() - 134);

        auto outb = rside.withHeight(22);
        outputRouting->setBounds(outb.withHeight(22));
        rside = rside.withTrimmedTop(30);

        auto ok = rside.withWidth(55).withHeight(75).translated(8, 0);
        outputKnob->setBounds(ok);
        ok = ok.translated(65, 0);
        panKnob->setBounds(ok);

        auto nb = rside.withTrimmedTop(75);
        if constexpr (!RPTraits::forZone)
        {
            nb = nb.withHeight(22);
            velocitySensitivityLabel->setBounds(nb.withWidth(25));
            velocitySensitivityLabel->setText("vel");
            velocitySensitivitySlider->setBounds(
                nb.withTrimmedLeft(28).withWidth(60).reduced(0, 3));
            oversampleButton->setBounds(nb.withTrimmedLeft(90).withWidth(45));
        }
    }

    std::string getRoutingName(typename RPTraits::route_t r)
    {
        auto zn = std::string();
        if constexpr (RPTraits::forZone)
            zn = scxt::engine::Zone::getProcRoutingPathDisplayName(r);
        else
            zn = scxt::engine::Group::getProcRoutingPathDisplayName(r);
        return zn;
    }
    void updateProcRoutingFromInfo()
    {
        auto c = info.procRoutingConsistent;
        multiW->setVisible(c);
        routingLayoutComp->setVisible(c);
        consistentButton->setVisible(!c);
        consistentLabel->setVisible(!c);

        if (!c)
        {
            consistentButton->setLabel("Set to " + getRoutingName(info.procRouting));
        }
        else
        {
            updateFromProcessorPanes();
        }
    }

    static constexpr int nOuts{scxt::processorsPerZoneAndGroup};
    std::array<std::unique_ptr<ProcessorPane::attachment_t>, nOuts> levelA;
    std::vector<std::unique_ptr<juce::Component>> jsonComponents;

    struct RoutingBox : sst::jucegui::components::Knob, HasEditor
    {

        RoutingBox(int idx, SCXTEditor *e, RoutingPaneContents *par)
            : index(idx), HasEditor(e), parent(par)
        {
        }
        void paint(juce::Graphics &g) override
        {
            juce::Colour boxOutline, boxFill, labelC, valueC, gutterC;
            gutterC = juce::Colours::black.withAlpha(0.0f);
            if (isEnabled())
            {
                boxOutline = editor->themeColor(theme::ColorMap::generic_content_medium);
                boxFill = juce::Colours::black.withAlpha(0.f);
                labelC = editor->themeColor(theme::ColorMap::generic_content_high);
                if (isHovered)
                    labelC = editor->themeColor(theme::ColorMap::generic_content_highest);
                valueC = editor->themeColor(theme::ColorMap::accent_1b);
                gutterC = style()->getColour(
                    Styles::styleClass, sst::jucegui::components::base_styles::ValueGutter::gutter);
                if (isHovered)
                    gutterC = style()->getColour(
                        Styles::styleClass,
                        sst::jucegui::components::base_styles::ValueGutter::gutter_hover);
            }
            else
            {
                boxOutline =
                    editor->themeColor(theme::ColorMap::generic_content_medium).withAlpha(0.5f);
                boxFill = editor->themeColor(theme::ColorMap::generic_content_lowest);
                labelC =
                    editor->themeColor(theme::ColorMap::generic_content_medium).withAlpha(0.5f);
                valueC = editor->themeColor(theme::ColorMap::accent_1b).withAlpha(0.5f);
            }
            auto bx = getLocalBounds().reduced(4);
            auto lbx = bx.withTrimmedRight(7);
            auto sbx = bx.withTrimmedLeft(bx.getWidth() - 6);

            g.setColour(boxFill);
            g.fillRoundedRectangle(bx.toFloat(), 2);

            {
                juce::Graphics::ScopedSaveState gs(g);
                auto dbx = sbx.withTrimmedLeft(-3);
                g.reduceClipRegion(sbx);
                g.setColour(gutterC);
                g.fillRoundedRectangle(dbx.toFloat(), 2);
                auto v = continuous()->getValue01();
                g.reduceClipRegion(sbx.withTrimmedTop(sbx.getHeight() * (1 - v)));
                g.setColour(valueC);
                g.fillRoundedRectangle(dbx.toFloat(), 2);
            }

            g.setColour(boxOutline);
            g.drawRoundedRectangle(bx.toFloat(), 2, 1);
            g.drawVerticalLine(sbx.getX(), sbx.getY(), sbx.getBottom());
            // g.drawRoundedRectangle(sbx.toFloat(), 2, 1);

            auto cy = bx.getCentreY();
            g.fillRect(0, cy - 2, 4, 5);
            g.fillRect(getWidth() - 4, cy - 2, 4, 5);

            g.setColour(labelC);
            g.setFont(editor->themeApplier.interMediumFor(11));
            g.drawText(boxLabel, lbx, juce::Justification::centred);
        }

        void mouseDoubleClick(const juce::MouseEvent &e) override
        {
            auto bx = getLocalBounds().reduced(4);
            auto lbx = bx.withTrimmedRight(7);

            if (lbx.contains(e.getPosition()))
            {
                auto w = parent->outputPane->procWeakRefs[index];
                if (w)
                {
                    w->toggleBypass();
                }
            }
            else
            {
                sst::jucegui::components::Knob::mouseDoubleClick(e);
            }
        }
        void setBoxLabel(const std::string &s)
        {
            boxLabel = s;
            repaint();
        }
        int index;
        std::string boxLabel;
        RoutingPaneContents *parent{nullptr};
    };

    struct SvgPaths : juce::Component
    {
        SvgPaths(RoutingPaneContents *pt) : owner(pt) {}
        void paint(juce::Graphics &g)
        {
            if (drawable)
                drawable->drawAt(g, 0, 0, 1.0);
        }

        std::unique_ptr<juce::Drawable> drawable;
        void setSvg(const std::string &s)
        {
            drawable.reset();
            if (!s.empty())
            {
                std::unique_ptr<juce::XmlElement> svgXml = juce::XmlDocument::parse(s);
                if (svgXml)
                {
                    drawable = juce::Drawable::createFromSVG(*svgXml);
                    if (drawable)
                    {
                        drawable->replaceColour(
                            juce::Colour(0x77, 0x77, 0x77),
                            owner->editor->themeColor(theme::ColorMap::generic_content_low));
                    }
                }
            }
        }
        RoutingPaneContents *owner{nullptr};
    };
    std::unique_ptr<SvgPaths> svgPaths;

    void updateFromProcessorPanes()
    {
        routingLayoutComp->removeAllChildren();
        jsonComponents.clear();
        sst::jucegui::layouts::JsonLayoutEngine engine(*this);
        std::string path = "procrouting-layouts/linear.json";
        switch (info.procRouting)
        {
        case RPTraits::route_t::procRoute_linear:
            path = "procrouting-layouts/linear.json";
            break;

        case RPTraits::route_t::procRoute_ser2:
            path = "procrouting-layouts/ser2.json";
            break;

        case RPTraits::route_t::procRoute_ser3:
            path = "procrouting-layouts/ser3.json";
            break;

        case RPTraits::route_t::procRoute_par1:
            path = "procrouting-layouts/par1.json";
            break;

        case RPTraits::route_t::procRoute_par2:
            path = "procrouting-layouts/par2.json";
            break;

        case RPTraits::route_t::procRoute_par3:
            path = "procrouting-layouts/par3.json";
            break;

        case RPTraits::route_t::procRoute_bypass:
            path = "procrouting-layouts/byp.json";
            break;
        }
        auto res = scxt::ui::connectors::jsonlayout::resolveJsonFile(path);
        if (!res.has_value())
            path = "procrouting-layouts/linear.json";

        auto parseRes = engine.processJsonPath(path);
        if (!parseRes)
        {
            editor->displayError("JSON Path Error", *parseRes.error + "\nWhile parsing " + path);
            return;
        }

        auto fn = path.find(".json");
        if (fn == std::string::npos)
            return;
        path = path.substr(0, fn) + "_path.svg";
        auto svg = scxt::ui::connectors::jsonlayout::resolveJsonFile(path);
        if (svg.has_value())
        {
            svgPaths->setSvg(*svg);
        }
        else
        {
            svgPaths->setSvg("");
        }
    }

    void updateRoutingLabel()
    {
        auto rt = info.routeTo;
        outputRouting->setLabel(getRoutingLabel(rt));
        outputRouting->repaint();
    }

    typename RPTraits::info_t &info;
};

template <typename RPTraits>
RoutingPane<RPTraits>::RoutingPane(SCXTEditor *e) : jcmp::NamedPanel("ROUTING"), HasEditor(e)
{
    hasHamburger = false;
    contents = std::make_unique<RoutingPaneContents<RPTraits>>(editor, this);
    addAndMakeVisible(*contents);
}

template <typename RPTraits> RoutingPane<RPTraits>::~RoutingPane() {}

template <typename RPTraits> void RoutingPane<RPTraits>::resized()
{
    contents->setBounds(getContentArea());
}

template <typename RPTraits> void RoutingPane<RPTraits>::setActive(bool b) { active = b; }

template <typename RPTraits> void RoutingPane<RPTraits>::updateFromOutputInfo()
{
    contents->updateProcRoutingFromInfo();
    contents->repaint();
}

template <typename RPTraits> void RoutingPane<RPTraits>::updateFromProcessorPanes()
{
    contents->updateFromProcessorPanes();
}

template struct RoutingPane<RoutingPaneGroupTraits>;
template struct RoutingPane<RoutingPaneZoneTraits>;
} // namespace scxt::ui::app::edit_screen
