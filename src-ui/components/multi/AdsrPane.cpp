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

#include "AdsrPane.h"
#include "components/SCXTEditor.h"

#include "messaging/client/client_serial.h"
#include "messaging/client/client_messages.h"
#include "connectors/SCXTStyleSheetCreator.h"
#include "sst/jucegui/components/Label.h"

namespace scxt::ui::multi
{
namespace cmsg = scxt::messaging::client;
using dma = datamodel::AdsrStorage;
namespace comp = sst::jucegui::components;

template <typename GZTrait>
AdsrPane<GZTrait>::AdsrPane(SCXTEditor *e, int idx, bool fz)
    : HasEditor(e), sst::jucegui::components::NamedPanel(idx == 0 ? "AMP EG" : "EG 2"), index(idx),
      forZone(fz)
{
    hasHamburger = true;

    using fac = connectors::SingleValueIndexedFactory<attachment_t, typename GZTrait::floatMsg_t>;

    fac::attachAndAdd(dma::paramAHD.withName("Attack"), adsrView, index, adsrView.a, this,
                      attachments.A, sliders.A);
    fac::attachAndAdd(dma::paramAHD.withName("Hold"), adsrView, index, adsrView.h, this,
                      attachments.H, sliders.H);
    fac::attachAndAdd(dma::paramAHD.withName("Decay"), adsrView, index, adsrView.d, this,
                      attachments.D, sliders.D);
    fac::attachAndAdd(dma::paramS.withName("Sustain"), adsrView, index, adsrView.s, this,
                      attachments.S, sliders.S);
    fac::attachAndAdd(dma::paramR.withName("Release"), adsrView, index, adsrView.r, this,
                      attachments.R, sliders.R);

    fac::attachAndAdd(dma::paramShape.withName("A Shape"), adsrView, index, adsrView.aShape, this,
                      attachments.Ash, knobs.Ash);
    fac::attachAndAdd(dma::paramShape.withName("D Shape"), adsrView, index, adsrView.dShape, this,
                      attachments.Dsh, knobs.Dsh);
    fac::attachAndAdd(dma::paramShape.withName("R Shape"), adsrView, index, adsrView.rShape, this,
                      attachments.Rsh, knobs.Rsh);

    auto makeLabel = [this](auto &lb, const std::string &l) {
        lb = std::make_unique<sst::jucegui::components::Label>();
        lb->setText(l);
        addAndMakeVisible(*lb);
    };
    makeLabel(labels.A, "A");
    makeLabel(labels.H, "H");
    makeLabel(labels.D, "D");
    makeLabel(labels.S, "S");
    makeLabel(labels.R, "R");

    for (const auto &k : knobs.members)
        if (k)
            k->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorKnob);
    for (const auto &k : sliders.members)
        if (k)
            k->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorKnob);
}

template <typename GZTrait>
void AdsrPane<GZTrait>::adsrChangedFromModel(const datamodel::AdsrStorage &d)
{
    adsrView = d;
    for (const auto &sl : sliders.members)
        if (sl)
            sl->setEnabled(true);

    for (const auto &sl : knobs.members)
        if (sl)
            sl->setEnabled(true);

    repaint();
}

template <typename GZTrait> void AdsrPane<GZTrait>::adsrDeactivated()
{
    for (const auto &sl : sliders.members)
        if (sl)
            sl->setEnabled(false);

    for (const auto &sl : knobs.members)
        if (sl)
            sl->setEnabled(false);

    repaint();
}

template <typename GZTrait> void AdsrPane<GZTrait>::resized()
{
    auto r = getContentArea();
    auto lh = 16.f;
    auto kh = 20.f;
    auto h = r.getHeight() - lh - kh;
    auto x = r.getX();
    auto y = r.getY() + kh;
    auto w = 34.f;
    x = x + (r.getWidth() - w * 5) * 0.5;

    sliders.A->setBounds(x, y, w, h);
    labels.A->setBounds(x, y + h, w, lh);
    knobs.Ash->setBounds(x + (w - kh) * 0.5, y - lh, kh, kh);
    x += w;
    sliders.H->setBounds(x, y, w, h);
    labels.H->setBounds(x, y + h, w, lh);
    x += w;
    sliders.D->setBounds(x, y, w, h);
    labels.D->setBounds(x, y + h, w, lh);
    knobs.Dsh->setBounds(x + (w - kh) * 0.5, y - lh, kh, kh);
    x += w;
    sliders.S->setBounds(x, y, w, h);
    labels.S->setBounds(x, y + h, w, lh);
    x += w;
    sliders.R->setBounds(x, y, w, h);
    labels.R->setBounds(x, y + h, w, lh);
    knobs.Rsh->setBounds(x + (w - kh) * 0.5, y - lh, kh, kh);
    x += w;
}

template <typename GZTrait> void AdsrPane<GZTrait>::showHamburgerMenu()
{
    juce::PopupMenu p;
    p.addSectionHeader("Menu");
    p.addItem("Coming", []() {});
    p.addItem("Soon", []() {});

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

template struct AdsrPane<AdsrGroupTraits>;
template struct AdsrPane<AdsrZoneTraits>;
} // namespace scxt::ui::multi