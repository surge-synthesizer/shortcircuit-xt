//
// Created by Paul Walker on 1/15/22.
//

#ifndef SHORTCIRCUIT_DATAMODEL_H
#define SHORTCIRCUIT_DATAMODEL_H

#include "sampler.h"
#include "data/ParameterProxy.h"

namespace scxt
{
namespace data
{

struct FilterData
{
    ParameterProxy<float> p[n_filter_parameters];
    ParameterProxy<int> ip[n_filter_iparameters];
    ParameterProxy<float> mix;
    ParameterProxy<int> type;
    ParameterProxy<int> bypass;
};

struct AuxBusData
{
    ParameterProxy<float> level, balance;
    ParameterProxy<int> output, outmode;
};

struct MMEntryData
{
    ParameterProxy<int> source, source2;
    ParameterProxy<int> destination;
    ParameterProxy<float> strength;
    ParameterProxy<int> active, curve;
};

struct NCEntryData
{
    ParameterProxy<int> source, low, high;
};

struct EnvelopeData
{
    ParameterProxy<float> a, h, d, s, r;
    ParameterProxy<float> s0, s1, s2;
};

struct LFOData
{
    ParameterProxy<float, vga_steplfo_data_single> data[32];
    ParameterProxy<int> repeat;
    ParameterProxy<float> rate;
    ParameterProxy<float> smooth;
    ParameterProxy<float> shuffle;
    ParameterProxy<int> temposync;
    ParameterProxy<int> triggermode;
    ParameterProxy<int> cyclemode;
    ParameterProxy<int> onlyonce;
};

struct ZoneData
{
    ParameterProxy<std::string> name;

    ParameterProxy<int> key_root, key_low, key_high;
    ParameterProxy<int> velocity_high, velocity_low;
    ParameterProxy<int> key_low_fade, key_high_fade, velocity_low_fade, velocity_high_fade;
    ParameterProxy<int> part, layer;
    ParameterProxy<int> transpose;
    ParameterProxy<float> finetune, pitchcorrection;
    ParameterProxy<int> playmode;
    ParameterProxy<int> loop_start, loop_end, sample_start, sample_stop, loop_crossfade_length;
    ParameterProxy<float> pre_filter_gain;
    ParameterProxy<int> pitch_bend_depth;
    ParameterProxy<float> velsense, keytrack;

    FilterData filter[2];

    MMEntryData mm[mm_entries];
    NCEntryData nc[nc_entries];
    EnvelopeData env[2];
    LFOData lfo[3];

    AuxBusData aux[3];

    ParameterProxy<int> sample_id;
    ParameterProxy<int> mute_group;
    ParameterProxy<int> ignore_part_polymode;
    ParameterProxy<int> mute;
    ParameterProxy<int> reverse;
    ParameterProxy<float> lag_generator[2];
};

struct PartData
{
    ParameterProxy<std::string> name;
    ParameterProxy<int> midichannel;
    ParameterProxy<int> polylimit;
    ParameterProxy<int> transpose;
    ParameterProxy<int> formant;
    ParameterProxy<int> polymode;
    ParameterProxy<float> portamento;
    ParameterProxy<int> portamento_mode;

    ParameterProxy<int> part_vs_layers;
    ParameterProxy<float> part_vs_distribution;
    ParameterProxy<int> part_vs_xf_equality;
    ParameterProxy<float> part_vs_xfade;

    FilterData filters[num_filters_per_part];

    AuxBusData aux[num_aux_busses];
    MMEntryData mm[mm_part_entries];
    NCEntryData nc[num_part_ncs];

    ParameterProxy<float> userparameter[num_midi_channels];
    ParameterProxy<std::string> userparameter_name[num_midi_channels];
    ParameterProxy<int> userparameter_polarity[num_midi_channels];
};
struct MultiData
{
    ParameterProxy<float> filter_pregain[num_fxunits], filter_postgain[num_fxunits];
    ParameterProxy<int> filter_output[num_fxunits];
    FilterData filters[num_fxunits];
};

struct ConfigData
{
    ParameterProxy<float> previewLevel;
    ParameterProxy<int> autoPreview, kbdMode, outputs;

    ParameterProxy<int> controllerId[n_custom_controllers], controllerMode[n_custom_controllers];
};
} // namespace data
} // namespace scxt
#endif // SHORTCIRCUIT_DATAMODEL_H
