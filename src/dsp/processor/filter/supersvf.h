/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_DSP_PROCESSOR_FILTER_SUPERSVF_H
#define SCXT_SRC_DSP_PROCESSOR_FILTER_SUPERSVF_H

#include "dsp/processor/processor.h"
#include "infrastructure/sse_include.h"
#include "sst/filters/HalfRateFilter.h"

namespace scxt::dsp::processor
{
namespace filter
{

template <bool OS> struct alignas(16) SuperSVF : public Processor
{
  private:
    __m128 Freq, dFreq, Q, dQ, MD, dMD, ClipDamp, dClipDamp, Gain, dGain, Reg[3], LastOutput;

    sst::filters::HalfRate::HalfRateFilter mPolyphase;

    inline __m128 process_internal(__m128 x, int Mode);

  public:
    static constexpr const char *processorName{"Super SVF"};
    static constexpr const char *processorStreamingName{"super-svf"};
    static constexpr const char *processorDisplayGroup{"Filters"};

    SuperSVF(engine::MemoryPool *, float *, int *);
    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch) override;
    bool canProcessMono() override { return true; }
    bool monoInputCreatesStereoOutput() override { return false; }
    void process_mono(float *datain, float *dataoutL, float *dataoutR, float pitch) override;

    template <bool Stereo, bool FourPole>
    void ProcessT(float *datainL, float *datainR, float *dataoutL, float *dataoutR, float pitch);

    void init_params() override;
    void suspend() override;
    void calc_coeffs();
    size_t getIntParameterCount() const override;
    std::string getIntParameterLabel(int ip_id) const override;
    size_t getIntParameterChoicesCount(int ip_id) const override;
    std::string getIntParameterChoicesLabel(int ip_id, int c_id) const override;
    int tail_length() override { return 0; }
};

} // namespace filter
} // namespace scxt::dsp::processor

#include "supersvf.cpp"

namespace scxt::dsp::processor
{
template <> struct ProcessorImplementor<ProcessorType::proct_SuperSVF>
{
    typedef filter::SuperSVF<false> T;
    typedef filter::SuperSVF<true> TOS;
};
} // namespace scxt::dsp::processor
#endif // SHORTCIRCUITXT_SUPERSVC_H
