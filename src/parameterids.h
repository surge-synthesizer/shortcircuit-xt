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
enum ctrl_mode
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

inline int get_mod_mode(int ct)
{
    switch (ct)
    {
    case cm_eq_bandwidth:
        ct = cm_mod_percent;
        break;
    case cm_mod_decibel:
    case cm_decibel:
        ct = cm_mod_decibel;
        break;
    case cm_mod_freq:
    case cm_frequency_audible:
    case cm_frequency_samplerate:
        ct = cm_mod_freq;
        break;
    case cm_frequency0_2k:
    case cm_percent:
        ct = cm_mod_percent;
        break;
    case cm_mod_pitch:
        ct = cm_mod_pitch;
        break;
    case cm_bitdepth_16:
        ct = cm_mod_percent;
        break;
    case cm_time1s:
        ct = cm_mod_time;
        break;
    case cm_frequency_khz:
    case cm_frequency_khz_bi:
        ct = cm_frequency_khz_bi;
        break;
    default:
        ct = cm_none;
    };
    return ct;
}
