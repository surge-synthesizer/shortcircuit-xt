//
// Created by Paul Walker on 1/30/23.
//

#include "voice.h"
namespace scxt::voice
{
bool Voice::process()
{
    // TODO: This is obvious garbage
    auto &s = zone.sample;
    if (!s)
        return false;

    calculateGeneratorRatio();

    // TODO : Start and End Points
    GD.sampleStart = 0;
    GD.sampleStop = s->sample_length;

    // TODO Obvious garbage
    GD.gated = (playState == GATED);
    GD.invertedBounds = 1.f / std::max(1, GD.upperBound - GD.lowerBound);
    Generator(&GD, &GDIO);

    // TODO Ringout - this is obvioulsy wrong
    if (GD.isFinished)
    {
        playState = OFF;
    }

    return true;
}

void Voice::initializeGenerator()
{
    auto &s = zone.sample;
    assert(s);

    GDIO.outputL = output[0];
    GDIO.outputR = output[1];
    GDIO.sampleDataL = s->sampleData[0];
    GDIO.sampleDataR = s->sampleData[1];
    GDIO.voicePtr = this;
    GDIO.waveSize = s->sample_length;

    // TODO: Correct sample start stop position
    GD.samplePos = 0;
    GD.sampleSubPos = 0;
    GD.lowerBound = 0;
    GD.upperBound = s->sample_length;
    GD.direction = 1;
    GD.isFinished = false;

    // TODO reverse and playmodes and stuff
    // TODO Oversampling
    GD.blockSize = blockSize;

    Generator = nullptr;

    // TODO port playmode
    int generateMode = 0; // forwrd or forward hitpoints

    Generator = dsp::GetFPtrGeneratorSample(s->channels == 2, !s->UseInt16, generateMode);
}

void Voice::calculateGeneratorRatio()
{
    // TODO all of this obviously
#if 0
    keytrack = (fkey + zone->pitch_bend_depth * ctrl[c_pitch_bend] - 60.0f) *
               (1 / 12.0f); // keytrack as modulation source
    float kt = (playmode == pm_forward_hitpoints) ? 0 : ((fkey - zone->key_root) * zone->keytrack);

    fpitch = part->transpose + zone->transpose + zone->finetune +
             zone->pitch_bend_depth * ctrl[c_pitch_bend] + mm.get_destination_value(md_pitch);

    // not important enough to sacrifice perf for.. better to use on demand in case
    // loop_pos = limit_range((sample_pos -
    // mm.get_destination_value_int(md_loop_start))/(float)mm.get_destination_value_int(md_loop_length),0,1);

    GD.ratio = Float2Int((float)((wave->sample_rate * samplerate_inv) * 16777216.f *
                                 note_to_pitch(fpitch + kt - zone->pitchcorrection) *
                                 mm.get_destination_value(md_rate)));
    fpitch += fkey - 69.f; // relative to A3 (440hz)
#else
    // gross for now
    auto ndiff = key - zone.rootKey;
    auto fac = pow(2.0, ndiff / 12.0);
    GD.ratio = (int32_t)((1 << 24) * fac * zone.sample->sample_rate * sampleRateInv);
#endif
}
} // namespace scxt::voice