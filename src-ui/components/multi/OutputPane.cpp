/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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
                                           jcmp::Label::Styles::controlLabelFont);
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
    OutputPane<OTTraits> *parent{nullptr};

    template <typename T>
    std::unique_ptr<T> attachContinuousTo(const std::unique_ptr<attachment_t> &at)
    {
        auto kn = std::make_unique<T>();
        kn->setSource(at.get());
        setupWidgetForValueTooltip(kn, at);
        addAndMakeVisible(*kn);
        return std::move(kn);
    }

    OutputTab(SCXTEditor *e, OutputPane<OTTraits> *p) : HasEditor(e), parent(p)
    {
        outputAttachment = std::make_unique<attachment_t>(
            datamodel::pmd().asPercent().withName("Amplitude"), // FIXME - move to decibel
            [w = juce::Component::SafePointer(this)](const auto &at) {
                if (w && w->parent)
                    w->outputChangedFromGUI(at);
            },
            info.amplitude);
        panAttachment = std::make_unique<attachment_t>(
            datamodel::pmd().asPercentBipolar().withName("Pan"),
            [w = juce::Component::SafePointer(this)](const auto &at) {
                if (w && w->parent)
                    w->outputChangedFromGUI(at);
            },
            info.pan);

        outputKnob = attachContinuousTo<jcmp::Knob>(outputAttachment);
        panKnob = attachContinuousTo<jcmp::Knob>(panAttachment);
        outputLabel = std::make_unique<jcmp::Label>();
        outputLabel->setText("Amplitude");
        addAndMakeVisible(*outputLabel);
        panLabel = std::make_unique<jcmp::Label>();
        panLabel->setText("Pan");
        addAndMakeVisible(*panLabel);
    }

    void resized()
    {
        auto b = getLocalBounds();
        b = b.reduced(5, 0);
        auto w = b.getWidth();
        b = b.withHeight(w / 2);

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
    }
    void outputChangedFromGUI(const attachment_t &at)
    {
        updateValueTooltip(at);

        if constexpr (OTTraits::forZone)
        {
            sendToSerialization(cmsg::UpdateZoneOutputInfo(info));
        }
        else
        {
            sendToSerialization(cmsg::UpdateGroupOutputInfo(info));
        }
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
    output->repaint();
}

template struct OutputPane<OutPaneGroupTraits>;
template struct OutputPane<OutPaneZoneTraits>;
} // namespace scxt::ui::multi
