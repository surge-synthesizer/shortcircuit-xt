/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#pragma once

#include "globals.h"
#include <string>

// timedata
struct timedata
{
    double tempo;
    double ppqPos;
    float pos_in_beat, pos_in_2beats, pos_in_bar, pos_in_2bars, pos_in_4bars;
    int timeSigNumerator{4}, timeSigDenominator{4};
};

#pragma pack(push, 1)

//-------------------------------------------------------------------------------------------------------

enum p_playmode
{
    pm_forward = 0,
    pm_forward_loop,
    // pm_forward_loop_crossfade,
    pm_forward_loop_until_release,
    pm_forward_loop_bidirectional,
    pm_forward_shot,
    pm_forward_hitpoints,
    pm_forward_release,
    // pm_reverse,
    // pm_reverse_shot,
    n_playmodes,

    pm_forwardRIFF = 0,
    pm_forward_loopRIFF = 1,
    pm_forward_loop_until_releaseRIFF = 3,
    pm_forward_loop_bidirectionalRIFF = 4,
    pm_forward_shotRIFF = 5,
    pm_forward_hitpointsRIFF = 6,
    pm_forward_releaseRIFF = 7,
};

inline bool pm_has_loop(int m)
{
    return (m == pm_forward_loop) /*||(m == pm_forward_loop_crossfade)*/ ||
           (m == pm_forward_loop_until_release) || (m == pm_forward_loop_bidirectional);
}

inline bool pm_has_slices(int m) { return (m == pm_forward_hitpoints); }
const char playmode_abbreviations[n_playmodes][16] = {
    "fwd",        "fwd_loop",   /*"fwd_loop_cf",*/ "fwd_loop_ur", "fwd_loop_bi", "fwd_shot",
    "fwd_sliced", "fwd_release" /*,"rev","rev_shot" */};
const char playmode_names[n_playmodes][32] = {
    "Standard",           "Loop",    /*"Forward Loop-Crossfade",*/ "Loop Until Release",
    "Loop Bidirectional", "Oneshot", "Slice Keymapped",
    "On Release" /*,"Reverse","Reverse Oneshot"*/};

//-------------------------------------------------------------------------------------------------------

enum
{
    out_part = 0,
    out_output1,
    out_output2,
    out_output3,
    out_output4,
    out_output5,
    out_output6,
    out_output7,
    out_output8,
    out_fx1,
    out_fx2,
    out_fx3,
    out_fx4,
    out_fx5,
    out_fx6,
    out_fx7,
    out_fx8,
    n_output_types,
};

inline bool is_output_visible(int id, int n_outputs)
{
    return (id == out_part) || (id >= out_fx1) || ((id - out_output1) < n_outputs);
}

const char output_abbreviations[n_output_types][16] = {
    "Part Bus", "Out 1", "Out 2", "Out 3", "Out 4", "Out 5", "Out 6", "Out 7", "Out 8",
    "FX 1",     "FX 2",  "FX 3",  "FX 4",  "FX 5",  "FX 6",  "FX 7",  "FX 8",
};

//-------------------------------------------------------------------------------------------------------

enum lfopresets
{
    lp_custom = 0,
    lp_clear,
    lp_sine,
    lp_tri,
    lp_square,
    lp_ramp_up,
    lp_ramp_down,
    lp_noise,
    lp_noise_mean3,
    lp_noise_mean5,
    lp_tremolo_tri,
    lp_tremolo_sin,
    n_lfopresets,
};

const char lfopreset_abbreviations[n_lfopresets][16] = {
    "Load",      "Clear", "Sine",     "Triangle", "Square",        "Ramp Up",
    "Ramp Down", "Noise", "Noise m3", "Noise m5", "Tremolo (Tri)", "Tremolo (Sin)"};

struct steplfostruct
{
    float data[32];
    int repeat;
    float rate;
    float smooth;
    float shuffle;
    int temposync;
    int triggermode; // 0 = voice, 1 = freerun/songsync, 2 = random
    int cyclemode;
    int onlyonce;
    // add midi sync capabilities
};

//-------------------------------------------------------------------------------------------------------

static constexpr int n_filter_parameters = 9;
static constexpr int n_filter_iparameters = 2;

struct filterstruct
{
    float p[n_filter_parameters];
    int ip[n_filter_iparameters];
    float mix;
    int type;
    int bypass;
};

enum filtertypes
{
    ft_none = 0,
    // ft_biquadLP2,
    ft_e_delay,
    ft_part_first = ft_e_delay,
    ft_fx_first = ft_e_delay,
    ft_e_reverb,
    ft_e_chorus,
    ft_e_phaser,
    ft_e_rotary,
    ft_e_fauxstereo,
    ft_e_fsflange,
    ft_e_fsdelay,
    // ft_biquadLP2B,
    ft_biquadSBQ,
    ft_zone_first = ft_biquadSBQ,
    ft_SuperSVF,

    // ft_moogLP4,
    ft_moogLP4sat,
    // ft_biquadHP2,
    // ft_biquadBP2,
    // ft_biquadBP2B,
    // ft_biquad_peak,
    // ft_biquad_notch,
    // ft_biquadBP2_dual,
    // ft_biquad_peak_dual,
    // ft_biquadLPHP_serial,
    // ft_biquadLPHP_parallel,
    // ft_biquadLP2HP2_morph,
    ft_eq_2band_parametric_A,
    // ft_eq_2band_parametric_B,
    ft_eq_6band,
    // ft_morpheq,
    ft_comb1,
    // ft_comb2,
    ft_fx_bitfucker,
    // ft_fx_overdrive,
    ft_fx_distortion1,
    // ft_fx_exciter,
    ft_fx_clipper,
    ft_fx_slewer,
    ft_fx_stereotools,
    ft_fx_limiter,
    ft_fx_gate,
    ft_fx_microgate,
    ft_fx_ringmod,
    ft_fx_freqshift,
    ft_fx_phasemod,

    ft_part_last = ft_fx_phasemod,
    ft_fx_last = ft_fx_phasemod,

    ft_fx_treemonster,
    ft_osc_pulse,
    ft_osc_pulse_sync,
    ft_osc_saw,
    ft_osc_sin,
    ft_zone_last = ft_osc_sin,
    ft_num_types,
};

inline bool valid_filtertype(int i) { return (i > 0) && (i < ft_num_types); }

const char filter_abbreviations_beta1[14][6] = {"NONE",  "LP2A", "HP2A", "BP2A", "PKA",
                                                "BP2AD", "PKAD", "BF",   "OD",   "SINS",
                                                "OCT",   "OSC1", "OSC2", "OSC3"};

const char filter_abbreviations[ft_num_types][16] = {
    "NONE",
    //	"LP2A",
    "DELAY", "REVERB", "CHORUS", "PHASER", "ROTARY", "FAUXSTEREO", "FSFLANGE", "FSDELAY", "SBQ",
    "SSVF",
    //"LP2B",
    //"LP4M",
    "LP4Msat",
    //"HP2A",
    //"BP2A",
    /*"BP2B",
    "PKA",
    "NOTCH",
    "BP2AD",
    "PKAD",
    "LPHPS",*/
    //"LPHPP",
    //"LPHPMRPH",
    "EQ2BPA",
    //"EQ2BPB",
    "EQ6B",
    //"MORPHEQ",
    "COMB1",
    //"COMB2",
    "BF",
    //"OD",		too expensive
    "DIST1",
    //"EXCITER",
    "CLIP", "SLEW", "STEREO", "LMTR", "GATE", "MGATE", "RING", "FREQSHIFT", "PMOD", "TMON",
    "OSCPUL", "OSCPSY", "OSCSAW", "OSCSIN"};

const char filter_descname[ft_num_types][32] = {"none",
                                                "Delay",
                                                "Reverb",
                                                "Chorus",
                                                "Phaser",
                                                "Rotary Speaker",
                                                "Faux Stereo",
                                                "Freqshift Flanger",
                                                "Freqshift Delay",
                                                "LP/HP/BP/N 2-8P",
                                                "LP/HP/BP 2-4P",
                                                /*"Lowpass 2P",*/ /*"Lowpass 4P M",*/ "LP 1-4P MG",
                                                /*"Highpass 2P",*/ /*"Bandpass 2P (bw)", "Peak 2P",
                                                "Notch 2P (bw)", "Dual Bandpass 2P", "Dual Peak 2P",
                                                "LP/HP Serial", "LP/HP Parallel",*/
                                                "EQ 2-band parametric",
                                                "EQ 6-band graphic",
                                                /*"morphEQ",*/ "Comb",
                                                "Lofi (digital)",
                                                "Distortion 1",
                                                "Clipper",
                                                "Slewer",
                                                "Stereo Tools",
                                                "Limiter",
                                                "Gate",
                                                "Microgate (glitch)",
                                                "Ring Modulator",
                                                "Frequency Shifter",
                                                "Phase Modulation",
                                                "Treemonster",
                                                "Osc: Pulse",
                                                "Osc: Pulse (sync)",
                                                "Osc: Sawtooth",
                                                "Osc: Sine"};

//-------------------------------------------------------------------------------------------------------

struct envelope_AHDSR
{
    float attack, hold, decay, sustain, release;
    float shape[3];
};

//-------------------------------------------------------------------------------------------------------

static constexpr int mm_entries = 12;
static constexpr int mm_part_entries = 6;

struct mm_entry
{
    int source, source2;
    int destination;
    float strength;
    int active;
    int curve;
};

enum mm_curves
{
    mmc_linear = 0,
    mmc_reverse,
    mmc_square,
    mmc_cube,
    mmc_squareroot,
    mmc_cuberoot,
    mmc_abs,
    mmc_up2bp,
    mmc_bp2up,
    mmc_tri,
    mmc_tribp,
    mmc_pos,
    mmc_neg,
    mmc_switch,
    mmc_switchi,
    mmc_switchh,
    mmc_switchhi,
    mmc_polarity,
    //	mmc_init,
    // mmc_exp,
    /*mmc_clip,
    mmc_clip_bp,	*/
    mmc_quantitize1,
    mmc_quantitize12 = mmc_quantitize1 + 11,
    // mmc_quantitize12,
    mmc_num_types,
};

const char mmc_abbreviations[mmc_num_types][8] = {
    "x",
    "1-x",
    "x^2",
    "x^3",
    "sqrt", // custom chars in font for sqrt(x)
    "cbrt", // custom chars in font for cubert(x)
    //"exp",
    "abs",
    "to bi",
    "to uni",
    "tri",
    "tri bi",
    "pos",
    "neg",
    ">0",
    "<0",
    ">1/2",
    "<1/2",
    "pol",
    //	"init",		never changes after attack
    "Q1",
    "Q2",
    "Q3",
    "Q4",
    "Q5",
    "Q6",
    "Q7",
    "Q8",
    "Q9",
    "Q10",
    "Q11",
    "Q12",
};
/*"clip","clip�",*/

//-------------------------------------------------------------------------------------------------------

struct trigger_condition_entry
{
    int source, low, high;
};

//-------------------------------------------------------------------------------------------------------

static constexpr int max_hitpoints = 128; // fler makes no sense (yet) :P

struct hitpoint
{
    unsigned int start_sample;
    unsigned int end_sample;
    float env;
    bool muted;
};

enum polymode
{
    polymode_poly = 0,
    polymode_mono,
    polymode_legato,
};

//-------------------------------------------------------------------------------------------------------

struct aux_buss
{
    float level, balance;
    int output, outmode;
    // out mode 0 = off, 1 = pre-fader, 2 = post-fader. ignored for main out
};

//-------------------------------------------------------------------------------------------------------

static constexpr int num_layers = 8;
static constexpr int num_fxunits = 8;
static constexpr int num_layer_trigger_conditions = 2;
static constexpr int num_zone_trigger_conditions = 2;
static constexpr int num_part_trigger_conditions = num_layers * num_layer_trigger_conditions;

static constexpr int num_midi_channels = 16;
static constexpr int num_aux_busses = 3;
static constexpr int num_filters_per_part = 2;
static constexpr int num_userparams = 16;

static constexpr int state_string_length = 32;

//-------------------------------------------------------------------------------------------------------

struct sample_multi
{
    float filter_pregain[num_fxunits], filter_postgain[num_fxunits];
    int filter_output[num_fxunits];
    filterstruct Filter[num_fxunits];

    float PreviewVolume;
    int AutoPreview;
};

//-------------------------------------------------------------------------------------------------------

struct sample_part
{
    char name[state_string_length];
    char userparametername[num_midi_channels][state_string_length];
    float userparameter[num_midi_channels];
    float userparameter_smoothed[num_midi_channels]; // used by interpolation
    int userparameterpolarity[num_midi_channels];

    aux_buss aux[num_aux_busses];
    mm_entry mm[mm_part_entries];

    float portamento, pfg;
    // int					polymode_partlevel;
    int polymode, polylimit;
    int portamento_mode, transpose, formant;
    int MIDIchannel; // channel part receives on (defaults to its id but can be stored in multi, but
                     // not patches)

    filterstruct Filter[num_filters_per_part]; // filters / fx

    // layer velocity split settings
    int vs_layercount;
    int vs_xf_equality; // 0 = equal gain, 1 = equal power
    float vs_distribution;
    float vs_xfade;
    int zonelist_mode;

    trigger_condition_entry
        trigger_conditions[num_part_trigger_conditions]; // 8 layers, 4 slots each

    int last_note;
    int activelayer; // editor state
    int database_id; // locator id for next/prev buttons
    int empty_part;  // if != 0 don't process
    int is_edited;   // if != 0, ask user for confirmation on patch load
};

//-------------------------------------------------------------------------------------------------------

struct sample_zone
{
    char name[state_string_length];

    int key_root, key_low, key_high;
    int velocity_high, velocity_low;
    int key_low_fade, key_high_fade, velocity_low_fade, velocity_high_fade;
    int part, layer;
    int transpose;
    float finetune, pitchcorrection;
    int playmode;
    unsigned int loop_start, loop_end, sample_start, sample_stop, loop_crossfade_length;
    float pre_filter_gain;
    int pitch_bend_depth;
    float velsense, keytrack;

    filterstruct Filter[2];
    envelope_AHDSR AEG, EG2;
    steplfostruct LFO[3];
    mm_entry mm[mm_entries];
    trigger_condition_entry trigger_conditions[num_zone_trigger_conditions];
    hitpoint hp[max_hitpoints];
    int n_hitpoints;

    aux_buss aux[3]; // 0 = main

    int sample_id;
    int mute_group;
    int ignore_part_polymode;
    int mute;
    int reverse;
    float lag_generator[2];

    // voice allocation (mono/legato & portamento) do not store/recall
    int last_note;
    int last_voice;
    int alternate;
    int database_id;             // locator id for next/prev buttons
    unsigned int element_active; // optimization
};

// These I hope will bite the dust one day
std::string debug_view(const sample_part &p);
std::string debug_view(const sample_zone &p);

//-------------------------------------------------------------------------------------------------------

enum voice_elements
{
    ve_LFO1 = 1 << 0,
    ve_LFO2 = 1 << 1,
    ve_LFO3 = 1 << 2,
    ve_EG2 = 1 << 3,
};

//-------------------------------------------------------------------------------------------------------

#pragma pack(pop)
