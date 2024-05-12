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

#include "datamodel/metadata.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "messaging/messaging.h"
#include "components/SCXTEditor.h"
#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/VSlider.h"

// Included so we can have UI-thread exceution for curve rendering
#include "modulation/modulators/steplfo.h"

namespace scxt::ui::multi
{

namespace jcmp = sst::jucegui::components;
namespace cmsg = scxt::messaging::client;

const int MG = 3;

struct StepLFOPane : juce::Component, HasEditor
{
    struct StepRender : juce::Component
    {
        LfoPane *parent{nullptr};
        StepRender(LfoPane *p) : parent{p} { recalcCurve(); }

        void paint(juce::Graphics &g) override
        {
            if (!parent)
                return;

            auto &cmap = *parent->editor->themeApplier.colorMap();
            auto bg = cmap.get(theme::ColorMap::bg_2);
            auto bgq = cmap.get(theme::ColorMap::bg_2).brighter(0.1);
            auto boxc = cmap.get(theme::ColorMap::generic_content_low);
            auto valc = cmap.get(theme::ColorMap::accent_2a);
            auto valhovc = valc.brighter(0.1);

            auto hanc = cmap.get(theme::ColorMap::generic_content_high);
            auto hanhovc = hanc.brighter(0.1);

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
            parent->stepLfoPane->setStep(idx, d);
            recalcCurve();
            repaint();
        }
        void mouseDown(const juce::MouseEvent &event) override { handleMouseAt(event.position); }
        void mouseDrag(const juce::MouseEvent &event) override { handleMouseAt(event.position); }
        void mouseDoubleClick(const juce::MouseEvent &event) override
        {
            auto idx = indexForPosition(event.position);
            if (idx < 0)
                return;
            parent->stepLfoPane->setStep(idx, 0);
            recalcCurve();
            repaint();
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
    StepLFOPane(LfoPane *p) : parent(p), HasEditor(p->editor)
    {
        stepRender = std::make_unique<StepRender>(parent);
        addAndMakeVisible(*stepRender);

        using fac = connectors::SingleValueFactory<LfoPane::attachment_t,
                                                   cmsg::UpdateZoneOrGroupModStorageFloatValue>;
        using bfac =
            connectors::BooleanSingleValueFactory<LfoPane::boolBaseAttachment_t,
                                                  cmsg::UpdateZoneOrGroupModStorageBoolValue>;
        using ifac = connectors::SingleValueFactory<LfoPane::int16Attachment_t,
                                                    cmsg::UpdateZoneOrGroupModStorageInt16TValue>;

        typedef LfoPane::attachment_t attachment_t;

        auto &ms = parent->modulatorStorageData[parent->selectedTab];
        auto &ls = ms.stepLfoStorage;

        ifac::attachAndAdd(ms, ms.stepLfoStorage.repeat, this, stepsA, stepsJ, parent->forZone,
                           parent->selectedTab);
        connectors::addGuiStep(*stepsA, [w = juce::Component::SafePointer(this)](const auto &a) {
            if (w)
                w->stepRender->recalcCurve();
        });

        bfac::attachAndAdd(ms, ms.stepLfoStorage.rateIsForSingleStep, this, cycleA, cycleB,
                           parent->forZone, parent->selectedTab);
        cycleB->setDrawMode(jcmp::ToggleButton::DrawMode::LABELED_BY_DATA);

        fac::attachAndAdd(ms, ms.rate, this, rateA, rateK, parent->forZone, parent->selectedTab);
        rateL = std::make_unique<jcmp::Label>();
        rateL->setText("Rate");
        addAndMakeVisible(*rateL);

        fac::attachAndAdd(ms, ms.stepLfoStorage.smooth, this, deformA, deformK, parent->forZone,
                          parent->selectedTab);
        deformL = std::make_unique<jcmp::Label>();
        deformL->setText("Deform");
        addAndMakeVisible(*deformL);
        connectors::addGuiStep(*deformA, [w = juce::Component::SafePointer(this)](const auto &a) {
            if (w)
                w->stepRender->recalcCurve();
        });

        fac::attachAndAdd(ms, ms.start_phase, this, phaseA, phaseK, parent->forZone,
                          parent->selectedTab);
        phaseL = std::make_unique<jcmp::Label>();
        phaseL->setText("Phase");
        addAndMakeVisible(*phaseL);

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
            if (j)
            {
                addAndMakeVisible(*j);
            }
        }
    }

    void setStep(int idx, float v)
    {
        auto &ms = parent->modulatorStorageData[parent->selectedTab];
        ms.stepLfoStorage.data[idx] = v;
        connectors::updateSingleValue<cmsg::UpdateZoneOrGroupModStorageFloatValue>(
            ms, ms.stepLfoStorage.data[idx], parent, parent->forZone, parent->selectedTab);
    }

    void rotate(int dir)
    {
        auto &ls = parent->modulatorStorageData[parent->selectedTab].stepLfoStorage;

        if (dir == -1)
        {
            auto p0 = ls.data[0];
            for (int i = 0; i < ls.repeat; ++i)
            {
                setStep(i, i == ls.repeat - 1 ? p0 : ls.data[i + 1]);
            }
        }
        else
        {
            auto p0 = ls.data[ls.repeat - 1];
            for (int i = ls.repeat - 1; i >= 0; --i)
            {
                setStep(i, i == 0 ? p0 : ls.data[i - 1]);
            }
        }
        stepRender->recalcCurve();
    }

    std::unique_ptr<LfoPane::attachment_t> rateA, deformA, phaseA;
    std::unique_ptr<jcmp::Knob> rateK, deformK, phaseK;
    std::unique_ptr<jcmp::Label> rateL, deformL, phaseL;

    std::unique_ptr<LfoPane::int16Attachment_t> stepsA;
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
            lb = std::make_unique<jcmp::Label>();
            lb->setText(l);
            addAndMakeVisible(*lb);
        };

        fac::attachAndAdd(ms, ms.rate, this, rateA, rateK, parent->forZone, parent->selectedTab);
        makeLabel(rateL, "Rate");

        fac::attachAndAdd(ms, ms.curveLfoStorage.deform, this, deformA, deformK, parent->forZone,
                          parent->selectedTab);
        makeLabel(deformL, "Deform");

        fac::attachAndAdd(ms, ms.start_phase, this, phaseA, phaseK, parent->forZone,
                          parent->selectedTab);
        makeLabel(phaseL, "Phase");

        //fac::attachAndAdd(ms, ms.start_phase, this, angleA, angleK, parent->forZone,
        //                  parent->selectedTab);
        angleK = connectors::makeConnectedToDummy<jcmp::Knob>();
        //angleK = std::make_unique<jcmp::Knob>();
        //angleK->setSource(fakeModel->getDummySourceFor('knb2'));
        addAndMakeVisible(*angleK);
        makeLabel(angleL, "Angle");

        fac::attachAndAdd(ms, ms.curveLfoStorage.delay, this, envA[0], envS[0], parent->forZone,
                          parent->selectedTab);
        makeLabel(envL[0], "Delay");

        fac::attachAndAdd(ms, ms.curveLfoStorage.attack, this, envA[1], envS[1], parent->forZone,
                          parent->selectedTab);
        makeLabel(envL[1], "Attack");

        fac::attachAndAdd(ms, ms.curveLfoStorage.release, this, envA[2], envS[2], parent->forZone,
                          parent->selectedTab);
        makeLabel(envL[2], "Release");

        bfac::attachAndAdd(ms, ms.curveLfoStorage.unipolar, this, unipolarA, unipolarB,
                           parent->forZone, parent->selectedTab);
        unipolarB->setLabel("UNI");

        bfac::attachAndAdd(ms, ms.curveLfoStorage.useenv, this, useenvA, useenvB, parent->forZone,
                           parent->selectedTab);
        useenvB->setLabel("ENV");

        curveDraw = std::make_unique<CurveDraw>(parent);
        addAndMakeVisible(*curveDraw);
        curveDraw->toBack();
    }

    void resized() override
    {
        auto lbHt = 12;

        auto b = getLocalBounds();

        auto knobw = b.getHeight() / 2 - lbHt;

        auto nKnobs = 4;
        auto allKnobsStartX = b.getWidth()/4 + MG*2;
        auto allKnobsStartY = 0;
        auto allKnobsWidth = 3*b.getWidth()/4 - MG*2;
        auto allKnobsHeight = b.getHeight()/2 - MG;

        auto knobMg = 12;
        auto knobWidth = (allKnobsWidth - knobMg * (nKnobs -1)) / nKnobs;
        auto knobHeight = allKnobsHeight;

        auto knobBounds = b.withWidth(knobWidth).withHeight(knobHeight).withX(allKnobsStartX).withY(allKnobsStartY);
        auto makeKnobBounds = [](auto &knob, auto &label, auto &knobBounds, int lbHt, int knobWidth, int knobHeight, int knobMg){
            knob->setBounds(knobBounds.withTrimmedBottom(17)); // remove modulator_storage label
            label->setBounds(knobBounds.withTrimmedTop(knobHeight -lbHt));
            knobBounds = knobBounds.translated(knobWidth + knobMg, 0);
        };
        makeKnobBounds(rateK, rateL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(phaseK, phaseL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(deformK, deformL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(angleK, angleL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);

        auto bx = b.withWidth(knobw).withHeight(b.getHeight() / 2).translated(b.getWidth()/2, b.getHeight() / 2);
        for (int i = 0; i < envSlots; ++i)
        {
            envS[i]->setBounds(bx.withHeight(knobw));
            envL[i]->setBounds(bx.withTrimmedTop(knobw));
            bx = bx.translated(knobw + MG, 0);
        }

        auto bh = b.getHeight();
        auto curveBox = b.withY(bh/2).withTrimmedRight(getWidth()/2).withTrimmedBottom(bh/2);
        curveDraw->setBounds(curveBox);

        auto buttonH = 18;
        unipolarB->setBounds(0, bh/2 - MG - buttonH, b.getWidth()/4, buttonH);
        useenvB->setBounds(0, bh/2, b.getWidth()/4, buttonH);
    }

    std::unique_ptr<LfoPane::attachment_t> rateA, deformA, phaseA, angleA;
    std::unique_ptr<jcmp::Knob> rateK, deformK, phaseK, angleK;
    //std::unique_ptr<connectors::FakeModel> fakeModel;
    std::unique_ptr<jcmp::Label> rateL, deformL, phaseL, angleL;

    std::unique_ptr<LfoPane::boolAttachment_t> unipolarA, useenvA;
    std::unique_ptr<jcmp::ToggleButton> unipolarB, useenvB;

    static constexpr int envSlots{3}; // DAR
    std::array<std::unique_ptr<LfoPane::attachment_t>, envSlots> envA;
    std::array<std::unique_ptr<jcmp::VSlider>, envSlots> envS;
    std::array<std::unique_ptr<jcmp::Label>, envSlots> envL;

    std::unique_ptr<CurveDraw> curveDraw;
};

struct ENVLFOPane : juce::Component, HasEditor
{
    LfoPane *parent{nullptr};
    ENVLFOPane(LfoPane *p) : parent(p), HasEditor(p->editor)
    {
        assert(parent);
        using fac = connectors::SingleValueFactory<LfoPane::attachment_t,
                                                   cmsg::UpdateZoneOrGroupModStorageFloatValue>;

        auto &ms = parent->modulatorStorageData[parent->selectedTab];

        auto makeLabel = [this](auto &lb, const std::string &l) {
            lb = std::make_unique<sst::jucegui::components::Label>();
            lb->setText(l);
            addAndMakeVisible(*lb);
        };

        auto makeO = [&, this](auto &mem, auto &A, auto &S, auto &L) {
            fac::attachAndAdd(ms, mem, this, A, S, parent->forZone, parent->selectedTab);

            makeLabel(L, A->getLabel());
        };

        makeO(ms.envLfoStorage.delay, delayA, delayS, delayL);
        makeO(ms.envLfoStorage.attack, attackA, attackS, attackL);
        makeO(ms.envLfoStorage.hold, holdA, holdS, holdL);
        makeO(ms.envLfoStorage.decay, decayA, decayS, decayL);
        makeO(ms.envLfoStorage.sustain, sustainA, sustainS, sustainL);
        makeO(ms.envLfoStorage.release, releaseA, releaseS, releaseL);
    }

    void resized() override
    {
        auto mg = 3;
        auto lbHt = 12;

        auto b = getLocalBounds();

        auto knobw = b.getHeight() / 2 - lbHt - mg;

        auto bx = b.withWidth(knobw).withHeight(b.getHeight() / 2);
        delayS->setBounds(bx.withHeight(knobw));
        delayL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);

        attackS->setBounds(bx.withHeight(knobw));
        attackL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);

        holdS->setBounds(bx.withHeight(knobw));
        holdL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);

        bx = b.withWidth(knobw).withHeight(b.getHeight() / 2).translated(0, b.getHeight() / 2);

        decayS->setBounds(bx.withHeight(knobw));
        decayL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);

        sustainS->setBounds(bx.withHeight(knobw));
        sustainL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);

        releaseS->setBounds(bx.withHeight(knobw));
        releaseL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);
        bx = bx.translated(knobw + mg, 0);
    }

    std::unique_ptr<LfoPane::attachment_t> delayA, attackA, holdA, decayA, sustainA, releaseA;
    std::unique_ptr<jcmp::VSlider> delayS, attackS, holdS, decayS, sustainS, releaseS;
    std::unique_ptr<jcmp::Label> delayL, attackL, holdL, decayL, sustainL, releaseL;
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
    // setCustomClass(connectors::SCXTStyleSheetCreator::ModulationTabs);
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

    using sfac = connectors::SingleValueFactory<shapeAttachment_t,
                                                cmsg::UpdateZoneOrGroupModStorageInt16TValue>;
    using tfac = connectors::SingleValueFactory<triggerAttachment_t,
                                                cmsg::UpdateZoneOrGroupModStorageInt16TValue>;


    tfac::attach(ms, ms.triggerMode, this, triggerModeA, triggerMode, forZone, selectedTab);
    getContentAreaComponent()->addAndMakeVisible(*triggerMode);

    triggerL = std::make_unique<jcmp::Label>();
    triggerL->setText("Trigger");
    getContentAreaComponent()->addAndMakeVisible(*triggerL);

    stepLfoPane = std::make_unique<StepLFOPane>(this);
    getContentAreaComponent()->addChildComponent(*stepLfoPane);

    msegLfoPane = std::make_unique<MSEGLFOPane>(ms);
    getContentAreaComponent()->addChildComponent(*msegLfoPane);

    envLfoPane = std::make_unique<ENVLFOPane>(this);
    getContentAreaComponent()->addChildComponent(*envLfoPane);

    curveLfoPane = std::make_unique<CurveLFOPane>(this);
    getContentAreaComponent()->addChildComponent(*curveLfoPane);

    sfac::attach(ms, ms.modulatorShape, this, modulatorShapeA, modulatorShape, forZone,
                 selectedTab);
    connectors::addGuiStep(*modulatorShapeA,
                           [w = juce::Component::SafePointer(this)](const auto &x) {
                               if (w)
                                   w->setSubPaneVisibility();
                           });

    getContentAreaComponent()->addAndMakeVisible(*modulatorShape);

    repositionContentAreaComponents();
    setSubPaneVisibility();

    if (forZone)
    {
        editor->themeApplier.applyZoneMultiScreenModulationTheme(this);
    }
    else
    {
        editor->themeApplier.applyGroupMultiScreenModulationTheme(this);
    }
}

void LfoPane::repositionContentAreaComponents()
{
    if (!modulatorShape) // these are all created at once so single check is fine
        return;

    auto ht = 18;
    auto triggerWidth = 66;
    auto mg = 5;

    triggerMode->setBounds(getContentArea().getWidth() - (triggerWidth + mg), 0, triggerWidth, 5 * ht);
    triggerL->setBounds(getContentArea().getWidth() - (triggerWidth + mg), triggerMode->getHeight() + mg, triggerWidth, ht);

    auto paneArea = getContentArea().withX(mg).withTrimmedRight(triggerWidth + mg*3).withY(0).withTrimmedBottom(mg);
    stepLfoPane->setBounds(paneArea);
    envLfoPane->setBounds(paneArea);
    msegLfoPane->setBounds(paneArea);
    curveLfoPane->setBounds(paneArea);
    modulatorShape->setBounds(mg, 0, paneArea.getWidth()/4, ht);
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