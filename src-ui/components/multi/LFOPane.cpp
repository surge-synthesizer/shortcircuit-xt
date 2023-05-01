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
#include "datamodel/parameter.h"
#include "messaging/messaging.h"
#include "components/SCXTEditor.h"

namespace scxt::ui::multi
{

// TODO: A Million things of course
struct LfoDataRender : juce::Component
{
    LfoPane *parent{nullptr};
    LfoDataRender(LfoPane *p) : parent{p} {}

    void paint(juce::Graphics &g) override
    {
        g.setColour(juce::Colours::white);
        g.drawRect(getLocalBounds(), 1);
        if (!parent)
            return;

        int sp = modulation::modulators::stepLfoSteps;
        auto &ls = parent->lfoData[parent->selectedTab];
        auto w = getWidth() * 1.f / ls.repeat;
        auto bx = getLocalBounds().toFloat().withWidth(w);
        auto hm = bx.getHeight() * 0.5;
        for (int i = 0; i < ls.repeat; ++i)
        {
            g.setColour(juce::Colours::darkblue);
            g.fillRect(bx);

            auto d = ls.data[i];

            if (d > 0)
            {
                g.setColour(juce::Colours::white);
                auto r = bx.withTrimmedTop((1.f - d) * hm).withBottom(hm);
                g.fillRect(r);
            }
            else
            {
                g.setColour(juce::Colours::white);
                auto r = bx.withTop(hm).withTrimmedBottom((1.f + d) * hm);
                g.fillRect(r);
            }

            g.setColour(juce::Colours::blue.brighter(0.4));
            g.drawRect(bx, 1);
            bx = bx.translated(w, 0);
        }
    }

    void handleMouseAt(const juce::Point<float> &f)
    {
        if (!getLocalBounds().toFloat().contains(f))
            return;
        if (!parent)
            return;

        int sp = modulation::modulators::stepLfoSteps;
        auto &ls = parent->lfoData[parent->selectedTab];
        auto w = getWidth() * 1.f / ls.repeat;

        auto idx = std::clamp((int)std::floor(f.x / w), 0, sp);
        auto d = (1 - f.y / getHeight()) * 2 - 1;
        parent->lfoData[parent->selectedTab].data[idx] = d;
        parent->pushCurrentLfoUpdate();
    }
    void mouseDown(const juce::MouseEvent &event) override { handleMouseAt(event.position); }
    void mouseDrag(const juce::MouseEvent &event) override { handleMouseAt(event.position); }
};

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

    auto update = [r = juce::Component::SafePointer(this)]() {
        return [w = juce::Component::SafePointer(r)](const auto &a) {
            if (w)
            {
                w->pushCurrentLfoUpdate();
                w->repaint();
            }
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
        this, datamodel::lfoModulationRate().withName("Rate"), update(),
        [](const auto &pl) { return pl.rate; }, lfoData[selectedTab].rate);
    deformA = std::make_unique<attachment_t>(
        this, datamodel::lfoSmoothing().withName("Deform"), update(),
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

    lfoDataRender = std::make_unique<LfoDataRender>(this);
    lfoDataRender->setBounds(r.withTrimmedLeft(columnOneWidth + 5).withTrimmedBottom(55));
    addAndMakeVisible(*lfoDataRender);
}

namespace cmsg = scxt::messaging::client;

void LfoPane::pushCurrentLfoUpdate()
{
    cmsg::clientSendToSerialization(
        cmsg::IndexedLfoUpdated({true, selectedTab, lfoData[selectedTab]}), editor->msgCont);

    repaint();
}

void LfoPane::resetAllComponents()
{
    oneshotB.reset();
    tempoSyncB.reset();
    cycleB.reset();
    rateK.reset();
    deformK.reset();
    stepsK.reset();
    lfoDataRender.reset();
}

void LfoPane::pickPresets()
{
    // TODO: it is safe to get this data in the UI thread but I should
    // really send it across as metadata soon!
    auto m = juce::PopupMenu();
    m.addSectionHeader("Presets (SUPER ROUGH)");
    m.addSeparator();
    for (int p = modulation::modulators::LFOPresets::lp_clear;
         p < modulation::modulators::LFOPresets::n_lfopresets; ++p)
    {
        auto lp = (modulation::modulators::LFOPresets)p;
        auto nm = modulation::modulators::getLfoPresetName(lp);
        m.addItem(nm, [wt = juce::Component::SafePointer(this), lp]() {
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