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
#include "app/mixer-screen/components/PartEffectsPane.h"

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
        auto pp = std::make_unique<mixer_screen::PartEffectsPane>(editor, this, i);
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

std::string MixerScreen::effectDisplayName(engine::AvailableBusEffects t, bool forMenu)
{
    switch (t)
    {
    case engine::none:
        return forMenu ? "None" : "FX";
    case engine::flanger:
        return forMenu ? "Flanger" : "FLANGER";
    case engine::phaser:
        return forMenu ? "Phaser" : "PHASER";
    case engine::delay:
        return forMenu ? "Delay" : "DELAY";
    case engine::reverb1:
        return forMenu ? "Reverb 1" : "REVERB 1";
    case engine::reverb2:
        return forMenu ? "Reverb 2" : "REVERB 2";
    case engine::nimbus:
        return forMenu ? "Nimbus" : "NIMBUS";
    case engine::treemonster:
        return forMenu ? "TreeMonster" : "TREEMONSTER";
    case engine::floatydelay:
        return forMenu ? "FloatyDelay" : "FLOATYDELAY";
    case engine::bonsai:
        return forMenu ? "Bonsai" : "BONSAI";
    }

    return "GCC gives strictly correct, but not useful in this case, warnings";
}

void MixerScreen::setFXSlotToType(int bus, int slot, engine::AvailableBusEffects t)
{
    sendToSerialization(cmsg::SetBusEffectToType({bus, slot, t}));
    busPane->channelStrips[bus]->effectsChanged();
}

void MixerScreen::showFXSelectionMenu(int bus, int slot)
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Effects");
    p.addSeparator();
    auto go = [w = juce::Component::SafePointer(this), bus, slot](auto q) {
        return [w, bus, slot, q]() {
            if (w)
            {
                w->setFXSlotToType(bus, slot, q);
            }
        };
    };
    auto add = [this, &go, &p](auto q) { p.addItem(effectDisplayName(q, true), go(q)); };
    add(engine::AvailableBusEffects::none);
    add(engine::AvailableBusEffects::reverb1);
    add(engine::AvailableBusEffects::reverb2);
    add(engine::AvailableBusEffects::delay);
    add(engine::AvailableBusEffects::flanger);
    add(engine::AvailableBusEffects::phaser);
    add(engine::AvailableBusEffects::treemonster);
    add(engine::AvailableBusEffects::nimbus);
    add(engine::AvailableBusEffects::floatydelay);
    add(engine::AvailableBusEffects::bonsai);
    p.showMenuAsync(editor->defaultPopupMenuOptions());
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
