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

#include <vector>

class modmatrix;
class sampler_voice;
class configuration;

struct sample_zone;
struct sample_part;
struct timedata;

const int namelen = 24;

enum switchable_sources
{
    ss_LFO1 = 0,
    ss_LFO2,
    ss_LFO3,
    ss_EG2,
    ss_lag0,
    ss_lag1,
    ss_envf,
    num_switchable_sources,
};

enum mm_destinations
{
    md_none,
    md_pitch,

    md_rate,
    md_sample_start,
    md_loop_start,
    md_loop_length,
    // md_granularpos,

    md_amplitude,
    md_pan,
    md_aux_level,
    md_aux_balance,
    md_aux2_level,
    md_aux2_balance,

    md_prefilter_gain,
    md_filter1mix,
    md_filter2mix,
    md_filter1prm0,
    md_filter1prm1,
    md_filter1prm2,
    md_filter1prm3,
    md_filter1prm4,
    md_filter1prm5,
    md_filter2prm0,
    md_filter2prm1,
    md_filter2prm2,
    md_filter2prm3,
    md_filter2prm4,
    md_filter2prm5,
    md_AEG_a,
    md_AEG_h,
    md_AEG_d,
    md_AEG_s,
    md_AEG_r,
    /*	md_AEG_shape1,
            md_AEG_shape2,
            md_AEG_shape3,*/
    md_EG2_a,
    md_EG2_h,
    md_EG2_d,
    md_EG2_s,
    md_EG2_r,
    /*md_EG2_shape1,
    md_EG2_shape2,
    md_EG2_shape3,*/
    md_LFO1_rate,
    md_LFO2_rate,
    md_LFO3_rate,
    md_lag0,
    md_lag1,
    md_num_zone_destinations,
    // part destinations
    md_part_amplitude = 1,
    md_part_pan,
    md_part_aux_level,
    md_part_aux_balance,
    md_part_aux2_level,
    md_part_aux2_balance,
    md_part_prefilter_gain,
    md_part_filter1mix,
    md_part_filter2mix,
    md_part_filter1prm0,
    md_part_filter1prm1,
    md_part_filter1prm2,
    md_part_filter1prm3,
    md_part_filter1prm4,
    md_part_filter1prm5,
    md_part_filter1prm6,
    md_part_filter1prm7,
    md_part_filter1prm8,
    md_part_filter2prm0,
    md_part_filter2prm1,
    md_part_filter2prm2,
    md_part_filter2prm3,
    md_part_filter2prm4,
    md_part_filter2prm5,
    md_part_filter2prm6,
    md_part_filter2prm7,
    md_part_filter2prm8,
    md_num_part_destinations,
    md_num_destinations =
        md_num_zone_destinations, // std::max(md_num_part_destinations,md_num_zone_destinations)
};

struct mm_src
{
    char display_name[namelen];
    char id_name[namelen];
    float *fptr;
    unsigned char RIFFID;
};

struct mm_dst
{
    char display_name[namelen];
    char id_name[namelen];
    int ctrlmode;
    unsigned char RIFFID;
};

class alignas(16) modmatrix
{
  public:
    modmatrix();
    ~modmatrix();
    void process();

    void assign(configuration *conf, sample_zone *zone, sample_part *part, sampler_voice *voice = 0,
                float *control = 0, float *automation = 0, timedata *td = 0);

    bool check_trigger_condition(sample_zone *z);
    void process_part();
    void process_group();
    int get_controltype(int destination);
    void add_destination(unsigned char RIFFID, int id, const char *name, int control_type,
                         const char *dispname = 0);
    void add_source(unsigned char RIFFID, const char *name, float *fptr, const char *dispname = 0);
    int is_source_used(int source);
    int get_n_sources();
    int get_n_destinations();
    char *get_source_name(int id) { return src[id].display_name; }
    char *get_source_idname(int id) { return src[id].id_name; }
    char *get_destination_name(int id) { return dst[id].display_name; }
    char *get_destination_idname(int id) { return dst[id].id_name; }
    int get_destination_ctrlmode(int id) { return dst[id].ctrlmode; }
    unsigned char get_source_RIFFID(int id) { return src[id].RIFFID; }
    unsigned char get_destination_RIFFID(int id) { return dst[id].RIFFID; }
    inline float get_destination_value(int id) { return fdst[id]; }
    int get_destination_value_int(int id);
    float *get_destination_ptr(int id) { return &fdst[id]; }

    int SourceRiffIDToInternal(unsigned char);
    unsigned char SourceInternalToRiffID(unsigned int);
    int DestinationRiffIDToInternal(unsigned char);
    unsigned char DestinationInternalToRiffID(unsigned int);

  private:
    std::vector<mm_src> src;
    mm_dst dst[md_num_destinations];
    float fdst[md_num_destinations];
    sample_zone *__restrict zone;
    sample_part *__restrict part;
    sampler_voice *__restrict voice;
    float *__restrict control, *__restrict automation;
    int ss_id[num_switchable_sources];
    bool first_run;
    float noisegen, alternate;
};

int get_mm_source_id(const char *);
int get_mm_dest_id(const char *);
