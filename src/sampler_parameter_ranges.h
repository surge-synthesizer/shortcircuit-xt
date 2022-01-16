//
// Created by Paul Walker on 1/14/22.
//

#ifndef SHORTCIRCUIT_SAMPLER_PARAMETER_RANGES_H
#define SHORTCIRCUIT_SAMPLER_PARAMETER_RANGES_H

#include "sampler_wrapper_actiondata.h"

struct parameter_ranges
{
    enum Units
    {
        UNDEF,
        PERCENT,
        SECONDS,
        KEYS
    } unit;
    enum Type
    {
        FLOAT = 1,
        INT
    } type{INT};
    int mmds_int[4]{};
    float mmds_float[4]{};
    InteractionId id;
    constexpr parameter_ranges(InteractionId id, int minv, int maxv, int defv, int stepv, Units u)
        : unit(u), id(id)
    {
        mmds_int[0] = minv;
        mmds_int[1] = maxv;
        mmds_int[2] = defv;
        mmds_int[3] = stepv;
    }
    constexpr parameter_ranges(InteractionId id, float minv, float maxv, float defv, float stepv,
                               Units u)
        : unit(u), type(FLOAT), id(id)
    {
        mmds_float[0] = minv;
        mmds_float[1] = maxv;
        mmds_float[2] = defv;
        mmds_float[3] = stepv;
    }

    constexpr static parameter_ranges midi127(InteractionId id)
    {
        return parameter_ranges(id, 0, 127, 60, 1, KEYS);
    }

    constexpr static parameter_ranges percent(InteractionId id, float def)
    {
        return parameter_ranges(id, 0.f, 1.f, def, 0.01f, PERCENT);
    }

    constexpr static parameter_ranges percent_bipolar(InteractionId id, float def)
    {
        return parameter_ranges(id, -1.f, 1.f, def, 0.01f, PERCENT);
    }
    void toActionData(actiondata &ad) const
    {
        ad.id = id;
        ad.subid = -1;
        ad.actiontype = vga_set_range_and_units;
        ad.data.i[0] = (type == FLOAT ? 'f' : 'i');
        if (type == FLOAT)
            for (int i = 0; i < 4; ++i)
                ad.data.f[i + 1] = mmds_float[i];
        else
            for (int i = 0; i < 4; ++i)
                ad.data.i[i + 1] = mmds_int[i];
    }
};

static constexpr parameter_ranges samplerParameterRanges[] = {
    parameter_ranges::midi127(ip_low_key_f),
    parameter_ranges::midi127(ip_low_key),
    parameter_ranges::midi127(ip_root_key),
    parameter_ranges::midi127(ip_high_key),
    parameter_ranges::midi127(ip_high_key_f),

    parameter_ranges::midi127(ip_low_vel_f),
    parameter_ranges::midi127(ip_low_vel),
    parameter_ranges::midi127(ip_high_vel),
    parameter_ranges::midi127(ip_high_vel_f),

    parameter_ranges::midi127(ip_nc_low), // maybe? i think? what's nc anyway
    parameter_ranges::midi127(ip_nc_high),

    {ip_EG_a, -10.f, 4.f, 0.0f, 0.01f, parameter_ranges::SECONDS},
    {ip_EG_h, -10.f, 4.f, 0.0f, 0.01f, parameter_ranges::SECONDS},
    {ip_EG_d, -10.f, 4.f, 0.0f, 0.01f, parameter_ranges::SECONDS},
    {ip_EG_r, -10.f, 4.f, 0.0f, 0.01f, parameter_ranges::SECONDS},
    parameter_ranges::percent(ip_EG_s, 0.5),

    // hmm what are these
    {ip_EG_s0, 0., 5., 0.0f, 0.1f, parameter_ranges::UNDEF},
    {ip_EG_s1, 0., 5., 0.0f, 0.1f, parameter_ranges::UNDEF},
    {ip_EG_s2, 0., 5., 0.0f, 0.1f, parameter_ranges::UNDEF},

    {ip_mm_amount, -20.f, 20.f, 0.f, 0.05f, parameter_ranges::UNDEF},
    parameter_ranges::percent(ip_filter_mix, 1),

    parameter_ranges::percent(ip_part_aux_level, 1),
    parameter_ranges::percent_bipolar(ip_part_aux_balance, 0),

    parameter_ranges::percent(ip_zone_aux_level, 1),
    parameter_ranges::percent_bipolar(ip_zone_aux_balance, 0)};

#endif // SHORTCIRCUIT_SAMPLER_PARAMETER_RANGES_H
