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

#ifndef SCXT_SRC_DSP_PROCESSOR_DISTORTION_MICROGATE_H
#define SCXT_SRC_DSP_PROCESSOR_DISTORTION_MICROGATE_H

#include "dsp/processor/processor.h"

namespace scxt::dsp::processor
{
namespace distortion
{
struct alignas(16) MicroGate : public Processor
{
    static constexpr int microgateBufferSize{4096};
    static constexpr int microgateBlockSize{microgateBufferSize * sizeof(float) * 2};

    // Must be a power of 2
    static_assert((microgateBufferSize >= 1) & !(microgateBufferSize & (microgateBufferSize - 1)));

    static constexpr bool isZoneProcessor{true};
    static constexpr bool isPartProcessor{true};
    static constexpr bool isFXProcessor{false};
    static constexpr const char *processorName{"MicroGate"};
    static constexpr const char *processorStreamingName{"micro-gate-fx"};

    MicroGate(engine::MemoryPool *mp, float *f, int32_t *i);
    ~MicroGate();

    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch) override;
    void init() override;
    void init_params() override;
    int tail_length() override { return tailInfinite; }

  protected:
    int holdtime{0};
    bool gate_state{false}, gate_zc_sync[2]{false, false};
    float onesampledelay[2]{-1, -1};
    // float loopbuffer[2][microgateBufferSize];
    float *loopbuffer[2]{nullptr, nullptr};
    int bufpos[2]{0, 0}, buflength[2]{0, 0};
    bool is_recording[2]{false, false};
};
} // namespace distortion
template <> struct ProcessorImplementor<ProcessorType::proct_fx_microgate>
{
    typedef distortion::MicroGate T;
    static_assert(sizeof(T) < processorMemoryBufferSize);
};
} // namespace scxt::dsp::processor
#endif // SHORTCIRCUITXT_MICROGATE_H
