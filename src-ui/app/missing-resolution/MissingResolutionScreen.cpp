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

#include "MissingResolutionScreen.h"

#include <sst/jucegui/components/NamedPanel.h>
#include <sst/jucegui/components/TextPushButton.h>

namespace scxt::ui::app::missing_resolution
{

struct Contents : juce::Component, HasEditor
{
    MissingResolutionScreen *parent{nullptr};
    std::unique_ptr<sst::jucegui::components::TextPushButton> okButton;
    Contents(MissingResolutionScreen *p, SCXTEditor *e) : parent(p), HasEditor(e)
    {
        okButton = std::make_unique<sst::jucegui::components::TextPushButton>();
        okButton->setLabel("OK");
        okButton->setOnCallback([w = juce::Component::SafePointer(parent)]() {
            if (!w)
                return;
            w->setVisible(false);
        });
        addAndMakeVisible(*okButton);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        b = b.withTrimmedTop(b.getHeight() - 22).withTrimmedLeft(b.getWidth() - 100);
        okButton->setBounds(b);
    }
    void paint(juce::Graphics &g) override
    {
        int bxH = 40;
        auto bx = getLocalBounds().reduced(2, 2).withHeight(bxH);
        for (const auto &i : parent->workItems)
        {
            auto bd = bx.reduced(2, 2);
            g.setFont(editor->themeApplier.interBoldFor(14));
            g.setColour(editor->themeColor(theme::ColorMap::generic_content_high));
            g.drawText(i.path.u8string(), bd, juce::Justification::topLeft);

            g.setFont(editor->themeApplier.interMediumFor(12));
            g.setColour(editor->themeColor(theme::ColorMap::generic_content_medium));
            g.drawText(i.md5sum, bd, juce::Justification::bottomLeft);

            g.setColour(editor->themeColor(theme::ColorMap::generic_content_low));
            g.drawRect(bx, 1);

            bx = bx.translated(0, bxH + 4);
        }
    }
};

MissingResolutionScreen::MissingResolutionScreen(SCXTEditor *e) : HasEditor(e)
{
    auto ct = std::make_unique<sst::jucegui::components::NamedPanel>("Missing Sample Resolution");
    addAndMakeVisible(*ct);

    auto contents = std::make_unique<Contents>(this, e);
    ct->setContentAreaComponent(std::move(contents));
    contentsArea = std::move(ct);
}

void MissingResolutionScreen::resized()
{
    contentsArea->setBounds(getLocalBounds().reduced(100, 80));
}

} // namespace scxt::ui::app::missing_resolution