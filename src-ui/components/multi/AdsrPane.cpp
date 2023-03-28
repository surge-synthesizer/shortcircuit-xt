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
#include "connectors/Connectors.h"

namespace scxt::ui::multi
{
namespace cmsg = scxt::messaging::client;
using dma = datamodel::AdsrStorage;
namespace comp = sst::jucegui::components;

AdsrPane::AdsrPane(SCXTEditor *e, int index)
    : HasEditor(e), sst::jucegui::components::NamedPanel(index == 0 ? "AMP EG" : "EG 2"),
      index(index)
{
    hasHamburger = true;

    auto stowOnto = [this](auto &tgt, auto c, auto pair) {
        auto &[a, s] = pair;
        attachments[c] = std::move(a);
        tgt[c] = std::move(s);
        tgt[c]->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorVSlider);

        // This is somewhat unsatisfying
        tgt[c]->onBeginEdit = [this, &slRef = *tgt[c], &atRef = *attachments[c]]() {
            editor->showTooltip(slRef);
            updateTooltip(atRef);
        };
        tgt[c]->onEndEdit = [this]() { editor->hideTooltip(); };

        addAndMakeVisible(*tgt[c]);
    };
    auto makeLabel = [this](auto c, const std::string &l) {
        auto lb = std::make_unique<sst::jucegui::components::Label>();
        lb->setText(l);
        addAndMakeVisible(*lb);
        labels[c] = std::move(lb);
    };

    connectors::ConnectorFactory<attachment_t, comp::VSlider> sliderFactory(
        [this](auto &a) { this->adsrChangedFromGui(a); }, this, adsrView);
    connectors::ConnectorFactory<attachment_t, comp::Knob> knobFactory(
        [this](auto &a) { this->adsrChangedFromGui(a); }, this, adsrView);

    stowOnto(sliders, Ctrl::A, sliderFactory.attach("Attack", dma::cdAHDR, &dma::a));
    makeLabel(Ctrl::A, "A");

    stowOnto(sliders, Ctrl::H, sliderFactory.attach("Hold", dma::cdAHDR, &dma::h));
    makeLabel(Ctrl::H, "H");

    stowOnto(sliders, Ctrl::D, sliderFactory.attach("Decay", dma::cdAHDR, &dma::d));
    makeLabel(Ctrl::D, "D");

    stowOnto(sliders, Ctrl::S, sliderFactory.attach("Sustain", dma::cdS, &dma::s));
    makeLabel(Ctrl::S, "S");

    stowOnto(sliders, Ctrl::R, sliderFactory.attach("Release", dma::cdAHDR, &dma::r));
    makeLabel(Ctrl::R, "R");

    stowOnto(knobs, Ctrl::Ash, knobFactory.attach("A Shape", dma::cdShape, &dma::aShape));
    stowOnto(knobs, Ctrl::Dsh, knobFactory.attach("D Shape", dma::cdShape, &dma::dShape));
    stowOnto(knobs, Ctrl::Rsh, knobFactory.attach("R Shape", dma::cdShape, &dma::rShape));

    onHamburger = [safeThis = juce::Component::SafePointer(this)]() {
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

void AdsrPane::updateTooltip(const attachment_t &at)
{
    editor->setTooltipContents(at.label + " = " + at.description.valueToString(at.value));
}

void AdsrPane::adsrChangedFromGui(const attachment_t &at)
{
    updateTooltip(at);
    cmsg::clientSendToSerialization(cmsg::AdsrSelectedZoneUpdateRequest({index, adsrView}),
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
            knobs[Ash]->setBounds(x + (w - kh) * 0.5, y - lh, kh, kh);
            break;
        case D:
            knobs[Dsh]->setBounds(x + (w - kh) * 0.5, y - lh, kh, kh);
            break;
        case R:
            knobs[Rsh]->setBounds(x + (w - kh) * 0.5, y - lh, kh, kh);
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
    p.addItem("Coming", []() {});
    p.addItem("Soon", []() {});

    p.showMenuAsync(juce::PopupMenu::Options());
}

} // namespace scxt::ui::multi