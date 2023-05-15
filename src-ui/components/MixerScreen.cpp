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

#include "HasEditor.h"
#include "browser/BrowserPane.h"
#include "mixer/BusPane.h"
#include "mixer/PartEffectsPane.h"

#include "messaging/messaging.h"
#include "utils.h"
#include "MixerScreen.h"
#include "SCXTEditor.h"

namespace scxt::ui
{
namespace cmsg = scxt::messaging::client;

MixerScreen::MixerScreen(SCXTEditor *e) : HasEditor(e)
{
    browser = std::make_unique<browser::BrowserPane>();
    addAndMakeVisible(*browser);

    busPane = std::make_unique<mixer::BusPane>();
    addAndMakeVisible(*busPane);

    for (auto i = 0; i < partPanes.size(); ++i)
    {
        auto pp = std::make_unique<mixer::PartEffectsPane>(editor, this, i);
        pp->setBusAddress(0);

        addAndMakeVisible(*pp);
        partPanes[i] = std::move(pp);
    }
}

MixerScreen::~MixerScreen() {}

void MixerScreen::visibilityChanged()
{
    if (isVisible())
    {
    }
}
void MixerScreen::resized()
{
    int pad = 0;
    int busHeight = 330;
    browser->setBounds(getWidth() - sideWidths - pad, pad, sideWidths, getHeight() - 3 * pad);

    auto rest = getLocalBounds().withTrimmedRight(sideWidths + pad);

    auto busArea = rest.withTrimmedTop(getHeight() - busHeight);
    busPane->setBounds(busArea);
    auto fxArea = rest.withTrimmedBottom(busHeight).reduced(0);

    auto fxq = fxArea.getWidth() * 1.f / partPanes.size();
    auto ifx = fxArea.withWidth(fxq);
    for (int i = 0; i < partPanes.size(); ++i)
    {
        partPanes[i]->setBounds(ifx.reduced(0));
        ifx = ifx.translated(fxq, 0);
    }
}

void MixerScreen::onBusEffectFullData(
    int bus, int slot,
    const std::array<datamodel::pmd, engine::BusEffectStorage::maxBusEffectParams> &pmd,
    const engine::BusEffectStorage &bes)
{
    busEffectsData[bus][slot].first = pmd;
    busEffectsData[bus][slot].second = bes;
    if (partPanes[slot]->busAddress == bus)
        partPanes[slot]->rebuild();
}
} // namespace scxt::ui