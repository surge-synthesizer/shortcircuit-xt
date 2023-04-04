/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#ifndef SCXT_SRC_DSP_PROCESSOR_OSCILLATOR_OSCPULSESYNC_H
#define SCXT_SRC_DSP_PROCESSOR_OSCILLATOR_OSCPULSESYNC_H

#include "dsp/processor/processor.h"

namespace scxt::dsp::processor
{
namespace oscillator
{

struct alignas(16) OscPulseSync : public Processor
{
    static constexpr bool isZoneProcessor{true};
    static constexpr bool isPartProcessor{false};
    static constexpr bool isFXProcessor{false};
    static constexpr const char *processorName{"OSC Pulse Sync"};
    static constexpr const char *processorStreamingName{"osc-pulse-sync"};

    static constexpr int oscillatorBufferLength{16}; // TODO should this be block size really?

    OscPulseSync(engine::MemoryPool *mp, float *f, int32_t *i);

    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch) override;

    bool canProcessMono() override { return true; }
    bool monoInputCreatesStereoOutput() override { return false; }
    void process_mono(float *datain, float *dataoutL, float *dataoutR, float pitch) override;

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
} // namespace oscillator

template <> struct ProcessorImplementor<ProcessorType::proct_osc_pulse_sync>
{
    typedef oscillator::OscPulseSync T;
};
} // namespace scxt::dsp::processor
#endif // SHORTCIRCUITXT_OSCPULSESYNC_H
