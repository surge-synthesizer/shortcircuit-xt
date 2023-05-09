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

#include "bus.h"

#include "configuration.h"

#include "dsp/data_tables.h"
#include "engine.h"

#include "infrastructure/sse_include.h"

#include "dsp/data_tables.h"
#include "tuning/equal.h"

#include "sst/effects/EffectCore.h"
#include "sst/effects/Reverb1.h"
#include "sst/effects/Flanger.h"
#include "sst/effects/Delay.h"

#include "sst/basic-blocks/mechanics/block-ops.h"

namespace mech = sst::basic_blocks::mechanics;

namespace scxt::engine
{
namespace dtl
{
struct EngineBiquadAdapter
{
    static inline float dbToLinear(Engine *e, float f) { return dsp::dbTable.dbToLinear(f); }
    static inline float noteToPitchIgnoringTuning(Engine *e, float f)
    {
        return tuning::equalTuning.note_to_pitch(f);
    }
    static inline float sampleRateInv(Engine *e) { return e->getSampleRateInv(); }
};
struct Config
{
    static constexpr int blockSize{scxt::blockSize};
    using BaseClass = BusEffect;
    using GlobalStorage = Engine;
    using EffectStorage = BusEffectStorage;
    using ValueStorage = float;

    using BiquadAdapter = EngineBiquadAdapter;

    static inline float floatValueAt(const BaseClass *const e, const ValueStorage *const v, int idx)
    {
        return v[idx];
    }
    static inline int intValueAt(const BaseClass *const e, const ValueStorage *const v, int idx)
    {
        return (int)std::round(v[idx]);
    }

    static inline float envelopeRateLinear(GlobalStorage *s, float f) { return 0; }

    static inline float temposyncRatio(GlobalStorage *s, EffectStorage *e, int idx) { return 1; }

    static inline bool isDeactivated(EffectStorage *e, int idx) { return false; }
    static inline bool isExtended(EffectStorage *e, int idx) { return false; }

    // TODO: Fix Me obvs
    static inline float rand01(GlobalStorage *s) { return s->rngGen.rand01(); }

    static inline double sampleRate(GlobalStorage *s) { return s->getSampleRate(); }

    static inline float noteToPitch(GlobalStorage *s, float p)
    {
        return noteToPitchIgnoringTuning(s, p);
    }
    static inline float noteToPitchIgnoringTuning(GlobalStorage *s, float p)
    {
        return tuning::equalTuning.note_to_pitch(p);
    }

    static inline float noteToPitchInv(GlobalStorage *s, float p)
    {
        return 1.f / tuning::equalTuning.note_to_pitch(p);
    }

    static inline float dbToLinear(GlobalStorage *s, float f) { return dsp::dbTable.dbToLinear(f); }
};

template <typename T> struct Impl : T
{
    static_assert(T::numParams <= BusEffectStorage::maxBusEffectParams);
    Engine *engine{nullptr};
    BusEffectStorage *pes{nullptr};
    float *values{nullptr};
    Impl(Engine *e, BusEffectStorage *f, float *v) : engine(e), pes(f), values(v), T(e, f, v) {}
    void init() override
    {
        for (int i = 0; i < T::numParams && i < BusEffectStorage::maxBusEffectParams; ++i)
        {
            values[i] = this->paramAt(i).defaultVal;
        }
        T::initialize();
    }
    void process(float *__restrict L, float *__restrict R) override { T::processBlock(L, R); }
};
} // namespace dtl

std::unique_ptr<BusEffect> createEffect(AvailableBusEffects p, Engine *e, BusEffectStorage *s)
{
    namespace sfx = sst::effects;
    switch (p)
    {
    case reverb1:
        return std::make_unique<dtl::Impl<sfx::Reverb1<dtl::Config>>>(e, s, s->params);
    case flanger:
        return std::make_unique<dtl::Impl<sfx::Flanger<dtl::Config>>>(e, s, s->params);
    case delay:
        return std::make_unique<dtl::Impl<sfx::Delay<dtl::Config>>>(e, s, s->params);
    }
    return nullptr;
}

void Bus::process()
{
    if (supportsSends && hasSends && auxLocation == PRE_FX)
        memcpy(auxoutput, output, sizeof(output));

    if (supportsEffects)
    {
        for (auto &fx : busEffects)
        {
            if (fx)
            {
                fx->process(output[0], output[1]);
            }
        }
    }

    if (supportsSends && hasSends && auxLocation == POST_FX_PRE_VCA)
        memcpy(auxoutput, output, sizeof(output));

    // If level becomes modulatable we want to lipol this
    if (level != 1.f)
    {
        mech::scale_by<blockSize>(level, output[0], output[1]);
    }

    if (supportsSends && hasSends && auxLocation == POST_VCA)
        memcpy(auxoutput, output, sizeof(output));
}

} // namespace scxt::engine
