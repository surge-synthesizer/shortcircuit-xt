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

#include "OutputPane.h"
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

engine::Zone::ZoneOutputInfo &OutPaneZoneTraits::outputInfo(SCXTEditor *e)
{
    return e->editorDataCache.zoneOutputInfo;
}

engine::Group::GroupOutputInfo &OutPaneGroupTraits::outputInfo(SCXTEditor *e)
{
    return e->editorDataCache.groupOutputInfo;
}

template <typename OTTraits>
struct ProcTab : juce::Component, HasEditor, sst::jucegui::layouts::JsonLayoutHost
{
    typedef connectors::DiscretePayloadDataAttachment<typename OTTraits::info_t,
                                                      typename OTTraits::route_t>
        int_attachment_t;

    OutputPane<OTTraits> *outputPane{nullptr};
    std::unique_ptr<jcmp::TextPushButton> consistentButton;
    std::unique_ptr<jcmp::Label> consistentLabel;

    std::unique_ptr<int_attachment_t> routingA;
    std::unique_ptr<jcmp::MultiSwitch> multiW;

    typedef connectors::PayloadDataAttachment<typename OTTraits::info_t> attachment_t;
    typedef connectors::BooleanPayloadDataAttachment<typename OTTraits::info_t> bool_attachment_t;

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

    ProcTab(SCXTEditor *e, OutputPane<OTTraits> *pane)
        : HasEditor(e), outputPane(pane), info(OTTraits::outputInfo(e))
    {
        using fac = connectors::SingleValueFactory<int_attachment_t, typename OTTraits::int16Msg_t>;
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
                w->template sendSingleToSerialization<typename OTTraits::int16RefreshMsg_t>(
                    w->info, w->info.procRouting);
            }
        });
        addChildComponent(*consistentButton);

        using ofac = connectors::SingleValueFactory<attachment_t, typename OTTraits::floatMsg_t>;

        ofac::attachAndAdd(info, info.amplitude, this, outputAttachment, outputKnob);
        ofac::attachAndAdd(info, info.pan, this, panAttachment, panKnob);

        if constexpr (!OTTraits::forZone)
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
        outputRouting->setLabel(OTTraits::defaultRoutingLocationName);
        updateRoutingLabel();
        outputRouting->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->selectNewRouting();
        });
        addAndMakeVisible(*outputRouting);
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
        return engine::getBusAddressLabel(rt, OTTraits::defaultRoutingLocationName);
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
                              w->template sendSingleToSerialization<typename OTTraits::int16Msg_t>(
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

            auto at = std::make_unique<ProcessorPane::attachment_t>(
                datamodel::describeValue(w->processorView, w->processorView.outputCubAmp),
                w->processorView.outputCubAmp);
            connectors::configureUpdater<cmsg::UpdateZoneOrGroupProcessorFloatValue,
                                         ProcessorPane::attachment_t>(*at, w->processorView, w,
                                                                      OTTraits::forZone, i);
            levelA[i] = std::move(at);

            auto ed = connectors::jsonlayout::createContinuousWidget(ctrl, cls);
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

            if (outputPane->style())
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
        if constexpr (!OTTraits::forZone)
        {
            nb = nb.withHeight(22);
            velocitySensitivityLabel->setBounds(nb.withWidth(25));
            velocitySensitivityLabel->setText("vel");
            velocitySensitivitySlider->setBounds(
                nb.withTrimmedLeft(28).withWidth(60).reduced(0, 3));
            oversampleButton->setBounds(nb.withTrimmedLeft(90).withWidth(45));
        }
    }

    std::string getRoutingName(typename OTTraits::route_t r)
    {
        auto zn = std::string();
        if constexpr (OTTraits::forZone)
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

    void updateFromProcessorPanes()
    {
        routingLayoutComp->removeAllChildren();
        jsonComponents.clear();
        sst::jucegui::layouts::JsonLayoutEngine engine(*this);
        std::string path = "procrouting-layouts/linear.json";
        switch (info.procRouting)
        {
        case OTTraits::route_t::procRoute_linear:
            path = "procrouting-layouts/linear.json";
            break;

        case OTTraits::route_t::procRoute_ser2:
            path = "procrouting-layouts/ser2.json";
            break;

        case OTTraits::route_t::procRoute_ser3:
            path = "procrouting-layouts/ser3.json";
            break;

        case OTTraits::route_t::procRoute_par1:
            path = "procrouting-layouts/par1.json";
            break;

        case OTTraits::route_t::procRoute_par2:
            path = "procrouting-layouts/par2.json";
            break;

        case OTTraits::route_t::procRoute_par3:
            path = "procrouting-layouts/par3.json";
            break;

        case OTTraits::route_t::procRoute_bypass:
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
    }

    void updateRoutingLabel()
    {
        auto rt = info.routeTo;
        outputRouting->setLabel(getRoutingLabel(rt));
        outputRouting->repaint();
    }

    typename OTTraits::info_t &info;
};

template <typename OTTraits>
OutputPane<OTTraits>::OutputPane(SCXTEditor *e) : jcmp::NamedPanel("ROUTING"), HasEditor(e)
{
    hasHamburger = false;
    proc = std::make_unique<ProcTab<OTTraits>>(editor, this);
    addAndMakeVisible(*proc);
}

template <typename OTTraits> OutputPane<OTTraits>::~OutputPane() {}

template <typename OTTraits> void OutputPane<OTTraits>::resized()
{
    proc->setBounds(getContentArea());
}

template <typename OTTraits> void OutputPane<OTTraits>::setActive(bool b) { active = b; }

template <typename OTTraits> void OutputPane<OTTraits>::updateFromOutputInfo()
{
    proc->updateProcRoutingFromInfo();
    proc->repaint();
}

template <typename OTTraits> void OutputPane<OTTraits>::updateFromProcessorPanes()
{
    proc->updateFromProcessorPanes();
}

template struct OutputPane<OutPaneGroupTraits>;
template struct OutputPane<OutPaneZoneTraits>;
} // namespace scxt::ui::app::edit_screen
