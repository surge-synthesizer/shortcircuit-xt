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

#include "sst/basic-blocks/mechanics/block-ops.h"
#include "preview_voice.h"
#include "dsp/generator.h"

namespace mech = sst::basic_blocks::mechanics;

namespace scxt::voice
{
struct PreviewVoice::Details
{
    dsp::GeneratorState GD;
    dsp::GeneratorIO GDIO;
    dsp::GeneratorFPtr Generator;

    float amplitude{1.0};

    PreviewVoice *parent{nullptr};
    Details(PreviewVoice *p) : parent(p) {}
    std::shared_ptr<sample::Sample> sample;

    void initiateGD()
    {
        GDIO.outputL = parent->output[0];
        GDIO.outputR = parent->output[1];
        if (sample->bitDepth == sample::Sample::BD_I16)
        {
            GDIO.sampleDataL = sample->GetSamplePtrI16(0);
            GDIO.sampleDataR = sample->GetSamplePtrI16(1);
        }
        else if (sample->bitDepth == sample::Sample::BD_F32)
        {
            GDIO.sampleDataL = sample->GetSamplePtrF32(0);
            GDIO.sampleDataR = sample->GetSamplePtrF32(1);
        }
        else
        {
            assert(false);
        }
        GDIO.waveSize = sample->sampleLengthPerChannel;

        GD.samplePos = 0;
        GD.sampleSubPos = 0;
        GD.loopLowerBound = 0;
        GD.loopUpperBound = sample->sampleLengthPerChannel;
        GD.loopFade = 0;
        GD.playbackLowerBound = 0;
        GD.playbackUpperBound = sample->sampleLengthPerChannel;
        GD.direction = 1;
        GD.isFinished = false;
        GD.directionAtOutset = GD.direction;
        GD.gated = true;

        GD.ratio = (int32_t)((double)(1 << 24) * sample->sample_rate * parent->samplerate_inv);

        Generator = dsp::GetFPtrGeneratorSample(
            sample->channels != 1, sample->bitDepth == sample::Sample::BD_F32, false, false, false);
        assert(Generator);
    }
};

PreviewVoice::PreviewVoice() { details = std::make_unique<Details>(this); }
PreviewVoice::~PreviewVoice() {}

bool PreviewVoice::attachAndStartUnlessPlaying(const std::shared_ptr<sample::Sample> &s,
                                               float amplitude)
{
    if (isActive)
    {
        schedulePurge = true;
        if (s->id == details->sample->id)
        {
            // You are re-starting the same playing one. This means you want
            // it stopped with our UI interaction.
            detatchAndStop();
            return true;
        }
    }

    details->sample = s;
    details->amplitude = amplitude;
    details->initiateGD();
    isActive = true;
    return true;
}
bool PreviewVoice::detatchAndStop()
{
    details->sample = nullptr;
    // TODO : Fade
    isActive = false;
    schedulePurge = true;
    return true;
}

void PreviewVoice::processBlock()
{
    if (details->GD.isFinished)
    {
        detatchAndStop();
        return;
    }
    assert(details->Generator);
    details->Generator(&details->GD, &details->GDIO);
    if (details->sample->channels == 1)
    {
        mech::copy_from_to<blockSize>(output[0], output[1]);
    }
    auto a3 = details->amplitude;
    a3 = a3 * a3 * a3;
    mech::scale_by<blockSize>(a3, output[0], output[1]);
}

void PreviewVoice::adjustAmplitude(float newA) { details->amplitude = newA; }

} // namespace scxt::voice