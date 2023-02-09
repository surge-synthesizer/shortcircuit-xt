//
// Created by Paul Walker on 1/30/23.
//

#include "voice.h"
#include <cassert>
#include <cmath>

namespace scxt::voice
{

Voice::Voice(engine::Zone *z) : zone(z)
{
    assert(zone);
    memset(output, 0, 2 * blockSize * sizeof(float));
    memset(processorFloatParams, 0, sizeof(processorFloatParams));
    memset(processorIntParams, 0, sizeof(processorIntParams));
}

Voice::~Voice()
{
    for (auto i = 0; i < engine::processorsPerZone; ++i)
    {
        dsp::processor::unspawnProcessor(processors[i]);
    }
}

void Voice::voiceStarted()
{
    initializeGenerator();
    initializeProcessors();

    modMatrix.snapRoutingFromZone(zone);
    modMatrix.copyBaseValuesFromZone(zone);
    modMatrix.attachSourcesFromVoice(this);

    for (auto i = 0; i < engine::lfosPerZone; ++i)
    {
        lfos[i].setSampleRate(sampleRate, sampleRateInv);

        lfos[i].assign(&zone->lfoStorage[i],
                       modMatrix.getValuePtr(
                           (modulation::VoiceModMatrixDestination)(modulation::vmd_LFO1_Rate + i)),
                       nullptr);
    }

    zone->addVoice(this);
}

bool Voice::process()
{
    assert(zone);
    if (playState == CLEANUP)
    {
        memset(output, 0, sizeof(output));
        return true;
    }
    auto &s = zone->sample;
    assert(s);

    modMatrix.copyBaseValuesFromZone(zone);

    // Run Modulators
    for (auto i = 0; i < engine::lfosPerZone; ++i)
    {
        // TODO - only if we need it
        lfos[i].process(blockSize);
    }

    modMatrix.process();

    // TODO : Proper pitch calculation
    auto fpitch = key - 69;

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
        if (playState == RELEASED)
        {
            playState = CLEANUP;
        }
        else
            playState = FINISHED;
    }

    float tempbuf alignas(16)[2][BLOCK_SIZE], postfader_buf alignas(16)[2][BLOCK_SIZE];

    for (auto i = 0; i < engine::processorsPerZone; ++i)
    {
        if (processors[i]) // TODO && (!zone->processors[0].bypass))
        {
            processorMix[i].set_target(modMatrix.getValue(
                (modulation::VoiceModMatrixDestination)(modulation::vmd_Processor1_Mix + i)));
            processors[i]->process_stereo(output[0], output[1], tempbuf[0], tempbuf[1], fpitch);
            processorMix[i].fade_2_blocks_to(output[0], tempbuf[0], output[1], tempbuf[1],
                                             output[0], output[1], BLOCK_SIZE_QUAD);

            // TODO: What was the filter_modout? Seems SC2 never finished it
            /*
            filter_modout[0] = voice_filter[0]->modulation_output;
                                   */
        }
    }

    return true;
}

void Voice::initializeGenerator()
{
    auto &s = zone->sample;
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
    // TODO Oversampling if ratio too big
    calculateGeneratorRatio();

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
    // TODO gross for now - correct
    auto ndiff = key - zone->rootKey;
    auto fac = std::pow(2.0, ndiff / 12.0);
    GD.ratio = (int32_t)((1 << 24) * fac * zone->sample->sample_rate * sampleRateInv);
#endif
}

void Voice::initializeProcessors()
{
    for (auto i = 0; i < engine::processorsPerZone; ++i)
    {
        processorMix[i].set_target(modMatrix.getValue(
            (modulation::VoiceModMatrixDestination)(modulation::vmd_Processor1_Mix + i)));
        processorMix[i].instantize();

        processorType[i] = zone->processorStorage[i].type;
        assert(dsp::processor::isZoneProcessor(processorType[i]));

        if (dsp::processor::canInPlaceNew(processorType[i]))
        {
            // TODO: Stereo
            processors[i] = dsp::processor::spawnProcessorInPlace(
                processorType[i], processorPlacementStorage[i],
                dsp::processor::processorMemoryBufferSize, processorFloatParams[i],
                processorIntParams[i], false);
        }
        else
        {
            processors[i] = dsp::processor::spawnProcessorAllocating(
                processorType[i], processorFloatParams[i], processorIntParams[i], false);
        }

        if (processors[i])
        {
            processors[i]->setSampleRate(sampleRate);
            processors[i]->init();
            // TODO: This init_params is temporary until we get values through the model
            processors[i]->init_params();
        }
    }
}
} // namespace scxt::voice