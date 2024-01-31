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

struct StepLFOPane : juce::Component
{
    struct StepRender : juce::Component
    {
        LfoPane *parent{nullptr};
        StepRender(LfoPane *p) : parent{p} {}

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
            parent->pushCurrentModulatorStorageUpdate();
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
                                                            update(), ls.rateIsEntireCycle);
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

struct CurveLFOPane : juce::Component
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

    CurveLFOPane(LfoPane *p) : parent(p)
    {
        assert(parent);
        auto update = [r = juce::Component::SafePointer(this)]() {
            return [w = juce::Component::SafePointer(r)](const auto &a) {
                if (w && w->parent)
                {
                    auto p = w->parent;
                    p->pushCurrentModulatorStorageUpdate();
                    p->updateValueTooltip(a);
                    p->repaint();
                }
            };
        };

        using att = LfoPane::attachment_t;
        auto &ms = parent->modulatorStorageData[parent->selectedTab];
        auto &cs = ms.curveLfoStorage;

        rateA = std::make_unique<att>(datamodel::lfoModulationRate().withName("Rate"), update(),
                                      ms.rate);
        deformA = std::make_unique<att>(datamodel::pmd().asPercentBipolar().withName("Deform"),
                                        update(), cs.deform);
        phaseA = std::make_unique<att>(datamodel::pmd().asPercent().withName("Phase"), update(),
                                       ms.start_phase);
        amplitudeA = std::make_unique<att>(
            datamodel::pmd().asPercentBipolar().withName("Amplitude").withDefault(1), update(),
            cs.amplitude);

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

        r = mk(deformA, "Deform");
        deformK = std::move(r.first);
        deformL = std::move(r.second);

        r = mk(phaseA, "Phase");
        phaseK = std::move(r.first);
        phaseL = std::move(r.second);

        r = mk(amplitudeA, "Amp");
        amplitudeK = std::move(r.first);
        amplitudeL = std::move(r.second);

        unipolarA = std::make_unique<LfoPane::boolAttachment_t>("Unipolar", update(), cs.unipolar);
        unipolarB = std::make_unique<jcmp::ToggleButton>();
        unipolarB->setSource(unipolarA.get());
        unipolarB->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationMatrixToggle);
        unipolarB->setLabel("UNI");
        addAndMakeVisible(*unipolarB);

        envA[0] = std::make_unique<att>(datamodel::pmd().asPercent().withName("Delay"), update(),
                                        cs.delay);
        envA[1] = std::make_unique<att>(datamodel::pmd().asPercent().withName("Attack"), update(),
                                        cs.attack);
        envA[2] = std::make_unique<att>(datamodel::pmd().asPercent().withName("Release"), update(),
                                        cs.release);

        auto mkE = [this](auto b, int idx) {
            auto L = std::make_unique<jcmp::Label>();
            L->setText(b);
            addAndMakeVisible(*L);
            auto K = std::make_unique<jcmp::Knob>();
            K->setSource(envA[idx].get());
            K->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorKnob);
            parent->setupWidgetForValueTooltip(K, envA[idx]);
            addAndMakeVisible(*K);
            envL[idx] = std::move(L);
            envS[idx] = std::move(K);
        };
        mkE("Delay", 0);
        mkE("Attack", 1);
        mkE("Release", 2);

        curveDraw = std::make_unique<CurveDraw>(parent);
        addAndMakeVisible(*curveDraw);
        curveDraw->toBack();
    }

    void resized() override
    {
        auto knobReg = 58;
        auto mg = 3;
        auto b = getLocalBounds();

        auto top = b.withTrimmedBottom(knobReg);

        curveDraw->setBounds(0, 0, b.getRight() - mg, b.getHeight() - knobReg - mg);
        unipolarB->setBounds(b.getRight() - 32 - mg, 2, 30, 18);

        auto knobw = knobReg - 16;
        auto bot = b.withTrimmedTop(b.getHeight() - knobReg);

        auto bx = bot.withWidth(knobw).translated(0, 0);
        rateK->setBounds(bx.withHeight(knobw));
        rateL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);
        deformK->setBounds(bx.withHeight(knobw));
        deformL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);
        phaseK->setBounds(bx.withHeight(knobw));
        phaseL->setBounds(bx.withTrimmedTop(knobw));
        bx = bx.translated(knobw + mg, 0);
        amplitudeK->setBounds(bx.withHeight(knobw));
        amplitudeL->setBounds(bx.withTrimmedTop(knobw));

        for (int i = 0; i < envSlots; ++i)
        {
            bx = bx.translated(knobw + mg, 0);
            envS[i]->setBounds(bx.withHeight(knobw));
            envL[i]->setBounds(bx.withTrimmedTop(knobw));
        }
    }

    std::unique_ptr<LfoPane::attachment_t> rateA, deformA, phaseA, amplitudeA;

    std::unique_ptr<jcmp::Knob> rateK, deformK, phaseK, amplitudeK;
    std::unique_ptr<jcmp::Label> rateL, deformL, phaseL, amplitudeL;

    std::unique_ptr<LfoPane::boolAttachment_t> unipolarA;
    std::unique_ptr<jcmp::ToggleButton> unipolarB;

    static constexpr int envSlots{3}; // DAR
    std::array<std::unique_ptr<LfoPane::attachment_t>, envSlots> envA;
    std::array<std::unique_ptr<jcmp::Knob>, envSlots> envS;
    std::array<std::unique_ptr<jcmp::Label>, envSlots> envL;

    std::unique_ptr<CurveDraw> curveDraw;
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
            .withRange(modulation::ModulatorStorage::STEP, modulation::ModulatorStorage::MSEG)
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
    msegLfoPane->setBounds(paneArea);
    curveLfoPane->setBounds(paneArea);
}

void LfoPane::setSubPaneVisibility()
{
    if (!stepLfoPane || !msegLfoPane || !curveLfoPane)
        return;

    auto &ms = modulatorStorageData[selectedTab];

    stepLfoPane->setVisible(ms.modulatorShape == modulation::ModulatorStorage::STEP);
    msegLfoPane->setVisible(ms.modulatorShape == modulation::ModulatorStorage::MSEG);
    curveLfoPane->setVisible((ms.modulatorShape != modulation::ModulatorStorage::STEP) &&
                             (ms.modulatorShape != modulation::ModulatorStorage::MSEG));
}

namespace cmsg = scxt::messaging::client;

void LfoPane::pushCurrentModulatorStorageUpdate()
{
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