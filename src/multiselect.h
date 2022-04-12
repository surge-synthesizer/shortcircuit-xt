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

class multiselect;

#include "globals.h"
#include <list>

class sampler;
struct steplfostruct;

enum editor_type
{
    edt_group = 0,
    edt_zone,
};

#define set_zone_float(field, value)                                                               \
    set_zone_parameter_float_internal(offsetof(sample_zone, field), value)
#define set_zone_int(field, value)                                                                 \
    set_zone_parameter_int_internal(offsetof(sample_zone, field), value)
#define set_zone_char(field, value)                                                                \
    set_zone_parameter_char_internal(offsetof(sample_zone, field), value)

class multiselect
{
  public:
    multiselect(sampler *nsobj);
    ~multiselect();

    void set_zone_parameter_float_internal(int offset, float value);
    void set_zone_parameter_cstr_internal(int offset, char *value);
    void set_zone_parameter_int_internal(int offset, int value);
    void set_zone_parameter_char_internal(int offset, char value);
    void toggle_zone_modmatrix_switches();

    void set_active_zone(int id);
    void set_active_group(int id);
    void set_active_entry(int type, int id);

    void select_zone(int id);
    void select_group(int id);
    void select_entry(int type, int id);

    void toggle_select_zone(int id);
    void toggle_select_group(int id);
    void toggle_select_entry(int type, int id);

    bool zone_is_selected(int id);
    bool group_is_selected(int id);
    bool entry_is_selected(int type, int id);
    bool zone_is_active(int id);
    bool group_is_active(int id);
    bool entry_is_active(int type, int id);

    void deselect_zone(int id);
    void deselect_group(int id);

    void deselect_all();
    void deselect_groups();
    void deselect_zones();

    void mute_zones(bool);

    void delete_selected();
    void join_selected_groups();
    void move_selected_zones_to_part(int part);
    void move_selected_zones_to_layer(int layer);
    void copy_lfo_waveform_to_selected_zones(steplfostruct *lfo, int entry);

    int get_active_type() { return active_type; }
    int get_active_id() { return active_id; }
    int get_active_zone();
    int get_current_group() { return current_group; }
    int n_selected_groups() { return (int)selected_groups.size(); }
    int n_selected_zones() { return (int)selected_zones.size(); }
    int get_selected_zone(int id);
    int get_selected_group(int id);
    bool get_solo() { return solo; }
    void set_solo(bool s) { solo = s; }

  private:
    int active_id;
    int active_type;
    int current_group;
    bool solo;

    std::list<int> selected_zones;
    std::list<int> selected_groups;
    sampler *sobj;
};
