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

const int MG = 5;
const int BUTTON_H = 18;

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
            auto bgq = cmap.get(theme::ColorMap::accent_2a_alpha_a);
            auto boxc = cmap.get(theme::ColorMap::generic_content_low);
            auto valc = cmap.get(theme::ColorMap::accent_2a);
            auto valhovc = valc.brighter(0.1);

            auto hanc = valhovc;
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

            g.setColour(hanc);
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
            // g.strokePath(p, juce::PathStrokeType(1.0));

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

        auto makeLabel = [this](auto &lb, const std::string &l) {
            lb = std::make_unique<jcmp::Label>();
            lb->setText(l);
            addAndMakeVisible(*lb);
        };
        auto aux = [&, this](auto &mem, auto &A, auto &S, auto &L) {
            fac::attachAndAdd(ms, mem, this, A, S, parent->forZone, parent->selectedTab);

            makeLabel(L, A->getLabel());
        };

        ifac::attachAndAdd(ms, ms.stepLfoStorage.repeat, this, stepsA, stepsJ, parent->forZone,
                           parent->selectedTab);
        connectors::addGuiStep(*stepsA, [w = juce::Component::SafePointer(this)](const auto &a) {
            if (w)
                w->stepRender->recalcCurve();
        });
        stepsA->setJogWrapsAtEnd(false);

        bfac::attachAndAdd(ms, ms.stepLfoStorage.rateIsForSingleStep, this, cycleA, cycleB,
                           parent->forZone, parent->selectedTab);
        cycleB->setDrawMode(jcmp::ToggleButton::DrawMode::LABELED_BY_DATA);

        aux(ms.rate, rateA, rateK, rateL);
        aux(ls.smooth, deformA, deformK, deformL);

        connectors::addGuiStep(*deformA, [w = juce::Component::SafePointer(this)](const auto &a) {
            if (w)
                w->stepRender->recalcCurve();
        });

        aux(ms.start_phase, phaseA, phaseK, phaseL);

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
        auto b = getLocalBounds();

        stepRender->setBounds(b.withTrimmedTop(b.getHeight() / 2));
        auto knobw = knobReg - 16;
        auto bot = b.withTrimmedTop(b.getHeight() - knobReg);

        auto buttonH = BUTTON_H;
        cycleB->setBounds(0, b.getHeight() / 2 - MG - buttonH, b.getWidth() / 4, buttonH);

        auto bx = bot.withWidth(knobw).translated(knobw * 2 + MG, 0);

        auto lbHt = 15;
        // Knobs
        auto nKnobs = 4;
        auto allKnobsStartX = b.getWidth() / 4 + MG * 2;
        auto allKnobsStartY = 0;
        auto allKnobsWidth = b.getWidth() - allKnobsStartX;
        auto allKnobsHeight = b.getHeight() / 2 - MG;

        auto knobMg = 15;
        auto knobWidth = (allKnobsWidth - knobMg * (nKnobs - 1)) / nKnobs;
        auto knobHeight = allKnobsHeight;

        auto knobBounds = b.withWidth(knobWidth)
                              .withHeight(knobHeight)
                              .withX(allKnobsStartX)
                              .withY(allKnobsStartY);
        auto makeKnobBounds = [](auto &knob, auto &label, auto &knobBounds, int lbHt, int knobWidth,
                                 int knobHeight, int knobMg) {
            knob->setBounds(
                knobBounds.withTrimmedBottom(23 - MG)); // remove modulator_storage label
            label->setBounds(knobBounds.withTrimmedTop(knobHeight - lbHt));
            knobBounds = knobBounds.translated(knobWidth + knobMg, 0);
        };
        makeKnobBounds(rateK, rateL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(phaseK, phaseL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(deformK, deformL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        // Knobs (END)

        // (use knobBounds to place this where the 4th knob would be)
        stepsJ->setBounds(knobBounds.withHeight(buttonH));
        auto jogBox = knobBounds.withHeight(buttonH)
                          .withY(knobBounds.getHeight() - buttonH)
                          .withWidth(knobBounds.getWidth() / 2);
        jog[2]->setBounds(jogBox);
        jog[3]->setBounds(jogBox.withX(jogBox.getX() + jogBox.getWidth()));
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
        auto aux = [&, this](auto &mem, auto &A, auto &S, auto &L) {
            fac::attachAndAdd(ms, mem, this, A, S, parent->forZone, parent->selectedTab);

            makeLabel(L, A->getLabel());
        };

        aux(ms.rate, rateA, rateK, rateL);
        aux(ms.curveLfoStorage.deform, deformA, deformK, deformL);
        aux(ms.start_phase, phaseA, phaseK, phaseL);

        // fac::attachAndAdd(ms, ms.start_phase, this, angleA, angleK, parent->forZone,
        //                   parent->selectedTab);
        // angleK = std::make_unique<jcmp::Knob>();
        // angleK->setSource(fakeModel->getDummySourceFor('knb2'));
        angleK = connectors::makeConnectedToDummy<jcmp::Knob>('angl');
        addAndMakeVisible(*angleK);
        makeLabel(angleL, "Angle");

        aux(ms.curveLfoStorage.delay, envA[0], envS[0], envL[0]);
        aux(ms.curveLfoStorage.attack, envA[1], envS[1], envL[1]);
        aux(ms.curveLfoStorage.release, envA[2], envS[2], envL[2]);

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
        auto lbHt = 15;
        auto b = getLocalBounds();

        // Knobs
        auto nKnobs = 4;
        auto allKnobsStartX = b.getWidth() / 4 + MG * 2;
        auto allKnobsStartY = 0;
        auto allKnobsWidth = b.getWidth() - allKnobsStartX;
        auto allKnobsHeight = b.getHeight() / 2 - MG;

        auto knobMg = 15;
        auto knobWidth = (allKnobsWidth - knobMg * (nKnobs - 1)) / nKnobs;
        auto knobHeight = allKnobsHeight;

        auto knobBounds = b.withWidth(knobWidth)
                              .withHeight(knobHeight)
                              .withX(allKnobsStartX)
                              .withY(allKnobsStartY);
        auto makeKnobBounds = [](auto &knob, auto &label, auto &knobBounds, int lbHt, int knobWidth,
                                 int knobHeight, int knobMg) {
            knob->setBounds(
                knobBounds.withTrimmedBottom(23 - MG)); // remove modulator_storage label
            label->setBounds(knobBounds.withTrimmedTop(knobHeight - lbHt));
            knobBounds = knobBounds.translated(knobWidth + knobMg, 0);
        };
        makeKnobBounds(rateK, rateL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(phaseK, phaseL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(deformK, deformL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(angleK, angleL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        // Knobs (END)

        // EnvSliders
        auto nEnvSliders = 3;
        auto allEnvSlidersStartX = 3 * b.getWidth() / 5 + MG * 2;
        auto allEnvSlidersStartY = b.getHeight() / 2;
        auto allEnvSlidersWidth = b.getWidth() - allEnvSlidersStartX;
        auto allEnvSlidersHeight = b.getHeight() / 2;

        auto envSliderMg = 6;
        auto envSliderWidth = (allEnvSlidersWidth - envSliderMg * (nEnvSliders - 1)) / nEnvSliders;
        auto envSliderHeight = allEnvSlidersHeight;

        auto envSliderBounds = b.withWidth(envSliderWidth)
                                   .withHeight(envSliderHeight)
                                   .withX(allEnvSlidersStartX)
                                   .withY(allEnvSlidersStartY);
        for (int i = 0; i < envSlots; ++i)
        {
            envS[i]->setBounds(envSliderBounds.withTrimmedBottom(17));
            envL[i]->setBounds(envSliderBounds.withTrimmedTop(envSliderHeight - lbHt));
            envSliderBounds = envSliderBounds.translated(envSliderWidth + envSliderMg, 0);
        }
        // EnvSliders (END)

        auto bh = b.getHeight();
        auto curveBox = b.withY(bh / 2).withWidth(3 * getWidth() / 5).withTrimmedBottom(bh / 2);
        curveDraw->setBounds(curveBox);

        auto buttonH = BUTTON_H;
        unipolarB->setBounds(0, bh / 2 - MG - buttonH, b.getWidth() / 4, buttonH);
        useenvB->setBounds(0, bh / 2, b.getWidth() / 4, buttonH);
    }

    std::unique_ptr<LfoPane::attachment_t> rateA, deformA, phaseA, angleA;
    std::unique_ptr<jcmp::Knob> rateK, deformK, phaseK, angleK;
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

        factorK = connectors::makeConnectedToDummy<jcmp::Knob>('fact');
        addAndMakeVisible(*factorK);
        makeLabel(factorL, "Factor");
        curveA = connectors::makeConnectedToDummy<jcmp::Knob>('crva');
        addAndMakeVisible(*curveA);
        makeLabel(curveAL, "Curve A");
        curveD = connectors::makeConnectedToDummy<jcmp::Knob>('crvd');
        addAndMakeVisible(*curveD);
        makeLabel(curveDL, "Curve D");
        curveR = connectors::makeConnectedToDummy<jcmp::Knob>('crvr');
        addAndMakeVisible(*curveR);
        makeLabel(curveRL, "Curve R");
    }

    void resized() override
    {
        auto lbHt = 15;

        auto b = getLocalBounds();

        // Knobs
        auto nKnobs = 4;
        auto allKnobsStartX = b.getWidth() / 4 + MG * 2;
        auto allKnobsStartY = 0;
        auto allKnobsWidth = b.getWidth() - allKnobsStartX;
        auto allKnobsHeight = b.getHeight() / 2 - MG;

        auto knobMg = 15;
        auto knobWidth = (allKnobsWidth - knobMg * (nKnobs - 1)) / nKnobs;
        auto knobHeight = allKnobsHeight;

        auto knobBounds = b.withWidth(knobWidth)
                              .withHeight(knobHeight)
                              .withX(allKnobsStartX)
                              .withY(allKnobsStartY);
        auto makeKnobBounds = [](auto &knob, auto &label, auto &knobBounds, int lbHt, int knobWidth,
                                 int knobHeight, int knobMg) {
            knob->setBounds(
                knobBounds.withTrimmedBottom(23 - MG)); // remove modulator_storage label
            label->setBounds(knobBounds.withTrimmedTop(knobHeight - lbHt));
            knobBounds = knobBounds.translated(knobWidth + knobMg, 0);
        };
        makeKnobBounds(factorK, factorL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(curveA, curveAL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(curveD, curveDL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(curveR, curveRL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        // Knobs (END)

        // Sliders
        auto nSliders = 6;
        auto allSlidersStartX = b.getWidth() / 3 + MG * 2;
        auto allSlidersStartY = b.getHeight() / 2;
        auto allSlidersWidth = b.getWidth() - allSlidersStartX;
        auto allSlidersHeight = b.getHeight() / 2;

        auto sliderMg = 6;
        auto sliderWidth = (allSlidersWidth - sliderMg * (nSliders - 1)) / nSliders;
        auto sliderHeight = allSlidersHeight;

        auto sliderBounds = b.withWidth(sliderWidth)
                                .withHeight(sliderHeight)
                                .withX(allSlidersStartX)
                                .withY(allSlidersStartY);
        auto makeSliderBounds = [](auto &slider, auto &label, auto &sliderBounds, int lbHt,
                                   int sliderWidth, int sliderHeight, int sliderMg) {
            slider->setBounds(sliderBounds.withTrimmedBottom(17)); // remove modulator_storage label
            label->setBounds(sliderBounds.withTrimmedTop(sliderHeight - lbHt));
            sliderBounds = sliderBounds.translated(sliderWidth + sliderMg, 0);
        };
        makeSliderBounds(delayS, delayL, sliderBounds, lbHt, sliderWidth, sliderHeight, sliderMg);
        makeSliderBounds(attackS, attackL, sliderBounds, lbHt, sliderWidth, sliderHeight, sliderMg);
        makeSliderBounds(holdS, holdL, sliderBounds, lbHt, sliderWidth, sliderHeight, sliderMg);
        makeSliderBounds(decayS, decayL, sliderBounds, lbHt, sliderWidth, sliderHeight, sliderMg);
        makeSliderBounds(sustainS, sustainL, sliderBounds, lbHt, sliderWidth, sliderHeight,
                         sliderMg);
        makeSliderBounds(releaseS, releaseL, sliderBounds, lbHt, sliderWidth, sliderHeight,
                         sliderMg);
        // Sliders (END)
    }

    std::unique_ptr<LfoPane::attachment_t> delayA, attackA, holdA, decayA, sustainA, releaseA;
    std::unique_ptr<jcmp::VSlider> delayS, attackS, holdS, decayS, sustainS, releaseS;
    std::unique_ptr<jcmp::Knob> factorK, curveA, curveD, curveR;
    std::unique_ptr<jcmp::Label> delayL, attackL, holdL, decayL, sustainL, releaseL, factorL,
        curveAL, curveDL, curveRL;
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
    modulatorShapeA->setJogWrapsAtEnd(false);

    getContentAreaComponent()->addAndMakeVisible(*modulatorShape);

    repositionContentAreaComponents();
    setSubPaneVisibility();

    std::unique_ptr<jcmp::ToggleButton> tsb;
    using tsfac = connectors::BooleanSingleValueFactory<boolAttachment_t,
                                                        cmsg::UpdateZoneOrGroupModStorageBoolValue>;
    tsfac::attach(ms, ms.temposync, this, tempoSyncA, tsb, forZone, selectedTab);
    tsb->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH);
    tsb->setGlyph(jcmp::GlyphPainter::METRONOME);

    clearAdditionalHamburgerComponents();
    addAdditionalHamburgerComponent(std::move(tsb));

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

    auto ht = BUTTON_H;
    auto triggerWidth = 72;
    auto mg = MG;

    triggerMode->setBounds(getContentArea().getWidth() - (triggerWidth + mg), 0, triggerWidth,
                           5 * ht);
    triggerL->setBounds(getContentArea().getWidth() - (triggerWidth + mg),
                        triggerMode->getHeight() + mg, triggerWidth, ht);

    auto paneArea = getContentArea()
                        .withX(mg)
                        .withTrimmedRight(triggerWidth + mg * 4)
                        .withY(0)
                        .withTrimmedBottom(mg);
    stepLfoPane->setBounds(paneArea);
    envLfoPane->setBounds(paneArea);
    msegLfoPane->setBounds(paneArea);
    curveLfoPane->setBounds(paneArea);
    modulatorShape->setBounds(mg, 0, paneArea.getWidth() / 4, ht);
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