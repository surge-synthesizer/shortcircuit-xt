//
// Created by Paul Walker on 1/31/23.
//

#ifndef __SCXT_PARAMETER_H
#define __SCXT_PARAMETER_H

namespace scxt::datamodel
{

enum ControlMode
{
    cm_none = 0,
    cm_integer,
    cm_notename,
    cm_float,
    cm_percent,
    cm_percent_bipolar,
    cm_decibel,
    cm_decibel_squared,
    cm_envelopetime,
    cm_lforate,
    cm_midichannel,
    cm_mutegroup,
    cm_lag,
    cm_frequency20_20k,
    cm_frequency50_50k,
    cm_bitdepth_16,
    cm_frequency0_2k,
    cm_decibelboost12,
    cm_octaves3,
    cm_frequency1hz,
    cm_time1s,
    cm_frequency_audible,
    cm_frequency_samplerate,
    cm_frequency_khz,
    cm_frequency_khz_bi,
    cm_frequency_hz_bi,
    cm_eq_bandwidth,
    cm_stereowidth,
    cm_mod_decibel,
    cm_mod_pitch,
    cm_mod_freq,
    cm_mod_percent,
    cm_mod_time,
    cm_polyphony,
    cm_envshape,
    cm_osccount,
    cm_count4,
    cm_noyes,
    cm_temposync,
    cm_int_menu,
};

struct ControlDescription
{
    enum Type
    {
        INT,
        FLOAT
    } type{FLOAT};
    float min{0};
    float step{0.01};
    float max{1};
    float def{0.5};
    char unit[32]{}; // fixed size makes in place news easier
};
} // namespace scxt::datamodel

#endif // __SCXT_PARAMETER_H
