/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "BusPane.h"
#include "app/SCXTEditor.h"
#include "sst/jucegui/components/Viewport.h"

namespace scxt::ui::app::mixer_screen
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

} // namespace scxt::ui::app::mixer_screen