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
#include "connectors/SCXTStyleSheetCreator.h"
#include "sst/jucegui/components/Label.h"

namespace scxt::ui::multi
{
namespace cmsg = scxt::messaging::client;

AdsrPane::AdsrPane(SCXTEditor *e, int index)
    : HasEditor(e), sst::jucegui::components::NamedPanel(index == 0 ? "AMP EG" : "EG 2"),
      index(index)
{
    hasHamburger = true;

    auto attachSlider = [this](Ctrl c, const std::string &l, const auto &fn, float &v) {
        auto at = std::make_unique<attachment_t>(
            this, l, [this]() { this->adsrChangedFromGui(); }, fn, v);
        auto sl = std::make_unique<sst::jucegui::components::VSlider>();
        sl->setSource(at.get());
        sl->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorVSlider);
        addAndMakeVisible(*sl);
        auto lb = std::make_unique<sst::jucegui::components::Label>();
        lb->setText(l);
        addAndMakeVisible(*lb);
        attachments[c] = std::move(at);
        sliders[c] = std::move(sl);
        labels[c] = std::move(lb);
    };

    attachSlider(
        Ctrl::A, "A", [](const auto &pl) { return pl.a; }, adsrView.a);
    attachSlider(
        Ctrl::H, "H", [](const auto &pl) { return pl.h; }, adsrView.h);
    attachSlider(
        Ctrl::D, "D", [](const auto &pl) { return pl.d; }, adsrView.d);
    attachSlider(
        Ctrl::S, "S", [](const auto &pl) { return pl.s; }, adsrView.s);
    attachSlider(
        Ctrl::R, "R", [](const auto &pl) { return pl.r; }, adsrView.r);

    auto attachKnob = [this](Ctrl c, const std::string &l, const auto &fn, float &v) {
        auto at = std::make_unique<attachment_t>(
            this, l, [this]() { this->adsrChangedFromGui(); }, fn, v);
        auto kn = std::make_unique<sst::jucegui::components::Knob>();
        kn->setSource(at.get());
        kn->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorVSlider);
        addAndMakeVisible(*kn);
        attachments[c] = std::move(at);
        knobs[c] = std::move(kn);
    };

    attachKnob(
        Ctrl::Ash, "A Shape", [](const auto &pl) { return pl.aShape; }, adsrView.aShape);
    attachKnob(
        Ctrl::Dsh, "D Shape", [](const auto &pl) { return pl.dShape; }, adsrView.dShape);
    attachKnob(
        Ctrl::Rsh, "R Shape", [](const auto &pl) { return pl.rShape; }, adsrView.rShape);

    cmsg::clientSendToSerialization(cmsg::AdsrSelectedZoneView(index), e->msgCont);

    onHamburger = [safeThis = juce::Component::SafePointer(this)]()
    {
        if (safeThis)
            safeThis->showHamburgerMenu();
    };
}

void AdsrPane::adsrChangedFromModel(const datamodel::AdsrStorage &d)
{
    for (const auto &[c, sl] : sliders)
        sl->setEnabled(true);

    for (const auto &[c, sl] : knobs)
        sl->setEnabled(true);

    for (const auto &[c, at] : attachments)
        at->setValueFromPayload(d);

    repaint();
}

void AdsrPane::adsrChangedFromGui()
{
    cmsg::clientSendToSerialization(cmsg::AdsrSelectedZoneUpdateRequest(index, adsrView),
                                    editor->msgCont);
}

void AdsrPane::adsrDeactivated()
{
    for (const auto &[c, sl] : sliders)
        sl->setEnabled(false);

    for (const auto &[c, sl] : knobs)
        sl->setEnabled(false);

    repaint();
}

void AdsrPane::resized()
{
    auto r = getContentArea();
    auto lh = 16.f;
    auto kh = 20.f;
    auto h = r.getHeight() - lh - kh;
    auto x = r.getX();
    auto y = r.getY() + kh;
    auto w = 34.f;
    x = x + (r.getWidth() - w * 5) * 0.5;

    for (const auto &c : {Ctrl::A, H, D, S, R})
    {
        switch (c)
        {
        case A:
            knobs[Ash]->setBounds(x + (w - lh) * 0.5, y - lh, kh, kh);
            break;
        case D:
            knobs[Dsh]->setBounds(x + (w - lh) * 0.5, y - lh, kh, kh);
            break;
        case R:
            knobs[Rsh]->setBounds(x + (w - lh) * 0.5, y - lh, kh, kh);
            break;
        default:
            break;
        }
        sliders[c]->setBounds(x, y, w, h);
        labels[c]->setBounds(x, y + h, w, lh);
        x += w;
    }
}

void AdsrPane::showHamburgerMenu()
{
    juce::PopupMenu p;
    p.addSectionHeader("Menu");
    p.addItem("Coming", [](){});
    p.addItem("Soon", [](){});

    p.showMenuAsync(juce::PopupMenu::Options());
}

} // namespace scxt::ui::multi