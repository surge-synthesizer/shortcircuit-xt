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

#include "ProcessorPane.h"
#include "components/SCXTEditor.h"
#include "sst/jucegui/components/VSlider.h"

// We include the DSP code here so we can do a UI-side render of the EQ cuve
#include "sst/voice-effects/eq/EqNBandParametric.h"
#include "sst/basic-blocks/tables/DbToLinearProvider.h"
#include "sst/basic-blocks/tables/EqualTuningProvider.h"

namespace scxt::ui::multi
{

struct EqDisplay : juce::Component
{
    const ProcessorPane &mProcessorPane;

    EqDisplay(const ProcessorPane &p) : mProcessorPane(p)
    {
        mOneBand.setStorage(&mProcessorPane.processorView);
        mTwoBand.setStorage(&mProcessorPane.processorView);
        mThreeBand.setStorage(&mProcessorPane.processorView);
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
        static int getIntParam(const BaseClass *, int) { return 0.f; }

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
    void paint(juce::Graphics &g)
    {
        auto np = getWidth();

        g.fillAll(juce::Colours::black);
        switch (mProcessorPane.processorView.type)
        {
        case dsp::processor::proct_eq_1band_parametric_A:
            mOneBand.calc_coeffs();
            break;
        case dsp::processor::proct_eq_2band_parametric_A:
            mTwoBand.calc_coeffs();
            break;
        case dsp::processor::proct_eq_3band_parametric_A:
            mThreeBand.calc_coeffs();
            break;
        default:
            return;
        }
        g.setColour(juce::Colours::white);
        for (int i = 0; i < np; ++i)
        {
            float norm = 1.0 * i / (np - 1);
            auto freq = pow(2.f, 5 + norm * 7.5);
            auto freqarg = freq * EqAdapter::getSampleRateInv(nullptr);
            auto res = 0.f;

            switch (mProcessorPane.processorView.type)
            {
            case dsp::processor::proct_eq_1band_parametric_A:
                res = mOneBand.getFrequencyGraph(freqarg);
                break;
            case dsp::processor::proct_eq_2band_parametric_A:
                res = mTwoBand.getFrequencyGraph(freqarg);
                break;
            case dsp::processor::proct_eq_3band_parametric_A:
                res = mThreeBand.getFrequencyGraph(freqarg);
                break;
            default:
                return;
            }

            auto y = getHeight() - res * 5;
            g.drawLine(i, y, i, getHeight(), 1);
        }
    }
};
void ProcessorPane::layoutControlsEQNBandParm()
{
    auto eqdisp = std::make_unique<EqDisplay>(*this);
    auto bd = getContentAreaComponent()->getLocalBounds();
    auto slWidth = 20;
    auto eq = bd.withTrimmedRight(slWidth);
    auto mx = bd.withLeft(bd.getWidth() - slWidth);

    eqdisp->setBounds(eq);
    getContentAreaComponent()->addAndMakeVisible(*eqdisp);
    otherEditors.push_back(std::move(eqdisp));

    mixEditor = attachContinuousTo<sst::jucegui::components::VSlider>(mixAttachment);
    mixEditor->setBounds(mx);
}
} // namespace scxt::ui::multi