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

#include "BusPane.h"

#include "sst/jucegui/components/Viewport.h"

namespace scxt::ui::mixer
{

BusPane::BusPane(SCXTEditor *e, MixerScreen *m)
    : HasEditor(e), mixer(m), sst::jucegui::components::NamedPanel("BUSSES")
{
    partBusViewport = std::make_unique<sst::jucegui::components::Viewport>();
    partBusContainer = std::make_unique<juce::Component>();

    for (int i = 0; i < maxBusses; ++i)
    {
        auto t = (i == 0)             ? ChannelStrip::BusType::MAIN
                 : (i < numParts + 1) ? ChannelStrip::BusType::PART
                                      : ChannelStrip::BusType::AUX;
        channelStrips[i] = std::make_unique<ChannelStrip>(e, m, i, t);
        if (t == ChannelStrip::BusType::PART)
        {
            partBusContainer->addAndMakeVisible(*(channelStrips[i]));
        }
        else
        {
            addAndMakeVisible(*(channelStrips[i]));
        }
    }
    partBusViewport->setViewedComponent(partBusContainer.get(), false);
    addAndMakeVisible(*partBusViewport);
    hasHamburger = true;
}
void BusPane::resized()
{
    auto vps = getContentArea().withTrimmedRight(5 * busWidth);
    partBusViewport->setBounds(vps);
    partBusContainer->setBounds(juce::Rectangle<int>(
        0, 0, numParts * busWidth,
        getContentArea().getHeight() - partBusViewport->getScrollBarThickness() - 2));

    auto r = getContentArea().withWidth(busWidth).withX(0).withY(0).withHeight(
        partBusContainer->getHeight());

    // Sizing in the viewport
    for (int i = 0; i < numParts; ++i)
    {
        channelStrips[i + 1]->setBounds(r);
        r = r.translated(busWidth, 0);
    }

    auto mb = getContentArea()
                  .withTrimmedLeft(getContentArea().getWidth() - 5 * busWidth)
                  .withWidth(busWidth)
                  .withHeight(partBusContainer->getHeight());
    for (int i = numParts + 1; i < maxBusses; ++i)
    {
        channelStrips[i]->setBounds(mb);
        mb = mb.translated(busWidth, 0);
    }
    channelStrips[0]->setBounds(mb);
}

} // namespace scxt::ui::mixer