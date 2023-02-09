/*
 * ShortCircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortCircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortCircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortCircuitXT is inspired by, and shares some code with, the
 * commercial product ShortCircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for ShortCircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortCircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_DSP_PROCESSOR_PROCESSOR_DEFS_H
#define SCXT_SRC_DSP_PROCESSOR_PROCESSOR_DEFS_H

#include "processor.h"
#include "datamodel/parameter.h"
#include "infrastructure/sse_include.h"

#include <sst/filters/HalfRateFilter.h>

/**
 * Adding a processor here. Used to be we had a bunch of case staements. But now we are all
 * driven by templates (see "processor.cpp" for some hairy template code you never need to touch).
 * So to add a processor
 *
 * 1. Add it here as a subtype of Processor
 * 2. Implement these four constexpr values
 *       static constexpr bool isZoneProcessor{true};
 *       static constexpr bool isPartProcessor{false};
 *       static constexpr bool isFXProcessor{false};
 *       static constexpr const char *processorName{"OSC Pulse"};
 *       static constexpr const char *processorStreamingName{"osc-pulse"};
 *
 *       The streaming name has to be stable across versions
 *
 * 3. Specialize the id-to-type structure
 *      template <> struct ProcessorImplementor<ProcessorType::proct_osc_pulse_sync>
 *      {
 *          typedef OscPulseSync T;
 *      };
 *
 *  and then evyerthing else will work
 */

namespace scxt::dsp::processor
{
typedef uint8_t unimpl_t;
template <ProcessorType ft> struct ProcessorImplementor
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

struct alignas(16) SuperSVF : public Processor
{
  private:
    __m128 Freq, dFreq, Q, dQ, MD, dMD, ClipDamp, dClipDamp, Gain, dGain, Reg[3], LastOutput;

    sst::filters::HalfRate::HalfRateFilter mPolyphase;

    inline __m128 process_internal(__m128 x, int Mode);

  public:
    static constexpr bool isZoneProcessor{true};
    static constexpr bool isPartProcessor{true};
    static constexpr bool isFXProcessor{false};
    static constexpr const char *processorName{"Super SVF"};
    static constexpr const char *processorStreamingName{"super-svf"};

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

template <> struct ProcessorImplementor<ProcessorType::proct_SuperSVF>
{
    typedef SuperSVF T;
};

/**
 * Oscillators
 */

static constexpr int oscillatorBufferLength{16}; // TODO should this be block size really?

struct alignas(16) OscPulseSync : public Processor
{
    static constexpr bool isZoneProcessor{true};
    static constexpr bool isPartProcessor{false};
    static constexpr bool isFXProcessor{false};
    static constexpr const char *processorName{"OSC Pulse Sync"};
    static constexpr const char *processorStreamingName{"osc-pulse-sync"};

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
template <> struct ProcessorImplementor<ProcessorType::proct_osc_pulse_sync>
{
    typedef OscPulseSync T;
};

} // namespace scxt::dsp::processor
#endif // __SCXT_PROCESSOR_DEFS_H
