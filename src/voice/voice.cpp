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

#include "voice.h"
#include "tuning/equal.h"
#include <cassert>
#include <cmath>

#include "sst/basic-blocks/mechanics/block-ops.h"
#include "engine/engine.h"

namespace scxt::voice
{

Voice::Voice(engine::Zone *z) : zone(z), aeg(this), eg2(this)
{
    assert(zone);
    memset(output, 0, 2 * blockSize * sizeof(float));
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
    modMatrix.snapDepthScalesFromZone(zone);
    modMatrix.copyBaseValuesFromZone(zone);
    modMatrix.attachSourcesFromVoice(this);
    modMatrix.initializeModulationValues();

    for (auto i = 0U; i < engine::lfosPerZone; ++i)
    {
        lfos[i].setSampleRate(sampleRate, sampleRateInv);

        lfos[i].assign(&zone->lfoStorage[i], modMatrix.getValuePtr(modulation::vmd_LFO_Rate, i),
                       nullptr);
    }

    aeg.attackFrom(0.0, modMatrix.getValue(modulation::vmd_eg_A, 0),
                   1, // this needs fixing in legato mode
                   zone->aegStorage.isDigital);
    eg2.attackFrom(0.0, modMatrix.getValue(modulation::vmd_eg_A, 1),
                   1, // this needs fixing in legato mode
                   zone->aegStorage.isDigital);

    zone->addVoice(this);
}

bool Voice::process()
{
    namespace mech = sst::basic_blocks::mechanics;

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

    // TODO probably want to process LFO modulations at this point so the AEG and EG2
    // are modulatable

    // TODO SHape Fixes
    aeg.processBlock(modMatrix.getValue(modulation::vmd_eg_A, 0),
                     modMatrix.getValue(modulation::vmd_eg_D, 0),
                     modMatrix.getValue(modulation::vmd_eg_S, 0),
                     modMatrix.getValue(modulation::vmd_eg_R, 0), 1, 1, 1, playState == GATED);
    eg2.processBlock(modMatrix.getValue(modulation::vmd_eg_A, 1),
                     modMatrix.getValue(modulation::vmd_eg_D, 1),
                     modMatrix.getValue(modulation::vmd_eg_S, 1),
                     modMatrix.getValue(modulation::vmd_eg_R, 1), 1, 1, 1, playState == GATED);

    // TODO and probably just want to process the envelopes here
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
            processorMix[i].set_target(modMatrix.getValue(modulation::vmd_Processor_Mix, i));
            processors[i]->process_stereo(output[0], output[1], tempbuf[0], tempbuf[1], fpitch);
            processorMix[i].fade_2_blocks_to(output[0], tempbuf[0], output[1], tempbuf[1],
                                             output[0], output[1], BLOCK_SIZE_QUAD);

            // TODO: What was the filter_modout? Seems SC2 never finished it
            /*
            filter_modout[0] = voice_filter[0]->modulation_output;
                                   */
        }
    }

    mech::scale_by<blockSize>(aeg.outputCache, output[0], output[1]);

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
    float ndiff = (float)key - zone->mapping.rootKey;
    auto fac = tuning::equalTuning.note_to_pitch(ndiff);
    GD.ratio = (int32_t)((1 << 24) * fac * zone->sample->sample_rate * sampleRateInv *
                         (1.0 + modMatrix.getValue(modulation::vmd_Sample_Playback, 0)));
#endif
}

void Voice::initializeProcessors()
{
    assert(zone);
    assert(zone->getEngine());
    for (auto i = 0; i < engine::processorsPerZone; ++i)
    {
        processorMix[i].set_target(modMatrix.getValue(modulation::vmd_Processor_Mix, i));
        processorMix[i].instantize();

        processorType[i] = zone->processorStorage[i].type;
        assert(dsp::processor::isZoneProcessor(processorType[i]));

        auto fp = modMatrix.getValuePtr(modulation::vmd_Processor_FP1, i);
        memcpy(&processorIntParams[i][0], zone->processorStorage[i].intParams.data(),
               sizeof(processorIntParams[i]));
        processors[i] = dsp::processor::spawnProcessorInPlace(
            processorType[i], zone->getEngine()->getMemoryPool().get(),
            processorPlacementStorage[i], dsp::processor::processorMemoryBufferSize, fp,
            processorIntParams[i]);

        if (processors[i])
        {
            processors[i]->setSampleRate(sampleRate);
            processors[i]->init();
        }
    }
}
} // namespace scxt::voice