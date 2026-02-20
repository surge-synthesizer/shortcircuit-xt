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

#include "ProcessorPane.h"
#include "app/SCXTEditor.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/components/JogUpDownButton.h"
#include "theme/Layout.h"

// We include the DSP code here so we can do a UI-side render of the EQ cuve
#include "sst/voice-effects/eq/EqNBandParametric.h"
#include "sst/voice-effects/eq/MorphEQ.h"
#include "sst/voice-effects/eq/EqGraphic6Band.h"
#include "sst/basic-blocks/tables/DbToLinearProvider.h"
#include "sst/basic-blocks/tables/EqualTuningProvider.h"

#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/components/VSlider.h"

#include "theme/Layout.h"

namespace scxt::ui::app::edit_screen
{

namespace jcmp = sst::jucegui::components;

namespace lo = theme::layout;
namespace locon = lo::constants;

// sigh - need this since we are virtual in component land
struct EqDisplayBase : juce::Component
{
    virtual void rebuildCurves() = 0;
    int nBands{0};

    // bit of a hack - I hold a multiswitch data source we add to my parent
    struct BandSelect : sst::jucegui::data::Discrete
    {
        int nBands{0}, selBand{0};
        std::function<void(int)> onSelChanged{nullptr};
        BandSelect(int nb) : nBands{nb} {}
        int getMin() const override { return 0; }
        int getMax() const override { return nBands - 1; }
        std::string getValueAsStringFor(int i) const override { return std::to_string(i + 1); }
        void setValueAsString(const std::string &s) override
        {
            setValueFromGUI(std::atof(s.c_str()) - 1);
        }

        int getValue() const override { return selBand; }
        void setValueFromGUI(const int &f) override
        {
            selBand = f;
            if (onSelChanged)
                onSelChanged(selBand);
        }
        void setValueFromModel(const int &f) override { selBand = f; }
        std::string getLabel() const override { return "Band"; }
    };
    std::unique_ptr<BandSelect> bandSelect;

    struct EqAdapter
    {
        struct BaseClass
        {
            BaseClass()
            {
                mDbToLinear.init();
                mEqualTuning.init();
            }
            sst::basic_blocks::tables::DbToLinearProvider mDbToLinear;
            sst::basic_blocks::tables::EqualTuningProvider mEqualTuning;

            const dsp::processor::ProcessorStorage *mStorage{nullptr};
            void setStorage(dsp::processor::ProcessorStorage *p) { mStorage = p; }
        };
        static constexpr int blockSize{scxt::blockSize};
        static void setFloatParam(BaseClass *, int, float) {}
        static float getFloatParam(const BaseClass *bc, int idx)
        {
            return bc->mStorage->floatParams[idx];
        }

        static void setIntParam(BaseClass *, int, int) {}
        static int getIntParam(const BaseClass *bc, int idx)
        {
            return bc->mStorage->intParams[idx];
        }

        static float dbToLinear(const BaseClass *that, float f)
        {
            return that->mDbToLinear.dbToLinear(f);
        }
        static float equalNoteToPitch(const BaseClass *that, float f)
        {
            return that->mEqualTuning.note_to_pitch(f);
        }
        static float getSampleRate(const BaseClass *) { return 48000.f; }
        static float getSampleRateInv(const BaseClass *) { return 1.0 / 48000.f; }

        static void preReservePool(BaseClass *, size_t) {}
        static uint8_t *checkoutBlock(BaseClass *, size_t) { return nullptr; }
        static void returnBlock(BaseClass *, uint8_t *, size_t) {}
    };
};

template <typename Proc, int nSub> struct EqDisplaySupport : EqDisplayBase
{
    const ProcessorPane &mProcessorPane;
    Proc mProcessor;

    std::function<void(Proc &, int)> mPrepareBand{nullptr};

    EqDisplaySupport(ProcessorPane &p) : mProcessorPane(p)
    {
        this->nBands = nSub;
        mProcessor.setStorage(&p.processorView);
        rebuildCurves();
    }

    struct pt
    {
        float freq{0}, x{0}, y{0};
    };
    using ptvec_t = std::vector<pt>;
    std::array<ptvec_t, nSub + 1> curves; // 0 is total response 1..nis per band
    bool curvesBuilt{false};
    void rebuildCurves() override
    {
        auto np = getWidth();

        float d[scxt::blockSize];
        for (int i = 0; i < scxt::blockSize; ++i)
            d[i] = 0.f;

        mProcessor.calc_coeffs();
        for (int i = 0; i < 50; ++i)
            mProcessor.processMonoToMono(d, d, 60);

        for (auto &c : curves)
            c.clear();

        auto nr = -1000000;
        auto fStart = 3.0;
        auto fRange = 11.5;
        if (std::is_same<Proc, sst::voice_effects::eq::EqGraphic6Band<EqAdapter>>::value)
        {
            fStart = 5.0;
            fRange = 9.3;
        }
        for (int band = 0; band < nSub + 1; ++band)
        {
            if (band > 0 && mPrepareBand)
                mPrepareBand(mProcessor, band - 1);
            for (int i = 0; i < np; ++i)
            {
                float norm = 1.0 * i / (np - 1);
                auto freq = pow(2.f, fStart + norm * fRange);
                auto freqarg = freq * EqAdapter::getSampleRateInv(nullptr);
                auto res = 0.f;

                if (band == 0)
                {
                    res = mProcessor.getFrequencyGraph(freqarg);
                    curves[0].push_back(pt{(float)freq, i * 1.f, res});
                }
                else
                {
                    if constexpr (nSub > 0)
                    {
                        curves[band].push_back(
                            pt{(float)freq, i * 1.f,
                               mProcessor.getBandFrequencyGraph(band - 1, freqarg)});
                    }
                }
            }
        }
        curvesBuilt = true;
        repaint();
    }

    float centerPoint{0.5f};

    void paint(juce::Graphics &g) override
    {
        if (!curvesBuilt)
            rebuildCurves();

        auto ed = mProcessorPane.editor;

        auto c2p = [this](auto &c) {
            auto p = juce::Path();
            bool first{true};
            for (auto &pt : c)
            {
                auto sp = getHeight() * centerPoint - log2(pt.y) * 9;
                if (first)
                {
                    p.startNewSubPath(pt.x, sp);
                }
                else
                {
                    p.lineTo(pt.x, sp);
                }
                first = false;
            }
            return p;
        };

        g.setColour(ed->themeColor(theme::ColorMap::bg_1));
        g.fillRect(getLocalBounds());

        g.setColour(ed->themeColor(theme::ColorMap::panel_outline_2));
        g.drawRect(getLocalBounds());

        g.setColour(ed->themeColor(theme::ColorMap::accent_2b));
        g.drawLine(0, getHeight() * centerPoint, getWidth(), getHeight() * centerPoint);

        for (int i = 0; i < nSub; ++i)
        {
            auto p = c2p(curves[i + 1]);
            g.setColour(ed->themeColor(theme::ColorMap::accent_1b).withAlpha(0.7f));
            g.strokePath(p, juce::PathStrokeType(1));
        }
        auto p = c2p(curves[0]);
        g.setColour(ed->themeColor(theme::ColorMap::accent_1a));
        g.strokePath(p, juce::PathStrokeType(3));
    }
};

template <typename Proc, int nSub> struct EqNBandDisplay : EqDisplaySupport<Proc, nSub>
{
    EqNBandDisplay(ProcessorPane &p) : EqDisplaySupport<Proc, nSub>(p)
    {
        this->bandSelect = std::make_unique<EqDisplayBase::BandSelect>(nSub);
    }

    void resized() override { this->rebuildCurves(); }
};

void ProcessorPane::layoutControlsEQNBandParm()
{
    std::unique_ptr<EqDisplayBase> eqdisp;

    eqdisp = std::make_unique<
        EqNBandDisplay<sst::voice_effects::eq::EqNBandParametric<EqDisplayBase::EqAdapter, 3>, 3>>(
        *this);
    auto bd = getContentAreaComponent()->getLocalBounds();
    auto slWidth = 0;
    auto eq = bd.withTrimmedRight(slWidth);
    auto mx = bd.withLeft(bd.getWidth() - slWidth);

    eqdisp->setBounds(eq.withTrimmedTop(65));
    getContentAreaComponent()->addAndMakeVisible(*eqdisp);

    auto cols = lo::columns(eq, 4);

    if (eqdisp->nBands > 1)
    {
        auto ms = std::make_unique<jcmp::MultiSwitch>();
        ms->setSource(eqdisp->bandSelect.get());

        eqdisp->bandSelect->onSelChanged = [nb = eqdisp->nBands,
                                            w = juce::Component::SafePointer(this)](int b) {
            if (!w)
                return;
            for (int i = 0; i < nb * 3; ++i)
            {
                if (w->floatEditors[i])
                {
                    w->floatEditors[i]->setVisible(i >= b * 3 && i < b * 3 + 3);
                }
            }

            for (int i = 0; i < nb; ++i)
            {
                if (w->intEditors[i])
                    w->intEditors[i]->item->setVisible(i == b);
            }
        };
        getContentAreaComponent()->addAndMakeVisible(*ms);
        ms->setBounds(cols[0].withHeight(50).withWidth(cols[0].getWidth() / 2));
        otherEditors.push_back(std::move(ms));
    }
    auto lab = std::array{"Gain", "Freq", "BW"};
    for (int i = 0; i < 3 * eqdisp->nBands; ++i)
    {
        floatEditors[i] = createWidgetAttachedTo(
            floatAttachments[i], std::string(lab[i % 3]) + " " + std::to_string(i / 3 + 1));
        auto orig = floatAttachments[i]->onGuiValueChanged;
        floatAttachments[i]->onGuiValueChanged =
            [orig, w = juce::Component::SafePointer(eqdisp.get())](const auto &a) {
                orig(a);
                if (w)
                {
                    w->rebuildCurves();
                }
            };
        lo::knobCX<locon::mediumKnob>(*floatEditors[i], cols[(i % 3) + 1].getCentreX(), 2);
        if (i < 3)
            floatEditors[i]->setVisible(true);
        else
            floatEditors[i]->setVisible(false);

        if (i % 3 == 0)
        {
            intEditors[i / 3] =
                createWidgetAttachedTo<jcmp::MultiSwitch>(intAttachments[i / 3], "");
            intEditors[i / 3]->label->setVisible(false);
            if (eqdisp->nBands > 1)
            {
                intEditors[i / 3]->item->setBounds(cols[0]
                                                       .withHeight(50)
                                                       .withWidth(cols[0].getWidth() / 2)
                                                       .translated(cols[0].getWidth() / 2, 0));
            }
            else
            {
                intEditors[i / 3]->item->setBounds(cols[0].withHeight(50).reduced(10, 0));
            }
            intEditors[i / 3]->item->setVisible(i == 0);

            auto iorig = intAttachments[i / 3]->onGuiValueChanged;
            intAttachments[i / 3]->onGuiValueChanged =
                [iorig, w = juce::Component::SafePointer(eqdisp.get())](const auto &a) {
                    iorig(a);
                    if (w)
                    {
                        w->rebuildCurves();
                    }
                };
        }
    }

    otherEditors.push_back(std::move(eqdisp));
}

void ProcessorPane::layoutControlsEQMorph()
{
    auto eqdisp = std::make_unique<
        EqNBandDisplay<sst::voice_effects::eq::MorphEQ<EqDisplayBase::EqAdapter>, 2>>(*this);
    auto eq = getContentAreaComponent()->getLocalBounds();

    eqdisp->mPrepareBand = [](auto &proc, int band) { proc.calc_coeffs(true); };

    auto presets = eq.withHeight(16);

    auto thenRecalc = [w = juce::Component::SafePointer(eqdisp.get())](const auto &a) {
        if (w)
        {
            w->rebuildCurves();
        }
    };

    intEditors[0] = createWidgetAttachedTo<jcmp::JogUpDownButton>(intAttachments[0], "S0");
    getContentAreaComponent()->addAndMakeVisible(*intEditors[0]->item);
    intEditors[0]->item->setBounds(presets.withWidth(presets.getWidth() / 2).reduced(0, 2));
    intAttachments[0]->andThenOnGui(thenRecalc);

    intEditors[1] = createWidgetAttachedTo<jcmp::JogUpDownButton>(intAttachments[1], "S0");
    getContentAreaComponent()->addAndMakeVisible(*intEditors[1]->item);

    intEditors[1]->item->setBounds(presets.withWidth(presets.getWidth() / 2)
                                       .translated(presets.getWidth() / 2, 0)
                                       .reduced(0, 2));
    intAttachments[1]->andThenOnGui(thenRecalc);

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], "Morph");

    auto lab = std::array{"Morph", "Freq1", "Freq2", "Gain", "BW"};
    auto cols = lo::columns(presets.translated(0, 18).withHeight(38), 5);

    static_assert(lab.size() < maxProcessorFloatParams);
    for (int i = 0; i < lab.size(); ++i)
    {
        floatEditors[i] = createWidgetAttachedTo(floatAttachments[i], lab[i]);
        floatAttachments[i]->andThenOnGui(thenRecalc);

        lo::knobCX<locon::mediumSmallKnob>(*floatEditors[i], cols[i].getCentreX(), cols[i].getY());
    }

    eqdisp->setBounds(eq.withTrimmedTop(65));
    getContentAreaComponent()->addAndMakeVisible(*eqdisp);

    otherEditors.push_back(std::move(eqdisp));
}

void ProcessorPane::layoutControlsEQGraphic()
{
    auto eqdisp = std::make_unique<
        EqNBandDisplay<sst::voice_effects::eq::EqGraphic6Band<EqDisplayBase::EqAdapter>, 0>>(*this);
    auto eq = getContentAreaComponent()->getLocalBounds();
    auto sliderHeight = 65;

    auto thenRecalc = [w = juce::Component::SafePointer(eqdisp.get())](const auto &a) {
        if (w)
        {
            w->rebuildCurves();
        }
    };

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], "Morph");

    auto cols = lo::columns(eq.withHeight(sliderHeight), 6);

    for (int i = 0; i < 6; ++i)
    {
        floatEditors[i] = createWidgetAttachedTo<jcmp::VSlider>(floatAttachments[i],
                                                                floatAttachments[i]->getLabel());
        floatAttachments[i]->andThenOnGui(thenRecalc);

        floatEditors[i]->item->setBounds(cols[i].reduced(3, 0));
    }

    eqdisp->setBounds(eq.withTrimmedTop(sliderHeight));
    getContentAreaComponent()->addAndMakeVisible(*eqdisp);

    otherEditors.push_back(std::move(eqdisp));
}

} // namespace scxt::ui::app::edit_screen
