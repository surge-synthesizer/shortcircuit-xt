//
// Created by Paul Walker on 4/25/23.
//

#include "part_effects.h"
#include "configuration.h"

#include "dsp/data_tables.h"
#include "engine.h"

#include "infrastructure/sse_include.h"

#include "dsp/data_tables.h"
#include "tuning/equal.h"

#include "sst/effects/EffectCore.h"
#include "sst/effects/Reverb1.h"
#include "sst/effects/Flanger.h"

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
    using BaseClass = PartEffect;
    using GlobalStorage = Engine;
    using EffectStorage = PartEffectStorage;
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

    // TODO: Fix Me obvs
    static inline float rand01(GlobalStorage *s) { return (float)rand() / (float)RAND_MAX; }

    static inline double sampleRate(GlobalStorage *s) { return 48000; }

    static inline float noteToPitch(GlobalStorage *s, float p) { return 1; }
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
    static_assert(T::numParams <= PartEffectStorage::maxPartEffectParams);
    Engine *engine{nullptr};
    PartEffectStorage *pes{nullptr};
    float *values{nullptr};
    Impl(Engine *e, PartEffectStorage *f, float *v) : engine(e), pes(f), values(v), T(e, f, v) {}
    void init() override
    {
        for (int i = 0; i < T::numParams && i < PartEffectStorage::maxPartEffectParams; ++i)
        {
            values[i] = this->paramAt(i).defaultVal;
        }
        T::initialize();
    }
    void process(float *__restrict L, float *__restrict R) override { T::processBlock(L, R); }
};
} // namespace dtl
std::unique_ptr<PartEffect> createEffect(AvailablePartEffects p, Engine *e, PartEffectStorage *s)
{
    namespace sfx = sst::effects;
    switch (p)
    {
    case reverb1:
        return std::make_unique<dtl::Impl<sfx::Reverb1<dtl::Config>>>(e, s, s->params);
    case flanger:
        return nullptr;
    }
    return nullptr;
}
} // namespace scxt::engine