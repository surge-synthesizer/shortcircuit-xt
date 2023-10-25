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

#include "ShortCircuitMenuButton.h"
#include "utils.h"
#include "sst/jucegui/components/ButtonPainter.h"
#include "connectors/SCXTResources.h"

namespace scxt::ui::widgets
{

void replaceWhiteWith(juce::Drawable *db, const juce::Colour &c)
{
    auto ds = dynamic_cast<juce::DrawableShape *>(db);
    auto dt = dynamic_cast<juce::DrawableText *>(db);

    if (ds)
    {
        auto ft = ds->getFill();
        if (ft.isColour() && ft.colour == juce::Colours::white)
        {
            ft.setColour(c);
            ds->setFill(ft);
        }
        auto fs = ds->getStrokeFill();
        if (fs.isColour() && fs.colour == juce::Colours::white)
        {
            fs.setColour(c);
            ds->setStrokeFill(fs);
        }
    }
    else
    {
        for (auto &ck : db->getChildren())
        {
            if (auto cd = dynamic_cast<juce::Drawable *>(ck))
            {
                replaceWhiteWith(cd, c);
            }
        }
    }
}

ShortCircuitMenuButton::ShortCircuitMenuButton()
{
    icon = connectors::resources::loadImageDrawable("images/SCicon.svg");
    orangeIcon = icon->createCopy();
    replaceWhiteWith(orangeIcon.get(), juce::Colour(0xFF, 0x90, 0x00));
}
void ShortCircuitMenuButton::paint(juce::Graphics &g)
{

    sst::jucegui::components::paintButtonBG(this, g);

    auto sx = 1.f * getWidth() / icon->getWidth();
    auto sy = 1.f * getHeight() / icon->getHeight();
    auto sz = std::min(sx, sy) * 0.8;
    {
        auto gs = juce::Graphics::ScopedSaveState(g);
        auto ox = (sz * icon->getWidth() - getWidth()) * 0.5 * sz;
        auto oy = (sz * icon->getHeight() - getHeight()) * 0.5 * sz;

        g.addTransform(juce::AffineTransform().translated(-ox, -oy).scaled(sz, sz));

        if (isHovered)
            icon->drawAt(g, 0, 0, 1.0);
        else
            orangeIcon->drawAt(g, 0, 0, 1.0);
    }

#if BUILD_IS_DEBUG
    g.setColour(juce::Colours::yellow);
    g.setFont(juce::Font(10, juce::Font::plain));
    g.drawFittedText("Debug", getLocalBounds().reduced(2, 2), juce::Justification::bottom, 1);
#endif
}
} // namespace scxt::ui::widgets