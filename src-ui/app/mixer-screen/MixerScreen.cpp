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

#include "MixerScreen.h"
#include "app/HasEditor.h"
#include "app/browser-ui/BrowserPane.h"
#include "app/mixer-screen/components/BusPane.h"
#include "app/shared/PartEffectsPane.h"

#include "messaging/messaging.h"
#include "utils.h"
#include "app/SCXTEditor.h"

namespace scxt::ui::app::mixer_screen
{
namespace cmsg = scxt::messaging::client;

MixerScreen::MixerScreen(SCXTEditor *e) : HasEditor(e)
{
    browser = std::make_unique<browser_ui::BrowserPane>(editor);
    addAndMakeVisible(*browser);

    busPane = std::make_unique<mixer_screen::BusPane>(e, this);
    addAndMakeVisible(*busPane);

    for (auto i = 0; i < partPanes.size(); ++i)
    {
        auto pp = std::make_unique<shared::PartEffectsPane<true>>(editor, this, i);
        pp->setBusAddress(0);

        addAndMakeVisible(*pp);
        partPanes[i] = std::move(pp);
    }

    // Select the part 0 bus
    selectBus(0);
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
    int busHeight = 425;
    browser->setBounds(getWidth() - sideWidths - pad, pad, sideWidths, getHeight() - 3 * pad);

    auto rest = getLocalBounds().withTrimmedRight(sideWidths + pad);

    auto busArea = rest.withTrimmedTop(getHeight() - busHeight);
    busPane->setBounds(busArea);
    auto fxArea =
        rest.withTrimmedBottom(busHeight).reduced(40, 0).withTrimmedTop(18).withTrimmedBottom(0);

    auto fxq = fxArea.getWidth() * 1.f / partPanes.size();
    auto ifx = fxArea.withWidth(fxq);
    for (int i = 0; i < partPanes.size(); ++i)
    {
        auto sh = (ifx.getHeight() - partPanes[i]->height) / 2;
        auto sw = (ifx.getWidth() - partPanes[i]->width) / 2;
        partPanes[i]->setBounds(ifx.reduced(sw, sh));
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
    if (partPanes[slot]->busAddressOrPart == bus)
        partPanes[slot]->rebuild();
    busPane->channelStrips[bus]->effectsChanged();
}

void MixerScreen::selectBus(int index)
{
    for (const auto &p : partPanes)
    {
        p->setBusAddress(index);
        p->rebuild();
    }
    for (const auto &b : busPane->channelStrips)
    {
        b->setSelected(false);
    }
    busPane->channelStrips[index]->selected = true;
    editor->setTabSelection("mixer_screen", std::to_string(index));
    repaint();
}

void MixerScreen::onBusSendData(int bus, const engine::Bus::BusSendStorage &s)
{
    assert(bus >= 0 && bus < busSendData.size());
    busSendData[bus] = s;
    busPane->channelStrips[bus]->onDataChanged();
}

void MixerScreen::setFXSlotToType(int bus, int slot, engine::AvailableBusEffects t)
{
    sendToSerialization(cmsg::SetBusEffectToType({bus, -1, slot, t}));
    busPane->channelStrips[bus]->effectsChanged();
}

void MixerScreen::sendBusSendStorage(int bus)
{
    sendToSerialization(cmsg::SetBusSendStorage({bus, busSendData[bus]}));
}

void MixerScreen::setVULevelForBusses(
    const std::array<std::array<std::atomic<float>, 2>, engine::Patch::Busses::busCount> &x)
{
    for (const auto &[i, cs] : sst::cpputils::enumerate(busPane->channelStrips))
    {
        cs->setVULevel(x[i][0], x[i][1]);
    }
}

void MixerScreen::onOtherTabSelection()
{
    auto bs = editor->queryTabSelection("mixer_screen");
    if (!bs.empty())
    {
        auto v = std::atoi(bs.c_str());
        if (v >= 0 && v < busPane->channelStrips.size())
        {
            selectBus(v);
        }
    }
}

} // namespace scxt::ui::app::mixer_screen
