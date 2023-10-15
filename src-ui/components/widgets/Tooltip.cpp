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

#include "Tooltip.h"
#include "utils.h"

namespace scxt::ui::widgets
{
Tooltip::Tooltip() : sst::jucegui::style::StyleConsumer(Styles::styleClass) { setSize(100, 100); }

static constexpr int margin{5}, rowPad{1}, rowTitlePad{0};

// TODO
// - Colors
// - Fixed Width Font for the data itself
// - Small rule under title

void Tooltip::paint(juce::Graphics &g)
{
    auto bg = style()->getColour(Styles::styleClass, Styles::background);
    auto bord = style()->getColour(Styles::styleClass, Styles::brightoutline);
    auto txt = style()->getColour(Styles::styleClass, Styles::labelcolor);

    g.setColour(bg);
    g.fillRect(getLocalBounds());
    g.setColour(bord);
    g.drawRect(getLocalBounds(), 1);

    auto f = style()->getFont(Styles::styleClass, Styles::labelfont);
    auto rowHeight = f.getHeight() + rowPad;

    g.setColour(txt);
    auto bx = juce::Rectangle<int>(margin, margin, getWidth() - 2 * margin, rowHeight);
    g.setFont(f);
    g.drawText(tooltipTitle, bx, juce::Justification::topLeft);

    auto df = style()->getFont(Styles::styleClass, Styles::datafont);
    rowHeight = df.getHeight() + rowPad;
    g.setFont(df);

    bx = bx.translated(0, rowTitlePad);
    g.setColour(txt);
    for (const auto &d : tooltipData)
    {
        bx = bx.translated(0, rowHeight);
        g.drawText(d, bx, juce::Justification::centredLeft);
    }
}

void Tooltip::resetSizeFromData()
{
    auto f = style()->getFont(Styles::styleClass, Styles::labelfont);
    auto rowHeight = f.getHeight() + rowPad;
    auto maxw = std::max(f.getStringWidthFloat(tooltipTitle), 60.f);

    auto df = style()->getFont(Styles::styleClass, Styles::datafont);
    auto drowHeight = df.getHeight() + rowPad;

    for (const auto &d : tooltipData)
        maxw = std::max(maxw, df.getStringWidthFloat(d));

    // round to nearest 20 to avoid jitters
    maxw = std::ceil(maxw / 20.f) * 20;

    setSize(maxw + 2 * margin,
            2 * margin + rowHeight + drowHeight * tooltipData.size() + rowTitlePad);
}
} // namespace scxt::ui::widgets