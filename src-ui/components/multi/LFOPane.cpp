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

#include "LFOPane.h"

#include <cmath>

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

// Included so we can have UI-thread exceution for curve rendering
#include "modulation/modulators/steplfo.h"

namespace scxt::ui::multi
{

namespace jcmp = sst::jucegui::components;
namespace cmsg = scxt::messaging::client;

struct StepLFOPane : juce::Component
{
    struct StepRender : juce::Component
    {
        LfoPane *parent{nullptr};
        StepRender(LfoPane *p) : parent{p} { recalcCurve(); }

        void paint(juce::Graphics &g) override
        {
            auto bg = parent->style()->getColour(jcmp::NamedPanel::Styles::styleClass,
                                                 jcmp::NamedPanel::Styles::background);
            auto bgq = parent->style()->getColour(
                connectors::SCXTStyleSheetCreator::ModulationEditorVSlider,
                jcmp::HSliderFilled::Styles::gutter);
            auto boxc = parent->style()->getColour(jcmp::NamedPanel::Styles::styleClass,
                                                   jcmp::NamedPanel::Styles::brightoutline);
            auto valc = parent->style()->getColour(
                connectors::SCXTStyleSheetCreator::ModulationEditorVSlider,
                jcmp::HSliderFilled::Styles::value);
            auto valhovc = parent->style()->getColour(
                connectors::SCXTStyleSheetCreator::ModulationEditorVSlider,
                jcmp::HSliderFilled::Styles::value_hover);

            auto hanc = parent->style()->getColour(
                connectors::SCXTStyleSheetCreator::ModulationEditorVSlider,
                jcmp::HSliderFilled::Styles::handle);

            auto hanhovc = parent->style()->getColour(
                connectors::SCXTStyleSheetCreator::ModulationEditorVSlider,
                jcmp::HSliderFilled::Styles::handle_hover);
            g.setColour(juce::Colours::white);
            g.drawRect(getLocalBounds(), 1);
            if (!parent)
                return;

            int sp = modulation::modulators::StepLFOStorage::stepLfoSteps;
            auto &ls = parent->modulatorStorageData[parent->selectedTab].stepLfoStorage;
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

            g.setColour(hanc.brighter(0.3));
            auto yscal = -getHeight() * 0.5;
            auto p = juce::Path();
            bool first{true};
            for (auto &[xf, yf] : cycleCurve)
            {
                auto yn = yf * yscal - yscal;
                auto xn = xf * getWidth();
                if (first)
                {
                    p.startNewSubPath(xn, yn);
                }
                else
                {
                    p.lineTo(xn, yn);
                }
                first = false;
            }
            g.strokePath(p, juce::PathStrokeType(2.0));

            g.setColour(boxc);
            g.drawRect(getLocalBounds());
        }

        int indexForPosition(const juce::Point<float> &f)
        {
            if (!getLocalBounds().toFloat().contains(f))
                return -1;
            if (!parent)
                return -1;

            int sp = modulation::modulators::StepLFOStorage::stepLfoSteps;
            auto &ls = parent->modulatorStorageData[parent->selectedTab].stepLfoStorage;
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
            parent->modulatorStorageData[parent->selectedTab].stepLfoStorage.data[idx] = d;
            recalcCurve();
            parent->pushCurrentModulatorStorageUpdate();
        }
        void mouseDown(const juce::MouseEvent &event) override { handleMouseAt(event.position); }
        void mouseDrag(const juce::MouseEvent &event) override { handleMouseAt(event.position); }
        void mouseDoubleClick(const juce::MouseEvent &event) override
        {
            auto idx = indexForPosition(event.position);
            if (idx < 0)
                return;
            parent->modulatorStorageData[parent->selectedTab].stepLfoStorage.data[idx] = 0.f;
            recalcCurve();
            parent->pushCurrentModulatorStorageUpdate();
        }

        std::vector<std::pair<float, float>> cycleCurve;
        void recalcCurve()
        {
            cycleCurve.clear();

            // explicitly make a copy here so we can screw with it
            auto ms = parent->modulatorStorageData[parent->selectedTab];
            ms.stepLfoStorage.rateIsForSingleStep = true; // rate specifies one step

            auto so = std::make_unique<scxt::modulation::modulators::StepLFO>();
            // always render display at 48k so we can reason about output density
            float renderSR{48000};
            so->setSampleRate(renderSR);
            float rate{3.f}; // 8 steps a second
            float stepSamples{renderSR / std::pow(2.0f, rate) /
                              blockSize}; // how manu samples in a step
            int captureEvery{(int)(stepSamples / (ms.stepLfoStorage.repeat * 10))};
            scxt::datamodel::TimeData td{};
            scxt::infrastructure::RNGGen gen;
            so->assign(&ms, &rate, &td, gen);

            so->UpdatePhaseIncrement();
            for (int i = 0; i < stepSamples * ms.stepLfoStorage.repeat; ++i)
            {
                if (i % captureEvery == 0)
                {
                    cycleCurve.emplace_back((float)i / (stepSamples * ms.stepLfoStorage.repeat),
                                            so->output);
                }
                so->process(blockSize);
            }
            auto bk = cycleCurve.back();
        }
    }; // namespace juce::Component

    std::unique_ptr<StepRender> stepRender;

    LfoPane *parent{nullptr};
    StepLFOPane(LfoPane *p) : parent(p)
    {
        stepRender = std::make_unique<StepRender>(parent);
        addAndMakeVisible(*stepRender);

        auto update = [r = juce::Component::SafePointer(this)]() {
            return [w = juce::Component::SafePointer(r)](const auto &a) {
                if (w && w->parent)
                {
                    auto p = w->parent;
                    p->pushCurrentModulatorStorageUpdate();
                    p->updateValueTooltip(a);
                    w->stepRender->recalcCurve();
                    p->repaint();
                }
            };
        };

        typedef LfoPane::attachment_t attachment_t;

        auto &ms = parent->modulatorStorageData[parent->selectedTab];
        auto &ls = ms.stepLfoStorage;

        stepsA = std::make_unique<LfoPane::intAttachment_t>(
            datamodel::pmd()
                .asInt()
                .withLinearScaleFormatting("")
                .withRange(1, modulation::modulators::StepLFOStorage::stepLfoSteps)
                .withDefault(16)
                .withName("Step Count"),
            update(), ls.repeat);
        stepsJ = std::make_unique<jcmp::JogUpDownButton>();
        stepsJ->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationJogButon);

        stepsJ->setSource(stepsA.get());
        addAndMakeVisible(*stepsJ);

        cycleA =
            std::make_unique<LfoPane::boolBaseAttachment_t>(datamodel::pmd()
                                                                .asBool()
                                                                .withName("Cycle")
                                                                .withCustomMinDisplay("RATE: CYCLE")
                                                                .withCustomMaxDisplay("RATE: STEP"),
                                                            update(), ls.rateIsForSingleStep);
        cycleB = std::make_unique<jcmp::ToggleButton>();
        cycleB->setSource(cycleA.get());
        cycleB->setDrawMode(sst::jucegui::components::ToggleButton::DrawMode::LABELED_BY_DATA);
        cycleB->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationMatrixToggle);
        addAndMakeVisible(*cycleB);

        rateA = std::make_unique<attachment_t>(datamodel::lfoModulationRate().withName("Rate"),
                                               update(), ms.rate);
        auto mk = [this](auto &a, auto b) {
            auto L = std::make_unique<jcmp::Label>();
            L->setText(b);
            addAndMakeVisible(*L);
            auto K = std::make_unique<jcmp::Knob>();
            K->setSource(a.get());
            K->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorKnob);
            parent->setupWidgetForValueTooltip(K, a);
            addAndMakeVisible(*K);
            return std::make_pair(std::move(K), std::move(L));
        };
        auto r = mk(rateA, "Rate");
        rateK = std::move(r.first);
        rateL = std::move(r.second);

        deformA = std::make_unique<attachment_t>(datamodel::lfoSmoothing().withName("Deform"),
                                                 update(), ls.smooth);
        auto d = mk(deformA, "Deform");
        deformK = std::move(d.first);
        deformL = std::move(d.second);

        phaseA = std::make_unique<attachment_t>(datamodel::pmd().asPercent().withName("Phase"),
                                                update(), ms.start_phase);
        auto ph = mk(phaseA, "Phase");
        phaseK = std::move(ph.first);
        phaseL = std::move(ph.second);

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

        for (const auto &j : jog)
        {
            addAndMakeVisible(*j);
            j->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationMatrixMenu);
        }
    }

    void shiftBy(float amt)
    {
        auto &ls = parent->modulatorStorageData[parent->selectedTab].stepLfoStorage;

        for (int i = 0; i < ls.repeat; ++i)
        {
            ls.data[i] = std::clamp(ls.data[i] + amt, -1.f, 1.f);
        }

        parent->pushCurrentModulatorStorageUpdate();
    }
    void rotate(int dir)
    {
        auto &ls = parent->modulatorStorageData[parent->selectedTab].stepLfoStorage;

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
        parent->pushCurrentModulatorStorageUpdate();
    }

    std::unique_ptr<LfoPane::attachment_t> rateA, deformA, phaseA;
    std::unique_ptr<jcmp::Knob> rateK, deformK, phaseK;
    std::unique_ptr<jcmp::Label> rateL, deformL, phaseL;

    std::unique_ptr<LfoPane::intAttachment_t> stepsA;
    std::unique_ptr<jcmp::JogUpDownButton> stepsJ;

    std::unique_ptr<LfoPane::boolBaseAttachment_t> cycleA;
    std::unique_ptr<jcmp::ToggleButton> cycleB;

    std::array<std::unique_ptr<sst::jucegui::components::GlyphButton>, 4> jog;

    void resized() override
    {
        auto knobReg = 58;
        auto mg = 3;
        auto b = getLocalBounds();
        stepRender->setBounds(b.withTrimmedBottom(knobReg + mg));

        auto knobw = knobReg - 16;
        auto bot = b.withTrimmedTop(b.getHeight() - knobReg);

        auto menht = 22;
        auto jx = bot.withWidth(knobw * 2 - mg).withHeight(menht);
        stepsJ->setBounds(jx);

        jx = jx.translated(0, menht + mg);
        cycleB->setBounds(jx);

        auto bx = bot.withWidth(knobw).translated(knobw * 2 + mg, 0);
        rateK->setBounds(bx.withHeight(knobw));
        rateL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);

        deformK->setBounds(bx.withHeight(knobw));
        deformL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);

        phaseK->setBounds(bx.withHeight(knobw));
        phaseL->setBounds(bx.withTrimmedTop(knobw));

        bx = bx.translated(knobw * 1.5 + mg, 0);
        bx = bx.withWidth(bx.getWidth() * 1.2).reduced(0, mg);
        auto bthrd = bx.getWidth() / 3;
        auto rc = bx.toFloat().withWidth(bthrd);
        jog[2]->setBounds(rc.withHeight(bthrd).withCentre(rc.getCentre()).toNearestInt());
        rc = rc.translated(bthrd, 0);
        jog[0]->setBounds(rc.withHeight(bthrd).toNearestInt());
        jog[1]->setBounds(rc.withHeight(bthrd).translated(0, 2 * bthrd).toNearestInt());

        rc = rc.translated(bthrd, 0);
        jog[3]->setBounds(rc.withHeight(bthrd).withCentre(rc.getCentre()).toNearestInt());
    }
};

struct CurveLFOPane : juce::Component, HasEditor
{
    LfoPane *parent{nullptr};

    struct CurveDraw : public juce::Component
    {
        LfoPane *parent{nullptr};
        CurveDraw(LfoPane *p) : parent(p) {}
        void paint(juce::Graphics &g) override
        {
            auto bg = parent->style()->getColour(jcmp::NamedPanel::Styles::styleClass,
                                                 jcmp::NamedPanel::Styles::background);
            auto boxc = parent->style()->getColour(jcmp::NamedPanel::Styles::styleClass,
                                                   jcmp::NamedPanel::Styles::brightoutline);

            g.fillAll(bg);
            g.setColour(boxc);
            g.drawRect(getLocalBounds(), 1);

            g.setFont(juce::Font("Comic Sans MS", 12, juce::Font::plain));
            g.drawText("Curve Viz Soon", getLocalBounds(), juce::Justification::centred);
        }
    };

    CurveLFOPane(LfoPane *p) : parent(p), HasEditor(p->editor)
    {
        assert(parent);
        using fac = connectors::SingleValueFactory<LfoPane::attachment_t,
                                                   cmsg::UpdateZoneOrGroupModStorageFloatValue>;
        using bfac =
            connectors::BooleanSingleValueFactory<LfoPane::boolAttachment_t,
                                                  cmsg::UpdateZoneOrGroupModStorageBoolValue>;

        auto &ms = parent->modulatorStorageData[parent->selectedTab];

        auto makeLabel = [this](auto &lb, const std::string &l) {
            lb = std::make_unique<sst::jucegui::components::Label>();
            lb->setText(l);
            addAndMakeVisible(*lb);
        };

        fac::attachAndAdd(datamodel::lfoModulationRate().withName("Rate"), ms, ms.rate, this, rateA,
                          rateK, parent->forZone, parent->selectedTab);
        rateK->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorKnob);
        makeLabel(rateL, "Rate");

        fac::attachAndAdd(datamodel::pmd().asPercentBipolar().withName("Deform"), ms,
                          ms.curveLfoStorage.deform, this, deformA, deformK, parent->forZone,
                          parent->selectedTab);
        deformK->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorKnob);
        makeLabel(deformL, "Deform");

        fac::attachAndAdd(datamodel::pmd().asPercent().withName("Phase"), ms, ms.start_phase, this,
                          phaseA, phaseK, parent->forZone, parent->selectedTab);
        phaseK->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorKnob);
        makeLabel(phaseL, "Phase");

        fac::attachAndAdd(datamodel::envelopeThirtyTwo().withName("Delay"), ms,
                          ms.curveLfoStorage.delay, this, envA[0], envS[0], parent->forZone,
                          parent->selectedTab);
        envS[0]->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorKnob);
        makeLabel(envL[0], "Delay");

        fac::attachAndAdd(datamodel::envelopeThirtyTwo().withName("Attack"), ms,
                          ms.curveLfoStorage.attack, this, envA[1], envS[1], parent->forZone,
                          parent->selectedTab);
        envS[1]->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorKnob);
        makeLabel(envL[1], "Attack");

        fac::attachAndAdd(datamodel::envelopeThirtyTwo().withName("Release"), ms,
                          ms.curveLfoStorage.release, this, envA[2], envS[2], parent->forZone,
                          parent->selectedTab);
        envS[2]->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorKnob);
        makeLabel(envL[2], "Release");

        bfac::attachAndAdd(datamodel::pmd().asBool().withName("Unipolar"), ms,
                           ms.curveLfoStorage.unipolar, this, unipolarA, unipolarB, parent->forZone,
                           parent->selectedTab);
        unipolarB->setLabel("UNI");

        bfac::attachAndAdd(datamodel::pmd().asBool().withName("Use Envelope"), ms,
                           ms.curveLfoStorage.useenv, this, useenvA, useenvB, parent->forZone,
                           parent->selectedTab);
        useenvB->setLabel("ENV");

        curveDraw = std::make_unique<CurveDraw>(parent);
        addAndMakeVisible(*curveDraw);
        curveDraw->toBack();
    }

    void resized() override
    {
        auto mg = 3;
        auto lbHt = 12;

        auto b = getLocalBounds();

        auto knobw = b.getHeight() / 2 - lbHt - mg;

        auto bx = b.withWidth(knobw).withHeight(b.getHeight() / 2);
        rateK->setBounds(bx.withHeight(knobw));
        rateL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);
        deformK->setBounds(bx.withHeight(knobw));
        deformL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);
        phaseK->setBounds(bx.withHeight(knobw));
        phaseL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);

        bx = b.withWidth(knobw).withHeight(b.getHeight() / 2).translated(0, b.getHeight() / 2);

        for (int i = 0; i < envSlots; ++i)
        {
            envS[i]->setBounds(bx.withHeight(knobw));
            envL[i]->setBounds(bx.withTrimmedTop(knobw));
            bx = bx.translated(knobw + mg, 0);
        }

        auto curveBox = b.withTrimmedLeft((knobw + mg) * 3).withTrimmedBottom(mg).reduced(mg, 0);
        curveDraw->setBounds(curveBox);

        unipolarB->setBounds(curveBox.getRight() - 32 - mg, 2, 32, 18);
        useenvB->setBounds(curveBox.getRight() - 70 - mg, 2, 32, 18);
    }

    std::unique_ptr<LfoPane::attachment_t> rateA, deformA, phaseA;
    std::unique_ptr<jcmp::Knob> rateK, deformK, phaseK;
    std::unique_ptr<jcmp::Label> rateL, deformL, phaseL;

    std::unique_ptr<LfoPane::boolAttachment_t> unipolarA, useenvA;
    std::unique_ptr<jcmp::ToggleButton> unipolarB, useenvB;

    static constexpr int envSlots{3}; // DAR
    std::array<std::unique_ptr<LfoPane::attachment_t>, envSlots> envA;
    std::array<std::unique_ptr<jcmp::Knob>, envSlots> envS;
    std::array<std::unique_ptr<jcmp::Label>, envSlots> envL;

    std::unique_ptr<CurveDraw> curveDraw;
};

struct ENVLFOPane : juce::Component
{
    LfoPane *parent{nullptr};
    ENVLFOPane(LfoPane *p) : parent(p) {}

    void paint(juce::Graphics &g) override
    {
        g.setColour(juce::Colours::orchid);
        g.setFont(juce::Font("Comic Sans MS", 30, juce::Font::plain));
        g.drawText("ENV", getLocalBounds(), juce::Justification::centred);
    }
};

struct MSEGLFOPane : juce::Component
{
    MSEGLFOPane(modulation::ModulatorStorage &s) {}

    void paint(juce::Graphics &g) override
    {
        g.setColour(juce::Colours::blue);
        g.setFont(juce::Font("Comic Sans MS", 30, juce::Font::plain));
        g.drawText("MSEG", getLocalBounds(), juce::Justification::centred);
    }
};

// TODO: A Million things of course

LfoPane::LfoPane(SCXTEditor *e, bool fz)
    : sst::jucegui::components::NamedPanel(""), HasEditor(e), forZone(fz)
{
    setContentAreaComponent(std::make_unique<juce::Component>());
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

LfoPane::~LfoPane() { getContentAreaComponent()->removeAllChildren(); }

void LfoPane::resized()
{
    getContentAreaComponent()->setBounds(getContentArea());
    repositionContentAreaComponents();
}

void LfoPane::tabChanged(int i)
{
    getContentAreaComponent()->removeAllChildren();
    rebuildPanelComponents();
}

void LfoPane::setActive(int i, bool b)
{
    setEnabled(b);
    if (!b)
    {
        getContentAreaComponent()->removeAllChildren();
    }
}

void LfoPane::setModulatorStorage(int index, const modulation::ModulatorStorage &mod)
{
    modulatorStorageData[index] = mod;

    if (index != selectedTab)
        return;

    getContentAreaComponent()->removeAllChildren();

    if (isEnabled())
    {
        rebuildPanelComponents();
    }
}

void LfoPane::rebuildPanelComponents()
{
    if (!isEnabled())
        return;

    auto &ms = modulatorStorageData[selectedTab];
    auto updateNoTT = [r = juce::Component::SafePointer(this)]() {
        return [w = juce::Component::SafePointer(r)](const auto &a) {
            if (w)
            {
                w->pushCurrentModulatorStorageUpdate();
                w->repaint();
            }
        };
    };
    modulatorShapeA = std::make_unique<shapeAttachment_t>(
        datamodel::pmd()
            .asInt()
            .withName("Modulator Shape")
            .withRange(modulation::ModulatorStorage::STEP,
                       modulation::ModulatorStorage::MSEG -
                           1) /* This -1 turns off MSEG in the menu */
            .withUnorderedMapFormatting({
                {modulation::ModulatorStorage::STEP, "STEP"},
                {modulation::ModulatorStorage::MSEG, "MSEG"},
                {modulation::ModulatorStorage::LFO_SINE, "SINE"},
                {modulation::ModulatorStorage::LFO_RAMP, "RAMP"},
                {modulation::ModulatorStorage::LFO_TRI, "TRI"},
                {modulation::ModulatorStorage::LFO_PULSE, "PULSE"},
                {modulation::ModulatorStorage::LFO_SMOOTH_NOISE, "NOISE"},
                {modulation::ModulatorStorage::LFO_ENV, "ENV"},
                {modulation::ModulatorStorage::LFO_SH_NOISE, "S&H"},
            }),
        updateNoTT(), ms.modulatorShape);
    modulatorShape = std::make_unique<jcmp::JogUpDownButton>();
    modulatorShape->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationJogButon);
    modulatorShape->setSource(modulatorShapeA.get());

    getContentAreaComponent()->addAndMakeVisible(*modulatorShape);

    triggerModeA = std::make_unique<triggerAttachment_t>(
        datamodel::pmd()
            .asInt()
            .withName("Trigger")
            .withRange(modulation::ModulatorStorage::KEYTRIGGER,
                       modulation::ModulatorStorage::ONESHOT)
            .withUnorderedMapFormatting({
                {modulation::ModulatorStorage::KEYTRIGGER, "KEYTRIG"},
                {modulation::ModulatorStorage::FREERUN, "SONGPOS"},
                {modulation::ModulatorStorage::RANDOM, "RANDOM"},
                {modulation::ModulatorStorage::RELEASE, "RELEASE"},
                {modulation::ModulatorStorage::ONESHOT, "ONESHOT"},
            }),
        updateNoTT(), ms.triggerMode);
    triggerMode = std::make_unique<jcmp::MultiSwitch>();
    triggerMode->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationMultiSwitch);
    triggerMode->setSource(triggerModeA.get());
    getContentAreaComponent()->addAndMakeVisible(*triggerMode);

    stepLfoPane = std::make_unique<StepLFOPane>(this);
    getContentAreaComponent()->addChildComponent(*stepLfoPane);

    msegLfoPane = std::make_unique<MSEGLFOPane>(ms);
    getContentAreaComponent()->addChildComponent(*msegLfoPane);

    envLfoPane = std::make_unique<ENVLFOPane>(this);
    getContentAreaComponent()->addChildComponent(*envLfoPane);

    curveLfoPane = std::make_unique<CurveLFOPane>(this);
    getContentAreaComponent()->addChildComponent(*curveLfoPane);

    repositionContentAreaComponents();
    setSubPaneVisibility();
}

void LfoPane::repositionContentAreaComponents()
{
    if (!modulatorShape) // these are all created at once so single check is fine
        return;

    auto ht = 18;
    auto wd = 66;
    auto mg = 5;

    modulatorShape->setBounds(0, 0, wd, ht);
    triggerMode->setBounds(0, ht + mg, wd, 5 * ht);
    auto paneArea = getContentArea().withX(0).withTrimmedLeft(wd + mg).withY(0);
    stepLfoPane->setBounds(paneArea);
    envLfoPane->setBounds(paneArea);
    msegLfoPane->setBounds(paneArea);
    curveLfoPane->setBounds(paneArea);
}

void LfoPane::setSubPaneVisibility()
{
    if (!stepLfoPane || !msegLfoPane || !curveLfoPane)
        return;

    auto &ms = modulatorStorageData[selectedTab];

    stepLfoPane->setVisible(ms.isStep());
    msegLfoPane->setVisible(ms.isMSEG());
    curveLfoPane->setVisible(ms.isCurve());
    envLfoPane->setVisible(ms.isEnv());
}

void LfoPane::pushCurrentModulatorStorageUpdate()
{
    modulatorStorageData[selectedTab].configureCalculatedState();
    sendToSerialization(cmsg::IndexedModulatorStorageUpdated(
        {forZone, true, selectedTab, modulatorStorageData[selectedTab]}));
    setSubPaneVisibility();
    repaint();
}

void LfoPane::pickPresets()
{
#if 0
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
#endif
}
} // namespace scxt::ui::multi