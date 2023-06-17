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

#include "dsp/processor/processor.h"

namespace scxt::dsp::processor
{
namespace distortion
{

struct alignas(16) BitCrusher : public Processor
{
    static constexpr bool isZoneProcessor{true};
    static constexpr bool isPartProcessor{true};
    static constexpr bool isFXProcessor{false};
    static constexpr const char *processorName{"BitCrusher"};
    static constexpr const char *processorStreamingName{"bit-crusher-fx"};

    BitCrusher(engine::MemoryPool *mp, float *f, int32_t *i);
    ~BitCrusher();

    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch) override;
    void init_params() override;
    int tail_length() override { return tailInfinite; }

  private:
    float time[2]{0.f, 0.f}, level[2]{0.f, 0.f};
};

} // namespace distortion
template <> struct ProcessorImplementor<ProcessorType::proct_fx_bitcrusher>
{
    typedef distortion::BitCrusher T;
    static_assert(sizeof(T) < processorMemoryBufferSize);
};
} // namespace scxt::dsp::processor