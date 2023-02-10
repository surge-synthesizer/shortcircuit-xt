//
// Created by Paul Walker on 2/11/23.
//

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

    cmsg::clientSendToSerialization(cmsg::AdsrSelectedZoneViewRequest(index), e->msgCont);
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