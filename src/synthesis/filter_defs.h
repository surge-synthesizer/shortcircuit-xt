#pragma once
#include "filter.h"
#include "morphEQ.h"
#include "parameterids.h"
#include "resampling.h"
#include "sampler_state.h"
#include "synthesis/biquadunit.h"
#include <vt_dsp/basic_dsp.h>
#include <vt_dsp/halfratefilter.h>
#include <vt_dsp/lattice.h>
#include <vt_dsp/lipol.h>

// FIXME
#define Align16

//-------------------------------------------------------------------------------------------------------

const char str_freqdef[] = ("f,-5,0.04,6,5,Hz"), str_freqmoddef[] = ("f,-12,0.04,12,0,oct"),
           str_timedef[] = ("f,-10,0.1,10,4,s"), str_lfofreqdef[] = ("f,-5,0.04,6,4,Hz"),
           str_percentdef[] = ("f,0,0.005,1,1,%"), str_percentbpdef[] = ("f,-1,0.005,1,1,%"),
           str_percentmoddef[] = ("f,-32,0.005,32,1,%"), str_dbdef[] = ("f,-96,0.1,12,0,dB"),
           str_dbbpdef[] = ("f,-48,0.1,48,0,dB"), str_dbmoddef[] = ("f,-96,0.1,96,0,dB"),
           str_mpitch[] = ("f,-96,0.04,96,0,cents"), str_bwdef[] = ("f,0.001,0.005,6,0,oct");

const int tail_infinite = 0x1000000;

//-------------------------------------------------------------------------------------------------------

class LP2A : public filter
{
    Align16 biquadunit lp;

  public:
    LP2A(float *);
    void process(float *data, float pitch);
    virtual void init_params();
    virtual void suspend() { lp.suspend(); }
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);

  protected:
};

//-------------------------------------------------------------------------------------------------------

class superbiquad : public filter
{
    Align16 biquadunit bq[4];

  public:
    superbiquad(float *, int *, int);
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void suspend();
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);
    virtual int get_ip_count();
    virtual const char *get_ip_label(int ip_id);
    virtual int get_ip_entry_count(int ip_id);
    virtual const char *get_ip_entry_label(int ip_id, int c_id);
    virtual int tail_length() { return tail_infinite; }

  protected:
    int initmode;
    // lattice_sd d;
};

//-------------------------------------------------------------------------------------------------------

class SuperSVF : public filter
{
  private:
    Align16 __m128 Freq, dFreq, Q, dQ, MD, dMD, ClipDamp, dClipDamp, Gain, dGain, Reg[3],
        LastOutput;

    Align16 halfrate_stereo mPolyphase;

    inline __m128 process_internal(__m128 x, int Mode);

  public:
    SuperSVF(float *, int *);
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);

    template <bool Stereo, bool FourPole>
    void ProcessT(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch);

    virtual void init_params();
    virtual void suspend();
    void calc_coeffs();
    virtual int get_ip_count();
    virtual const char *get_ip_label(int ip_id);
    virtual int get_ip_entry_count(int ip_id);
    virtual const char *get_ip_entry_label(int ip_id, int c_id);
    virtual int tail_length() { return tail_infinite; }
};

//-------------------------------------------------------------------------------------------------------

class LP2B : public filter
{
    Align16 biquadunit lp;

  public:
    LP2B(float *);
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void suspend() { lp.suspend(); }
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);

  protected:
};

//-------------------------------------------------------------------------------------------------------

class LP4M_sat : public filter
{
    Align16 lipol_ps gain;
    Align16 halfrate_stereo pre_filter, post_filter;
    Align16 float reg[10];

  public:
    LP4M_sat(float *, int *);
    virtual ~LP4M_sat();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual int get_ip_count();
    virtual const char *get_ip_label(int ip_id);
    virtual int get_ip_entry_count(int ip_id);
    virtual const char *get_ip_entry_label(int ip_id, int c_id);
    virtual int tail_length() { return tail_infinite; }

  protected:
    bool first_run;
    lag<double> g, r;
};

//-------------------------------------------------------------------------------------------------------

/*	HP2A				*/

class HP2A : public filter
{
    Align16 biquadunit hp;

  public:
    HP2A(float *);
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void suspend() { hp.suspend(); }
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);

  protected:
};

//-------------------------------------------------------------------------------------------------------

/*	BP2A				*/

class BP2A : public filter
{
    Align16 biquadunit bq;

  public:
    BP2A(float *);
    void process(float *data, float pitch);
    virtual void init_params();
    virtual void suspend() { bq.suspend(); }
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);

  protected:
};

//-------------------------------------------------------------------------------------------------------

class BP2B : public filter
{
    Align16 biquadunit bq;

  public:
    BP2B(float *);
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void suspend() { bq.suspend(); }
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);

  protected:
};

//-------------------------------------------------------------------------------------------------------

class BP2AD : public filter
{
    Align16 biquadunit bq[2];

  public:
    BP2AD(float *);
    virtual ~BP2AD();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void suspend()
    {
        bq[0].suspend();
        bq[1].suspend();
    }
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);

  protected:
    void calcfreq(float *a, float *b);
};

//-------------------------------------------------------------------------------------------------------

/*	PKA				*/

class PKA : public filter
{
    Align16 biquadunit bq;

  public:
    PKA(float *);
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void suspend() { bq.suspend(); }
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);

  protected:
};

//-------------------------------------------------------------------------------------------------------

class PKAD : public filter
{
    Align16 biquadunit bq[2];

  public:
    PKAD(float *);
    virtual ~PKAD();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void suspend()
    {
        bq[0].suspend();
        bq[1].suspend();
    }
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);

  protected:
    void calcfreq(float *a, float *b);
};

//-------------------------------------------------------------------------------------------------------

/* LP+HP serial	*/

class LPHP_par : public filter
{
    Align16 biquadunit bq[2];

  public:
    LPHP_par(float *);
    virtual ~LPHP_par();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void suspend()
    {
        bq[0].suspend();
        bq[1].suspend();
    }
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);

  protected:
};

//-------------------------------------------------------------------------------------------------------

class LPHP_ser : public filter
{
    Align16 biquadunit bq[2];

  public:
    LPHP_ser(float *);
    virtual ~LPHP_ser();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void suspend()
    {
        bq[0].suspend();
        bq[1].suspend();
    }
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);

  protected:
};

//-------------------------------------------------------------------------------------------------------

/*	NOTCH				*/

class NOTCH : public filter
{
  protected:
    Align16 biquadunit bq;

  public:
    NOTCH(float *);
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void suspend() { bq.suspend(); }
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);
};

//-------------------------------------------------------------------------------------------------------

/*	EQ2BP - 2 band parametric EQ				*/

class EQ2BP_A : public filter
{
  protected:
    Align16 biquadunit parametric[2];

  public:
    EQ2BP_A(float *, int *);
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void suspend()
    {
        parametric[0].suspend();
        parametric[1].suspend();
    }
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);
    virtual int get_ip_count();
    virtual const char *get_ip_label(int ip_id);
    virtual int get_ip_entry_count(int ip_id);
    virtual const char *get_ip_entry_label(int ip_id, int c_id);
};

//-------------------------------------------------------------------------------------------------------

/*	EQ6B - 6 band graphic EQ				*/

class EQ6B : public filter
{
  protected:
    Align16 biquadunit parametric[6];

  public:
    EQ6B(float *);
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    void calc_coeffs();
    virtual void suspend()
    {
        parametric[0].suspend();
        parametric[1].suspend();
        parametric[2].suspend();
        parametric[3].suspend();
        parametric[4].suspend();
        parametric[5].suspend();
    }
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    } // filter z-plot honk (show per waveformdisplay)
    virtual float get_freq_graph(float f);
};

//-------------------------------------------------------------------------------------------------------

class morphEQ : public filter
{
    Align16 lipol_ps gain;
    Align16 biquadunit b[8];

  public:
    morphEQ(float *, void *, int *);

    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual int get_ip_count();
    virtual const char *get_ip_label(int ip_id);
    virtual int get_ip_entry_count(int ip_id);
    virtual const char *get_ip_entry_label(int ip_id, int c_id);

    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    } // filter z-plot honk (show per waveformdisplay)
    virtual float get_freq_graph(float f);
    // virtual int get_entry_count(int p_id);
    // virtual const char*	get_entry_label(int p_id, int c_id);
    virtual void suspend()
    {
        b[0].suspend();
        b[1].suspend();
        b[2].suspend();
        b[3].suspend();
        b[4].suspend();
        b[5].suspend();
        b[6].suspend();
        b[7].suspend();
    }

  protected:
    bool b_active[8];
    float gaintarget;
    meq_snapshot snap[2];
    void *loader;
};

//-------------------------------------------------------------------------------------------------------

/*	EQ2BP - 2 band parametric EQ				*/

class LP2HP2_morph : public filter
{
  protected:
    Align16 biquadunit f;

  public:
    LP2HP2_morph(float *);
    void process(float *data, float pitch);
    virtual void init_params();
    void calc_coeffs();
    virtual void suspend() { f.suspend(); }
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    } // filter z-plot honk (show per waveformdisplay)
    virtual float get_freq_graph(float f);
};

//-------------------------------------------------------------------------------------------------------

/* comb filter	*/

const int comb_max_delay = 8192;

class COMB1 : public filter
{
  public:
    COMB1(float *);
    virtual ~COMB1();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual int tail_length() { return tail_infinite; }

  protected:
    float delayloop[2][comb_max_delay];
    lipol<float> delaytime, feedback;
    int wpos;
};

//-------------------------------------------------------------------------------------------------------

class COMB3 : public filter
{
  public:
    COMB3(float *);
    virtual ~COMB3();
    void process(float *data, float pitch);
    virtual void init_params();
    virtual int tail_length() { return tail_infinite; }

  protected:
    float delayloop[comb_max_delay];
    lipol<float> delaytime, feedback;
    int wpos;
};

//-------------------------------------------------------------------------------------------------------

/*	comb filter 2			*/

class COMB2 : public filter
{
  public:
    COMB2(float *);
    void process(float *data, float pitch);
    virtual void init_params();
    virtual void suspend()
    {
        f.suspend();
        feedback = 0;
    }
    virtual int tail_length() { return tail_infinite; }

  protected:
    biquadunit f;
    double feedback;
    lag<double> fbval;
};

//-------------------------------------------------------------------------------------------------------

/*	BF				*/

class BF : public filter
{
  public:
    BF(float *);
    virtual ~BF();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();

  protected:
    float time[2], level[2], postslew[2], lp_params[6];
    LP2B *lp;
};

//-------------------------------------------------------------------------------------------------------

/*	OD				*/

class OD : public filter
{
  public:
    OD(float *);
    virtual ~OD();
    void process(float *data, float pitch);
    virtual void init_params();

  protected:
    float time, level, postslew, lp_params[6], pk_params[6];
    filter *lp2a, *peak;
};

//-------------------------------------------------------------------------------------------------------

/*	treemonster				*/

class treemonster : public filter
{
    Align16 biquadunit locut;
    Align16 quadr_osc osc[2];
    Align16 lipol_ps gain[2];

  public:
    treemonster(float *, int *);
    virtual ~treemonster();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual int get_ip_count();
    virtual const char *get_ip_label(int ip_id);
    virtual int get_ip_entry_count(int ip_id);
    virtual const char *get_ip_entry_label(int ip_id, int c_id);

  protected:
    float length[2];
    float lastval[2];
};

//-------------------------------------------------------------------------------------------------------

/*	clipper				*/

class clipper : public filter
{
  public:
    Align16 lipol_ps pregain, postgain;
    clipper(float *);
    virtual ~clipper();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();

  protected:
};

//-------------------------------------------------------------------------------------------------------

/*	stereotools				*/

class stereotools : public filter
{
  public:
    Align16 lipol_ps ampL, ampR, width;
    stereotools(float *, int *);
    virtual ~stereotools();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual int get_ip_count();
    virtual const char *get_ip_label(int ip_id);
    virtual int get_ip_entry_count(int ip_id);
    virtual const char *get_ip_entry_label(int ip_id, int c_id);

  protected:
};

//-------------------------------------------------------------------------------------------------------

class limiter : public filter
{
  protected:
    // Align16 biquadunit bq;
  public:
    limiter(float *, int *);
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void suspend();
    virtual void init();
    virtual int get_ip_count();
    virtual const char *get_ip_label(int ip_id);
    virtual int get_ip_entry_count(int ip_id);
    virtual const char *get_ip_entry_label(int ip_id, int c_id);

  protected:
    Align16 lipol_ps pregain, postgain;
    float ef, at, re;
};

//-------------------------------------------------------------------------------------------------------

class fdistortion : public filter
{
  public:
    Align16 lipol_ps gain;
    halfrate_stereo pre, post;

    fdistortion(float *);
    virtual ~fdistortion();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    // void suspend();
    virtual void init_params();

  protected:
};

//-------------------------------------------------------------------------------------------------------

class fslewer : public filter
{
    Align16 biquadunit bq[2];

  public:
    fslewer(float *);
    virtual ~fslewer();

    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);

    virtual void init_params();
    virtual void suspend()
    {
        bq[0].suspend();
        bq[1].suspend();
        v[0] = 0;
        v[1] = 0;
    }
    void calc_coeffs();
    virtual bool init_freq_graph()
    {
        calc_coeffs();
        return true;
    }
    virtual float get_freq_graph(float f);

  protected:
    lipol<float> rate;
    float v[2];
};
/*
class fexciter : public filter
{
public:
        fexciter(float*);
        virtual ~fexciter();
        void process(float *data, float pitch);
        //void suspend();
        virtual void init_params();
protected:
        lipol<float> amount;
        float reg;
        CHalfBandFilter<2> pre;
        CHalfBandFilter<4> post;
};*/

/*
bygg clipper med x times OS

class clipperHQ : public filter
{
public:
        clipper(float*);
        ~clipper();
        virtual void process(float *data, float pitch);
        virtual void init_params();
protected:
};*/

//-------------------------------------------------------------------------------------------------------

/*	gate				*/

class gate : public filter
{
  public:
    gate(float *);
    virtual ~gate();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();

  protected:
    int holdtime;
    bool gate_state, gate_zc_sync[2];
    float onesampledelay[2];
};

//-------------------------------------------------------------------------------------------------------

const int mg_bufsize = 4096;
class microgate : public filter
{
  public:
    microgate(float *);
    virtual ~microgate();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual int tail_length() { return tail_infinite; }

  protected:
    int holdtime;
    bool gate_state, gate_zc_sync[2];
    float onesampledelay[2];
    float loopbuffer[2][mg_bufsize];
    int bufpos[2], buflength[2];
    bool is_recording[2];
};

//-------------------------------------------------------------------------------------------------------

/* RING					*/

class RING : public filter
{
    Align16 halfrate_stereo pre, post;
    Align16 quadr_osc qosc;

  public:
    RING(float *);
    virtual ~RING();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();

  protected:
    lipol<float, true> amount;
};

//-------------------------------------------------------------------------------------------------------

/* frequency shafter					*/

class FREQSHIFT : public filter
{
  protected:
    Align16 halfrate_stereo fcL, fcR;
    Align16 quadr_osc o1, o2;

  public:
    FREQSHIFT(float *, int *);
    virtual ~FREQSHIFT();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual int get_ip_count();
    virtual const char *get_ip_label(int ip_id);
    virtual int get_ip_entry_count(int ip_id);
    virtual const char *get_ip_entry_label(int ip_id, int c_id);
};

//-------------------------------------------------------------------------------------------------------

/* PMOD					*/

class PMOD : public filter
{
    Align16 lipol_ps pregain, postgain;
    Align16 halfrate_stereo pre, post;

  public:
    PMOD(float *);
    virtual ~PMOD();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();

  protected:
    double phase[2];
    lipol<float, true> omega;
};

//-------------------------------------------------------------------------------------------------------

/* oscillators			*/
/* pulse				*/
const int ob_length = 16;

class osc_pulse : public filter
{
    Align16 float oscbuffer[ob_length];

  public:
    osc_pulse(float *);
    virtual ~osc_pulse();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual int tail_length() { return tail_infinite; }

  protected:
    void convolute();
    bool first_run;
    int64_t oscstate;
    float pitch;
    int polarity;
    float osc_out;
    int bufpos;
};

//-------------------------------------------------------------------------------------------------------

class osc_pulse_sync : public filter
{
    Align16 float oscbuffer[ob_length];

  public:
    osc_pulse_sync(float *);
    virtual ~osc_pulse_sync();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual int tail_length() { return tail_infinite; }

  protected:
    void convolute();
    bool first_run;
    int64_t oscstate, syncstate, lastpulselength;
    float pitch;
    int polarity;
    float osc_out;
    int bufpos;
};

//-------------------------------------------------------------------------------------------------------

/* saw	*/
const int max_unison = 16;

class osc_saw : public filter
{
    Align16 float oscbuffer[ob_length];

  public:
    osc_saw(float *, int *);
    virtual ~osc_saw();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual int get_ip_count();
    virtual const char *get_ip_label(int ip_id);
    virtual int get_ip_entry_count(int ip_id);
    virtual const char *get_ip_entry_label(int ip_id, int c_id);
    virtual int tail_length() { return tail_infinite; }

  protected:
    void convolute(int);
    bool first_run;
    int64_t oscstate[max_unison];
    float pitch;
    float osc_out;
    float out_attenuation;
    float detune_bias, detune_offset;
    float dc, dc_uni[max_unison];
    int bufpos;
    int n_unison;
};

//-------------------------------------------------------------------------------------------------------

/* sin	*/
class osc_sin : public filter
{
  public:
    osc_sin(float *);
    virtual ~osc_sin();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual int tail_length() { return tail_infinite; }

  protected:
    quadr_osc osc;
};

//-------------------------------------------------------------------------------------------------------

// new effects (from SURGE)

const int max_delay_length = 1 << 18;
const int slowrate = 8;
const int slowrate_m1 = slowrate - 1;

//-------------------------------------------------------------------------------------------------------

class dualdelay : public filter
{
    Align16 lipol_ps feedback, crossfeed, pan;
    Align16 float buffer[2][max_delay_length + FIRipol_N];

  public:
    dualdelay(float *, int *);
    virtual ~dualdelay();
    virtual void init();
    virtual void init_params();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void suspend();
    void setvars(bool init);

  private:
    lag<double, true> timeL, timeR;
    float envf;
    int wpos;
    biquadunit lp, hp;
    double lfophase;
    float LFOval;
    bool LFOdirection;
    int ringout_time;
};

//-------------------------------------------------------------------------------------------------------

/* rotary speaker	*/
class rotary_speaker : public filter
{
    Align16 biquadunit xover, lowbass;

  public:
    rotary_speaker(float *, int *);
    virtual ~rotary_speaker();
    // void process_only_control();
    virtual void suspend();
    void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void init();

  protected:
    float buffer[max_delay_length];
    int wpos;
    quadr_osc lfo;
    quadr_osc lf_lfo;
    lipol<float> dL, dR, drive, hornamp[2];
};

//-------------------------------------------------------------------------------------------------------

class phaser : public filter
{
    Align16 float L[block_size], R[block_size];
    Align16 biquadunit *biquad[8];

  public:
    phaser(float *, int *);
    virtual ~phaser();
    // void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void init();
    virtual void suspend();
    void setvars();

  protected:
    lipol<float, true> feedback;
    static const int n_bq = 4;
    static const int n_bq_units = n_bq << 1;
    float dL, dR;
    __m128d ddL, ddR;
    float lfophase;
    int bi; // block increment (to keep track of events not occurring every n blocks)
};

//-------------------------------------------------------------------------------------------------------

/* SC V1 effects	*/

class fauxstereo : public filter
{
    Align16 lipol_ps l_amplitude, l_source;

  public:
    fauxstereo(float *, int *);
    virtual ~fauxstereo();
    // void process(float *datain, float *dataout, float pitch);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    // virtual void init();
    // virtual void suspend();
  protected:
    // lag<float,true> l_amplitude,l_source;
    COMB3 *combfilter;
    float fp[n_filter_parameters];
};

//-------------------------------------------------------------------------------------------------------

/*	fs_flange			*/
class fs_flange : public filter
{
  public:
    fs_flange(float *, int *);
    virtual ~fs_flange();
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();

  protected:
    lipol<float> dry, wet, feedback;
    filter *freqshift[2];
    float f_fs[2][n_filter_parameters];
    int i_fs[2][n_filter_iparameters];
    float fs_buf[2][block_size];
};

//-------------------------------------------------------------------------------------------------------

/*	freqshiftdelay				*/
class freqshiftdelay : public filter
{
  public:
    freqshiftdelay(float *, int *);
    virtual ~freqshiftdelay();
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void suspend();

  protected:
    float *buffer;
    int bufferlength;
    float delaytime_filtered;
    int wpos;
    filter *freqshift;
    float f_fs[n_filter_parameters];
    int i_fs[n_filter_iparameters];
};

//-------------------------------------------------------------------------------------------------------

const int revbits = 15;
const int max_rev_dly = 1 << revbits;
const int rev_tap_bits = 4;
const int rev_taps = 1 << rev_tap_bits;

class reverb : public filter
{
    Align16 float delay_pan_L[rev_taps], delay_pan_R[rev_taps];
    Align16 float delay_fb[rev_taps];
    Align16 float delay[rev_taps * max_rev_dly];
    Align16 float out_tap[rev_taps];
    Align16 float predelay[max_rev_dly];
    Align16 biquadunit band1, locut, hicut;
    Align16 int delay_time[rev_taps];
    Align16 lipol_ps width;

  public:
    reverb(float *, int *);
    virtual ~reverb();
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void init();
    virtual void suspend();
    virtual int get_ip_count();
    virtual const char *get_ip_label(int ip_id);
    virtual int get_ip_entry_count(int ip_id);
    virtual const char *get_ip_entry_label(int ip_id, int c_id);

  protected:
    void update_rtime();
    void update_rsize();
    void clear_buffers();
    void loadpreset(int id);
    int delay_pos;
    double modphase;
    int shape;
    float lastf[n_filter_parameters];
    int ringout_time;
    int b;
};

//-------------------------------------------------------------------------------------------------------

class chorus : public filter
{
    Align16 lipol_ps feedback, width;
    Align16 __m128 voicepanL4[4], voicepanR4[4];
    Align16 biquadunit lp, hp;
    Align16 float buffer[max_delay_length + FIRipol_N]; // s� kan den interpoleras med SSE utan wrap
  public:
    chorus(float *, int *);
    virtual ~chorus();
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch);
    virtual void init_params();
    virtual void init();
    virtual void suspend();

  protected:
    void setvars(bool init);
    lag<float, true> time[4];
    float voicepan[4][2];
    float envf;
    int wpos;
    double lfophase[4];
};
