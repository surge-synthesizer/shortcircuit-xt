#pragma once

#include <string>
#include "sampler_state.h"
#include <iostream>

enum InteractionId
{
    ip_none = 0,
    ip_partselect,
    ip_layerselect,
    ip_kgv_or_list,
    ip_wavedisplay,
    ip_lfo_load,
    ip_browser,
    ip_browser_mode,
    ip_browser_searchtext,
    ip_browser_previewbutton,
    ip_config_outputs,
    ip_config_slidersensitivity,
    ip_config_controller_id,
    ip_config_controller_mode,
    ip_config_browserdirs,
    ip_config_refresh_db,
    ip_config_kbdmode,
    ip_config_autopreview,
    ip_config_previewvolume,
    ip_config_save,
    ip_sample_prevnext,
    ip_patch_prevnext,
    ip_replace_sample,
    ip_sample_name,
    ip_sample_metadata,
    ip_solo,
    ip_select_layer,
    ip_select_all,
    ip_polyphony,
    ip_vumeter,
    n_ip_free_items = ip_vumeter,
    ip_zone_name,
    ip_zone_params_begin = ip_zone_name, // start: properties that should be handled by multiselect
    ip_channel,
    ip_low_key,
    ip_root_key,
    ip_high_key,
    ip_low_vel,
    ip_high_vel,
    ip_low_key_f,
    ip_high_key_f,
    ip_low_vel_f,
    ip_high_vel_f,
    ip_coarse_tune,
    ip_fine_tune,
    ip_pitchcorrect,
    ip_reverse,
    ip_pbdepth,
    ip_playmode,
    ip_velsense,
    ip_keytrack,
    ip_mute_group,
    ip_EG_a,
    ip_EG_h,
    ip_EG_d,
    ip_EG_s,
    ip_EG_r,
    ip_EG_s0,
    ip_EG_s1,
    ip_EG_s2,

    ip_lforate,
    ip_lfoshape,
    ip_lforepeat,
    ip_lfocycle,
    ip_lfosync,
    ip_lfotrigger,
    ip_lfoshuffle,
    ip_lfoonce,
    ip_lfosteps,

    // ip_ef_attack,
    // ip_ef_release,
    ip_lag,
    ip_filter_object,
    ip_filter_type,
    ip_filter_bypass,
    ip_filter_mix,
    ip_filter1_fp,
    ip_filter2_fp,
    ip_filter1_ip,
    ip_filter2_ip,
    ip_mm_src,
    ip_mm_src2,
    ip_mm_dst,
    ip_mm_amount,
    ip_mm_curve,
    ip_mm_active,
    ip_nc_src,
    ip_nc_low,
    ip_nc_high,
    ip_ignore_part_polymode,
    // ip_polymode,
    // ip_portamento,
    // ip_portamento_mode,
    ip_mute,
    ip_pfg,
    ip_zone_aux_level,
    ip_zone_aux_balance,
    ip_zone_aux_output,
    ip_zone_aux_outmode,
    ip_zone_params_end = ip_zone_aux_outmode,
    // end: properties that should be handled by multiselect

    ip_part_name,
    ip_part_params_begin = ip_part_name,
    ip_part_midichannel,
    ip_part_polylimit,
    ip_part_transpose,
    ip_part_formant,
    // ip_part_polymode_partlevel,
    ip_part_polymode,
    ip_part_portamento,
    ip_part_portamento_mode,
    ip_part_userparam_name,
    ip_part_userparam_polarity,
    ip_part_userparam_value,
    ip_part_filter_object,
    ip_part_filter_type,
    ip_part_filter_bypass,
    ip_part_filter_mix,
    ip_part_filter1_fp,
    ip_part_filter2_fp,
    ip_part_filter1_ip,
    ip_part_filter2_ip,
    ip_part_aux_level,
    ip_part_aux_balance,
    ip_part_aux_output,
    ip_part_aux_outmode,
    ip_part_vs_layers,
    ip_part_vs_distribution,
    ip_part_vs_xf_equality,
    ip_part_vs_xfade,
    ip_part_mm_src,
    ip_part_mm_src2,
    ip_part_mm_dst,
    ip_part_mm_amount,
    ip_part_mm_curve,
    ip_part_mm_active,
    ip_part_nc_src,
    ip_part_nc_low,
    ip_part_nc_high,

    ip_part_params_end = ip_part_nc_high,

    ip_multi_filter_object,
    ip_multi_params_begin = ip_multi_filter_object,
    ip_multi_filter_type,
    ip_multi_filter_bypass,
    ip_multi_filter_fp1,
    ip_multi_filter_fp2,
    ip_multi_filter_fp3,
    ip_multi_filter_fp4,
    ip_multi_filter_fp5,
    ip_multi_filter_fp6,
    ip_multi_filter_fp7,
    ip_multi_filter_fp8,
    ip_multi_filter_fp9,
    ip_multi_filter_ip1,
    ip_multi_filter_ip2,
    ip_multi_filter_output,
    ip_multi_filter_pregain,
    ip_multi_filter_postgain,
    ip_multi_params_end = ip_multi_filter_postgain,

    n_ip_entries,

};

enum IPType
{
    ipvt_int = 0,
    ipvt_char,
    ipvt_float,
    ipvt_string,
    ipvt_bdata, // binary data for fx preset save/load (size from subid_ptr_offset)
    ipvt_other
};

struct ip_range_and_units
{
    enum Units
    {
        UNDEF,
        PERCENT,
        SECONDS,
        KEYS
    } unit{UNDEF};
    enum Type
    {
        UNINITIALIZED,
        FLOAT = 1,
        INT,
        DATAMODE,
        DISCRETE_DATAMODE
    } type{UNINITIALIZED};
    int mmds_int[4]{};
    float mmds_float[4]{};
    char label[256]{0};
    char datamodestring[256]{0};

    constexpr ip_range_and_units() : unit(UNDEF), type(UNINITIALIZED)
    {
        mmds_float[0] = 0.f;
        mmds_float[1] = 1.f;
        mmds_float[2] = 0.f;
        mmds_float[3] = 0.01f;
    }

    constexpr ip_range_and_units(int minv, int maxv, int defv, int stepv, Units u)
        : unit(u), type(INT)
    {
        mmds_int[0] = minv;
        mmds_int[1] = maxv;
        mmds_int[2] = defv;
        mmds_int[3] = stepv;
    }
    constexpr ip_range_and_units(float minv, float maxv, float defv, float stepv, Units u)
        : unit(u), type(FLOAT)
    {
        mmds_float[0] = minv;
        mmds_float[1] = maxv;
        mmds_float[2] = defv;
        mmds_float[3] = stepv;
    }

    constexpr static ip_range_and_units midi127()
    {
        return ip_range_and_units(0, 127, 60, 1, KEYS);
    }

    constexpr static ip_range_and_units percent(float def)
    {
        return ip_range_and_units(0.f, 1.f, def, 0.01f, PERCENT);
    }

    constexpr static ip_range_and_units floatZeroTo(float up, float def = 0.f)
    {
        return ip_range_and_units(0.f, up, def, 0.01, UNDEF);
    }

    constexpr static ip_range_and_units percent_bipolar(float def)
    {
        return ip_range_and_units(-1.f, 1.f, def, 0.01f, PERCENT);
    }

    constexpr static ip_range_and_units fromDatamodeString(const char *s)
    {
        auto res = ip_range_and_units();
        res.type = DATAMODE;
        for (int i = 0; i < 256; ++i)
        {
            res.datamodestring[i] = s[i];
            if (s[i] == 0)
                break;
        }
        res.datamodestring[255] = 0;
        return res;
    }

    constexpr static ip_range_and_units fromLabel(const char *label, int nChoices)
    {
        // sprintf isn't constexpr. Sigh
        char dm[256]{'i', ',', '0', ',', ' ', ',', '0', ',', 0};
        if (nChoices > 9)
            nChoices = 0;
        dm[4] = nChoices + '0';
        auto res = fromDatamodeString(dm);
        res.type = DISCRETE_DATAMODE;
        // oh why is strncpy not constexpr? sigh.
        for (int i = 0; i < 256; ++i)
        {
            res.label[i] = label[i];
            if (label[i] == 0)
                break;
        }
        res.label[255] = 0;
        return res;
    }
    // sampler_wrapperinteraction sends this over
    /*
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
     */
};
struct interactiondata
{
    InteractionId id;
    IPType vtype;
    int ptr_offset;
    int n_subid;
    int subid_ptr_offset;
    std::string name;
    ip_range_and_units rangeAndUnits;
};

inline std::ostream &operator<<(std::ostream &os, const interactiondata &d)
{
    os << "interaction[" << d.name << " ";
    switch (d.vtype)
    {
    case ipvt_int:
        os << "int";
        break;
    case ipvt_char:
        os << "char";
        break;
    case ipvt_float:
        os << "float";
        break;
    case ipvt_string:
        os << "string";
        break;
    case ipvt_bdata:
        os << "bdata";
        break;
    case ipvt_other:
        os << "other";
        break;
    }

    os << " ptroff=" << d.ptr_offset << " n_subid=" << d.n_subid << " subpo=" << d.subid_ptr_offset
       << "]";
    return os;
}

static const interactiondata ip_data[] = {
    {
        ip_none,
        ipvt_int,
        0,
        1,
        0,
        "none",
    },
    {
        ip_partselect,
        ipvt_int,
        0,
        1,
        0,
        "part select",
    },
    {
        ip_layerselect,
        ipvt_int,
        0,
        1,
        0,
        "layer select",
    },
    {
        ip_kgv_or_list,
        ipvt_int,
        0,
        1,
        0,
        "kgv",
    },
    {
        ip_wavedisplay,
        ipvt_int,
        0,
        1,
        0,
        "wavedisplay",
    },
    {
        ip_lfo_load,
        ipvt_int,
        0,
        1,
        0,
        "LFO load shape",
    },
    {
        ip_browser,
        ipvt_int,
        0,
        1,
        0,
        "filebrowser",
    },
    {
        ip_browser_mode,
        ipvt_int,
        0,
        1,
        0,
        "fb mode",
    },
    {
        ip_browser_searchtext,
        ipvt_int,
        0,
        1,
        0,
        "fb searchtext",
    },
    {
        ip_browser_previewbutton,
        ipvt_int,
        0,
        1,
        0,
        "fb preview",
    },
    {
        ip_config_outputs,
        ipvt_int,
        0,
        1,
        0,
        "conf outputs",
    },
    {
        ip_config_slidersensitivity,
        ipvt_float,
        0,
        1,
        0,
        "conf slider sens",
    },
    {
        ip_config_controller_id,
        ipvt_int,
        0,
        16,
        0,
        "conf ctrl id",
        ip_range_and_units::fromDatamodeString("i,0,16383,0,"),
    },
    {
        ip_config_controller_mode,
        ipvt_int,
        0,
        16,
        0,
        "conf ctrl mode",
    },
    {
        ip_config_browserdirs,
        ipvt_string,
        0,
        4,
        0,
        "conf browserdirs",
    },
    {
        ip_config_refresh_db,
        ipvt_int,
        0,
        1,
        0,
        "conf refresh db",
    },
    {
        ip_config_kbdmode,
        ipvt_int,
        0,
        1,
        0,
        "conf kbdmode",
    },
    {
        ip_config_autopreview,
        ipvt_int,
        0,
        1,
        0,
        "conf autopreview",
    },
    {
        ip_config_previewvolume,
        ipvt_float,
        0,
        1,
        0,
        "conf preview volume",
        ip_range_and_units::fromDatamodeString("f,-96.0,0.100,0.0,0,dB"),
    },
    {
        ip_config_save,
        ipvt_int,
        0,
        1,
        0,
        "conf save",
    },
    {
        ip_sample_prevnext,
        ipvt_int,
        0,
        2,
        0,
        "sample prev/next",
    },
    {
        ip_patch_prevnext,
        ipvt_int,
        0,
        2,
        0,
        "patch prev/next",
    },
    {
        ip_replace_sample,
        ipvt_int,
        0,
        1,
        0,
        "replace sample",
    },
    {
        ip_sample_name,
        ipvt_string,
        0,
        1,
        0,
        "sample name",
    },
    {
        ip_sample_metadata,
        ipvt_int,
        0,
        1,
        0,
        "sample metadata",
    },
    {
        ip_solo,
        ipvt_int,
        0,
        1,
        0,
        "solo",
    },
    {
        ip_select_layer,
        ipvt_int,
        0,
        1,
        0,
        "select layer",
    },
    {
        ip_select_all,
        ipvt_int,
        0,
        1,
        0,
        "select all",
    },
    {
        ip_polyphony,
        ipvt_int,
        0,
        1,
        0,
        "polyphony",
    },
    {
        ip_vumeter,
        ipvt_int,
        0,
        1,
        0,
        "VU",
    },
    {
        ip_zone_name,
        ipvt_string,
        (int)offsetof(sample_zone, name),
        1,
        0,
        "zone name",
    },
    {
        ip_channel,
        ipvt_int,
        (int)offsetof(sample_zone, part),
        1,
        0,
        "part",
    },
    {
        ip_low_key,
        ipvt_int,
        (int)offsetof(sample_zone, key_low),
        1,
        0,
        "low key",
        ip_range_and_units::fromDatamodeString("i,0,127,2,midikey"),
    },
    {
        ip_root_key,
        ipvt_int,
        (int)offsetof(sample_zone, key_root),
        1,
        0,
        "root key",
        ip_range_and_units::fromDatamodeString("i,0,127,2,midikey"),
    },
    {
        ip_high_key,
        ipvt_int,
        (int)offsetof(sample_zone, key_high),
        1,
        0,
        "high key",
        ip_range_and_units::fromDatamodeString("i,0,127,2,midikey"),
    },
    {
        ip_low_vel,
        ipvt_int,
        (int)offsetof(sample_zone, velocity_low),
        1,
        0,
        "low vel",
        ip_range_and_units::fromDatamodeString("i,0,127,0,"),
    },
    {
        ip_high_vel,
        ipvt_int,
        (int)offsetof(sample_zone, velocity_high),
        1,
        0,
        "high vel",
        ip_range_and_units::fromDatamodeString("i,0,127,0,"),
    },
    {
        ip_low_key_f,
        ipvt_int,
        (int)offsetof(sample_zone, key_low_fade),
        1,
        0,
        "low key F",
        ip_range_and_units::fromDatamodeString("i,0,127,0,"),
    },
    {
        ip_high_key_f,
        ipvt_int,
        (int)offsetof(sample_zone, key_high_fade),
        1,
        0,
        "high key F",
        ip_range_and_units::fromDatamodeString("i,0,127,0,"),
    },
    {
        ip_low_vel_f,
        ipvt_int,
        (int)offsetof(sample_zone, velocity_low_fade),
        1,
        0,
        "low vel F",
        ip_range_and_units::fromDatamodeString("i,0,127,0,"),
    },
    {
        ip_high_vel_f,
        ipvt_int,
        (int)offsetof(sample_zone, velocity_high_fade),
        1,
        0,
        "high vel F",
        ip_range_and_units::fromDatamodeString("i,0,127,0,"),
    },
    {
        ip_coarse_tune,
        ipvt_int,
        (int)offsetof(sample_zone, transpose),
        1,
        0,
        "coarse tune",
        ip_range_and_units::fromDatamodeString("i,-96,96,0,"),
    },
    {
        ip_fine_tune,
        ipvt_float,
        (int)offsetof(sample_zone, finetune),
        1,
        0,
        "fine tune",
        ip_range_and_units::fromDatamodeString("f,-1.0,0.010,1.0,1,c"),
    },
    {
        ip_pitchcorrect,
        ipvt_float,
        (int)offsetof(sample_zone, pitchcorrection),
        1,
        0,
        "pitch correction",
        ip_range_and_units::fromDatamodeString("f,-1.0,0.010,1.0,1,cents"),
    },
    {
        ip_reverse,
        ipvt_int,
        (int)offsetof(sample_zone, reverse),
        1,
        0,
        "reverse",
    },
    {
        ip_pbdepth,
        ipvt_int,
        (int)offsetof(sample_zone, pitch_bend_depth),
        1,
        0,
        "PB depth",
        ip_range_and_units::fromDatamodeString("i,-36,36,0,"),

    },
    {
        ip_playmode,
        ipvt_int,
        (int)offsetof(sample_zone, playmode),
        1,
        0,
        "playmode",
    },
    {
        ip_velsense,
        ipvt_float,
        (int)offsetof(sample_zone, velsense),
        1,
        0,
        "velsense",
        ip_range_and_units::fromDatamodeString("f,-96.0,0.100,0.0,0,dB"),
    },
    {
        ip_keytrack,
        ipvt_float,
        (int)offsetof(sample_zone, keytrack),
        1,
        0,
        "keytrack",
        ip_range_and_units::fromDatamodeString("f,-1.0,0.010,1.0,1,%"),
    },
    {
        ip_mute_group,
        ipvt_int,
        (int)offsetof(sample_zone, mute_group),
        1,
        0,
        "mute group",
        ip_range_and_units::fromDatamodeString("i,0,63,0,"),
    },
    {
        ip_EG_a,
        ipvt_float,
        (int)offsetof(sample_zone, AEG.attack),
        2,
        (int)sizeof(envelope_AHDSR),
        "EG attack",
        ip_range_and_units::fromDatamodeString("f,-10.0,0.080,5.0,0,"),
    },
    {
        ip_EG_h,
        ipvt_float,
        (int)offsetof(sample_zone, AEG.hold),
        2,
        (int)sizeof(envelope_AHDSR),
        "EG hold",
        ip_range_and_units::fromDatamodeString("f,-10.0,0.080,5.0,0,"),

    },
    {
        ip_EG_d,
        ipvt_float,
        (int)offsetof(sample_zone, AEG.decay),
        2,
        (int)sizeof(envelope_AHDSR),
        "EG decay",
        ip_range_and_units::fromDatamodeString("f,-10.0,0.080,5.0,0,"),
    },
    {
        ip_EG_s,
        ipvt_float,
        (int)offsetof(sample_zone, AEG.sustain),
        2,
        (int)sizeof(envelope_AHDSR),
        "EG sustain",
        ip_range_and_units::fromDatamodeString("f,0.0,0.010,1.0,0,"),
    },
    {
        ip_EG_r,
        ipvt_float,
        (int)offsetof(sample_zone, AEG.release),
        2,
        (int)sizeof(envelope_AHDSR),
        "EG release",
        ip_range_and_units::fromDatamodeString("f,-10.0,0.080,5.0,0,"),
    },
    {
        ip_EG_s0,
        ipvt_float,
        (int)offsetof(sample_zone, AEG.shape[0]),
        2,
        (int)sizeof(envelope_AHDSR),
        "EG attack shape",
        ip_range_and_units::fromDatamodeString("f,-10.0,0.050,10.0,0,"),
    },
    {
        ip_EG_s1,
        ipvt_float,
        (int)offsetof(sample_zone, AEG.shape[1]),
        2,
        (int)sizeof(envelope_AHDSR),
        "EG decay shape",
        ip_range_and_units::fromDatamodeString("f,-10.0,0.050,10.0,0,"),

    },
    {
        ip_EG_s2,
        ipvt_float,
        (int)offsetof(sample_zone, AEG.shape[2]),
        2,
        (int)sizeof(envelope_AHDSR),
        "EG release shape",
        ip_range_and_units::fromDatamodeString("f,-10.0,0.050,10.0,0,"),

    },
    {
        ip_lforate,
        ipvt_float,
        (int)offsetof(sample_zone, LFO[0].rate),
        3,
        (int)sizeof(steplfostruct),
        "LFO rate",
        ip_range_and_units::fromDatamodeString("f,-8.0,0.020,5.0,4,LFHz"),
    },
    {
        ip_lfoshape,
        ipvt_float,
        (int)offsetof(sample_zone, LFO[0].smooth),
        3,
        (int)sizeof(steplfostruct),
        "LFO smooth",
        ip_range_and_units::fromDatamodeString("f,-2.0,0.010,2.0,0,"),
    },
    {
        ip_lforepeat,
        ipvt_int,
        (int)offsetof(sample_zone, LFO[0].repeat),
        3,
        (int)sizeof(steplfostruct),
        "LFO repeat",
        ip_range_and_units::fromDatamodeString("i,2,32,0,"),
    },
    {
        ip_lfocycle,
        ipvt_int,
        (int)offsetof(sample_zone, LFO[0].cyclemode),
        3,
        (int)sizeof(steplfostruct),
        "LFO cycle",
        ip_range_and_units::fromLabel("cycle;step", 2),
    },
    {
        ip_lfosync,
        ipvt_int,
        (int)offsetof(sample_zone, LFO[0].temposync),
        3,
        (int)sizeof(steplfostruct),
        "LFO temposync",
    },
    {
        ip_lfotrigger,
        ipvt_int,
        (int)offsetof(sample_zone, LFO[0].triggermode),
        3,
        (int)sizeof(steplfostruct),
        "LFO trigger",
        ip_range_and_units::fromLabel("key;sng;rnd", 3),
    },
    {
        ip_lfoshuffle,
        ipvt_float,
        (int)offsetof(sample_zone, LFO[0].shuffle),
        3,
        (int)sizeof(steplfostruct),
        "LFO shuffle",
    },
    {
        ip_lfoonce,
        ipvt_int,
        (int)offsetof(sample_zone, LFO[0].onlyonce),
        3,
        (int)sizeof(steplfostruct),
        "LFO once",
    },
    {
        ip_lfosteps,
        ipvt_float,
        (int)offsetof(sample_zone, LFO[0].data[0]),
        3,
        (int)sizeof(steplfostruct),
        "LFO stepdata",
    },
    {
        ip_lag,
        ipvt_float,
        (int)offsetof(sample_zone, lag_generator[0]),
        2,
        (int)sizeof(float),
        "Lag generator",
        ip_range_and_units::fromDatamodeString("f,-10.0,0.080,5.0,4,s"),
    },
    {
        ip_filter_object,
        ipvt_bdata,
        (int)offsetof(sample_zone, Filter),
        2,
        (int)sizeof(filterstruct),
        "filter object",
    },
    {
        ip_filter_type,
        ipvt_int,
        (int)offsetof(sample_zone, Filter[0].type),
        2,
        (int)sizeof(filterstruct),
        "filter type",
    },
    {
        ip_filter_bypass,
        ipvt_int,
        (int)offsetof(sample_zone, Filter[0].bypass),
        2,
        (int)sizeof(filterstruct),
        "filter bypass",
    },
    {
        ip_filter_mix,
        ipvt_float,
        (int)offsetof(sample_zone, Filter[0].mix),
        2,
        (int)sizeof(filterstruct),
        "filter mix",
        ip_range_and_units::fromDatamodeString("f,0.0,0.010,1.0,1,%"),
    },
    {
        ip_filter1_fp,
        ipvt_float,
        (int)offsetof(sample_zone, Filter[0].p[0]),
        n_filter_parameters,
        (int)sizeof(float),
        "filter1 fp",
    },
    {
        ip_filter2_fp,
        ipvt_float,
        (int)offsetof(sample_zone, Filter[1].p[0]),
        n_filter_parameters,
        (int)sizeof(float),
        "filter2 fp",
    },
    {
        ip_filter1_ip,
        ipvt_int,
        (int)offsetof(sample_zone, Filter[0].ip[0]),
        n_filter_iparameters,
        (int)sizeof(int),
        "filter1 ip",
    },
    {
        ip_filter2_ip,
        ipvt_int,
        (int)offsetof(sample_zone, Filter[1].ip[0]),
        n_filter_iparameters,
        (int)sizeof(int),
        "filter2 ip",
    },
    {
        ip_mm_src,
        ipvt_int,
        (int)offsetof(sample_zone, mm[0].source),
        mm_entries,
        (int)sizeof(mm_entry),
        "MM source",
    },
    {
        ip_mm_src2,
        ipvt_int,
        (int)offsetof(sample_zone, mm[0].source2),
        mm_entries,
        (int)sizeof(mm_entry),
        "MM source2",
    },
    {
        ip_mm_dst,
        ipvt_int,
        (int)offsetof(sample_zone, mm[0].destination),
        mm_entries,
        (int)sizeof(mm_entry),
        "MM destination",
    },
    {
        ip_mm_amount,
        ipvt_float,
        (int)offsetof(sample_zone, mm[0].strength),
        mm_entries,
        (int)sizeof(mm_entry),
        "MM amount",
        ip_range_and_units::fromDatamodeString("f,-32.0,0.100,32.0,0,"),
    },
    {
        ip_mm_curve,
        ipvt_int,
        (int)offsetof(sample_zone, mm[0].curve),
        mm_entries,
        (int)sizeof(mm_entry),
        "MM curve",
    },
    {
        ip_mm_active,
        ipvt_int,
        (int)offsetof(sample_zone, mm[0].active),
        mm_entries,
        (int)sizeof(mm_entry),
        "MM active",
    },
    {
        ip_nc_src,
        ipvt_int,
        (int)offsetof(sample_zone, nc[0].source),
        nc_entries,
        (int)sizeof(nc_entry),
        "zone nc src",
    },
    {
        ip_nc_low,
        ipvt_int,
        (int)offsetof(sample_zone, nc[0].low),
        nc_entries,
        (int)sizeof(nc_entry),
        "zone nc low",
        ip_range_and_units::fromDatamodeString("i,0,127,0,"),
    },
    {
        ip_nc_high,
        ipvt_int,
        (int)offsetof(sample_zone, nc[0].high),
        nc_entries,
        (int)sizeof(nc_entry),
        "zone nc high",
        ip_range_and_units::fromDatamodeString("i,0,127,0,"),
    },
    {
        ip_ignore_part_polymode,
        ipvt_int,
        (int)offsetof(sample_zone, ignore_part_polymode),
        1,
        0,
        "ignore playmode",
    },
    {
        ip_mute,
        ipvt_int,
        (int)offsetof(sample_zone, mute),
        1,
        0,
        "mute",
    },
    {
        ip_pfg,
        ipvt_float,
        (int)offsetof(sample_zone, pre_filter_gain),
        1,
        0,
        "prefilter gain",
        ip_range_and_units::fromDatamodeString("f,-96.0,0.100,48.0,0,dB"),
    },
    {
        ip_zone_aux_level,
        ipvt_float,
        (int)offsetof(sample_zone, aux[0].level),
        3,
        (int)sizeof(aux_buss),
        "main/AUX level",
        ip_range_and_units::fromDatamodeString("f,-96.0,0.200,12.0,0,dB"),
    },
    {
        ip_zone_aux_balance,
        ipvt_float,
        (int)offsetof(sample_zone, aux[0].balance),
        3,
        (int)sizeof(aux_buss),
        "main/AUX balance",
        ip_range_and_units::fromDatamodeString("f,-2.0,0.010,2.0,1,%"),
    },
    {
        ip_zone_aux_output,
        ipvt_int,
        (int)offsetof(sample_zone, aux[0].output),
        3,
        (int)sizeof(aux_buss),
        "main/AUX output",
    },
    {
        ip_zone_aux_outmode,
        ipvt_int,
        (int)offsetof(sample_zone, aux[0].outmode),
        3,
        (int)sizeof(aux_buss),
        "main/AUX mode",
        ip_range_and_units::fromLabel("main;AUX off;pre;post", 4),
    },
    {
        ip_part_name,
        ipvt_string,
        (int)offsetof(sample_part, name),
        1,
        0,
        "part name",
    },
    {
        ip_part_midichannel,
        ipvt_int,
        (int)offsetof(sample_part, MIDIchannel),
        1,
        0,
        "MIDI channel",
        ip_range_and_units::fromDatamodeString("i,0,15,1,"),
    },
    {
        ip_part_polylimit,
        ipvt_int,
        (int)offsetof(sample_part, polylimit),
        1,
        0,
        "part polylimit",
        ip_range_and_units::fromDatamodeString("i,0,256,0,"),
    },
    {
        ip_part_transpose,
        ipvt_int,
        (int)offsetof(sample_part, transpose),
        1,
        0,
        "part transpose",
        ip_range_and_units::fromDatamodeString("i,-36,36,0,"),
    },
    {
        ip_part_formant,
        ipvt_int,
        (int)offsetof(sample_part, formant),
        1,
        0,
        "part formant",
        ip_range_and_units::fromDatamodeString("i,-36,36,0,"),
    },
    {
        ip_part_polymode,
        ipvt_int,
        (int)offsetof(sample_part, polymode),
        1,
        0,
        "part polymode",
        ip_range_and_units::fromLabel("poly;mono;legato", 3),
    },
    {
        ip_part_portamento,
        ipvt_float,
        (int)offsetof(sample_part, portamento),
        1,
        0,
        "part portamento",
        ip_range_and_units::fromDatamodeString("f,-10.0,0.080,2.0,4,s"),

    },
    {
        ip_part_portamento_mode,
        ipvt_int,
        (int)offsetof(sample_part, portamento_mode),
        1,
        0,
        "part portamento mode",
        ip_range_and_units::fromLabel("fixed;linear", 2),
    },
    {
        ip_part_userparam_name,
        ipvt_string,
        (int)offsetof(sample_part, userparametername),
        num_userparams,
        state_string_length,
        "part UP name",
    },
    {
        ip_part_userparam_polarity,
        ipvt_int,
        (int)offsetof(sample_part, userparameterpolarity),
        num_userparams,
        (int)sizeof(int),
        "part UP polarity",
    },
    {
        ip_part_userparam_value,
        ipvt_float,
        (int)offsetof(sample_part, userparameter),
        num_userparams,
        (int)sizeof(float),
        "part UP value",
        ip_range_and_units::fromDatamodeString("f,0.0,0.005,1.0,1,%"),
    },
    {
        ip_part_filter_object,
        ipvt_bdata,
        (int)offsetof(sample_part, Filter),
        2,
        (int)sizeof(filterstruct),
        "part filter object",
    },
    {
        ip_part_filter_type,
        ipvt_int,
        (int)offsetof(sample_part, Filter[0].type),
        2,
        (int)sizeof(filterstruct),
        "part filter type",
    },
    {
        ip_part_filter_bypass,
        ipvt_int,
        (int)offsetof(sample_part, Filter[0].bypass),
        2,
        (int)sizeof(filterstruct),
        "part filter bypass",
    },
    {
        ip_part_filter_mix,
        ipvt_float,
        (int)offsetof(sample_part, Filter[0].mix),
        2,
        (int)sizeof(filterstruct),
        "part filter mix",
        ip_range_and_units::fromDatamodeString("f,0.0,0.005,1.0,1,%"),
    },
    {
        ip_part_filter1_fp,
        ipvt_float,
        (int)offsetof(sample_part, Filter[0].p[0]),
        n_filter_parameters,
        (int)sizeof(float),
        "part filter1 fp",
    },
    {
        ip_part_filter2_fp,
        ipvt_float,
        (int)offsetof(sample_part, Filter[1].p[0]),
        n_filter_parameters,
        (int)sizeof(float),
        "part filter2 fp",
    },
    {
        ip_part_filter1_ip,
        ipvt_int,
        (int)offsetof(sample_part, Filter[0].ip[0]),
        n_filter_iparameters,
        (int)sizeof(int),
        "part filter1 ip",
    },
    {
        ip_part_filter2_ip,
        ipvt_int,
        (int)offsetof(sample_part, Filter[1].ip[0]),
        n_filter_iparameters,
        (int)sizeof(int),
        "part filter2 ip",
    },
    {
        ip_part_aux_level,
        ipvt_float,
        (int)offsetof(sample_part, aux[0].level),
        3,
        (int)sizeof(aux_buss),
        "part level",
        ip_range_and_units::fromDatamodeString("f,-96.0,0.100,12.0,0,dB"),
    },
    {
        ip_part_aux_balance,
        ipvt_float,
        (int)offsetof(sample_part, aux[0].balance),
        3,
        (int)sizeof(aux_buss),
        "part balance",
        ip_range_and_units::fromDatamodeString("f,-2.0,0.010,2.0,1,%"),
    },
    {
        ip_part_aux_output,
        ipvt_int,
        (int)offsetof(sample_part, aux[0].output),
        3,
        (int)sizeof(aux_buss),
        "part output",
    },
    {
        ip_part_aux_outmode,
        ipvt_int,
        (int)offsetof(sample_part, aux[0].outmode),
        3,
        (int)sizeof(aux_buss),
        "part off;pre;post",
    },
    {
        ip_part_vs_layers,
        ipvt_int,
        (int)offsetof(sample_part, vs_layercount),
        1,
        0,
        "velsplit layers",
        ip_range_and_units::fromLabel("o;ab;ac;ad;ae;af;ag;ah", 8),
    },
    {
        ip_part_vs_distribution,
        ipvt_float,
        (int)offsetof(sample_part, vs_distribution),
        1,
        0,
        "split distribution",
        ip_range_and_units::fromDatamodeString("f,-1.0,0.010,1.0,1,%"),
    },
    {
        ip_part_vs_xf_equality,
        ipvt_int,
        (int)offsetof(sample_part, vs_xf_equality),
        1,
        0,
        "xfade equality",
        ip_range_and_units::fromLabel("equal gain;equal power", 2),
    },
    {
        ip_part_vs_xfade,
        ipvt_float,
        (int)offsetof(sample_part, vs_xfade),
        1,
        0,
        "xfade amount",
        ip_range_and_units::fromDatamodeString("f,0.0,0.005,1.0,1,%"),
    },
    {
        ip_part_mm_src,
        ipvt_int,
        (int)offsetof(sample_part, mm[0].source),
        mm_part_entries,
        (int)sizeof(mm_entry),
        "MM part source",
    },
    {
        ip_part_mm_src2,
        ipvt_int,
        (int)offsetof(sample_part, mm[0].source2),
        mm_part_entries,
        (int)sizeof(mm_entry),
        "MM part source2",
    },
    {
        ip_part_mm_dst,
        ipvt_int,
        (int)offsetof(sample_part, mm[0].destination),
        mm_part_entries,
        (int)sizeof(mm_entry),
        "MM part destination",
    },
    {
        ip_part_mm_amount,
        ipvt_float,
        (int)offsetof(sample_part, mm[0].strength),
        mm_part_entries,
        (int)sizeof(mm_entry),
        "MM part amount",
    },
    {
        ip_part_mm_curve,
        ipvt_int,
        (int)offsetof(sample_part, mm[0].curve),
        mm_part_entries,
        (int)sizeof(mm_entry),
        "MM part curve",
    },
    {
        ip_part_mm_active,
        ipvt_int,
        (int)offsetof(sample_part, mm[0].active),
        mm_part_entries,
        (int)sizeof(mm_entry),
        "MM part active",
    },
    {
        ip_part_nc_src,
        ipvt_int,
        (int)offsetof(sample_part, nc[0].source),
        num_part_ncs,
        (int)sizeof(nc_entry),
        "part nc src",
    },
    {
        ip_part_nc_low,
        ipvt_int,
        (int)offsetof(sample_part, nc[0].low),
        num_part_ncs,
        (int)sizeof(nc_entry),
        "part nc low",
        ip_range_and_units::fromDatamodeString("i,0,128,0,"),
    },
    {
        ip_part_nc_high,
        ipvt_int,
        (int)offsetof(sample_part, nc[0].high),
        num_part_ncs,
        (int)sizeof(nc_entry),
        "part nc high",
        ip_range_and_units::fromDatamodeString("i,0,128,0,"),
    },
    {
        ip_multi_filter_object,
        ipvt_bdata,
        (int)offsetof(sample_multi, Filter),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter object",
    },
    {
        ip_multi_filter_type,
        ipvt_int,
        (int)offsetof(sample_multi, Filter[0].type),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter type",
    },
    {
        ip_multi_filter_bypass,
        ipvt_int,
        (int)offsetof(sample_multi, Filter[0].bypass),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter bypass",
    },
    {
        ip_multi_filter_fp1,
        ipvt_float,
        (int)offsetof(sample_multi, Filter[0].p[0]),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter fp1",
    },
    {
        ip_multi_filter_fp2,
        ipvt_float,
        (int)offsetof(sample_multi, Filter[0].p[1]),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter fp2",
    },
    {
        ip_multi_filter_fp3,
        ipvt_float,
        (int)offsetof(sample_multi, Filter[0].p[2]),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter fp3",
    },
    {
        ip_multi_filter_fp4,
        ipvt_float,
        (int)offsetof(sample_multi, Filter[0].p[3]),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter fp4",
    },
    {
        ip_multi_filter_fp5,
        ipvt_float,
        (int)offsetof(sample_multi, Filter[0].p[4]),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter fp5",
    },
    {
        ip_multi_filter_fp6,
        ipvt_float,
        (int)offsetof(sample_multi, Filter[0].p[5]),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter fp6",
    },
    {
        ip_multi_filter_fp7,
        ipvt_float,
        (int)offsetof(sample_multi, Filter[0].p[6]),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter fp7",
    },
    {
        ip_multi_filter_fp8,
        ipvt_float,
        (int)offsetof(sample_multi, Filter[0].p[7]),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter fp8",
    },
    {
        ip_multi_filter_fp9,
        ipvt_float,
        (int)offsetof(sample_multi, Filter[0].p[8]),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter fp9",
    },
    {
        ip_multi_filter_ip1,
        ipvt_int,
        (int)offsetof(sample_multi, Filter[0].ip[0]),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter ip1",
    },
    {
        ip_multi_filter_ip2,
        ipvt_int,
        (int)offsetof(sample_multi, Filter[0].ip[1]),
        num_fxunits,
        (int)sizeof(filterstruct),
        "multi filter ip2",
    },
    {
        ip_multi_filter_output,
        ipvt_int,
        (int)offsetof(sample_multi, filter_output[0]),
        num_fxunits,
        (int)sizeof(int),
        "multi filter output",
    },
    {
        ip_multi_filter_pregain,
        ipvt_float,
        (int)offsetof(sample_multi, filter_pregain[0]),
        num_fxunits,
        (int)sizeof(float),
        "multi filter pregain",
    },
    {
        ip_multi_filter_postgain,
        ipvt_float,
        (int)offsetof(sample_multi, filter_postgain[0]),
        num_fxunits,
        (int)sizeof(float),
        "multi filter postgain",
    },

};

enum InteractionTarget
{
    Free,
    Zone,
    Part,
    Multi
};

inline InteractionTarget targetForInteractionId(InteractionId id)
{
    if (id >= ip_zone_params_begin && id <= ip_zone_params_end)
        return Zone;
    if (id >= ip_part_params_begin && id <= ip_part_params_end)
        return Part;
    if (id >= ip_multi_params_begin && id <= ip_multi_params_end)
        return Multi;
    return Free;
}

std::string datamode_from_cmode(int cmode);
