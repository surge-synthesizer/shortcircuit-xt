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
        GDIO.waveSize = sample->sample_length;

        GD.samplePos = 0;
        GD.sampleSubPos = 0;
        GD.loopLowerBound = 0;
        GD.loopUpperBound = sample->sample_length;
        GD.loopFade = 0;
        GD.playbackLowerBound = 0;
        GD.playbackUpperBound = sample->sample_length;
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

bool PreviewVoice::attachAndStart(const std::shared_ptr<sample::Sample> &s)
{
    SCLOG("Starting Preview : " << s->mFileName.u8string());
    details->sample = s;
    details->initiateGD();
    isActive = true;
    return true;
}
bool PreviewVoice::detatchAndStop()
{
    details->sample = nullptr;
    // TODO : Fade
    isActive = false;
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
    // SCLOG(output[0][0] << " " << output[1][0]);
}

} // namespace scxt::voice