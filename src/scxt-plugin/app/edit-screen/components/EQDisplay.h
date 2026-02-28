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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_EQDISPLAY_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_EQDISPLAY_H

#include <functional>
#include <string>

#include "app/SCXTEditor.h"

#include "sst/jucegui/components/MultiSwitch.h"

// We include the DSP code here so we can do a UI-side render of the EQ curve

#include "sst/voice-effects/eq/EqNBandParametric.h"
#include "sst/voice-effects/eq/MorphEQ.h"
#include "sst/voice-effects/eq/EqGraphic6Band.h"
#include "sst/basic-blocks/tables/DbToLinearProvider.h"
#include "sst/basic-blocks/tables/EqualTuningProvider.h"

namespace scxt::ui::app::edit_screen
{

struct EqDisplayBase : juce::Component
{
    virtual void rebuildCurves() = 0;
    int nBands{0};

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
    ProcessorPane &mProcessorPane;
    Proc mProcessor;

    std::function<void(Proc &, int)> mPrepareBand{nullptr};

    explicit EqDisplaySupport(ProcessorPane &p) : mProcessorPane(p)
    {
        this->nBands = nSub;
        mProcessor.setStorage(&p.processorView);
        EqDisplaySupport<Proc, nSub>::rebuildCurves();
    }

    struct pt
    {
        float freq{0}, x{0}, y{0};
    };
    using ptvec_t = std::vector<pt>;
    std::array<ptvec_t, nSub + 1> curves; // 0 is total response 1..nis per band
    std::array<juce::Rectangle<float>, nSub> bandHitRects;
    std::array<bool, nSub> isBandHovered{};
    int draggingBand{-1};
    static constexpr int hotzoneSize = 5;
    bool paintHotzones{true};
    bool curvesBuilt{false};
    void rebuildCurves() override
    {
        auto width = getWidth();
        auto height = getHeight();

        float d[scxt::blockSize];
        for (int i = 0; i < scxt::blockSize; ++i)
            d[i] = 0.f;

        mProcessor.calc_coeffs();
        for (int i = 0; i < 50; ++i)
            mProcessor.processMonoToMono(d, d, 60);

        for (auto &c : curves)
            c.clear();

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
            for (int i = 0; i < width; ++i)
            {
                float norm = 1.0 * i / (width - 1);
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
        // Compute band hit rectangles from processor storage frequencies
        for (int band = 0; band < nSub; ++band)
        {
            // Freq param is at index band*3+1, stored as semitones with 0=440Hz
            auto freqParam = mProcessor.mStorage->floatParams[band * 3 + 1];
            auto freqHz = 440.f * std::pow(2.f, freqParam / 12.f);
            auto freqNorm = (std::log2(freqHz) - fStart) / fRange;
            auto xPos = freqNorm * (width - 1);

            auto gainParam = mProcessor.mStorage->floatParams[band * 3];
            auto gainNorm = (gainParam + 24.f) * 0.0208333f; // one 48th
            auto yPos = -gainNorm * height + height;

            bandHitRects[band] = juce::Rectangle<float>(
                xPos - hotzoneSize / 2.f, yPos - hotzoneSize / 2.f, hotzoneSize, hotzoneSize);
        }

        curvesBuilt = true;
        repaint();
    }

    float centerPoint{0.5f};

    void mouseMove(const juce::MouseEvent &e) override
    {
        bool anyChanged = false;
        for (int i = 0; i < nSub; ++i)
        {
            bool hovered = bandHitRects[i].expanded(hotzoneSize).contains(e.position);
            if (hovered != isBandHovered[i])
            {
                isBandHovered[i] = hovered;
                anyChanged = true;
            }
        }
        if (anyChanged)
            repaint();
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        draggingBand = -1;
        for (int i = 0; i < nSub; ++i)
        {
            if (bandHitRects[i].expanded(hotzoneSize).contains(e.position))
            {
                draggingBand = i;
                break;
            }
        }
    }
    void mouseDrag(const juce::MouseEvent &e) override
    {
        if (draggingBand < 0)
            return;

        auto fStart = 3.0f;
        auto fRange = 11.5f;
        if (std::is_same<Proc, sst::voice_effects::eq::EqGraphic6Band<EqAdapter>>::value)
        {
            fStart = 5.0f;
            fRange = 9.3f;
        }

        auto width = getWidth();
        auto logfreqHz = fStart + (e.position.x / (width - 1)) * fRange;
        auto freqParam = 12.f * (logfreqHz - std::log2(440.f));

        auto gainNorm = (-e.position.y + getHeight()) / getHeight();
        auto gain = gainNorm * 48.f - 24.f;

        {
            auto &a = mProcessorPane.floatAttachments[draggingBand * 3 + 1];
            if (freqParam >= a->getMin() && freqParam <= a->getMax())
                a->setValueFromGUI(freqParam);
        }
        {
            auto &a = mProcessorPane.floatAttachments[draggingBand * 3 + 0];
            if (gain >= a->getMin() && gain <= a->getMax())
                a->setValueFromGUI(gain);
        }
        repaint();
    }
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

        if (paintHotzones)
        {
            for (int i = 0; i < nSub; ++i)
            {
                auto col = ed->themeColor(theme::ColorMap::accent_1a).withAlpha(1.f);
                g.setColour(col);
                g.fillEllipse(bandHitRects[i].expanded(hotzoneSize));
                g.setColour(ed->themeColor(theme::ColorMap::accent_2b));
                g.drawText(std::to_string(i + 1), bandHitRects[i].expanded(hotzoneSize),
                           juce::Justification::centred, false);
                if (isBandHovered[i])
                {
                    auto c2 = ed->themeColor(theme::ColorMap::generic_content_high);
                    g.setColour(c2);
                    g.drawEllipse(bandHitRects[i].expanded(hotzoneSize), 1);
                }
            }
        }
    }
};

template <typename Proc, int nSub, bool showHotzones = true>
struct EqNBandDisplay : EqDisplaySupport<Proc, nSub>
{
    EqNBandDisplay(ProcessorPane &p) : EqDisplaySupport<Proc, nSub>(p)
    {
        this->bandSelect = std::make_unique<EqDisplayBase::BandSelect>(nSub);
        this->paintHotzones = showHotzones;
    }

    void resized() override { this->rebuildCurves(); }

    void mouseMove(const juce::MouseEvent &e) override
    {
        if constexpr (showHotzones)
            EqDisplaySupport<Proc, nSub>::mouseMove(e);
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        if constexpr (showHotzones)
        {
            EqDisplaySupport<Proc, nSub>::mouseDown(e);
            if (this->draggingBand >= 0 && this->bandSelect &&
                this->bandSelect->getValue() != this->draggingBand)
            {
                this->bandSelect->setValueFromGUI(this->draggingBand);
                this->mProcessorPane.repaint();
            }
        }
    }
    void mouseDrag(const juce::MouseEvent &e) override
    {
        if constexpr (showHotzones)
            EqDisplaySupport<Proc, nSub>::mouseDrag(e);
    }
    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override
    {
        if constexpr (showHotzones)
        {
            for (int i = 0; i < nSub; ++i)
            {
                if (this->isBandHovered[i])
                {
                    SCLOG_IF(debug,
                             "mouseWheelMove: band " << i << " wheel delta " << wheel.deltaY);
                    if (this->bandSelect && this->bandSelect->getValue() != i)
                        this->bandSelect->setValueFromGUI(i);
                    auto &a = this->mProcessorPane.floatAttachments[i * 3 + 2];
                    auto newVal = a->getValue() - wheel.deltaY * 5;
                    if (newVal >= a->getMin() && newVal <= a->getMax())
                        a->setValueFromGUI(newVal);
                    this->mProcessorPane.repaint();
                    break;
                }
            }
        }
    }
    void mouseMagnify(const juce::MouseEvent &e, float scaleFactor) override
    {
        if constexpr (showHotzones)
        {
            for (int i = 0; i < nSub; ++i)
            {
                if (this->isBandHovered[i])
                {
                    if (this->bandSelect && this->bandSelect->getValue() != i)
                        this->bandSelect->setValueFromGUI(i);
                    auto &a = this->mProcessorPane.floatAttachments[i * 3 + 2];
                    auto newVal = a->getValue() * scaleFactor;
                    if (newVal >= a->getMin() && newVal <= a->getMax())
                        a->setValueFromGUI(newVal);
                    this->mProcessorPane.repaint();
                    break;
                }
            }
        }
    }
};

using eq_t = sst::voice_effects::eq::EqNBandParametric<EqDisplayBase::EqAdapter, 3>;
struct EqRenderer3Band : EqNBandDisplay<eq_t, 3>
{
    explicit EqRenderer3Band(ProcessorPane &p) : EqNBandDisplay<eq_t, 3>(p) {}
};

} // namespace scxt::ui::app::edit_screen

#endif // SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_EQDISPLAY_H
