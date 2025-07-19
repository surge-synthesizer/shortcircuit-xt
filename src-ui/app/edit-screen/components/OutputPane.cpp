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

#include "OutputPane.h"
#include "ProcessorPane.h"
#include "app/SCXTEditor.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/HSliderFilled.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/LabeledItem.h"
#include "connectors/PayloadDataAttachment.h"
#include "datamodel/metadata.h"
#include "theme/Layout.h"
#include "app/edit-screen/EditScreen.h"

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

template <typename OTTraits> struct ProcTab : juce::Component, HasEditor
{
    OutputPane<OTTraits> *outputPane{nullptr};
    std::unique_ptr<jcmp::MenuButton> procRouting;
    std::unique_ptr<jcmp::TextPushButton> consistentButton;
    std::unique_ptr<jcmp::Label> consistentLabel;

    ProcTab(SCXTEditor *e, OutputPane<OTTraits> *pane)
        : HasEditor(e), outputPane(pane), info(OTTraits::outputInfo(e))
    {
        procRouting = std::make_unique<jcmp::MenuButton>();
        procRouting->setLabel(OTTraits::defaultRoutingLocationName);
        updateProcRoutingLabel();
        procRouting->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->selectNewProcRouting();
        });
        addAndMakeVisible(*procRouting);

        consistentLabel = std::make_unique<jcmp::Label>();
        consistentLabel->setText("Routing inconsistent across selection");
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
    }

    void resized() override
    {
        procRouting->setBounds(getLocalBounds().reduced(3, 5).withHeight(24));
        consistentLabel->setBounds(procRouting->getBounds());
        consistentButton->setBounds(procRouting->getBounds().translated(0, 30));
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
        updateProcRoutingLabel();
        if constexpr (OTTraits::forZone)
        {
            auto c = info.procRoutingConsistent;
            procRouting->setVisible(c);
            for (int i = 0; i < nOuts; ++i)
            {
                levelK[i]->setVisible(c);
                levelL[i]->setVisible(c);
            }
            consistentButton->setVisible(!c);
            consistentLabel->setVisible(!c);

            if (!c)
            {
                consistentButton->setLabel("Set to " + getRoutingName(info.procRouting));
            }
        }
    }
    void updateProcRoutingLabel()
    {
        procRouting->setLabel(getRoutingName(info.procRouting));
        procRouting->repaint();
    }
    void selectNewProcRouting()
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Proc Routing");
        p.addSeparator();
        for (int i = OTTraits::route_t::procRoute_linear; i <= OTTraits::route_t::procRoute_bypass;
             ++i)
        {
            auto r = (typename OTTraits::route_t)i;
            p.addItem(getRoutingName(r), true, info.procRouting == r,
                      [w = juce::Component::SafePointer(this), r]() {
                          if (w)
                          {
                              w->info.procRouting = r;
                              w->template sendSingleToSerialization<typename OTTraits::int16Msg_t>(
                                  w->info, w->info.procRouting);
                              w->updateProcRoutingLabel();
                          }
                      });
        }
        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }

    static constexpr int nOuts{scxt::processorsPerZoneAndGroup};
    std::array<std::unique_ptr<ProcessorPane::attachment_t>, nOuts> levelA;
    std::array<std::unique_ptr<jcmp::Knob>, nOuts> levelK;
    std::array<std::unique_ptr<jcmp::Label>, nOuts> levelL;

    void updateFromProcessorPanes()
    {
        for (auto &k : levelK)
        {
            if (k)
            {
                removeChildComponent(k.get());
            }
        }

        for (int i = 0; i < nOuts; ++i)
        {
            auto w = outputPane->procWeakRefs[i];

            auto kw = 30, margin{4}, yPos{36}, lw{12};
            if (w)
            {
                auto at = std::make_unique<ProcessorPane::attachment_t>(
                    datamodel::describeValue(w->processorView, w->processorView.outputCubAmp),
                    w->processorView.outputCubAmp);
                connectors::configureUpdater<cmsg::UpdateZoneOrGroupProcessorFloatValue,
                                             ProcessorPane::attachment_t>(*at, w->processorView, w,
                                                                          OTTraits::forZone, i);
                levelA[i] = std::move(at);

                levelK[i] = std::make_unique<jcmp::Knob>();
                levelK[i]->setSource(levelA[i].get());

                levelK[i]->setBounds(margin + lw + i * (kw + margin + lw), yPos, kw, kw);
                levelK[i]->setEnabled(w->processorView.type != dsp::processor::proct_none);

                levelL[i] = std::make_unique<jcmp::Label>();
                levelL[i]->setText(std::to_string(i + 1));
                levelL[i]->setBounds(margin + i * (kw + margin + lw), yPos, lw, kw);

                setupWidgetForValueTooltip(levelK[i].get(), levelA[i].get());
                addAndMakeVisible(*levelK[i]);
                addAndMakeVisible(*levelL[i]);
            }
        }
    }

    void paint(juce::Graphics &g) override
    {
        if constexpr (OTTraits::forZone)
        {
            if (!info.procRoutingConsistent)
                return;
        }

        auto startP{68};
        auto b = getLocalBounds().withTrimmedTop(startP);
        g.setColour(juce::Colours::red);
        // g.fillRect(b);
    }

    typename OTTraits::info_t &info;
};

template <typename OTTraits> struct OutputTab : juce::Component, HasEditor
{
    typedef connectors::PayloadDataAttachment<typename OTTraits::info_t> attachment_t;
    typedef connectors::BooleanPayloadDataAttachment<typename OTTraits::info_t> bool_attachment_t;

    std::unique_ptr<attachment_t> outputAttachment, panAttachment, velocitySensitivityAttachment;
    std::unique_ptr<jcmp::MenuButton> outputRouting;
    OutputPane<OTTraits> *parent{nullptr};
    std::unique_ptr<jcmp::Labeled<jcmp::Knob>> outputKnob, panKnob;
    std::unique_ptr<jcmp::Labeled<jcmp::HSliderFilled>> velocitySensitivitySlider;

    std::unique_ptr<bool_attachment_t> oversampleAttachment;
    std::unique_ptr<jcmp::ToggleButton> oversampleButton;

    OutputTab(SCXTEditor *e, OutputPane<OTTraits> *p)
        : HasEditor(e), parent(p), info(OTTraits::outputInfo(e))
    {
        using fac = connectors::SingleValueFactory<attachment_t, typename OTTraits::floatMsg_t>;

        fac::attachLabelAndAdd(info, info.amplitude, this, outputAttachment, outputKnob,
                               "Amplitude");
        fac::attachLabelAndAdd(info, info.pan, this, panAttachment, panKnob, "Pan");

        if constexpr (!OTTraits::forZone)
        {
            fac::attachLabelAndAdd(info, info.velocitySensitivity, this,
                                   velocitySensitivityAttachment, velocitySensitivitySlider, "Vel");
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

    void resized()
    {
        namespace lo = theme::layout;
        auto cc = lo::columnCenterX(getLocalBounds().reduced(10, 0), 2);
        lo::knobCX<theme::layout::constants::largeKnob>(*outputKnob, cc[0], 10);
        lo::knobCX<theme::layout::constants::largeKnob>(*panKnob, cc[1], 10);

        if (!OTTraits::forZone && velocitySensitivitySlider)
        {
            auto b = getLocalBounds();
            auto op = b.withTop(b.getBottom() - 20).translated(0, -27).reduced(30, 4);
            velocitySensitivitySlider->item->setBounds(op.withTrimmedLeft(30));
            velocitySensitivitySlider->label->setBounds(op.withWidth(30));
        }

        if (!OTTraits::forZone && oversampleButton)
        {
            auto b = getLocalBounds();
            oversampleButton->setBounds(
                b.withTrimmedLeft(b.getWidth() - 30).withTrimmedBottom(b.getHeight() - 18));
        }

        auto b = getLocalBounds();
        auto op = b.withTop(b.getBottom() - 20).translated(0, -5).reduced(10, 0);
        outputRouting->setBounds(op);
    }

    std::string getRoutingLabel(scxt::engine::BusAddress rt)
    {
        return engine::getBusAddressLabel(rt, OTTraits::defaultRoutingLocationName);
    }

    void updateRoutingLabel()
    {
        auto rt = info.routeTo;
        outputRouting->setLabel(getRoutingLabel(rt));
        outputRouting->repaint();
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
    typename OTTraits::info_t &info;
};

template <typename OTTraits>
OutputPane<OTTraits>::OutputPane(SCXTEditor *e) : jcmp::NamedPanel(""), HasEditor(e)
{
    hasHamburger = false;
    isTabbed = true;
    if (OTTraits::forZone)
        tabNames = {"PROC ROUTING", "OUTPUT"};
    else
        tabNames = {
            "OUTPUT",
            "PROC ROUTING",
        };

    output = std::make_unique<OutputTab<OTTraits>>(editor, this);
    addAndMakeVisible(*output);
    proc = std::make_unique<ProcTab<OTTraits>>(editor, this);
    addChildComponent(*proc);

    resetTabState();

    onTabSelected = [wt = juce::Component::SafePointer(this)](int i) {
        if (!wt)
            return;
        wt->setSelectedTab(i);
    };
    onTabSelected(selectedTab);
}

template <typename OTTraits> OutputPane<OTTraits>::~OutputPane() {}

template <typename OTTraits> void OutputPane<OTTraits>::setSelectedTab(int i)
{
    if ((OTTraits::forZone && i == 1) || (!OTTraits::forZone && i == 0))
    {
        output->setVisible(active);
        proc->setVisible(false);
    }
    else
    {
        output->setVisible(false);
        proc->setVisible(active);
    }
    if (editor->editScreen)
    {
        auto kn = std::string("multi") + (OTTraits::forZone ? ".zone.output" : ".group.output");
        editor->setTabSelection(editor->editScreen->tabKey(kn), std::to_string(i));
    }
}

template <typename OTTraits> void OutputPane<OTTraits>::resized()
{
    output->setBounds(getContentArea());
    proc->setBounds(getContentArea());
}

template <typename OTTraits> void OutputPane<OTTraits>::setActive(bool b)
{
    active = b;
    onTabSelected(selectedTab);
}

template <typename OTTraits> void OutputPane<OTTraits>::updateFromOutputInfo()
{
    output->updateRoutingLabel();
    output->repaint();

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
