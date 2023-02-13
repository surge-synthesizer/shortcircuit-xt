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

#include "AdsrPane.h"
#include "components/SCXTEditor.h"

#include "messaging/client/client_serial.h"
#include "messaging/client/client_messages.h"

namespace scxt::ui::multi
{
namespace cmsg = scxt::messaging::client;

AdsrPane::AdsrPane(SCXTEditor *e, int index)
    : HasEditor(e), sst::jucegui::components::NamedPanel(index == 0 ? "AEG" : "EG2"), index(index)
{
    auto attach =
        [this](const std::string &l,
               float &v) -> std::tuple<std::unique_ptr<attachment>,
                                       std::unique_ptr<sst::jucegui::components::VSlider>> {
        auto at = std::make_unique<attachment>(this, l, v);
        auto sl = std::make_unique<sst::jucegui::components::VSlider>();
        sl->setSource(at.get());
        addAndMakeVisible(*sl);
        return {std::move(at), std::move(sl)};
    };

    {
        auto [a, s] = attach("A", adsrView.a);
        atA = std::move(a);
        slA = std::move(s);
    }
    {
        auto [a, s] = attach("D", adsrView.d);
        atD = std::move(a);
        slD = std::move(s);
    }
    {
        auto [a, s] = attach("S", adsrView.s);
        atS = std::move(a);
        slS = std::move(s);
    }
    {
        auto [a, s] = attach("R", adsrView.r);
        atR = std::move(a);
        slR = std::move(s);
    }

    cmsg::clientSendToSerialization(cmsg::AdsrSelectedZoneView(index), e->msgCont);
}

void AdsrPane::adsrChangedFromModel(const datamodel::AdsrStorage &d)
{
    atA->setValueFromModel(d.a);
    atD->setValueFromModel(d.d);
    atS->setValueFromModel(d.s);
    atR->setValueFromModel(d.r);

    repaint();
}

void AdsrPane::adsrChangedFromGui()
{
    cmsg::clientSendToSerialization(cmsg::AdsrSelectedZoneUpdateRequest(index, adsrView),
                                    editor->msgCont);
}

void AdsrPane::adsrDeactivated()
{
    std::cout << "DEACTIVATED" << std::endl;
}

void AdsrPane::resized()
{
    auto r = getContentArea();
    auto h = r.getHeight();
    auto x = r.getX();
    auto y = r.getY();
    auto w = 18;
    auto pad = 2;

    slA->setBounds(x, y, w, h);
    x += w + pad;
    slD->setBounds(x, y, w, h);
    x += w + pad;
    slS->setBounds(x, y, w, h);
    x += w + pad;
    slR->setBounds(x, y, w, h);
}

} // namespace scxt::ui::multi