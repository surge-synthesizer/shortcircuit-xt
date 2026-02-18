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

#include "LFOPane.h"

#include <cmath>

#include "datamodel/metadata.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "messaging/messaging.h"
#include "app/SCXTEditor.h"
#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/components/NamedPanelDivider.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"

#include "sst/jucegui/layouts/ListLayout.h"

// Included so we can have UI-thread exceution for curve rendering
#include "modulation/modulators/steplfo.h"
#include "app/edit-screen/EditScreen.h"
#include "app/shared/MenuValueTypein.h"
#include "sst/jucegui/components/TextPushButton.h"

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>
#include <tao/json/events/from_string.hpp>

#include "json/scxt_traits.h"

namespace scxt::ui::app::edit_screen
{

namespace jcmp = sst::jucegui::components;
namespace cmsg = scxt::messaging::client;

template <typename T> void LfoPane::setAttachmentAsTemposync(T &t)
{
    t.setTemposyncFunction([w = juce::Component::SafePointer(this)](const auto &a) {
        if (w)
            return w->modulatorStorageData[w->selectedTab].temposync;
        return false;
    });
}

const int MG = 5;
const int BUTTON_H = 18;

struct StepLFOPane : juce::Component, app::HasEditor
{
    struct StepRender : juce::Component
    {
        LfoPane *parent{nullptr};
        StepRender(LfoPane *p) : parent{p} { setWantsKeyboardFocus(true); }

        modulation::ModulatorStorage ms;
        juce::Path curvePath{};

        void paint(juce::Graphics &g) override
        {
            if (!parent)
                return;

            auto *ed = parent->editor;
            auto bg = ed->themeColor(theme::ColorMap::accent_2b_alpha_a);
            auto bgq = ed->themeColor(theme::ColorMap::accent_2a_alpha_a);
            auto boxc = ed->themeColor(theme::ColorMap::generic_content_low);
            auto valc = ed->themeColor(theme::ColorMap::accent_2b);
            auto valhovc = valc.brighter(0.1);

            auto hanc = ed->themeColor(theme::ColorMap::accent_2a);
            auto hanhovc = hanc.brighter(0.1);

            int sp = modulation::modulators::StepLFOStorage::stepLfoSteps;
            auto &ls = parent->modulatorStorageData[parent->selectedTab].stepLfoStorage;
            auto w = getWidth() * 1.f / ls.repeat;
            auto bx = getLocalBounds().toFloat().withWidth(w);

            for (int i = 0; i < ls.repeat; ++i)
            {
                g.setColour(i % 2 == 0 ? bg : bgq);
                g.fillRect(bx.reduced(0.5, 0));

                auto d = ls.data[i];
                auto hancbx =
                    bx.reduced(0.5, 0).translated(0, bx.getHeight() / 2 - 0.5).withHeight(1);

                g.setColour(valc);
                if (d > 0)
                {
                    g.fillRect(bx.reduced(0.5, 0)
                                   .withTrimmedBottom(bx.getHeight() / 2)
                                   .withTrimmedTop((1 - d) * bx.getHeight() / 2));
                }
                else
                {
                    g.fillRect(bx.reduced(0.5, 0)
                                   .withTrimmedTop(bx.getHeight() / 2)
                                   .withTrimmedBottom((1 + d) * bx.getHeight() / 2));
                }

                g.setColour(hanc);
                g.fillRect(hancbx.translated(0, -d * bx.getHeight() / 2));

                bx = bx.translated(w, 0);
            }

            if (curvePath.isEmpty() ||
                memcmp(&ms, &parent->modulatorStorageData[parent->selectedTab],
                       sizeof(modulation::ModulatorStorage)) != 0)
            {
                recalcCurve();
            }

            g.setColour(parent->editor->themeColor(theme::ColorMap::Colors::generic_content_high));
            g.strokePath(curvePath, juce::PathStrokeType(1));

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

        struct StepMenuTypein : shared::MenuValueTypeinBase
        {
            LfoPane *parent{nullptr};
            int idx;
            StepMenuTypein(SCXTEditor *e, int idx, LfoPane *r)
                : parent(r), shared::MenuValueTypeinBase(e), idx(idx)
            {
            }
            std::string getInitialText() const override
            {
                auto &ls = parent->modulatorStorageData[parent->selectedTab].stepLfoStorage;
                auto v = ls.data[idx];
                return fmt::format("{:7f}", v);
            }
            void setValueString(const std::string &s) override
            {
                auto lf = std::clamp(std::atof(s.c_str()), -1.0, 1.0);
                parent->stepLfoPane->setStep(idx, lf);
            }
            juce::Colour getValueColour() const override
            {
                return editor->themeColor(theme::ColorMap::accent_2b);
            }
        };
        void showTypeinPopup(const juce::Point<float> &f)
        {
            auto idx = indexForPosition(f);
            if (idx < 0)
                return;
            juce::PopupMenu p;
            p.addSectionHeader("LFO Step " + std::to_string(idx + 1));
            p.addSeparator();
            p.addCustomItem(-1, std::make_unique<StepMenuTypein>(parent->editor, idx, parent));
            p.showMenuAsync(parent->editor->defaultPopupMenuOptions());
        }
        void mouseDown(const juce::MouseEvent &event) override
        {
            if (event.mods.isPopupMenu())
            {
                showTypeinPopup(event.position);
                return;
            }
            handleMouseAt(event.position);
        }
        void mouseDrag(const juce::MouseEvent &event) override
        {
            if (event.mods.isPopupMenu())
            {
                return;
            }
            handleMouseAt(event.position);
        }
        void mouseDoubleClick(const juce::MouseEvent &event) override
        {
            auto idx = indexForPosition(event.position);
            if (idx < 0)
                return;
            parent->stepLfoPane->setStep(idx, 0);
            recalcCurve();
            repaint();
        }

        void recalcCurve()
        {
            curvePath.clear();

            // retain for compare
            ms = parent->modulatorStorageData[parent->selectedTab];
            // make a local copy for modification

            auto msCopy = ms;
            msCopy.stepLfoStorage.rateIsForSingleStep = true; // rate specifies one step
            msCopy.temposync = false;

            // Figure out how many steps. By forcing to rate for single the calculation
            // is pretty easy
            auto renderSR{48000};
            float rate{3.f}; // 8 steps a second
            float stepSamples{renderSR * msCopy.stepLfoStorage.repeat / std::pow(2.0f, rate) /
                              blockSize}; // how many samples in a step
            int sampleEvery = std::min((int)(stepSamples / (getWidth() * 3)), 1);

            sst::basic_blocks::modulators::Transport td{};
            sst::basic_blocks::dsp::RNG gen;

            auto so = std::make_unique<scxt::modulation::modulators::StepLFO>();
            so->setSampleRate(renderSR);
            gen.reseed(8675309);
            so->assign(&msCopy, &rate, &td, gen);

            for (int i = 0; i < stepSamples; ++i)
            {
                so->process(blockSize);
                auto outy = getHeight() / 2.f - so->output * getHeight() / 2.f;
                if (i == 0)
                {
                    curvePath.startNewSubPath(0, outy);
                }
                else if (i % sampleEvery == 0)
                {
                    curvePath.lineTo(i * 1.0 * getWidth() / (stepSamples), outy);
                }
            }
        }
    };

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

            if (A->description.canTemposync)
            {
                parent->setAttachmentAsTemposync(*A);
            }

            makeLabel(L, A->getLabel());
        };
        auto makeGlyph = [this](auto &c, sst::jucegui::components::GlyphPainter::GlyphType g) {
            c = std::make_unique<sst::jucegui::components::GlyphPainter>(g);
            addAndMakeVisible(*c);
        };

        ifac::attachAndAdd(ms, ms.stepLfoStorage.repeat, this, stepsA, stepsEd, parent->forZone,
                           parent->selectedTab);
        connectors::addGuiStep(*stepsA, [w = juce::Component::SafePointer(this)](const auto &a) {
            if (w)
                w->stepRender->recalcCurve();
        });
        makeGlyph(stepsGlyph, sst::jucegui::components::GlyphPainter::STEP_COUNT);
        // stepsA->setJogWrapsAtEnd(false);

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
        repaint();
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
    std::unique_ptr<jcmp::DraggableTextEditableValue> stepsEd;
    std::unique_ptr<jcmp::GlyphPainter> stepsGlyph;

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
        auto allKnobsWidth = b.getWidth() - allKnobsStartX - 40;
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
        stepsEd->setBounds(
            knobBounds.withHeight(buttonH).withTrimmedLeft(knobBounds.getWidth() / 2));
        stepsGlyph->setBounds(
            knobBounds.withHeight(buttonH).withTrimmedRight(knobBounds.getWidth() / 2));
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

    struct CurveDataSource : public sst::jucegui::data::CompactPlotSource
    {
        modulation::modulators::CurveLFO curveLfo;

        LfoPane *parent{nullptr};
        sst::basic_blocks::modulators::Transport dummyTransport;
        CurveDataSource(LfoPane *p) : parent(p)
        {
            dummyTransport.tempo = 120;
            dummyTransport.signature = {4, 4};
        }
        modulation::ModulatorStorage ms;

        bool curvePathIsValid() const override
        {
            auto msCopy = parent->modulatorStorageData[parent->selectedTab];
            return memcmp(&ms, &msCopy, sizeof(modulation::ModulatorStorage)) == 0;
        }

        void recalculateCurvePath(plotData_t &into) override
        {
            auto msCopy = parent->modulatorStorageData[parent->selectedTab];

            // msCopy.rate = 1.;
            auto sr{48000};
            auto ds{10};
            curveLfo.assign(&msCopy, &dummyTransport);
            curveLfo.setSampleRate(sr);
            curveLfo.attack(msCopy.start_phase, msCopy.curveLfoStorage.delay,
                            msCopy.modulatorShape);
            // OK so we want to run for two seconds
            auto pts = 2 * sr / blockSize;

            for (int i = 0; i < pts; ++i)
            {
                curveLfo.process(msCopy.rate, msCopy.curveLfoStorage.deform,
                                 msCopy.curveLfoStorage.angle, msCopy.curveLfoStorage.delay,
                                 msCopy.curveLfoStorage.attack, msCopy.curveLfoStorage.release,
                                 false /*msCopy.curveLfoStorage.useenv*/,
                                 msCopy.curveLfoStorage.unipolar, true);
                auto yp = -0.9 * curveLfo.output * 0.5 + 0.5;

                into.emplace_back(i * 1.0 / pts, yp);
            }
            ms = msCopy;
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
        auto aux = [&, this](auto &mem, auto &A, auto &S, auto &L, std::string lab = "") {
            fac::attachAndAdd(ms, mem, this, A, S, parent->forZone, parent->selectedTab);

            if (A->description.canTemposync)
            {
                parent->setAttachmentAsTemposync(*A);
            }
            if (lab.empty())
            {
                makeLabel(L, A->getLabel());
            }
            else
            {
                makeLabel(L, lab);
            }
        };

        aux(ms.rate, rateA, rateK, rateL);
        aux(ms.curveLfoStorage.deform, deformA, deformK, deformL);
        aux(ms.start_phase, phaseA, phaseK, phaseL);
        aux(ms.curveLfoStorage.angle, angleA, angleK, angleL);

        rateK->setDrawLabel(false);
        deformK->setDrawLabel(false);
        phaseK->setDrawLabel(false);

        aux(ms.curveLfoStorage.delay, envA[0], envS[0], envL[0], "D");
        aux(ms.curveLfoStorage.attack, envA[1], envS[1], envL[1], "A");
        aux(ms.curveLfoStorage.release, envA[2], envS[2], envL[2], "R");

        bfac::attachAndAdd(ms, ms.curveLfoStorage.unipolar, this, unipolarA, unipolarB,
                           parent->forZone, parent->selectedTab);
        unipolarB->setLabel("UNI");

        bfac::attachAndAdd(ms, ms.curveLfoStorage.useenv, this, useenvA, useenvB, parent->forZone,
                           parent->selectedTab);
        useenvB->setLabel("ENV");

        curveDraw = std::make_unique<jcmp::CompactPlot>();
        curveData = std::make_unique<CurveDataSource>(parent);
        curveDraw->setSource(curveData.get());
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
        auto allKnobsWidth = b.getWidth() - allKnobsStartX - 40;
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
        unipolarB->setBounds(0, bh / 2 - MG - 2 * buttonH, b.getWidth() / 4, buttonH);
        useenvB->setBounds(0, bh / 2 - buttonH, b.getWidth() / 4, buttonH);
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

    std::unique_ptr<CurveDataSource> curveData;
    std::unique_ptr<jcmp::CompactPlot> curveDraw;
};

struct ENVLFOPane : juce::Component, HasEditor
{
    LfoPane *parent{nullptr};
    struct ENVPlotData : sst::jucegui::data::CompactPlotSource
    {
        LfoPane *parent{nullptr};
        sst::basic_blocks::modulators::Transport dummyTransport;

        ENVPlotData(LfoPane *p) : parent(p)
        {
            dummyTransport.tempo = 120;
            dummyTransport.signature = {4, 4};
        }

        modulation::ModulatorStorage ms;
        axisBounds_t getYAxisBounds() const override { return {0, 1}; }
        bool curvePathIsValid() const override
        {
            const auto &msCopy = parent->modulatorStorageData[parent->selectedTab];

            auto res = memcmp(&ms, &msCopy, sizeof(modulation::ModulatorStorage)) == 0;
            return res;
        }
        void recalculateCurvePath(plotData_t &into) override
        {
            ms = parent->modulatorStorageData[parent->selectedTab];
            auto sr{48000};

            modulation::modulators::EnvLFO envLfo;
            envLfo.setSampleRate(sr);

            auto &es = ms.envLfoStorage;

            auto tt = envLfo.timeTakenBy(es.delay, es.attack, es.hold, es.decay, es.release);
            auto rt = envLfo.timeTakenBy(es.release);
            auto dahd = tt - rt;

            // So how long to 'gate' it for. Its just a choice really
            auto sustainTime = std::max(0.1f, dahd / 3);
            if (ms.envLfoStorage.loop)
                sustainTime = 0;
            auto gateTime = dahd + sustainTime;
            auto totalTime = tt + sustainTime;
            // Add a bit of post release
            totalTime *= 1.02;

            // Now we want to run the whole thing in a second so
            auto rateMulLin = totalTime; // think "1s / (1/totaltime) s" - rate!
            auto rateMul = std::log2(rateMulLin);

            auto obs = 1.0 * sr / blockSize;
            auto ds = (int)(obs / (500));
            auto relAfter = (int)std::floor(obs * gateTime / totalTime);

            envLfo.attack(es.delay, es.attack);
            for (int i = 1; i < obs; ++i)
            {
                bool isGated = i < relAfter;
                envLfo.process(es.delay, es.attack, es.hold, es.decay, es.sustain, es.release,
                               es.aShape, es.dShape, es.rShape, rateMul, isGated);
                if (i % ds == 0)
                {
                    into.emplace_back(i * 1.0 / obs, 1.0 - 0.98 * envLfo.output);
                }
            }
        }
    };

    std::unique_ptr<ENVPlotData> envData;
    std::unique_ptr<jcmp::CompactPlot> envDraw;

    ENVLFOPane(LfoPane *p) : parent(p), HasEditor(p->editor)
    {
        assert(parent);
        envDraw = std::make_unique<jcmp::CompactPlot>();
        envData = std::make_unique<ENVPlotData>(parent);
        envDraw->setSource(envData.get());
        addAndMakeVisible(*envDraw);

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

        auto makeO = [&, this](auto &mem, auto &A, auto &S, auto &L, const std::string &ln = "") {
            fac::attachAndAdd(ms, mem, this, A, S, parent->forZone, parent->selectedTab);

            if (ln.empty())
                makeLabel(L, A->getLabel());
            else
                makeLabel(L, ln);
        };

        makeO(ms.envLfoStorage.delay, delayA, delayS, delayL, "Dly");
        makeO(ms.envLfoStorage.attack, attackA, attackS, attackL, "A");
        makeO(ms.envLfoStorage.hold, holdA, holdS, holdL, "H");
        makeO(ms.envLfoStorage.decay, decayA, decayS, decayL, "D");
        makeO(ms.envLfoStorage.sustain, sustainA, sustainS, sustainL, "S");
        makeO(ms.envLfoStorage.release, releaseA, releaseS, releaseL, "R");

        makeO(ms.envLfoStorage.aShape, curveAA, curveAK, curveAL, "Curve A");
        makeO(ms.envLfoStorage.dShape, curveDA, curveDK, curveDL, "Curve D");
        makeO(ms.envLfoStorage.rShape, curveRA, curveRK, curveRL, "Curve R");
        makeO(ms.envLfoStorage.rateMul, rateMulA, rateMulK, rateMulL, "Rate");

        bfac::attachAndAdd(ms, ms.envLfoStorage.loop, this, loopA, loopB, parent->forZone,
                           parent->selectedTab);
        loopB->setLabel("LOOP");
    }

    void resized() override
    {
        auto lbHt = 15;

        auto b = getLocalBounds();

        auto bh = b.getHeight();
        auto curveBox = b.withY(bh / 2).withWidth(getWidth() / 3).withHeight(bh / 2);
        envDraw->setBounds(curveBox);

        // Knobs
        auto nKnobs = 4;
        auto allKnobsStartX = b.getWidth() / 4 + MG * 2;
        auto allKnobsStartY = 0;
        auto allKnobsWidth = b.getWidth() - allKnobsStartX - 40;
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
        makeKnobBounds(rateMulK, rateMulL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(curveAK, curveAL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(curveDK, curveDL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
        makeKnobBounds(curveRK, curveRL, knobBounds, lbHt, knobWidth, knobHeight, knobMg);
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

        auto buttonH = BUTTON_H;
        loopB->setBounds(0, bh / 2 - MG - 2 * buttonH, b.getWidth() / 4, buttonH);
    }

    std::unique_ptr<LfoPane::boolAttachment_t> loopA;
    std::unique_ptr<jcmp::ToggleButton> loopB;

    std::unique_ptr<LfoPane::attachment_t> delayA, attackA, holdA, decayA, sustainA, releaseA;
    std::unique_ptr<jcmp::VSlider> delayS, attackS, holdS, decayS, sustainS, releaseS;
    std::unique_ptr<jcmp::Knob> rateMulK, curveAK, curveDK, curveRK;
    std::unique_ptr<LfoPane::attachment_t> rateMulA, curveAA, curveDA, curveRA;
    std::unique_ptr<jcmp::Label> delayL, attackL, holdL, decayL, sustainL, releaseL, rateMulL,
        curveAL, curveDL, curveRL;
};

struct MSEGLFOPane : juce::Component
{
    MSEGLFOPane(modulation::ModulatorStorage &s) {}

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::red); }
};

struct MiscPanel : juce::Component, HasEditor
{
    modulation::MiscSourceStorage &ms;
    bool forZone{true};
    MiscPanel(bool forZone, HasEditor *pe, modulation::MiscSourceStorage &s)
        : forZone(forZone), ms(s), HasEditor(pe->editor)
    {
        buildComponents();
    }

    using intatt_t = connectors::PayloadDataAttachment<modulation::MiscSourceStorage, int32_t>;

    std::unique_ptr<jcmp::Label> phasorTitle, randomTitle;
    std::array<std::unique_ptr<jcmp::Label>, scxt::phasorsPerGroupOrZone> phasorABCs;
    std::array<std::unique_ptr<jcmp::Label>, scxt::phasorsPerGroupOrZone> phasorSlashes;
    std::array<std::unique_ptr<jcmp::MenuButton>, scxt::phasorsPerGroupOrZone> syncButtons;
    std::array<std::unique_ptr<jcmp::MenuButton>, scxt::phasorsPerGroupOrZone> divButtons;

    std::array<std::unique_ptr<jcmp::DraggableTextEditableValue>, scxt::phasorsPerGroupOrZone>
        numText, denText;
    std::array<std::unique_ptr<intatt_t>, scxt::phasorsPerGroupOrZone> numTextA, denTextA;

    std::array<bool, scxt::phasorsPerGroupOrZone> showNOverD{};

    struct NumDenQuantAtt : sst::jucegui::data::Continuous
    {
        intatt_t &num, &den;
        MiscPanel *parent{nullptr};
        int idx{-1};
        NumDenQuantAtt(intatt_t &n, intatt_t &d, MiscPanel *p, int i)
            : num(n), den(d), parent(p), idx(i)
        {
        }

        float value{0};
        float getValue() const override { return value; }
        float getDefaultValue() const override { return 0; }
        std::string getLabel() const override { return "NQ"; }
        void syncValue()
        {
            auto n = num.getValue();
            auto d = den.getValue();
            auto r = (int)std::round(log2(n * 1.f / d));

            value = r;
        }
        void setValueFromGUI(const float &f) override
        {
            value = f;
            auto v = (int)std::round(f);
            if (v < 0)
            {
                num.setValueFromGUI(1);
                den.setValueFromGUI(1 << (-v));
            }
            else
            {
                num.setValueFromGUI(1 << v);
                den.setValueFromGUI(1);
            }
        }
        void setValueFromModel(const float &f) override { value = f; }
        std::string getValueAsStringFor(float f) const override
        {
            auto v = (int)std::round(f);
            if (v < 0)
            {
                return std::string("1 / ") + std::to_string(1 << (-v));
            }

            return std::to_string(1 << v) + " Wh";
        }
        void setValueAsString(const std::string &s) override
        {
            if (s.empty())
            {
                setValueFromGUI(-2);
                return;
            }
            auto div = s.find("/");
            if (div == std::string::npos)
            {
                auto f = std::atoi(s.c_str());
                if (f <= 0)
                {
                    setValueFromGUI(-2);
                }
                else
                {
                    auto cv = (int)log2(f);
                    if (cv > getMax())
                    {
                        setValueFromGUI(-2);
                    }
                    else
                    {
                        setValueFromGUI(cv);
                    }
                }
            }
            else
            {
                auto s1 = s.substr(0, div);
                auto s2 = s.substr(div + 1);
                auto v1 = std::atoi(s1.c_str());
                auto v2 = std::atoi(s2.c_str());

                if (v1 != 1)
                {
                    setValueFromGUI(-2);
                    return;
                }
                if (v2 <= 0)
                {
                    setValueFromGUI(-2);
                }
                else
                {
                    auto cv = (int)log2(1.0 / v2);
                    setValueFromGUI(cv);
                }
            }

            if (s.find("d") != std::string::npos || s.find("D") != std::string::npos)
            {
                parent->setDivision(idx, modulation::modulators::PhasorStorage::DOTTED);
            }
            else if (s.find("t") != std::string::npos || s.find("T") != std::string::npos)
            {
                parent->setDivision(idx, modulation::modulators::PhasorStorage::TRIPLET);
            }
            else if (s.find("n") != std::string::npos || s.find("N") != std::string::npos)
            {
                parent->setDivision(idx, modulation::modulators::PhasorStorage::NOTE);
            }
        }
        float getMin() const override { return -6; }
        float getMax() const override { return 4; }
    };

    struct CustomDTEV : public jcmp::DraggableTextEditableValue
    {
        void paint(juce::Graphics &g) override
        {
            std::string msg;
            if (continuous())
            {
                msg = continuous()->getValueAsString();
                if (!displayUnits)
                    msg = continuous()->getValueAsStringWithoutUnits();
            }
            auto slp = msg.find("/");

            if (underlyingEditor->isVisible())
            {
                g.setColour(getColour(Styles::background));
                if (isHovered)
                    g.setColour(getColour(Styles::background_hover));
                g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.f);

                return;
            }

            g.setColour(getColour(Styles::background));
            if (isHovered)
                g.setColour(getColour(Styles::background_hover));

            if (slp == std::string::npos)
            {
                g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.f);
                g.setFont(getFont(Styles::labelfont));
                g.setColour(getColour(
                    Styles::value)); // on Hover, the text colour is intensionally the same.
                g.drawText(msg, getLocalBounds(), juce::Justification::centred);
            }
            else
            {
                // this matches resized below
                auto ls = getLocalBounds().withWidth(20);
                auto rs = getLocalBounds().withWidth(20).translated(getWidth() - 20, 0);

                g.fillRoundedRectangle(ls.toFloat(), 3.f);
                g.fillRoundedRectangle(rs.toFloat(), 3.f);

                g.setFont(getFont(Styles::labelfont));
                g.setColour(getColour(
                    Styles::value)); // on Hover, the text colour is intensionally the same.
                auto ns = msg.substr(0, slp);
                auto ds = msg.substr(slp + 1);
                g.drawText(ns, ls, juce::Justification::centred);
                g.drawText(ds, rs, juce::Justification::centred);
                g.setColour(style()->getColour(jcmp::Label::Styles::styleClass,
                                               jcmp::Label::Styles::labelcolor));
                g.drawText("/", getLocalBounds(), juce::Justification::centred);
            }
        }
    };

    std::array<std::unique_ptr<NumDenQuantAtt>, scxt::phasorsPerGroupOrZone> numDenQuantA;
    std::array<std::unique_ptr<CustomDTEV>, scxt::phasorsPerGroupOrZone> numDenQuantW;

    std::unique_ptr<jcmp::NamedPanelDivider> phasorRandDiv;

    std::array<std::unique_ptr<jcmp::Label>, scxt::randomsPerGroupOrZone> randomABCs;
    std::array<std::unique_ptr<jcmp::MenuButton>, scxt::randomsPerGroupOrZone> randomModeButtons;

    void buildComponents()
    {
        phasorTitle = std::make_unique<jcmp::Label>();
        phasorTitle->setText("PHASORS");
        phasorTitle->setJustification(juce::Justification::centred);
        addAndMakeVisible(*phasorTitle);
        randomTitle = std::make_unique<jcmp::Label>();
        randomTitle->setText("RANDOM");
        randomTitle->setJustification(juce::Justification::centred);
        addAndMakeVisible(*randomTitle);

        phasorRandDiv = std::make_unique<jcmp::NamedPanelDivider>();
        phasorRandDiv->isHorizontal = false;
        addAndMakeVisible(*phasorRandDiv);

        auto onChange = [w = juce::Component::SafePointer(this)](const auto &a) {
            if (!w)
                return;
            w->repushData();
            w->updateFromValues();
        };
        for (int i = 0; i < scxt::phasorsPerGroupOrZone; ++i)
        {
            phasorABCs[i] = std::make_unique<jcmp::Label>();
            std::string lab = "A";
            lab[0] += i;
            phasorABCs[i]->setText(lab);
            addAndMakeVisible(*phasorABCs[i]);

            syncButtons[i] = std::make_unique<jcmp::MenuButton>();
            syncButtons[i]->setLabel("SELECT MODE");
            syncButtons[i]->setOnCallback([i, w = juce::Component::SafePointer(this)]() {
                if (w)
                    w->showSyncMenu(i);
            });
            addAndMakeVisible(*syncButtons[i]);

            divButtons[i] = std::make_unique<jcmp::MenuButton>();
            divButtons[i]->setLabel("DIV");
            divButtons[i]->setOnCallback([i, w = juce::Component::SafePointer(this)]() {
                if (w)
                    w->showDivisionMenu(i);
            });
            addAndMakeVisible(*divButtons[i]);

            const auto &nd = scxt::datamodel::describeValue(ms.phasors[i], ms.phasors[i].numerator);
            const auto &dd =
                scxt::datamodel::describeValue(ms.phasors[i], ms.phasors[i].denominator);
            numTextA[i] = std::make_unique<intatt_t>(nd, onChange, ms.phasors[i].numerator);
            numText[i] = std::make_unique<jcmp::DraggableTextEditableValue>();
            numText[i]->setSource(numTextA[i].get());
            addChildComponent(*numText[i]);

            denTextA[i] = std::make_unique<intatt_t>(dd, onChange, ms.phasors[i].denominator);
            denText[i] = std::make_unique<jcmp::DraggableTextEditableValue>();
            denText[i]->setSource(denTextA[i].get());
            addChildComponent(*denText[i]);

            phasorSlashes[i] = std::make_unique<jcmp::Label>();
            phasorSlashes[i]->setText("/");
            addChildComponent(*phasorSlashes[i]);

            numDenQuantA[i] = std::make_unique<NumDenQuantAtt>(*numTextA[i], *denTextA[i], this, i);
            numDenQuantA[i]->syncValue();
            numDenQuantW[i] = std::make_unique<CustomDTEV>();
            numDenQuantW[i]->setSource(numDenQuantA[i].get());
            numDenQuantW[i]->onPopupMenu = [i, w = juce::Component::SafePointer(this)](auto &) {
                if (w)
                {
                    w->showQuantValueMenu(i);
                }
            };
            addAndMakeVisible(*numDenQuantW[i]);
            showNOverD[i] = false;
        }

        for (int i = 0; i < scxt::randomsPerGroupOrZone; ++i)
        {
            randomABCs[i] = std::make_unique<jcmp::Label>();
            std::string lab = "A";
            lab[0] += i;
            randomABCs[i]->setText(lab);
            addAndMakeVisible(*randomABCs[i]);

            randomModeButtons[i] = std::make_unique<jcmp::MenuButton>();
            randomModeButtons[i]->setLabel("MODE");
            randomModeButtons[i]->setOnCallback([i, w = juce::Component::SafePointer(this)]() {
                if (w)
                    w->showRandomModeMenu(i);
            });

            addAndMakeVisible(*randomModeButtons[i]);
        }

        updateFromValues();
        resized();
    }
    void resized()
    {
        int margin{3}, vmargin{5}, divw{3};
        float split{0.65f};
        float rowHeight = (getHeight() - 4 * vmargin) / (scxt::phasorsPerGroupOrZone + 1);
        static_assert(scxt::phasorsPerGroupOrZone == scxt::randomsPerGroupOrZone,
                      "Must be same for this layout to work");
        namespace jlo = sst::jucegui::layouts;
        auto lo = jlo::HList().at(0, 0).withAutoGap(margin);

        auto plo =
            jlo::VList().withWidth(getWidth() * split - 2 * margin - divw).withAutoGap(vmargin);
        plo.add(jlo::Component(*phasorTitle).withHeight(rowHeight));
        for (int i = 0; i < scxt::phasorsPerGroupOrZone; ++i)
        {
            auto prlo = jlo::HList().withAutoGap(margin).withHeight(rowHeight);
            prlo.add(jlo::Component(*phasorABCs[i]).withWidth(20));
            prlo.add(jlo::Component(*syncButtons[i]).expandToFill());
            prlo.addGap(5);
            if (showNOverD[i])
            {
                // this matches CustomDTEV::paint above
                prlo.add(jlo::Component(*numText[i]).withWidth(20));
                prlo.add(jlo::Component(*phasorSlashes[i]).withWidth(15));
                prlo.add(jlo::Component(*denText[i]).withWidth(20));
            }
            else
            {
                prlo.add(jlo::Component(*numDenQuantW[i]).withWidth(55 + 2 * margin));
            }
            prlo.add(jlo::Component(*divButtons[i]).withWidth(60));

            plo.add(prlo);
        }
        lo.add(plo);

        lo.add(jlo::Component(*phasorRandDiv).withWidth(divw).withHeight(getHeight()));

        auto rlo = jlo::VList()
                       .withWidth(getWidth() * (1 - split) - 2 * margin - divw)
                       .withAutoGap(vmargin);
        rlo.add(jlo::Component(*randomTitle).withHeight(20));
        for (int i = 0; i < scxt::randomsPerGroupOrZone; ++i)
        {
            auto rrlo = jlo::HList().withAutoGap(margin).withHeight(rowHeight);
            rrlo.add(jlo::Component(*randomABCs[i]).withWidth(20));
            rrlo.add(jlo::Component(*randomModeButtons[i]).expandToFill());

            rlo.add(rrlo);
        }
        lo.add(rlo);

        lo.doLayout();
    }
    void repushData()
    {
        sendToSerialization(cmsg::UpdateMiscmodStorageForSelectedGroupOrZone({forZone, ms}));
    }

    void showQuantValueMenu(int i)
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Value");
        p.addSeparator();
        auto gen = [&p, i, this](auto v, auto l) {
            auto that = this; // grrrr msvc
            p.addItem(l, true, v == (int)(numDenQuantA[i]->getValue()),
                      [i, w = juce::Component::SafePointer(that), v] {
                          if (!w)
                              return;
                          w->numDenQuantA[i]->setValueFromGUI(v);
                          w->repushData();
                          w->updateFromValues();
                          w->repaint();
                      });
        };
        for (int vv = numDenQuantA[i]->getMax(); vv >= numDenQuantA[i]->getMin(); --vv)
        {
            gen(vv, numDenQuantA[i]->getValueAsStringFor(vv));
        }

        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }
    void showSyncMenu(int i)
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Phasor Mode");
        p.addSeparator();
        auto gen = [&p, i, this](auto v, auto l) {
            auto that = this; // grrrr msvc
            p.addItem(l, true, v == ms.phasors[i].syncMode,
                      [i, w = juce::Component::SafePointer(that), v] {
                          if (!w)
                              return;
                          w->ms.phasors[i].syncMode = v;
                          w->repushData();
                          w->updateFromValues();
                      });
        };

        gen(modulation::modulators::PhasorStorage::SyncMode::SONGPOS, "Song Position");
        gen(modulation::modulators::PhasorStorage::SyncMode::VOICEPOS,
            forZone ? "Voice Position" : "Group Position");

        p.addSeparator();
        auto genP = [&p, i, this](auto ph, auto l) {
            auto that = this; // grrrr msvc
            p.addItem(juce::CharPointer_UTF8(l), true, std::fabs(ph - ms.phasors[i].phase) < 0.001,
                      [i, w = juce::Component::SafePointer(that), ph] {
                          if (!w)
                              return;
                          w->ms.phasors[i].phase = ph;
                          w->repushData();
                          w->updateFromValues();
                      });
        };
        genP(0.0, u8"0° phase");
        genP(0.25, u8"90° phase");
        genP(0.5, u8"180° phase");
        genP(0.75, u8"270° phase");
        genP(1.f, u8"Random phase");

        p.addSeparator();
        auto genD = [&p, i, this](auto d, auto l) {
            auto that = this; // grrrr msvc
            p.addItem(l, true, ms.phasors[i].direction == d,
                      [i, w = juce::Component::SafePointer(that), d] {
                          if (!w)
                              return;
                          w->ms.phasors[i].direction = d;
                          w->repushData();
                          w->updateFromValues();
                      });
        };
        genD(modulation::modulators::PhasorStorage::Direction::ASCENDING, "Ascending");
        genD(modulation::modulators::PhasorStorage::Direction::DESCENDING, "Descending");

        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }
    void showDivisionMenu(int i)
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Phasor Division");
        p.addSeparator();
        auto gen = [&p, i, this](auto v, auto l) {
            auto that = this;
            p.addItem(l, true, v == ms.phasors[i].division,
                      [i, w = juce::Component::SafePointer(that), v] {
                          if (!w)
                              return;
                          w->setDivision(i, v);
                      });
        };

        gen(modulation::modulators::PhasorStorage::Division::NOTE, "Note");
        gen(modulation::modulators::PhasorStorage::Division::DOTTED, "Dotted");
        gen(modulation::modulators::PhasorStorage::Division::TRIPLET, "Triplet");
        gen(modulation::modulators::PhasorStorage::Division::X_BPM, "x BPM");

        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }

    void setDivision(int i, modulation::modulators::PhasorStorage::Division d)
    {
        ms.phasors[i].division = d;
        quantizeForNTD(i);
        repushData();
        updateFromValues();
    }

    void quantizeForNTD(int i)
    {
        auto d = ms.phasors[i].division;
        if (d == modulation::modulators::PhasorStorage::Division::NOTE ||
            d == modulation::modulators::PhasorStorage::Division::DOTTED ||
            d == modulation::modulators::PhasorStorage::Division::TRIPLET)
        {
            auto n = ms.phasors[i].numerator;
            auto d = ms.phasors[i].denominator;
            auto r = (int)std::round(log2(n * 1.f / d));
            if (r < 0)
            {
                ms.phasors[i].numerator = 1;
                ms.phasors[i].denominator = 1 << (-r);
            }
            else
            {
                ms.phasors[i].numerator = 1 << r;
                ms.phasors[i].denominator = 1;
            }
            numDenQuantA[i]->syncValue();
        }
    }
    void showRandomModeMenu(int i)
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Random Distribution");
        p.addSeparator();
        auto gen = [&p, i, this](auto v, auto l) {
            auto that = this;
            p.addItem(l, true, v == ms.randoms[i].style,
                      [i, w = juce::Component::SafePointer(that), v] {
                          if (!w)
                              return;
                          w->ms.randoms[i].style = v;
                          w->repushData();
                          w->updateFromValues();
                      });
        };

        gen(modulation::modulators::RandomStorage::Style::UNIFORM_01, "Uniform 0-1");
        gen(modulation::modulators::RandomStorage::Style::UNIFORM_BIPOLAR, "Uniform Bipolar");
        gen(modulation::modulators::RandomStorage::Style::NORMAL, "Normal");
        gen(modulation::modulators::RandomStorage::Style::HALF_NORMAL, "Half Normal");

        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }

    void updateFromValues()
    {
        assert(phasorTitle);
        for (int i = 0; i < scxt::phasorsPerGroupOrZone; ++i)
        {
            const auto &ps = ms.phasors[i];
            std::string phLab{};
            if (ps.phase < 0.001)
                phLab = "";
            else if (ps.phase < 0.251)
                phLab = " +90°";
            else if (ps.phase < 0.501)
                phLab = " +180°";
            else if (ps.phase < 0.751)
                phLab = " +270°";
            else if (ps.phase > 0.999)
                phLab = " RND";
            switch (ps.syncMode)
            {
            case modulation::modulators::PhasorStorage::VOICEPOS:
                phLab = (forZone ? "VOICE" : "GROUP") + phLab;
                break;
            case modulation::modulators::PhasorStorage::SONGPOS:
                phLab = "SONG" + phLab;
                break;
            }
            syncButtons[i]->setLabel(phLab);

            switch (ps.division)
            {
            case modulation::modulators::PhasorStorage::NOTE:
                divButtons[i]->setLabel("NOTE");
                showNOverD[i] = false;
                break;
            case modulation::modulators::PhasorStorage::TRIPLET:
                divButtons[i]->setLabel("TRIP");
                showNOverD[i] = false;
                break;
            case modulation::modulators::PhasorStorage::DOTTED:
                divButtons[i]->setLabel("DOT");
                showNOverD[i] = false;
                break;
            case modulation::modulators::PhasorStorage::X_BPM:
                divButtons[i]->setLabel("x BPM");
                showNOverD[i] = true;
                break;
            }
            numDenQuantA[i]->syncValue();
            numDenQuantW[i]->setVisible(!showNOverD[i]);
            denText[i]->setVisible(showNOverD[i]);
            numText[i]->setVisible(showNOverD[i]);
            phasorSlashes[i]->setVisible(showNOverD[i]);
        }

        for (int i = 0; i < scxt::randomsPerGroupOrZone; ++i)
        {
            auto rs = ms.randoms[i];
            switch (rs.style)
            {
            case modulation::modulators::RandomStorage::UNIFORM_01:
                randomModeButtons[i]->setLabel("UNIFORM 0-1");
                break;
            case modulation::modulators::RandomStorage::UNIFORM_BIPOLAR:
                randomModeButtons[i]->setLabel(std::string("UNIFORM ") + u8"\U000000B1" + "1");
                break;
            case modulation::modulators::RandomStorage::NORMAL:
                randomModeButtons[i]->setLabel("NORMAL");
                break;
            case modulation::modulators::RandomStorage::HALF_NORMAL:
                randomModeButtons[i]->setLabel(std::string() + u8"\U000000BD" + " NORMAL");
                break;
            }
        }
        resized();
    }
};

struct ConsistencyLFOPane : juce::Component
{
    LfoPane *parent{nullptr};
    std::unique_ptr<jcmp::Label> conLabel;
    std::unique_ptr<jcmp::TextPushButton> conButton;
    ConsistencyLFOPane(LfoPane *p) : parent(p)
    {
        conLabel = std::make_unique<jcmp::Label>();
        conLabel->setText("Multiple LFO Types Selected");
        addAndMakeVisible(*conLabel);

        conButton = std::make_unique<jcmp::TextPushButton>();
        conButton->setLabel("Label This");
        conButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            w->parent->modulatorShapeA->setValueFromGUI(w->parent->modulatorShapeA->getValue());
        });
        addAndMakeVisible(*conButton);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(10).withTrimmedTop(20).withHeight(24);
        conLabel->setBounds(b);
        b = b.translated(0, 30);
        conButton->setBounds(b);
    }

    void visibilityChanged() override
    {
        auto &ms = parent->modulatorStorageData[parent->selectedTab];
        if (isVisible())
        {
            conButton->setLabel("Set all LFO shapes to " +
                                parent->modulatorShapeA->getValueAsString());
        }
    }
};

LfoPane::LfoPane(SCXTEditor *e, bool fz)
    : sst::jucegui::components::NamedPanel(""), HasEditor(e), forZone(fz)
{
    setContentAreaComponent(std::make_unique<juce::Component>());
    // setCustomClass(connectors::SCXTStyleSheetCreator::ModulationTabs);
    isTabbed = true;
    tabNames = {"LFO 1", "LFO 2", "LFO 3", "LFO 4", "MISC"};

    resetTabState();

    onTabSelected = [wt = juce::Component::SafePointer(this)](int i) {
        if (wt)
            wt->tabChanged(i);
    };
    setEnabled(false);
}

LfoPane::~LfoPane() { getContentAreaComponent()->removeAllChildren(); }

void LfoPane::setTabsForGLFO()
{
    tabNames = {"GLFO 1", "GLFO 2", "GLFO 3", "GLFO 4", "MISC"};
    resetTabState();
}

void LfoPane::resized()
{
    getContentAreaComponent()->setBounds(getContentArea());
    repositionContentAreaComponents();
}

void LfoPane::tabChanged(int i)
{
    getContentAreaComponent()->removeAllChildren();
    rebuildPanelComponents();
    auto kn = std::string("multi") + (forZone ? ".zone.lfo" : ".group.lfo");
    editor->setTabSelection(editor->editScreen->tabKey(kn), std::to_string(i));
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

void LfoPane::setMiscModStorage(const modulation::MiscSourceStorage &mod)
{
    miscStorageData = mod;

    if (selectedTab != 4)
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

    if (selectedTab == 4)
    {
        clearAdditionalHamburgerComponents();

        miscPanel = std::make_unique<MiscPanel>(forZone, this, miscStorageData);
        getContentAreaComponent()->addAndMakeVisible(*miscPanel);
        resized();

        if (forZone)
        {
            editor->themeApplier.applyZoneMultiScreenModulationTheme(this);
        }
        else
        {
            editor->themeApplier.applyGroupMultiScreenModulationTheme(this);
        }
        return;
    }

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

    consistencyLfoPane = std::make_unique<ConsistencyLFOPane>(this);
    getContentAreaComponent()->addChildComponent(*consistencyLfoPane);

    modulatorShapeA = sfac::attachOnly(ms, ms.modulatorShape, this, forZone, selectedTab);
    modulatorShape = std::make_unique<jcmp::MenuButton>();
    modulatorShape->setLabel(getShapeMenuLabel());
    modulatorShape->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->showModulatorShapeMenu();
    });
    getContentAreaComponent()->addAndMakeVisible(*modulatorShape);

    // getContentAreaComponent()->addAndMakeVisible(*modulatorShape);

    repositionContentAreaComponents();
    setSubPaneVisibility();

    std::unique_ptr<jcmp::ToggleButton> tsb;
    using tsfac = connectors::BooleanSingleValueFactory<boolAttachment_t,
                                                        cmsg::UpdateZoneOrGroupModStorageBoolValue>;
    tsfac::attach(ms, ms.temposync, this, tempoSyncA, tsb, forZone, selectedTab);
    tsb->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH);
    tsb->setGlyph(jcmp::GlyphPainter::METRONOME);
    setupWidgetForValueTooltip(tsb.get(), tempoSyncA);

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
    if (miscPanel && selectedTab == 4)
    {
        miscPanel->setBounds(0, 0, getContentArea().getWidth(), getContentArea().getHeight());
        return;
    }
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
    consistencyLfoPane->setBounds(paneArea);
    modulatorShape->setBounds(mg, 0, paneArea.getWidth() / 4, ht);
}

void LfoPane::setSubPaneVisibility()
{
    if (!stepLfoPane || !msegLfoPane || !curveLfoPane || !consistencyLfoPane || !envLfoPane)
        return;

    auto &ms = modulatorStorageData[selectedTab];

    auto con = ms.modulatorConsistent;

    stepLfoPane->setVisible(ms.isStep() && con);
    msegLfoPane->setVisible(ms.isMSEG() && con);
    curveLfoPane->setVisible(ms.isCurve() && con);
    envLfoPane->setVisible(ms.isEnv() && con);
    consistencyLfoPane->setVisible(!con);
    modulatorShape->setVisible(con);
}

void LfoPane::showModulatorShapeMenu()
{
    auto &ms = modulatorStorageData[selectedTab];
    auto pmd = scxt::datamodel::describeValue(modulatorStorageData[selectedTab],
                                              modulatorStorageData[selectedTab].modulatorShape);

    auto makeInit = [that = this](auto v, float angle = 0.f) {
        return [w = juce::Component::SafePointer(that), v, angle]() {
            if (!w)
                return;

            auto &ms = w->modulatorStorageData[w->selectedTab];
            ms.modulatorShape = v;
            ms.curveLfoStorage.angle = angle;
            w->sendToSerialization(cmsg::UpdateFullModStorageForGroupsOrZones(
                {w->forZone, w->selectedTab, w->modulatorStorageData[w->selectedTab]}));
        };
    };
    auto p = juce::PopupMenu();
    p.addSectionHeader("Modulator Shapes");
    p.addSeparator();
    p.addItem("Step Sequencer", makeInit(modulation::ModulatorStorage::ModulatorShape::STEP));
    p.addItem("Envelope", makeInit(modulation::ModulatorStorage::ModulatorShape::LFO_ENV));
    auto c = juce::PopupMenu();
    c.addItem("Sine", makeInit(modulation::ModulatorStorage::ModulatorShape::LFO_SINE));
    c.addItem("Saw", makeInit(modulation::ModulatorStorage::ModulatorShape::LFO_SAW_TRI_RAMP, 1.0));
    c.addItem("Ramp",
              makeInit(modulation::ModulatorStorage::ModulatorShape::LFO_SAW_TRI_RAMP, -1.0));
    c.addItem("Pulse", makeInit(modulation::ModulatorStorage::ModulatorShape::LFO_PULSE));
    c.addItem("Tri", makeInit(modulation::ModulatorStorage::ModulatorShape::LFO_PULSE, 1.0));
    c.addItem("Smooth Noise",
              makeInit(modulation::ModulatorStorage::ModulatorShape::LFO_SMOOTH_NOISE));
    c.addItem("S&H Noise", makeInit(modulation::ModulatorStorage::ModulatorShape::LFO_SH_NOISE));
    p.addSubMenu("Curves", c);
    p.addSeparator();
    p.addItem("Factory Presets", editor->makeComingSoon("Factory LFO Presets"));
    p.addItem("User Presets", editor->makeComingSoon("Factory LFO Presets"));
    p.addSeparator();
    p.addItem("Save User Preset", [this]() { doSavePreset(); });
    p.addItem("Load User Preset", [this]() { doLoadPreset(); });

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

std::string LfoPane::getShapeMenuLabel()
{
    auto &ms = modulatorStorageData[selectedTab];
    return *modulatorShapeA->description.valueToString((float)ms.modulatorShape);
}

void LfoPane::doLoadPreset()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Modulator Preset", juce::File(editor->browser.modulatorPresetDirectory.u8string()),
        "*.scmod");
    fileChooser->launchAsync(juce::FileBrowserComponent::canSelectFiles |
                                 juce::FileBrowserComponent::openMode,
                             [w = juce::Component::SafePointer(this)](const juce::FileChooser &c) {
                                 auto result = c.getResults();
                                 if (result.isEmpty() || result.size() > 1)
                                 {
                                     return;
                                 }
                                 auto f = result.getFirst();
                                 auto content = f.loadFileAsString().toStdString();
                                 if (!content.empty())
                                 {
                                     w->unstreamFromJSON(content);
                                 }
                             });
}
void LfoPane::doSavePreset()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Modulator Preset", juce::File(editor->browser.modulatorPresetDirectory.u8string()),
        "*.scmod");
    fileChooser->launchAsync(juce::FileBrowserComponent::canSelectFiles |
                                 juce::FileBrowserComponent::saveMode |
                                 juce::FileBrowserComponent::warnAboutOverwriting,
                             [w = juce::Component::SafePointer(this)](const juce::FileChooser &c) {
                                 auto result = c.getResults();
                                 if (result.isEmpty() || result.size() > 1)
                                 {
                                     return;
                                 }
                                 auto f = result.getFirst();
                                 if (f.existsAsFile() || f.create())
                                 {
                                     auto s = w->streamToJSON();
                                     if (s.has_value())
                                         f.replaceWithText(*s);
                                 }
                             });
}

using ssmap_t = std::map<std::string, uint64_t>;
using streamLfo_t = std::tuple<ssmap_t, modulation::ModulatorStorage>;
std::optional<std::string> LfoPane::streamToJSON() const
{
    using ssmap_t = std::map<std::string, uint64_t>;
    ssmap_t ssmap;
    ssmap["modulatorStreamVersion"] = 1;
    ssmap["engineStreamVersion"] = scxt::currentStreamingVersion;
    streamLfo_t versionedStorage{ssmap, modulatorStorageData[selectedTab]};

    try
    {
        auto jsonv = json::scxt_value(versionedStorage);
        std::ostringstream oss;
        tao::json::events::transformer<tao::json::events::to_pretty_stream> consumer(oss, 3);
        tao::json::events::from_value(consumer, jsonv);
        return oss.str();
    }
    catch (const std::exception &e)
    {
        editor->displayError("LFO Preset Stream Error", e.what());
    }
    return std::nullopt;
}

void LfoPane::unstreamFromJSON(const std::string &json)
{
    try
    {
        streamLfo_t versionedStorage;
        tao::json::events::transformer<tao::json::events::to_basic_value<json::scxt_traits>>
            consumer;
        tao::json::events::from_string(consumer, json);
        auto jv = std::move(consumer.value);
        jv.to(versionedStorage);

        auto &config = std::get<0>(versionedStorage);
        auto &ms = std::get<1>(versionedStorage);

        modulatorStorageData[selectedTab] = ms;
        sendToSerialization(cmsg::UpdateFullModStorageForGroupsOrZones({forZone, selectedTab, ms}));
    }
    catch (const std::exception &e)
    {
        editor->displayError("LFO Preset Unstream Error", e.what());
    }
}

} // namespace scxt::ui::app::edit_screen
