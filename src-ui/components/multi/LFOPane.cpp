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
#include "juce_gui_basics/juce_gui_basics.h"
#include "messaging/messaging.h"
#include "components/SCXTEditor.h"
#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/components/HSliderFilled.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/VSlider.h"

namespace scxt::ui::multi
{

namespace jcmp = sst::jucegui::components;

// TODO: A Million things of course
struct LfoDataRender : juce::Component
{
    LfoPane *parent{nullptr};
    LfoDataRender(LfoPane *p) : parent{p} {}

    void paint(juce::Graphics &g) override
    {
        auto bg = parent->style()->getColour(jcmp::NamedPanel::Styles::styleClass,
                                             jcmp::NamedPanel::Styles::regionBG);
        auto bgq =
            parent->style()->getColour(connectors::SCXTStyleSheetCreator::ModulationEditorVSlider,
                                       jcmp::HSliderFilled::Styles::guttercol);
        auto boxc =
            parent->style()->getColour(connectors::SCXTStyleSheetCreator::ModulationEditorVSlider,
                                       jcmp::HSliderFilled::Styles::backgroundcol);
        auto valc =
            parent->style()->getColour(connectors::SCXTStyleSheetCreator::ModulationEditorVSlider,
                                       jcmp::HSliderFilled::Styles::valcol);

        auto hanc =
            parent->style()->getColour(connectors::SCXTStyleSheetCreator::ModulationEditorVSlider,
                                       jcmp::HSliderFilled::Styles::handlecol);

        auto hanhovc =
            parent->style()->getColour(connectors::SCXTStyleSheetCreator::ModulationEditorVSlider,
                                       jcmp::HSliderFilled::Styles::handlehovcol);
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
            g.setColour(i % 2 == 0 ? bg : bgq);
            g.fillRect(bx);

            auto d = ls.data[i];

            if (d > 0)
            {
                g.setColour(valc);
                auto r = bx.withTrimmedTop((1.f - d) * hm).withBottom(hm).reduced(0.5, 0);
                g.fillRect(r);

                g.setColour(hanc);
                auto rh = bx.withTrimmedTop((1.f - d) * hm)
                              .withHeight(1)
                              .reduced(0.5, 0)
                              .translated(0, -0.5);
                g.fillRect(rh);
            }
            else
            {
                g.setColour(valc);
                auto r = bx.withTop(hm).withTrimmedBottom((1.f + d) * hm).reduced(0.5, 0);
                g.fillRect(r);
                g.setColour(hanc);
                auto rh = bx.withTrimmedBottom((1.f + d) * hm);
                rh = rh.withTrimmedTop(rh.getHeight() - 1).reduced(0.5, 0).translated(0, -0.5);
                g.fillRect(rh);
            }

            bx = bx.translated(w, 0);
        }
        g.setColour(boxc);
        g.drawRect(getLocalBounds());
    }

    int indexForPosition(const juce::Point<float> &f)
    {
        if (!getLocalBounds().toFloat().contains(f))
            return -1;
        if (!parent)
            return -1;

        int sp = modulation::modulators::stepLfoSteps;
        auto &ls = parent->lfoData[parent->selectedTab];
        auto w = getWidth() * 1.f / ls.repeat;

        auto idx = std::clamp((int)std::floor(f.x / w), 0, sp);
        return idx;
    }
    void handleMouseAt(const juce::Point<float> &f)
    {
        auto idx = indexForPosition(f);
        if (idx < 0)
            return;

        auto d = (1 - f.y / getHeight()) * 2 - 1;
        parent->lfoData[parent->selectedTab].data[idx] = d;
        parent->pushCurrentLfoUpdate();
    }
    void mouseDown(const juce::MouseEvent &event) override { handleMouseAt(event.position); }
    void mouseDrag(const juce::MouseEvent &event) override { handleMouseAt(event.position); }
    void mouseDoubleClick(const juce::MouseEvent &event) override
    {
        auto idx = indexForPosition(event.position);
        if (idx < 0)
            return;
        parent->lfoData[parent->selectedTab].data[idx] = 0.f;
        parent->pushCurrentLfoUpdate();
    }
}; // namespace juce::Component

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
                w->updateValueTooltip(a);
                w->repaint();
            }
        };
    };

    auto updateNoTT = [r = juce::Component::SafePointer(this)]() {
        return [w = juce::Component::SafePointer(r)](const auto &a) {
            if (w)
            {
                w->pushCurrentLfoUpdate();
                w->repaint();
            }
        };
    };
    oneshotA =
        std::make_unique<boolAttachment_t>("OneShot", updateNoTT(), lfoData[selectedTab].onlyonce);
    tempoSyncA = std::make_unique<boolAttachment_t>("TempoSync", updateNoTT(),
                                                    lfoData[selectedTab].temposync);
    cycleA =
        std::make_unique<boolAttachment_t>("OneShot", updateNoTT(), lfoData[selectedTab].cyclemode);

    rateA = std::make_unique<attachment_t>(datamodel::lfoModulationRate().withName("Rate"),
                                           update(), lfoData[selectedTab].rate);
    deformA = std::make_unique<attachment_t>(datamodel::lfoSmoothing().withName("Deform"), update(),
                                             lfoData[selectedTab].smooth);

    static constexpr int columnOneWidth{60};
    auto r = getContentArea();
    auto col = r.withWidth(columnOneWidth).withHeight(20);

    oneshotB = std::make_unique<sst::jucegui::components::ToggleButton>();
    oneshotB->setSource(oneshotA.get());
    oneshotB->setLabel("OneShot");
    oneshotB->setBounds(col);
    addAndMakeVisible(*oneshotB);

    // TODO - a cleaner way to do this copy and paste
    auto b5 = r.withTop(r.getBottom() - 50).withLeft(r.getWidth() - 50);
    rateK = std::make_unique<sst::jucegui::components::Knob>();
    rateK->setSource(rateA.get());
    rateK->setBounds(b5.translated(-2 * 50, 0));
    rateK->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorKnob);
    setupWidgetForValueTooltip(rateK, rateA);
    addAndMakeVisible(*rateK);

    deformK = std::make_unique<sst::jucegui::components::Knob>();
    deformK->setSource(deformA.get());
    deformK->setBounds(b5.translated(-50, 0));
    deformK->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorKnob);
    setupWidgetForValueTooltip(deformK, deformA);
    addAndMakeVisible(*deformK);

    lfoDataRender = std::make_unique<LfoDataRender>(this);
    lfoDataRender->setBounds(r.withTrimmedLeft(columnOneWidth + 5).withTrimmedBottom(55));
    addAndMakeVisible(*lfoDataRender);

    jog[0] = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::JOG_UP);
    jog[0]->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->shiftBy(0.05);
    });

    jog[1] = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::JOG_DOWN);
    jog[1]->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->shiftBy(-0.05);
    });
    jog[2] = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::JOG_LEFT);
    jog[2]->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->rotate(-1);
    });
    jog[3] = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::JOG_RIGHT);
    jog[3]->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->rotate(1);
    });

    auto jogbx =
        r.withTrimmedLeft(columnOneWidth + 5).withTop(getHeight() - 55).withWidth(55).reduced(2);

    for (const auto &j : jog)
    {
        addAndMakeVisible(*j);
        j->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationMatrixMenu);
    }
    auto bthrd = jogbx.getWidth() / 3;
    std::array<juce::Rectangle<float>, 4> boxes;
    auto rc = jogbx.toFloat().withWidth(bthrd);
    jog[2]->setBounds(rc.withHeight(bthrd).withCentre(rc.getCentre()).toNearestInt());
    rc = rc.translated(bthrd, 0);
    jog[0]->setBounds(rc.withHeight(bthrd).toNearestInt());
    jog[1]->setBounds(rc.withHeight(bthrd).translated(0, 2 * bthrd).toNearestInt());

    rc = rc.translated(bthrd, 0);
    jog[3]->setBounds(rc.withHeight(bthrd).withCentre(rc.getCentre()).toNearestInt());
}

void LfoPane::rotate(int dir)
{
    auto &ls = lfoData[selectedTab];

    if (dir == -1)
    {
        auto p0 = ls.data[0];
        for (int i = 0; i < ls.repeat; ++i)
        {
            ls.data[i] = (i == ls.repeat - 1 ? p0 : ls.data[i + 1]);
        }
    }
    else
    {
        auto p0 = ls.data[ls.repeat - 1];
        for (int i = ls.repeat - 1; i >= 0; --i)
        {
            ls.data[i] = (i == 0 ? p0 : ls.data[i - 1]);
        }
    }
    pushCurrentLfoUpdate();
}

void LfoPane::shiftBy(float amt)
{
    auto &ls = lfoData[selectedTab];

    for (int i = 0; i < ls.repeat; ++i)
    {
        ls.data[i] = std::clamp(ls.data[i] + amt, -1.f, 1.f);
    }

    pushCurrentLfoUpdate();
}

namespace cmsg = scxt::messaging::client;

void LfoPane::pushCurrentLfoUpdate()
{
    sendToSerialization(cmsg::IndexedLfoUpdated({true, selectedTab, lfoData[selectedTab]}));
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

    for (auto &j : jog)
        j.reset();
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
            modulation::modulators::load_lfo_preset(lp, ld, wt->editor->rngGen);
            wt->pushCurrentLfoUpdate();
        });
    }
    m.showMenuAsync(editor->defaultPopupMenuOptions());
}
} // namespace scxt::ui::multi