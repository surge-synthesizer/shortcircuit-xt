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

const int max_fparams = 9;
const int labelsize = 32;

/*	base class			*/

class filter
{
  public:
    filter(float *params, void *loader = 0, bool stereo = true, int *iparams = 0);
    virtual ~filter() { return; }

    int get_parameter_count() { return parameter_count; }
    char *get_parameter_label(int p_id);
    char *get_parameter_ctrlmode_descriptor(int p_id);
    virtual int get_ip_count() { return 0; }
    virtual const char *get_ip_label(int ip_id) { return (""); }
    virtual int get_ip_entry_count(int ip_id) { return 0; }
    virtual const char *get_ip_entry_label(int ip_id, int c_id) { return (""); }

    int get_parameter_ctrlmode(int p_id);         // deprecated
    char *get_filtername() { return filtername; } // deprecated
    // future

    virtual bool init_freq_graph() { return false; } // filter z-plot honk (visa p� waveformdisplay)
    virtual float get_freq_graph(float f) { return 0; }

    /*void set_entry(int p_id, int c_id) - // set memory tag does not need to map 1:1
    use ctrlmode > num_controlmodes to indicate that it uses an alternative honk?
    void parameter_was_updated(){ bool updated = true;}		// it would be good to update
    stuff at parameter settings
    */
    virtual void init_params() {}
    virtual void init() {}
    virtual void process(float *datain, float *dataout, float pitch) {}
    virtual void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                                float pitch)
    {
    }
    // filters are required to be able to process stereo blocks if stereo is true in the contructor
    virtual void suspend() {}
    virtual int tail_length() { return 1000; }

    float modulation_output; // filters can use this to output modulation data to the matrix

#ifdef _DEBUG
    char debugstr[256];
#endif
  protected:
    float *param;
    int *iparam;
    float lastparam[max_fparams];
    int lastiparam[2];
    int parameter_count;
    int ctrlmode[max_fparams];
    char ctrllabel[max_fparams][labelsize];
    char ctrlmode_desc[max_fparams][labelsize];
    char filtername[32];
    void *loader;
    bool is_stereo;
};

filter *spawn_filter(int id, float *fp, int *ip, void *loader, bool stereo);
bool spawn_filter_release(filter *);