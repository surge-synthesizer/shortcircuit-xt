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
#include "components/SCXTEditor.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/MenuButton.h"
#include "connectors/PayloadDataAttachment.h"
#include "datamodel/parameter.h"

namespace scxt::ui::multi
{

namespace jcmp = sst::jucegui::components;
namespace cmsg = scxt::messaging::client;

struct cc : juce::Component, HasEditor
{
    cc(SCXTEditor *e) : HasEditor(e) {}
    void paint(juce::Graphics &g)
    {
        auto ft = editor->style()->getFont(jcmp::Label::Styles::styleClass,
                                           jcmp::Label::Styles::labelfont);
        g.setFont(ft.withHeight(14));
        g.setColour(juce::Colours::white);
        g.drawText("Proc Routing", getLocalBounds().withTrimmedBottom(20),
                   juce::Justification::centred);

        g.setFont(ft.withHeight(12));
        g.setColour(juce::Colours::white);
        g.drawText("Coming Soon", getLocalBounds().withTrimmedTop(20),
                   juce::Justification::centred);
    }
};

template <typename OTTraits> struct OutputTab : juce::Component, HasEditor
{
    typedef connectors::PayloadDataAttachment<typename OTTraits::info_t> attachment_t;

    std::unique_ptr<attachment_t> outputAttachment, panAttachment;
    std::unique_ptr<jcmp::Knob> outputKnob, panKnob;
    std::unique_ptr<jcmp::Label> outputLabel, panLabel;
    std::unique_ptr<jcmp::MenuButton> outputRouting;
    OutputPane<OTTraits> *parent{nullptr};

    OutputTab(SCXTEditor *e, OutputPane<OTTraits> *p) : HasEditor(e), parent(p)
    {
        using fac = connectors::SingleValueFactory<attachment_t, typename OTTraits::floatMsg_t>;

        fac::attachAndAdd(datamodel::pmd().asCubicDecibelAttenuation().withName("Amplitude"), info,
                          info.amplitude, this, outputAttachment, outputKnob);
        fac::attachAndAdd(datamodel::pmd().asPercentBipolar().withName("Pan"), info, info.pan, this,
                          panAttachment, panKnob);

        outputLabel = std::make_unique<jcmp::Label>();
        outputLabel->setText("Amplitude");
        addAndMakeVisible(*outputLabel);
        panLabel = std::make_unique<jcmp::Label>();
        panLabel->setText("Pan");
        addAndMakeVisible(*panLabel);

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
        auto b = getLocalBounds();
        b = b.reduced(5, 0);
        auto w = b.getWidth();
        auto op = b;
        b = b.withHeight(w / 2);
        op = op.withTop(op.getBottom() - 20).translated(0, -5);

        auto bl = b.withWidth(w / 2);

        {
            auto ll = bl.reduced(3);
            auto lh = ll.getHeight();
            auto th = 20;

            auto kb = ll.reduced(th / 2, 0).withTrimmedBottom(th);
            outputKnob->setBounds(kb);
            auto pb = ll.withTrimmedTop(lh - th);
            outputLabel->setBounds(pb);
        }

        bl = bl.translated(w / 2, 0);

        {
            auto ll = bl.reduced(3);
            auto lh = ll.getHeight();
            auto th = 20;

            auto kb = ll.reduced(th / 2, 0).withTrimmedBottom(th);
            panKnob->setBounds(kb);
            auto pb = ll.withTrimmedTop(lh - th);
            panLabel->setBounds(pb);
        }

        outputRouting->setBounds(op);
    }

    std::string getRoutingLabel(scxt::engine::BusAddress rt)
    {
        if (rt == scxt::engine::BusAddress::ERROR_BUS)
        {
            return "Error";
        }
        else if (rt == scxt::engine::BusAddress::DEFAULT_BUS)
        {
            return OTTraits::defaultRoutingLocationName;
        }
        else if (rt == scxt::engine::BusAddress::MAIN_0)
        {
            return "Main";
        }
        else if (rt < scxt::engine::BusAddress::AUX_0)
        {
            return "Part " + std::to_string(rt - scxt::engine::BusAddress::PART_0 + 1);
        }
        else
        {
            return "Aux " + std::to_string(rt - scxt::engine::BusAddress::AUX_0 + 1);
        }
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
    typename OTTraits::info_t info;
};

template <typename OTTraits>
OutputPane<OTTraits>::OutputPane(SCXTEditor *e) : jcmp::NamedPanel(""), HasEditor(e)
{
    hasHamburger = false;
    isTabbed = true;
    tabNames = {"OUTPUT", "PROC ROUTING"};

    output = std::make_unique<OutputTab<OTTraits>>(editor, this);
    addAndMakeVisible(*output);
    proc = std::make_unique<cc>(editor);
    addChildComponent(*proc);

    resetTabState();

    onTabSelected = [wt = juce::Component::SafePointer(this)](int i) {
        if (!wt)
            return;
        if (i == 0)
        {
            wt->output->setVisible(wt->active);
            wt->proc->setVisible(false);
        }
        else
        {
            wt->output->setVisible(false);
            wt->proc->setVisible(wt->active);
        }
    };
    onTabSelected(selectedTab);
}

template <typename OTTraits> OutputPane<OTTraits>::~OutputPane() {}

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

template <typename OTTraits>
void OutputPane<OTTraits>::setOutputData(const typename OTTraits::info_t &d)
{
    output->info = d;
    output->updateRoutingLabel();
    output->repaint();
}

template struct OutputPane<OutPaneGroupTraits>;
template struct OutputPane<OutPaneZoneTraits>;
} // namespace scxt::ui::multi
