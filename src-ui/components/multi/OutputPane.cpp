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

namespace scxt::ui::multi
{

struct cc : juce::Component
{
    juce::Colour c;
    cc(juce::Colour ci) : c(ci) {}
    void paint(juce::Graphics &g) { g.fillAll(c); }
};

struct OutputTab : juce::Component, HasEditor
{
    OutputTab(SCXTEditor *e) : HasEditor(e) {}

    engine::Zone::ZoneOutputInfo info;
};

OutputPane::OutputPane(SCXTEditor *e) : sst::jucegui::components::NamedPanel(""), HasEditor(e)
{
    hasHamburger = false;
    isTabbed = true;
    tabNames = {"OUTPUT", "PROC ROUTING"};

    output = std::make_unique<OutputTab>(editor);
    addAndMakeVisible(*output);
    proc = std::make_unique<cc>(juce::Colours::black.withAlpha(0.f));
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

OutputPane::~OutputPane() {}

void OutputPane::resized()
{
    output->setBounds(getContentArea());
    proc->setBounds(getContentArea());
}

void OutputPane::setActive(bool b) { SCLOG_UNIMPL("setActive " << (b ? "ON" : "OFF")); }

void OutputPane::setOutputData(const engine::Zone::ZoneOutputInfo &d)
{
    output->info = d;
    output->repaint();
}
} // namespace scxt::ui::multi
