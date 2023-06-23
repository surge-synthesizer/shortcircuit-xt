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

#include "BrowserPane.h"
#include "components/SCXTEditor.h"
#include "browser/browser.h"

namespace scxt::ui::browser
{
struct DriveArea : juce::Component, HasEditor
{
    BrowserPane *browserPane{nullptr};
    DriveArea(BrowserPane *b, SCXTEditor *e) : browserPane(b), HasEditor(e){};

    void paint(juce::Graphics &g)
    {
        auto yp = 5, xp = 5;
        g.setFont(10);
        g.setColour(juce::Colours::pink);
        for (const auto &pt : browserPane->roots)
        {
            g.drawText(pt.u8string(), xp, yp, getWidth(), 20, juce::Justification::topLeft);
            yp += 20;
        }
    }
};

BrowserPane::BrowserPane(SCXTEditor *e)
    : HasEditor(e), sst::jucegui::components::NamedPanel("Browser")
{
    hasHamburger = true;
    driveArea = std::make_unique<DriveArea>(this, editor);
    addAndMakeVisible(*driveArea);
    resetRoots();
}

void BrowserPane::resized() { driveArea->setBounds(getContentArea()); }

void BrowserPane::resetRoots()
{
    roots = editor->browser.getRootPathsForDeviceView();
    for (auto &pt : roots)
    {
        SCLOG(pt.u8string());
    }
    repaint();
}
} // namespace scxt::ui::browser
