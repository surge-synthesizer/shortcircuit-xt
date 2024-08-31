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

#include "SampleWaveform.h"
#include "app/SCXTEditor.h"

#include "VariantDisplay.h"

namespace scxt::ui::app::edit_screen
{

SampleWaveform::SampleWaveform(VariantDisplay *d) : display(d), HasEditor(d->editor)
{
    addAndMakeVisible(samplePlaybackPosition);
}

void SampleWaveform::rebuildHotZones()
{
    static constexpr int hotZoneSize{10};
    auto &v = display->variantView.variants[display->selectedVariation];
    auto samp = editor->sampleManager.getSample(v.sampleID);
    if (!samp)
    {
        return;
    }
    auto r = getLocalBounds();

    auto fade = xPixelForSampleDistance(v.loopFade);
    auto start = xPixelForSample(v.startSample);
    auto end = xPixelForSample(v.endSample);
    auto ls = xPixelForSample(v.startLoop);
    auto le = xPixelForSample(v.endLoop);

    startSampleHZ = juce::Rectangle<int>(start + r.getX(), r.getBottom() - hotZoneSize, hotZoneSize,
                                         hotZoneSize);
    endSampleHZ = juce::Rectangle<int>(end + r.getX() - hotZoneSize, r.getBottom() - hotZoneSize,
                                       hotZoneSize, hotZoneSize);
    startLoopHZ = juce::Rectangle<int>(ls + r.getX(), r.getY(), hotZoneSize, hotZoneSize);
    endLoopHZ =
        juce::Rectangle<int>(le + r.getX() - hotZoneSize, r.getY(), hotZoneSize, hotZoneSize);

    fadeLoopHz = juce::Rectangle<int>(r.getX() + ls - fade, r.getY(), fade, r.getHeight());
}

int64_t SampleWaveform::sampleForXPixel(float xpos)
{
    auto r = getLocalBounds();
    auto &v = display->variantView.variants[display->selectedVariation];
    auto samp = editor->sampleManager.getSample(v.sampleID);
    if (!samp)
        return -1;

    // OK so going the other way
    // (sp / l - start) * zf * width = px (taht's below) so
    // px / (zf * width) = sp / l - start
    // (px / (zf * width) + start) * l = sp

    auto l = samp->getSampleLength();
    auto res = (xpos / (zoomFactor * getWidth()) + pctStart) * l;
    return (int64_t)std::clamp(res, 0.f, l * 1.f);
}

int SampleWaveform::xPixelForSampleDistance(int64_t sampleDistance)
{
    auto r = getLocalBounds();
    auto &v = display->variantView.variants[display->selectedVariation];
    auto sample = editor->sampleManager.getSample(v.sampleID);
    if (sample)
    {
        auto l = sample->getSampleLength();
        float sPct = 1.0 * sampleDistance / l;
        sPct *= zoomFactor;
        sPct *= getWidth();
        return sPct;
    }
    return 0;
}
int SampleWaveform::xPixelForSample(int64_t samplePos, bool doClamp)
{
    auto r = getLocalBounds();
    auto &v = display->variantView.variants[display->selectedVariation];
    auto sample = editor->sampleManager.getSample(v.sampleID);
    if (sample)
    {
        auto l = sample->getSampleLength();
        float sPct = 1.0 * samplePos / l;
        sPct -= pctStart;
        sPct *= zoomFactor;

        if (doClamp)
            return std::clamp(static_cast<int>(r.getWidth() * sPct), 0, r.getWidth());
        else
            return static_cast<int>(r.getWidth() * sPct);
    }
    else
    {
        return -1;
    }
}

juce::Path SampleWaveform::pathForSample()
{
    juce::Path res;

    auto r = getLocalBounds();
    auto &v = display->variantView.variants[display->selectedVariation];
    auto samp = editor->sampleManager.getSample(v.sampleID);
    if (!samp)
    {
        SCLOG("Null Sample: Null Path");
        return res;
    }

    /*
     * OK so we have pctStart and zoomFactor so whats that in sample space
     */
    auto l = samp->getSampleLength();
    auto startSample = std::clamp((int)std::floor(l * pctStart), 0, (int)l);
    auto numSamples = (int)std::ceil(1.f * l / zoomFactor);
    auto endSample = std::clamp(startSample + numSamples, 0, (int)l);

    std::vector<std::pair<size_t, float>> topLine, bottomLine;
    auto fac = 1.0 * numSamples / r.getWidth();
    fac = std::max(fac, 20.0);

    auto downSampleForUI = [startSample, endSample, fac, &topLine, &bottomLine](auto *data) {
        using T = std::remove_pointer_t<decltype(data)>;
        double c = startSample;
        int ct = 0;
        T mx = std::numeric_limits<T>::min();
        T mn = std::numeric_limits<T>::max();

        T normFactor{1};
        if constexpr (std::is_same_v<T, int16_t>)
        {
            normFactor = std::numeric_limits<T>::max();
        }
        for (int s = startSample; s < endSample; ++s)
        {
            if (c + fac < s)
            {
                double nmx = 1.0 - mx * 1.0 / normFactor;
                double nmn = 1.0 - mn * 1.0 / normFactor;

                nmx = (nmx + 1) * 0.25;
                nmn = (nmn + 1) * 0.25;

                topLine.emplace_back(s, nmx);
                bottomLine.emplace_back(s, nmn);

                c += fac;
                ct++;
                mx = std::numeric_limits<T>::min();
                mn = std::numeric_limits<T>::max();
            }
            mx = std::max(data[s], mx);
            mn = std::min(data[s], mn);
        }
    };

    if (samp->bitDepth == sample::Sample::BD_I16)
    {
        auto d = samp->GetSamplePtrI16(0);
        downSampleForUI(d);
    }
    else if (samp->bitDepth == sample::Sample::BD_F32)
    {
        auto d = samp->GetSamplePtrF32(0);
        downSampleForUI(d);
    }
    else
    {
        jassertfalse;
    }

    std::reverse(bottomLine.begin(), bottomLine.end());
    bool first{true};
    for (auto &[smp, val] : topLine)
    {
        auto pos = xPixelForSample(smp);
        if (first)
        {
            res.startNewSubPath(pos, val * r.getHeight());
            first = false;
        }
        else
        {
            res.lineTo(pos, val * r.getHeight());
        }
    }

    for (auto &[smp, val] : bottomLine)
    {
        auto pos = xPixelForSample(smp);

        res.lineTo(pos, val * r.getHeight());
    }
    res.closeSubPath();

    return res;
}

void SampleWaveform::mouseDown(const juce::MouseEvent &e)
{
    auto posi = e.position.roundToInt();
    if (startSampleHZ.contains(posi))
        mouseState = MouseState::HZ_DRAG_SAMPSTART;
    else if (endSampleHZ.contains(posi))
        mouseState = MouseState::HZ_DRAG_SAMPEND;
    // TODO loopActive check here
    else if (startLoopHZ.contains(posi))
        mouseState = MouseState::HZ_DRAG_LOOPSTART;
    else if (endLoopHZ.contains(posi))
        mouseState = MouseState::HZ_DRAG_LOOPEND;
    else
        mouseState = MouseState::NONE;

    // TODO cursor change and so on
}

void SampleWaveform::mouseDrag(const juce::MouseEvent &e)
{
    if (mouseState == MouseState::NONE)
        return;

    switch (mouseState)
    {
    case MouseState::HZ_DRAG_SAMPSTART:
        display->variantView.variants[display->selectedVariation].startSample =
            sampleForXPixel(e.position.x);
        break;
    case MouseState::HZ_DRAG_SAMPEND:
        display->variantView.variants[display->selectedVariation].endSample =
            sampleForXPixel(e.position.x);
        break;
    case MouseState::HZ_DRAG_LOOPSTART:
        display->variantView.variants[display->selectedVariation].startLoop =
            sampleForXPixel(e.position.x);
        break;
    case MouseState::HZ_DRAG_LOOPEND:
        display->variantView.variants[display->selectedVariation].endLoop =
            sampleForXPixel(e.position.x);
        break;
    default:
        break;
    }
    display->onSamplePointChangedFromGUI();
}

void SampleWaveform::mouseUp(const juce::MouseEvent &e)
{
    if (mouseState != MouseState::NONE)
        display->onSamplePointChangedFromGUI();
}

void SampleWaveform::mouseMove(const juce::MouseEvent &e)
{
    auto posi = e.position.roundToInt();
    if (startSampleHZ.contains(posi) || endSampleHZ.contains(posi) || startLoopHZ.contains(posi) ||
        endLoopHZ.contains(posi))
    {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        return;
    }

    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void SampleWaveform::mouseDoubleClick(const juce::MouseEvent &e)
{
    display->parentPane->selectTab(1);
}

void SampleWaveform::paint(juce::Graphics &g)
{
    if (!display->active)
        return;

    auto r = getLocalBounds();
    auto &v = display->variantView.variants[display->selectedVariation];
    auto samp = editor->sampleManager.getSample(v.sampleID);
    if (!samp)
    {
        return;
    }

    auto l = samp->getSampleLength();

    auto wfp = pathForSample();
    {
        juce::Graphics::ScopedSaveState gs(g);

        g.setColour(juce::Colour(0x30, 0x30, 0x40));
        g.fillRect(r);

        g.setColour(juce::Colour(0xFF, 0x90, 0x00).withAlpha(0.2f));
        g.fillPath(wfp);
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.strokePath(wfp, juce::PathStrokeType(1.f));
    }

    {
        juce::Graphics::ScopedSaveState gs(g);
        auto cr =
            r.withLeft(xPixelForSample(v.startSample)).withRight(xPixelForSample(v.endSample));
        g.reduceClipRegion(cr);

        g.setColour(juce::Colour(0x15, 0x15, 0x15));
        g.fillRect(r);

        g.setColour(juce::Colour(0xFF, 0x90, 0x00));
        g.fillPath(wfp);

        g.setColour(juce::Colours::white);
        g.strokePath(wfp, juce::PathStrokeType(1.f));
    }

    if (v.loopActive)
    {
        juce::Graphics::ScopedSaveState gs(g);
        auto cr = r.withLeft(xPixelForSample(v.startLoop)).withRight(xPixelForSample(v.endLoop));
        g.reduceClipRegion(cr);

        g.setColour(juce::Colour(0x15, 0x15, 0x35));
        g.fillRect(r);

        g.setColour(juce::Colour(0x90, 0x90, 0xFF));
        g.fillPath(wfp);

        g.setColour(juce::Colours::white);
        g.strokePath(wfp, juce::PathStrokeType(1.f));
    }

    g.setColour(juce::Colours::white);

    auto ss = xPixelForSample(v.startSample, false);
    auto se = xPixelForSample(v.endSample, false);
    if (ss >= 0 && ss < getWidth())
    {
        g.fillRect(startSampleHZ);
        g.drawVerticalLine(startSampleHZ.getX(), 0, getHeight());
    }
    if (se >= 0 && se < getWidth())
    {
        g.fillRect(endSampleHZ);
        g.drawVerticalLine(endSampleHZ.getRight(), 0, getHeight());
    }
    if (v.loopActive)
    {
        auto ls = xPixelForSample(v.startLoop, false);
        auto fs = xPixelForSample(v.startLoop - v.loopFade, false);
        auto fe = xPixelForSample(v.startLoop + v.loopFade, false);
        auto le = xPixelForSample(v.endLoop, false);

        g.setColour(juce::Colours::aliceblue);
        if (ls >= 0 && ls < getWidth())
        {
            g.fillRect(startLoopHZ);
            g.drawVerticalLine(startLoopHZ.getX(), 0, getHeight());
        }
        if (le >= 0 && le < getWidth())
        {
            g.fillRect(endLoopHZ);
            g.drawVerticalLine(endLoopHZ.getRight(), 0, getHeight());
        }
        if (v.loopFade > 0 && ((fe >= 0 && fe < getWidth()) || (fs >= 0 && fs <= getWidth())))
        {
            g.drawLine(fs, getHeight(), ls, 0);
            g.drawLine(ls, 0, fe, getHeight());
        }
    }

    if (samp->meta.n_slices > 0)
    {
        g.setColour(editor->themeColor(theme::ColorMap::grid_primary));
        for (int i = 0; i < samp->meta.n_slices; ++i)
        {
            auto sp = xPixelForSample(samp->meta.slice_start[i]);
            auto ep = xPixelForSample(samp->meta.slice_end[i]);
            g.drawVerticalLine(sp, 0, getHeight());
            g.drawVerticalLine(ep, 0, getHeight());
        }
    }

    g.setColour(juce::Colours::white);
    g.drawRect(r, 1);
}

void SampleWaveform::resized()
{
    rebuildHotZones();
    samplePlaybackPosition.setSize(1, getHeight());
}

void SampleWaveform::updateSamplePlaybackPosition(int64_t samplePos)
{
    auto x = xPixelForSample(samplePos);
    samplePlaybackPosition.setTopLeftPosition(x, 0);
}
} // namespace scxt::ui::app::edit_screen