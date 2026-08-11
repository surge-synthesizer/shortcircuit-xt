/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "AdsrPane.h"
#include "app/SCXTEditor.h"
#include "messaging/client/group_or_zone_messages.h"
#include "app/edit-screen/EditScreen.h"
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

    rebuildPanelComponents(idx);
    hasHamburger = true;
    onHamburger = [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showHamburgerMenu();
    };

    if (forZone)
    {
        if (idx == 1)
        {
            isTabbed = true;
            tabNames = {"EG2", "EG3", "EG4", "EG5"};
            onTabSelected = [w = juce::Component::SafePointer(this)](int nt) {
                if (!w)
                    return;
                w->tabChanged(nt, true);
            };
            tabChanged(0, false);
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

    updateForGateMode();
    repaint();
}

void AdsrPane::adsrChangedFromModel(const modulation::modulators::AdsrStorage &d, int cacheIdx)
{
    zoneAdsrCache[cacheIdx - 1] = d;
    if (cacheIdx - 1 == selectedTab)
    {
        adsrView = d;
        updateForGateMode();
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

void AdsrPane::tabChanged(int newIndex, bool updateState)
{
    assert(newIndex < zoneAdsrCache.size());
    assert(forZone);

    // We need to preserve our local cache
    zoneAdsrCache[displayedTabIndex] = adsrView;
    displayedTabIndex = newIndex;
    adsrView = zoneAdsrCache[newIndex];
    updateForGateMode();

    getContentAreaComponent()->removeAllChildren();
    rebuildPanelComponents(newIndex + 1);

    if (updateState)
    {
        // Handle group case even though we dont use it today
        auto kn = std::string("multi") + (forZone ? ".zone.eg" : ".group.eg");
        editor->setTabSelection(editor->editScreen->tabKey(kn), std::to_string(newIndex));
    }

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
        if (a->description.canTemposync)
            setAttachmentAsTemposync(*a);
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
        // no widget for this one - it is a hamburger item
        gateToggleA =
            bfac::attachOnly(adsrView, adsrView.gateGroupEGOnAnyPlaying, this, forZone, index);
        assert(gateToggleA);
    }

    {
        // blanket temposync toggle in the panel header (mirrors the LFO metronome)
        using tsfac = connectors::BooleanSingleValueFactory<boolAttachment_t,
                                                            cmsg::UpdateZoneOrGroupEGBoolValue>;
        std::unique_ptr<comp::ToggleButton> tsb;
        tsfac::attach(adsrView, adsrView.isTemposync, this, tempoSyncA, tsb, forZone, useIdx);
        tsb->setDrawMode(comp::ToggleButton::DrawMode::GLYPH);
        tsb->setGlyph(comp::GlyphPainter::METRONOME);
        setupFloatWidget(tsb.get(), tempoSyncA);
        clearAdditionalHamburgerComponents();
        addAdditionalHamburgerComponent(std::move(tsb));
    }

    if (forZone)
    {
        editor->themeApplier.applyZoneMultiScreenModulationTheme(this);
    }
    else
    {
        editor->themeApplier.applyGroupMultiScreenModulationTheme(this);
    }

    updateForGateMode();

    resized();
}

void AdsrPane::updateForGateMode()
{
    using gm_t = modulation::modulators::AdsrStorage::GateMode;
    switch (adsrView.gateMode)
    {
    case gm_t::ONESHOT:
    case gm_t::SEMI_GATED:
        attachments.S->labelOverride = "Breakpoint";
        labels.S->setText("B");
        break;
    default:
        attachments.S->labelOverride = std::nullopt;
        labels.S->setText("S");
        break;
    }

    /*
     * The zone AEG is the one envelope whose gate mode changes what the whole zone does -
     * it is what makes a zone a one shot - so it wears the mode in its title. The group EGs
     * and the zone EG2..5 are plain modulators and keep their names.
     */
    if (forZone && index == 0)
    {
        switch (adsrView.gateMode)
        {
        case gm_t::ONESHOT:
            setName("AMP EG (OneShot)");
            break;
        case gm_t::SEMI_GATED:
            setName("AMP EG (SemiGated)");
            break;
        case gm_t::SAMPLE_GATED:
            setName("AMP EG (SampleGated)");
            break;
        case gm_t::GATED:
            setName("AMP EG");
            break;
        }
    }
}

void AdsrPane::setGateMode(modulation::modulators::AdsrStorage::GateMode gm)
{
    adsrView.gateMode = gm;
    updateForGateMode();
    repaint();
    sendToSerialization(cmsg::UpdateFullAdsrStorageForGroupsOrZones(
        {forZone, (int)(forZone && index != 0 ? displayedTabIndex + 1 : index), adsrView}));
}

void AdsrPane::resized()
{
    NamedPanel::resized();
    auto r = getContentArea();
    getContentAreaComponent()->setBounds(r);

    auto lh = 16.f;
    auto kh = 20.f;
    auto h = r.getHeight() - lh - kh;
    auto x = 0;  // r.getX() * 1.f;
    auto y = kh; // r.getY() + kh;
    auto w = 35.f;
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
    x += w;
    sliders.R->setBounds(x, y, w, h);
    labels.R->setBounds(x, y + h, w, lh);
    knobs.Rsh->setBounds(x + (w - kh) * 0.5, y - lh, kh, kh);
}

void AdsrPane::showHamburgerMenu()
{
    using gm_t = modulation::modulators::AdsrStorage::GateMode;

    juce::PopupMenu p;
    if (forZone)
    {
        if (index == 0)
            p.addSectionHeader("Amp EG");
        else
            p.addSectionHeader("EG " + std::to_string(displayedTabIndex + 2));
    }
    else
    {
        p.addSectionHeader("Group EG " + std::to_string(index + 1));
    }
    p.addSeparator();

    auto addGateMode = [&p, this](gm_t gm, const std::string &n) {
        p.addItem(n, true, adsrView.gateMode == gm, [gm, w = juce::Component::SafePointer(this)]() {
            if (w)
                w->setGateMode(gm);
        });
    };
    addGateMode(gm_t::GATED, "Gated (DAHDSR)");
    addGateMode(gm_t::SEMI_GATED, "No Sustain");
    addGateMode(gm_t::ONESHOT, "Oneshot");

    if (forZone)
        addGateMode(gm_t::SAMPLE_GATED, "Sample Gated");

    if (!forZone)
    {
        p.addSeparator();
        p.addItem("Gate on Voice Sounding", true, adsrView.gateGroupEGOnAnyPlaying,
                  [w = juce::Component::SafePointer(this)]() {
                      if (!w)
                          return;
                      if (!w->gateToggleA)
                          return;

                      w->gateToggleA->setValueFromGUI(!w->adsrView.gateGroupEGOnAnyPlaying);
                      w->repaint();
                  });
    }

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

} // namespace scxt::ui::app::edit_screen