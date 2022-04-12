/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include "DebugPanel.h"
#include "SCXTEditor.h"

DebugPanel::DebugPanel() : Component("Debug Panel")
{
    warning = std::make_unique<juce::Label>();
    warning->setText("This is a debug panel. Close it and SC will crash. Please ignore.",
                     juce::dontSendNotification);
    addAndMakeVisible(*warning);

    // sampler state window
    samplerT = std::make_unique<juce::TextEditor>();
    samplerT->setMultiLine(true, false);
    addAndMakeVisible(samplerT.get());

    // log window
    logT = std::make_unique<juce::TextEditor>();
    logT->setMultiLine(true, false);
    addAndMakeVisible(logT.get());
}
void DebugPanel::resized()
{
    auto r = getLocalBounds();
    r.reduce(5, 5);

    auto h = r.getHeight() - 5; // 5 is space between

    // state occupies half rest of space
    auto t = r.removeFromTop(h / 2);
    samplerT->setBounds(t.withTrimmedTop(18));
    warning->setBounds(t.withHeight(18));

    // log occupies the other half
    r.removeFromTop(5);
    t = r.removeFromTop(h / 2);
    logT->setBounds(t);
}
DebugPanelWindow::DebugPanelWindow()
    : juce::DocumentWindow(juce::String("scxt Debug - ") + scxt::build::GitHash + "/" +
                               scxt::build::GitBranch,
                           juce::Colour(0xFF000000), juce::DocumentWindow::allButtons)
{
    setResizable(true, true);
    setUsingNativeTitleBar(false);
    setSize(500, 640);
    setTopLeftPosition(100, 100);
    panel = new DebugPanel();
    setContentOwned(panel, false); // DebugPanelWindow takes ownership
}
