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

#include "bus_effect.h"

#include "dsp/data_tables.h"
#include "tuning/equal.h"

#include "engine/engine.h"

#include "sst/effects/EffectCore.h"
#include "sst/effects/Reverb1.h"
#include "sst/effects/Reverb2.h"
#include "sst/effects/Flanger.h"
#include "sst/effects/Phaser.h"
#include "sst/effects/Delay.h"
#include "sst/effects/FloatyDelay.h"
#include "sst/effects/Bonsai.h"
#include "sst/effects/TreeMonster.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-float-conversion" // Ignore unused variable warnings
#include "sst/effects/NimbusImpl.h"
#pragma GCC diagnostic pop

#include "sst/effects/RotarySpeaker.h"
#include "sst/effects/EffectCoreDetails.h"

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

    static constexpr bool widthIsLinear{true}; // use +/- 200% not +/- 24db

    static inline float floatValueAt(const BaseClass *const e, const ValueStorage *const v, int idx)
    {
        return v[idx];
    }
    static inline int intValueAt(const BaseClass *const e, const ValueStorage *const v, int idx)
    {
        return (int)std::round(v[idx]);
    }

    static inline float envelopeRateLinear(GlobalStorage *s, float f)
    {
        return blockSize * s->getSampleRateInv() * dsp::twoToTheXTable.twoToThe(-f);
    }

    static inline float temposyncRatio(GlobalStorage *s, EffectStorage *e, int idx)
    {
        if (e->isTemposync)
            return s->transport.tempo / 120.f;
        else
            return 1.f;
    }

    static inline bool isDeactivated(EffectStorage *e, int idx) { return e->isExtended(idx); }
    static inline bool isExtended(EffectStorage *e, int idx) { return e->isExtended(idx); }

    static inline float rand01(GlobalStorage *s) { return s->rng.unif01(); }

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

#define HAS_MEMFN(M)                                                                               \
    template <typename T> class HasMemFn_##M                                                       \
    {                                                                                              \
        using No = uint8_t;                                                                        \
        using Yes = uint64_t;                                                                      \
        static_assert(sizeof(No) != sizeof(Yes));                                                  \
        template <typename C> static Yes test(decltype(&C::M) *);                                  \
        template <typename C> static No test(...);                                                 \
                                                                                                   \
      public:                                                                                      \
        enum                                                                                       \
        {                                                                                          \
            value = sizeof(test<T>(nullptr)) == sizeof(Yes)                                        \
        };                                                                                         \
    };

HAS_MEMFN(remapParametersForStreamingVersion);
#undef HAS_MEMFN

template <typename T> struct Impl : T
{
    static_assert(T::numParams <= BusEffectStorage::maxBusEffectParams);
    Engine *engine{nullptr};
    BusEffectStorage *pes{nullptr};
    float *values{nullptr};
    Impl(Engine *e, BusEffectStorage *f, float *v) : engine(e), pes(f), values(v), T(e, f, v)
    {
        f->streamingVersion = T::streamingVersion;
    }
    void init(bool defaultsOverride) override
    {
        static_assert(T::streamingVersion > 0,
                      "All template bus fx need independent streaming version");
        static_assert(HasMemFn_remapParametersForStreamingVersion<T>::value,
                      "All template bus fx need streaming version support");
        if (defaultsOverride)
        {
            for (int i = 0; i < T::numParams && i < BusEffectStorage::maxBusEffectParams; ++i)
            {
                values[i] = this->paramAt(i).defaultVal;
            }
        }
        T::initialize();
    }

    void process(float *__restrict L, float *__restrict R) override { T::processBlock(L, R); }

    datamodel::pmd paramAt(int i) const override { return T::paramAt(i); }

    int numParams() const override { return T::numParams; }

    void onSampleRateChanged() override { T::onSampleRateChanged(); }
};

} // namespace dtl

std::unique_ptr<BusEffect> createEffect(AvailableBusEffects p, Engine *e, BusEffectStorage *s)
{
    namespace sfx = sst::effects;
    if (s)
    {
        s->type = p;
    }
    switch (p)
    {
    case none:
        return nullptr;
    case reverb1:
        return std::make_unique<dtl::Impl<sfx::reverb1::Reverb1<dtl::Config>>>(e, s,
                                                                               s->params.data());
    case reverb2:
        return std::make_unique<dtl::Impl<sfx::reverb2::Reverb2<dtl::Config>>>(e, s,
                                                                               s->params.data());
    case flanger:
        return std::make_unique<dtl::Impl<sfx::flanger::Flanger<dtl::Config>>>(e, s,
                                                                               s->params.data());
    case phaser:
        return std::make_unique<dtl::Impl<sfx::phaser::Phaser<dtl::Config>>>(e, s,
                                                                             s->params.data());
    case treemonster:
        return std::make_unique<dtl::Impl<sfx::treemonster::TreeMonster<dtl::Config>>>(
            e, s, s->params.data());
    case delay:
        return std::make_unique<dtl::Impl<sfx::delay::Delay<dtl::Config>>>(e, s, s->params.data());

    case floatydelay:
        return std::make_unique<dtl::Impl<sfx::floatydelay::FloatyDelay<dtl::Config>>>(
            e, s, s->params.data());

    case nimbus:
        return std::make_unique<dtl::Impl<sfx::nimbus::Nimbus<dtl::Config>>>(e, s,
                                                                             s->params.data());
    case rotaryspeaker:
        return std::make_unique<dtl::Impl<sfx::rotaryspeaker::RotarySpeaker<dtl::Config>>>(
            e, s, s->params.data());
    case bonsai:
        return std::make_unique<dtl::Impl<sfx::bonsai::Bonsai<dtl::Config>>>(e, s,
                                                                             s->params.data());
    }
    return nullptr;
}

std::pair<int16_t, busRemapFn_t> getBusEffectRemapStreamingFunction(AvailableBusEffects p)
{
    namespace sfx = sst::effects;

#define RETVAL(x)                                                                                  \
    {                                                                                              \
        sfx::x<dtl::Config>::streamingVersion,                                                     \
            &sfx::x<dtl::Config>::remapParametersForStreamingVersion                               \
    }
    switch (p)
    {
    case none:
        return {0, nullptr};
    case reverb1:
        return RETVAL(reverb1::Reverb1);
    case reverb2:
        return RETVAL(reverb2::Reverb2);
    case flanger:
        return RETVAL(flanger::Flanger);
    case phaser:
        return RETVAL(phaser::Phaser);
    case treemonster:
        return RETVAL(treemonster::TreeMonster);
    case delay:
        return RETVAL(delay::Delay);
    case floatydelay:
        return RETVAL(floatydelay::FloatyDelay);
    case nimbus:
        return RETVAL(nimbus::Nimbus);
    case rotaryspeaker:
        return RETVAL(rotaryspeaker::RotarySpeaker);
    case bonsai:
        return RETVAL(bonsai::Bonsai);
    }
    return {0, nullptr};
}

} // namespace scxt::engine