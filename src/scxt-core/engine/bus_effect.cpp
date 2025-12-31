/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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
#pragma GCC diagnostic ignored                                                                     \
    "-Wdeprecated-enum-float-conversion" // Ignore unused variable warnings
#include "sst/effects/NimbusImpl.h"
#pragma GCC diagnostic pop

#include "sst/effects/RotarySpeaker.h"
#include "sst/effects/EffectCoreDetails.h"

namespace scxt::engine
{

namespace dtl
{
typedef uint8_t unimpl_t;
template <AvailableBusEffects ft> struct BusFxImplementor
{
    typedef unimpl_t T;
};

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

template <typename T>
concept BusEffectImplProvider =
    requires(T obj, Engine *e, BusEffectStorage *s, int16_t i, float *f) {
        std::is_constructible_v<T, Engine *, BusEffectStorage *, float *>;
        T::streamingVersion > 0;
        { T::streamingName } -> std::convertible_to<std::string>;
        { T::displayName } -> std::convertible_to<std::string>;
        T::numParams < BusEffectStorage::maxBusEffectParams;
        { obj.remapParametersForStreamingVersion(i, f) } -> std::same_as<void>;
        { obj.initialize() };
        { obj.processBlock(f, f) };
        { obj.paramAt(i) } -> std::same_as<datamodel::pmd>;
        { obj.onSampleRateChanged() };
    };
template <typename T>
concept HasSilentSamplesLength = requires(T obj) {
    { obj.silentSamplesLength() } -> std::same_as<size_t>;
};

template <BusEffectImplProvider T> struct Impl : T
{
    Engine *engine{nullptr};
    BusEffectStorage *pes{nullptr};
    float *values{nullptr};
    Impl(Engine *e, BusEffectStorage *f, float *v) : engine(e), pes(f), values(v), T(e, f, v)
    {
        f->streamingVersion = T::streamingVersion;
    }
    void init(bool defaultsOverride) override
    {
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

    size_t silentSamplesLength() override
    {
        if constexpr (HasSilentSamplesLength<T>)
        {
            return T::silentSamplesLength();
        }
        return 0;
    }

    int numParams() const override { return T::numParams; }

    void onSampleRateChanged() override { T::onSampleRateChanged(); }
};

} // namespace dtl

#define DEFINE_BUSFX(ABS, BASE)                                                                    \
    namespace dtl                                                                                  \
    {                                                                                              \
    struct impl_##ABS : dtl::Impl<BASE<dtl::Config>>                                               \
    {                                                                                              \
        impl_##ABS(Engine *e, BusEffectStorage *f, float *v)                                       \
            : dtl::Impl<BASE<dtl::Config>>(e, f, v)                                                \
        {                                                                                          \
        }                                                                                          \
    };                                                                                             \
    template <> struct BusFxImplementor<AvailableBusEffects::ABS>                                  \
    {                                                                                              \
        typedef impl_##ABS T;                                                                      \
    };                                                                                             \
    }

DEFINE_BUSFX(reverb1, sst::effects::reverb1::Reverb1);
DEFINE_BUSFX(reverb2, sst::effects::reverb2::Reverb2);
DEFINE_BUSFX(flanger, sst::effects::flanger::Flanger);
DEFINE_BUSFX(phaser, sst::effects::phaser::Phaser);
DEFINE_BUSFX(treemonster, sst::effects::treemonster::TreeMonster);
DEFINE_BUSFX(delay, sst::effects::delay::Delay);
DEFINE_BUSFX(floatydelay, sst::effects::floatydelay::FloatyDelay);
DEFINE_BUSFX(nimbus, sst::effects::nimbus::Nimbus);
DEFINE_BUSFX(rotaryspeaker, sst::effects::rotaryspeaker::RotarySpeaker);
DEFINE_BUSFX(bonsai, sst::effects::bonsai::Bonsai);

namespace dtl
{
template <typename ReturnType, ReturnType (*...Funcs)()> auto genericWrapper(size_t ft)
{
    constexpr ReturnType (*fnc[])() = {Funcs...};
    return fnc[ft]();
}

template <size_t I> std::string implGetBusFxStreamingName()
{
    if constexpr (I == AvailableBusEffects::none)
        return "none";
    else
        return BusFxImplementor<(AvailableBusEffects)I>::T::streamingName;
}

template <size_t I> std::string implGetBusFxDisplayName()
{
    if constexpr (I == AvailableBusEffects::none)
        return "None";
    else
        return BusFxImplementor<(AvailableBusEffects)I>::T::displayName;
}

using createEffectFn_t = std::function<std::unique_ptr<BusEffect>(Engine *, BusEffectStorage *)>;
template <size_t I> createEffectFn_t implCreateEffect()
{
    if constexpr (I == AvailableBusEffects::none)
        return [](auto, auto) { return nullptr; };
    else
        return [](auto a, auto b) {
            return std::make_unique<typename BusFxImplementor<(AvailableBusEffects)I>::T>(
                a, b, b->params.data());
        };
}

template <size_t I> std::pair<int16_t, busRemapFn_t> implGetRemap()
{
    if constexpr (I == AvailableBusEffects::none)
        return {0, nullptr};
    else
    {
        using T = typename BusFxImplementor<(AvailableBusEffects)I>::T;
        return {T::streamingVersion, &T::remapParametersForStreamingVersion};
    }
}
} // namespace dtl

std::string getBusEffectStreamingName(AvailableBusEffects p)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return dtl::genericWrapper<std::string, dtl::implGetBusFxStreamingName<Is>...>(ft);
    }(p, std::make_index_sequence<(size_t)(LastAvailableBusEffect + 1)>());
}

std::string getBusEffectDisplayName(AvailableBusEffects p)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return dtl::genericWrapper<std::string, dtl::implGetBusFxDisplayName<Is>...>(ft);
    }(p, std::make_index_sequence<(size_t)(LastAvailableBusEffect + 1)>());
}

std::unique_ptr<BusEffect> createEffect(AvailableBusEffects p, Engine *e, BusEffectStorage *s)
{
    namespace sfx = sst::effects;
    if (s)
    {
        s->type = p;
    }

    auto cf = []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return dtl::genericWrapper<dtl::createEffectFn_t, dtl::implCreateEffect<Is>...>(ft);
    }(p, std::make_index_sequence<(size_t)(LastAvailableBusEffect + 1)>());

    return cf(e, s);
}

std::pair<int16_t, busRemapFn_t> getBusEffectRemapStreamingFunction(AvailableBusEffects p)
{
    return []<size_t... Is>(size_t ft, std::index_sequence<Is...>) {
        return dtl::genericWrapper<std::pair<int16_t, busRemapFn_t>, dtl::implGetRemap<Is>...>(ft);
    }(p, std::make_index_sequence<(size_t)(LastAvailableBusEffect + 1)>());
}

} // namespace scxt::engine