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

#include "ProcessorPane.h"
#include "components/SCXTEditor.h"
#include "sst/jucegui/components/VSlider.h"

// We include the DSP code here so we can do a UI-side render of the EQ cuve
#include "sst/voice-effects/eq/EqNBandParametric.h"
#include "sst/basic-blocks/tables/DbToLinearProvider.h"
#include "sst/basic-blocks/tables/EqualTuningProvider.h"

#include "sst/jucegui/components/MultiSwitch.h"

#include "theme/Layout.h"

namespace scxt::ui::multi
{

namespace jcmp = sst::jucegui::components;

struct EqDisplay : juce::Component
{
    const ProcessorPane &mProcessorPane;

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

    EqDisplay(const ProcessorPane &p) : mProcessorPane(p)
    {
        mOneBand.setStorage(&mProcessorPane.processorView);
        mTwoBand.setStorage(&mProcessorPane.processorView);
        mThreeBand.setStorage(&mProcessorPane.processorView);
        rebuildCurves();
        bandSelect = std::make_unique<BandSelect>(nBands);
    }

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
            void setStorage(const dsp::processor::ProcessorStorage *p) { mStorage = p; }
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
            // SCLOG("Query Int Param " << idx << " " << bc->mStorage->intParams[idx]);
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
    sst::voice_effects::eq::EqNBandParametric<EqAdapter, 1> mOneBand;
    sst::voice_effects::eq::EqNBandParametric<EqAdapter, 2> mTwoBand;
    sst::voice_effects::eq::EqNBandParametric<EqAdapter, 3> mThreeBand;

    struct pt
    {
        float freq{0}, x{0}, y{0};
    };
    using ptvec_t = std::vector<pt>;
    std::array<ptvec_t, 4> curves; // 0 is total response 1..nis per band
    int nBands{-1};
    void rebuildCurves()
    {
        auto np = getWidth();

        float d[scxt::blockSize];
        for (int i = 0; i < scxt::blockSize; ++i)
            d[i] = 0.f;
        switch (mProcessorPane.processorView.type)
        {
        case dsp::processor::proct_eq_1band_parametric_A:
        {
            nBands = 1;
            mOneBand.calc_coeffs();
            for (int i = 0; i < 50; ++i)
                mOneBand.processMonoToMono(d, d, 60);
            break;
        }
        case dsp::processor::proct_eq_2band_parametric_A:
        {
            nBands = 2;
            mTwoBand.calc_coeffs();
            for (int i = 0; i < 50; ++i)
                mTwoBand.processMonoToMono(d, d, 60);
            break;
        }
        case dsp::processor::proct_eq_3band_parametric_A:
        {
            nBands = 3;
            mThreeBand.calc_coeffs();
            for (int i = 0; i < 50; ++i)
                mThreeBand.processMonoToMono(d, d, 60);
        }
        break;
        default:
            return;
        }

        for (auto &c : curves)
            c.clear();

        auto nr = -1000000;
        for (int i = 0; i < np; ++i)
        {
            float norm = 1.0 * i / (np - 1);
            auto freq = pow(2.f, 3 + norm * 11.5);
            auto freqarg = freq * EqAdapter::getSampleRateInv(nullptr);
            auto res = 0.f;

            switch (mProcessorPane.processorView.type)
            {
            case dsp::processor::proct_eq_1band_parametric_A:
            {
                res = mOneBand.getFrequencyGraph(freqarg);
                curves[0].push_back(pt{(float)freq, i * 1.f, res});
                curves[1].push_back(pt{(float)freq, i * 1.f, res});
            }
            break;
            case dsp::processor::proct_eq_2band_parametric_A:
            {
                res = mTwoBand.getFrequencyGraph(freqarg);
                curves[0].push_back(pt{(float)freq, i * 1.f, res});
                for (int j = 0; j < nBands; ++j)
                    curves[j + 1].push_back(
                        pt{(float)freq, i * 1.f, mTwoBand.getBandFrequencyGraph(j, freqarg)});
            }
            break;
            case dsp::processor::proct_eq_3band_parametric_A:
            {
                res = mThreeBand.getFrequencyGraph(freqarg);
                curves[0].push_back(pt{(float)freq, i * 1.f, res});
                for (int j = 0; j < nBands; ++j)
                    curves[j + 1].push_back(
                        pt{(float)freq, i * 1.f, mThreeBand.getBandFrequencyGraph(j, freqarg)});
            }
            break;
            default:
                return;
            }
        }
        repaint();
    }
    void paint(juce::Graphics &g) override
    {
        if (nBands < 0)
            rebuildCurves();

        auto &colorMap = mProcessorPane.editor->themeApplier.colorMap();

        auto c2p = [this](auto &c) {
            auto p = juce::Path();
            bool first{true};
            for (auto &pt : c)
            {
                auto sp = getHeight() / 2 - log2(pt.y) * 9;
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

        g.setColour(colorMap->get(theme::ColorMap::bg_1));
        g.fillRect(getLocalBounds());

        g.setColour(colorMap->get(theme::ColorMap::panel_outline_2));
        g.drawRect(getLocalBounds());

        for (int i = 0; i < nBands; ++i)
        {
            auto p = c2p(curves[i + 1]);
            g.setColour(colorMap->get(theme::ColorMap::accent_1b).withAlpha(0.7f));
            g.strokePath(p, juce::PathStrokeType(1));
        }
        auto p = c2p(curves[0]);
        g.setColour(colorMap->get(theme::ColorMap::accent_1a));
        g.strokePath(p, juce::PathStrokeType(3));
    }
    void resized() override { rebuildCurves(); }
};
void ProcessorPane::layoutControlsEQNBandParm()
{
    auto eqdisp = std::make_unique<EqDisplay>(*this);
    auto bd = getContentAreaComponent()->getLocalBounds();
    auto slWidth = 20;
    auto eq = bd.withTrimmedRight(slWidth);
    auto mx = bd.withLeft(bd.getWidth() - slWidth);

    eqdisp->setBounds(eq.withTrimmedTop(60));
    getContentAreaComponent()->addAndMakeVisible(*eqdisp);

    namespace lo = theme::layout;
    namespace locon = lo::constants;
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
    std::vector<std::string> lab = {"Gain", "Freq", "BW"};
    for (int i = 0; i < 3 * eqdisp->nBands; ++i)
    {
        floatEditors[i] = createWidgetAttachedTo(floatAttachments[i],
                                                 lab[i % 3] + " " + std::to_string(i / 3 + 1));
        auto orig = floatAttachments[i]->onGuiValueChanged;
        floatAttachments[i]->onGuiValueChanged =
            [orig, w = juce::Component::SafePointer(eqdisp.get())](const auto &a) {
                orig(a);
                if (w)
                {
                    w->rebuildCurves();
                }
            };
        lo::knobCX<locon::mediumSmallKnob>(*floatEditors[i], cols[(i % 3) + 1].getCentreX(), 5);
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

    mixEditor = createWidgetAttachedTo<sst::jucegui::components::VSlider>(mixAttachment, "Mix");
    mixEditor->item->setBounds(mx);

    otherEditors.push_back(std::move(eqdisp));
}
} // namespace scxt::ui::multi