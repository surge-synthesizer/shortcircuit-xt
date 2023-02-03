//
// Created by Paul Walker on 1/31/23.
//

#ifndef __SCXT_FILTER_DEFS_H
#define __SCXT_FILTER_DEFS_H

#include "filter.h"
#include "datamodel/parameter.h"
#include "infrastructure/sse_include.h"

#include <sst/filters/HalfRateFilter.h>

/**
 * Adding a filter here. Used to be we had a bunch of case staements. But now we are all
 * driven by templates (see "filter.cpp" for some hairy template code you never need to touch).
 * So to add a filter
 *
 * 1. Add it here as a subtype of Filter
 * 2. Implement these four constexpr values
 *       static constexpr bool isZoneFilter{true};
 *       static constexpr bool isPartFilter{false};
 *       static constexpr bool isFXFilter{false};
 *       static constexpr const char *filterName{"OSC Pulse"};
 * 3. Specialize the id-to-type structure
 *      template <> struct FilterImplementor<FilterType::ft_osc_pulse_sync>
 *      {
 *          typedef OscPulseSync T;
 *      };
 *
 *  and then evyerthing else will work
 */

namespace scxt::dsp::filter
{
typedef uint8_t unimpl_t;
template <FilterType ft> struct FilterImplementor
{
    typedef unimpl_t T;
};
/*const char str_freqdef[] = ("f,-5,0.04,6,5,Hz"), str_freqmoddef[] = ("f,-12,0.04,12,0,oct"),
           str_timedef[] = ("f,-10,0.1,10,4,s"), str_lfofreqdef[] = ("f,-5,0.04,6,4,Hz"),
           str_percentdef[] = ("f,0,0.005,1,1,%"), str_percentbpdef[] = ("f,-1,0.005,1,1,%"),
           str_percentmoddef[] = ("f,-32,0.005,32,1,%"), str_dbdef[] = ("f,-96,0.1,12,0,dB"),
           str_dbbpdef[] = ("f,-48,0.1,48,0,dB"), str_dbmoddef[] = ("f,-96,0.1,96,0,dB"),
           str_mpitch[] = ("f,-96,0.04,96,0,cents"), str_bwdef[] = ("f,0.001,0.005,6,0,oct");*/

static constexpr datamodel::ControlDescription cdPercentDef{
    datamodel::ControlDescription::FLOAT, 0, 0.005, 1, 1, "%"},
    cdMPitch{datamodel::ControlDescription::FLOAT, -96, 0.04, 96, 0, "cents"},
    cdFreqDef{datamodel::ControlDescription::FLOAT, -5, 0.04, 6, 5, "Hz"};

/**
 * Standard filters
 */

struct alignas(16) SuperSVF : public Filter
{
  private:
    __m128 Freq, dFreq, Q, dQ, MD, dMD, ClipDamp, dClipDamp, Gain, dGain, Reg[3], LastOutput;

    sst::filters::HalfRate::HalfRateFilter mPolyphase;

    inline __m128 process_internal(__m128 x, int Mode);

  public:
    static constexpr bool isZoneFilter{true};
    static constexpr bool isPartFilter{true};
    static constexpr bool isFXFilter{false};
    static constexpr const char *filterName{"Super SVF"};
    static constexpr const char *filterStreamingName{"super-svf"};

    SuperSVF(float *, int *, bool);
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
    virtual int tail_length() { return tailInfinite; }
};

template <> struct FilterImplementor<FilterType::ft_SuperSVF>
{
    typedef SuperSVF T;
};

/**
 * Oscillators
 */

static constexpr int oscillatorBufferLength{16}; // TODO should this be block size really?

struct alignas(16) OscPulseSync : public Filter
{
    static constexpr bool isZoneFilter{true};
    static constexpr bool isPartFilter{false};
    static constexpr bool isFXFilter{false};
    static constexpr const char *filterName{"OSC Pulse Sync"};
    static constexpr const char *filterStreamingName{"osc-pulse-sync"};

    OscPulseSync(float *f, int32_t *i, bool s);

    void process(float *datain, float *dataout, float pitch) override;
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch) override;
    void init_params() override;
    int tail_length() override { return tailInfinite; }

  protected:
    float oscbuffer alignas(16)[oscillatorBufferLength];

    void convolute();
    bool first_run{true};
    int64_t oscstate{0}, syncstate{0};
    float pitch{0};
    bool polarity{false};
    float osc_out{0};
    size_t bufpos{0};
};
template <> struct FilterImplementor<FilterType::ft_osc_pulse_sync>
{
    typedef OscPulseSync T;
};

} // namespace scxt::dsp::filter
#endif // __SCXT_FILTER_DEFS_H
