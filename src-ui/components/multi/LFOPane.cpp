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

#include "LFOPane.h"
#include "connectors/SCXTStyleSheetCreator.h"
#include "messaging/messaging.h"
#include "components/SCXTEditor.h"

namespace scxt::ui::multi
{

LfoPane::LfoPane(SCXTEditor *e) : sst::jucegui::components::NamedPanel(""), HasEditor(e)
{
    setCustomClass(connectors::SCXTStyleSheetCreator::ModulationTabs);
    hasHamburger = true;
    isTabbed = true;
    tabNames = {"LFO 1", "LFO 2", "LFO 3"};

    resetTabState();

    onTabSelected = [wt = juce::Component::SafePointer(this)](int i) {
        if (wt)
            wt->tabChanged(i);
    };
    onHamburger = [wt = juce::Component::SafePointer(this)] {
        if (wt)
            wt->pickPresets();
    };
    setEnabled(false);
}

LfoPane::~LfoPane()
{
    removeAllChildren();
    resetAllComponents();
}

void LfoPane::resized()
{
    removeAllChildren();
    resetAllComponents();
    if (isEnabled() && getWidth() > 10 && isEnabled())
        rebuildLfo();
}

void LfoPane::tabChanged(int i)
{
    removeAllChildren();
    resetAllComponents();
    rebuildLfo();
}

void LfoPane::setActive(int i, bool b)
{
    setEnabled(b);
    if (!b)
    {
        removeAllChildren();
        resetAllComponents();
    }
}

void LfoPane::setLfo(int index, const modulation::modulators::StepLFOStorage &lfo)
{
    lfoData[index] = lfo;

    if (index != selectedTab)
        return;

    removeAllChildren();
    resetAllComponents();

    if (!isEnabled())
        return;

    if (selectedTab == index)
    {
        rebuildLfo();
    }
}

void LfoPane::rebuildLfo()
{
    if (!isEnabled())
        return;

    auto update = [this]() {
        return [w = juce::Component::SafePointer(this)](const auto &a) {
            if (w)
                w->pushCurrentLfoUpdate();
        };
    };
    oneshotA = std::make_unique<boolAttachment_t>(
        this, "OneShot", update(), [](const auto &pl) { return pl.onlyonce; },
        lfoData[selectedTab].onlyonce);
    tempoSyncA = std::make_unique<boolAttachment_t>(
        this, "TempoSync", update(), [](const auto &pl) { return pl.temposync; },
        lfoData[selectedTab].temposync);
    cycleA = std::make_unique<boolAttachment_t>(
        this, "OneShot", update(), [](const auto &pl) { return pl.cyclemode; },
        lfoData[selectedTab].cyclemode);

    rateA = std::make_unique<attachment_t>(
        this, datamodel::cdTimeUnscaledThirtyTwo, "Rate", update(),
        [](const auto &pl) { return pl.rate; }, lfoData[selectedTab].rate);
    deformA = std::make_unique<attachment_t>(
        this, datamodel::cdPercentBipolar, "Deform", update(),
        [](const auto &pl) { return pl.smooth; }, lfoData[selectedTab].smooth);

    static constexpr int columnOneWidth{60};
    auto r = getContentArea();
    auto col = r.withWidth(columnOneWidth).withHeight(20);

    oneshotB = std::make_unique<sst::jucegui::components::ToggleButton>();
    oneshotB->setSource(oneshotA.get());
    oneshotB->setLabel("OneShot");
    oneshotB->setBounds(col);
    addAndMakeVisible(*oneshotB);

    auto b5 = r.withTop(r.getBottom() - 50).withLeft(r.getWidth() - 50);
    rateK = std::make_unique<sst::jucegui::components::Knob>();
    rateK->setSource(rateA.get());
    rateK->setBounds(b5.translated(-2 * 50, 0));
    addAndMakeVisible(*rateK);

    deformK = std::make_unique<sst::jucegui::components::Knob>();
    deformK->setSource(deformA.get());
    deformK->setBounds(b5.translated(-50, 0));
    addAndMakeVisible(*deformK);
}

namespace cmsg = scxt::messaging::client;

void LfoPane::pushCurrentLfoUpdate()
{
    cmsg::clientSendToSerialization(
        cmsg::IndexedLfoUpdated({true, selectedTab, lfoData[selectedTab]}), editor->msgCont);
}

void LfoPane::resetAllComponents()
{
    oneshotB.reset();
    tempoSyncB.reset();
    cycleB.reset();
    rateK.reset();
    deformK.reset();
    stepsK.reset();
}

void LfoPane::pickPresets()
{
    // TODO: THIS IS ALL GARBAGE CODE FIXME
    auto m = juce::PopupMenu();
    m.addSectionHeader("Presets (SUPER ROUGH)");
    m.addSeparator();
    for (int p = modulation::modulators::LFOPresets::lp_clear;
         p < modulation::modulators::LFOPresets::n_lfopresets; ++p)
    {
        auto lp = (modulation::modulators::LFOPresets)p;
        m.addItem("Preset " + std::to_string(p), [wt = juce::Component::SafePointer(this), lp]() {
            if (!wt)
                return;
            auto &ld = wt->lfoData[wt->selectedTab];
            modulation::modulators::load_lfo_preset(lp, &ld);
            wt->pushCurrentLfoUpdate();
        });
    }
    m.showMenuAsync(juce::PopupMenu::Options());
}

} // namespace scxt::ui::multi