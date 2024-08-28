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
#include "app/SCXTEditor.h"
#include "sst/jucegui/components/Label.h"

namespace scxt::ui::app::edit_screen
{
namespace cmsg = scxt::messaging::client;
namespace comp = sst::jucegui::components;

AdsrPane::AdsrPane(SCXTEditor *e, int idx, bool fz)
    : HasEditor(e), sst::jucegui::components::NamedPanel(idx == 0 ? "AMP EG" : "EG 2"), index(idx),
      forZone(fz)
{
    hasHamburger = true;

    using fac = connectors::SingleValueFactory<attachment_t, cmsg::UpdateZoneOrGroupEGFloatValue>;

    // c++ partial application is a bummer
    auto attc = [&](auto &t, auto &a, auto &w) {
        fac::attachAndAdd(adsrView, t, this, a, w, forZone, index);
    };
    attc(adsrView.a, attachments.A, sliders.A);
    attc(adsrView.h, attachments.H, sliders.H);
    attc(adsrView.d, attachments.D, sliders.D);
    attc(adsrView.s, attachments.S, sliders.S);
    attc(adsrView.r, attachments.R, sliders.R);

    attc(adsrView.aShape, attachments.Ash, knobs.Ash);
    attc(adsrView.dShape, attachments.Dsh, knobs.Dsh);
    attc(adsrView.rShape, attachments.Rsh, knobs.Rsh);

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

    if (forZone)
    {
        editor->themeApplier.applyZoneMultiScreenModulationTheme(this);
    }
    else
    {
        editor->themeApplier.applyGroupMultiScreenModulationTheme(this);
    }
}

void AdsrPane::adsrChangedFromModel(const modulation::modulators::AdsrStorage &d)
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

void AdsrPane::adsrDeactivated()
{
    for (const auto &sl : sliders.members)
        if (sl)
            sl->setEnabled(false);

    for (const auto &sl : knobs.members)
        if (sl)
            sl->setEnabled(false);

    repaint();
}

void AdsrPane::resized()
{
    auto r = getContentArea();
    auto lh = 16.f;
    auto kh = 20.f;
    auto h = r.getHeight() - lh - kh;
    auto x = r.getX() * 1.f;
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
}

void AdsrPane::showHamburgerMenu()
{
    juce::PopupMenu p;
    p.addSectionHeader("Menu");
    p.addItem("Coming", []() {});
    p.addItem("Soon", []() {});

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

} // namespace scxt::ui::app::edit_screen