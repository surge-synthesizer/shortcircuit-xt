//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "globals.h"
#include "resampling.h"

#include "generator.h"
#include "synthesis/envelope.h"
#include "synthesis/modmatrix.h"
#include "synthesis/steplfo.h"

#include <vt_dsp/lipol.h>

class sampler_voice;
class filter;
class sample;
class halfrate_stereo;

struct sample_zone;
struct sample_part;
struct timedata;

// sampler voice class
class alignas(16) sampler_voice
{
  public:
    float output alignas(16)[2][block_size * 2];
    lipol_ps vca, faderL, faderR, pfg, aux1L, aux1R, aux2L, aux2R, fmix1, fmix2;

    sampler_voice(uint32 voice_id, timedata *);
    virtual ~sampler_voice();

    void play(sample *wave, sample_zone *zone, sample_part *part, uint32 key, uint32 velocity,
              int detune, float *ctrl, float *autom, float crossfade_amp);
    void release(uint32 velocity);
    void uberrelease();
    void change_key(int key, int vel, int detune);

    // bool (sampler_voice::*process_block_f)(float*, float*, float*,float*, float*, float*);
    // bool process_block(float *L, float *R, float *aux1L, float *aux1R, float *aux2L, float
    // *aux2R);
    float portasrc_key;
    float portaphase;
    float fkey, alternate;
    float crossfade_amp;

    uint32 voice_id;

    int32_t key, velocity, channel, detune;
    float *__restrict ctrl;
    float *__restrict automation;

    sample_zone *__restrict zone;
    sample_part *__restrict part;
    sample *__restrict wave;
    timedata *__restrict td;

    GeneratorState GD;
    GeneratorIO GDIO;
    GeneratorFPtr Generator;
    // uint32 sample_pos;
    // uint32 sample_subpos;
    // int32 resample_ratio;
    uint32 last_loopstart, last_loopend, last_cflength;
    uint32 *__restrict slice_end;
    int slice_id;
    uint32 end_offset;
    // audio path

    halfrate_stereo *halfrate;

    // modules
    filter *__restrict voice_filter[2];
    int last_ft[2];

    // control path
    modmatrix mm;
    void CalcRatio();
    Envelope AEG, EG2;
    steplfo stepLFO[3];
    bool gate, is_uberrelease;
    float time, time60, random, randombp, keytrack, fvelocity, fgate, slice_env, loop_pos,
        loop_gate, filter_modout[2];

    float envelope_follower, fpitch;
    float lag[2];

    inline int get_filter_type(int id) const;
    // void filter_ctrldata(int);
    bool sample_started, first_run;
    bool upsampling;
    bool invert_polarity;
    bool finished;
    int playmode;
    uint32 grain_id;
    int32_t RingOut; // when sample playback is finished, this will be decremented until zero
    // template<bool stereo, bool oversampling, bool xfadeloop, int architecture>
    // __declspec(noalias) bool process_t(float *L, float *R, float *aux1L, float *aux1R, float
    // *aux2L, float *aux2R);
    bool process_block(float *L, float *R, float *aux1L, float *aux1R, float *aux2L, float *aux2R);
    inline void check_filtertypes();
    inline void update_lag_gen(int);
    inline void update_portamento();
    bool use_oversampling, use_stereo, use_xfade;
    bool looping_active, portamento_active;
};