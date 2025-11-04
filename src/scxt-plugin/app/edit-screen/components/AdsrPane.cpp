/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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
#include "sst/jucegui/components/ToggleButton.h"

namespace scxt::ui::app::edit_screen
{
namespace cmsg = scxt::messaging::client;
namespace comp = sst::jucegui::components;

AdsrPane::AdsrPane(SCXTEditor *e, int idx, bool fz)
    : HasEditor(e), sst::jucegui::components::NamedPanel(idx == 0 ? "AMP EG" : "EG 2"), index(idx),
      forZone(fz)
{
    setContentAreaComponent(std::make_unique<juce::Component>());

    hasHamburger = hasFeature::alternateEGModes;

    rebuildPanelComponents(idx);

    if (forZone)
    {
        if (idx == 1)
        {
            isTabbed = true;
            tabNames = {"EG2", "EG3", "EG4", "EG5"};
            onTabSelected = [w = juce::Component::SafePointer(this)](int nt) {
                if (!w)
                    return;
                w->tabChanged(nt);
            };
            tabChanged(0);
        }
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

void AdsrPane::adsrChangedFromModel(const modulation::modulators::AdsrStorage &d, int cacheIdx)
{
    zoneAdsrCache[cacheIdx - 1] = d;
    if (cacheIdx - 1 == selectedTab)
    {
        adsrView = d;
    }
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

void AdsrPane::tabChanged(int newIndex)
{
    assert(newIndex < zoneAdsrCache.size());
    assert(forZone);

    // We need to preserve our local cache
    zoneAdsrCache[displayedTabIndex] = adsrView;
    displayedTabIndex = newIndex;
    adsrView = zoneAdsrCache[newIndex];

    getContentAreaComponent()->removeAllChildren();
    rebuildPanelComponents(newIndex + 1);

    repaint();
}

void AdsrPane::rebuildPanelComponents(int useIdx)
{
    using fac = connectors::SingleValueFactory<attachment_t, cmsg::UpdateZoneOrGroupEGFloatValue>;
    using bfac =
        connectors::BooleanSingleValueFactory<boolAttachment_t, cmsg::UpdateZoneOrGroupEGBoolValue>;

    // c++ partial application is a bummer
    auto attc = [&](auto &t, auto &a, auto &w) {
        fac::attach(adsrView, t, this, a, w, forZone, useIdx);
        getContentAreaComponent()->addAndMakeVisible(*w);
    };
    attc(adsrView.dly, attachments.dly, sliders.dly);
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
        getContentAreaComponent()->addAndMakeVisible(*lb);
    };
    makeLabel(labels.dly, "Dly");
    makeLabel(labels.A, "A");
    makeLabel(labels.H, "H");
    makeLabel(labels.D, "D");
    makeLabel(labels.S, "S");
    makeLabel(labels.R, "R");

    if (!forZone)
    {
        gateToggleA =
            bfac::attachOnly(adsrView, adsrView.gateGroupEGOnAnyPlaying, this, forZone, index);
        assert(gateToggleA);
        gateToggle = std::make_unique<sst::jucegui::components::ToggleButton>();
        gateToggle->setDrawMode(sst::jucegui::components::ToggleButton::DrawMode::LABELED);
        gateToggle->setLabel(u8"\xE2\x88\x80");
        gateToggle->setSource(gateToggleA.get());
        setupWidgetForValueTooltip(gateToggle.get(), gateToggleA.get());
        getContentAreaComponent()->addAndMakeVisible(*gateToggle);
    }

    if (forZone)
    {
        editor->themeApplier.applyZoneMultiScreenModulationTheme(this);
    }
    else
    {
        editor->themeApplier.applyGroupMultiScreenModulationTheme(this);
    }

    resized();
}

void AdsrPane::resized()
{
    auto r = getContentArea();
    getContentAreaComponent()->setBounds(r);

    auto lh = 16.f;
    auto kh = 20.f;
    auto h = r.getHeight() - lh - kh;
    auto x = 0;  // r.getX() * 1.f;
    auto y = kh; // r.getY() + kh;
    auto w = 31.f;
    x = x + (r.getWidth() - w * 6) * 0.5;

    sliders.dly->setBounds(x, y, w, h);
    labels.dly->setBounds(x, y + h, w, lh);
    x += w;

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
    if (!forZone && gateToggle)
    {
        gateToggle->setBounds(x + (w - kh) * 0.5, y - lh, kh, kh - 3);
    }
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