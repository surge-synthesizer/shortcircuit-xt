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

namespace scxt::ui::multi
{

namespace jcmp = sst::jucegui::components;
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
    OutputTab(SCXTEditor *e) : HasEditor(e) {}

    typename OTTraits::info_t info;
};

template <typename OTTraits>
OutputPane<OTTraits>::OutputPane(SCXTEditor *e)
    : sst::jucegui::components::NamedPanel(""), HasEditor(e)
{
    hasHamburger = false;
    isTabbed = true;
    tabNames = {"OUTPUT", "PROC ROUTING"};

    output = std::make_unique<OutputTab<OTTraits>>(editor);
    addAndMakeVisible(*output);
    proc = std::make_unique<cc>(editor);
    addChildComponent(*proc);

    resetTabState();

    onTabSelected = [wt = juce::Component::SafePointer(this)](int i) {
        if (!wt)
            return;
        if (i == 0)
        {
            wt->output->setVisible(true);
            wt->proc->setVisible(false);
        }
        else
        {
            wt->output->setVisible(false);
            wt->proc->setVisible(true);
        }
    };
}

template <typename OTTraits> OutputPane<OTTraits>::~OutputPane() {}

template <typename OTTraits> void OutputPane<OTTraits>::resized()
{
    output->setBounds(getContentArea());
    proc->setBounds(getContentArea());
}

template <typename OTTraits> void OutputPane<OTTraits>::setActive(bool b)
{
    SCLOG_ONCE("Output Pane setActive ignored " << (b ? "ON" : "OFF"));
}

template <typename OTTraits>
void OutputPane<OTTraits>::setOutputData(const typename OTTraits::info_t &d)
{
    SCLOG_ONCE("Output Pane Set Output Data");
    output->info = d;
    output->repaint();
}

template struct OutputPane<OutPaneGroupTraits>;
template struct OutputPane<OutPaneZoneTraits>;
} // namespace scxt::ui::multi
