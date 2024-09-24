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

#include "bus.h"

#include "configuration.h"

#include "dsp/data_tables.h"
#include "engine.h"

#include "infrastructure/sse_include.h"

#include "dsp/data_tables.h"
#include "tuning/equal.h"

#include "sst/effects/EffectCore.h"
#include "sst/effects/Reverb1.h"
#include "sst/effects/Reverb2.h"
#include "sst/effects/Flanger.h"
#include "sst/effects/Phaser.h"
#include "sst/effects/Delay.h"
#include "sst/effects/FloatyDelay.h"
#include "sst/effects/Bonsai.h"
#include "sst/effects/TreeMonster.h"
#include "sst/effects/NimbusImpl.h"
#include "sst/effects/RotarySpeaker.h"
#include "sst/effects/EffectCoreDetails.h"

#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/basic-blocks/dsp/PanLaws.h"

#include "messaging/messaging.h"

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
        return std::make_unique<dtl::Impl<sfx::floatydelay::FloatyDelay<dtl::Config>>>(e, s, s->params.data());
            
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
    case nimbus:
        return RETVAL(nimbus::Nimbus);
    case rotaryspeaker:
        return RETVAL(rotaryspeaker::RotarySpeaker);
    case bonsai:
        return RETVAL(bonsai::Bonsai);
    }
    return {0, nullptr};
}

void Bus::setBusEffectType(Engine &e, int idx, scxt::engine::AvailableBusEffects t)
{
    assert(idx >= 0 && idx < maxEffectsPerBus);
    busEffects[idx] = createEffect(t, &e, &busEffectStorage[idx]);
    if (busEffects[idx])
        busEffects[idx]->init(true);
}

void Bus::initializeAfterUnstream(Engine &e)
{
    for (int idx = 0; idx < maxEffectsPerBus; ++idx)
    {
        busEffects[idx] = createEffect(busEffectStorage[idx].type, &e, &busEffectStorage[idx]);
        if (busEffects[idx])
        {
            busEffects[idx]->init(false);
            sendBusEffectInfoToClient(e, idx);
        }
    }
}
void Bus::process()
{
    if (hasOSSignal)
    {
        if (!previousHadOSSignal)
        {
            downsampleFilter.reset();
        }
        downsampleFilter.process_block_D2(outputOS[0], outputOS[1], blockSize << 1);
        mech::accumulate_from_to<blockSize>(outputOS[0], output[0]);
        mech::accumulate_from_to<blockSize>(outputOS[1], output[1]);
    }
    previousHadOSSignal = hasOSSignal;
    // ToDo - we can skip these if theres no pre or at routing
    if (busSendStorage.supportsSends && busSendStorage.hasSends)
    {
        memcpy(auxoutputPreFX, output, sizeof(output));
    }

    int idx{0};
    for (auto &fx : busEffects)
    {
        if (fx && busEffectStorage[idx].isActive)
        {
            fx->process(output[0], output[1]);
        }
        idx++;
    }

    if (busSendStorage.supportsSends && busSendStorage.hasSends)
    {
        memcpy(auxoutputPreVCA, output, sizeof(output));
    }

    // If level becomes modulatable we want to lipol this
    if (busSendStorage.pan != 0.f)
    {
        namespace pl = sst::basic_blocks::dsp::pan_laws;
        // For now we don't interpolate over the block for pan
        auto pv = (std::clamp(busSendStorage.pan, -1.f, 1.f) + 1) * 0.5;
        pl::panmatrix_t pmat{1, 1, 0, 0};

        pl::stereoEqualPower(pv, pmat);

        for (int i = 0; i < blockSize; ++i)
        {
            auto il = output[0][i];
            auto ir = output[1][i];
            output[0][i] = pmat[0] * il + pmat[2] * ir;
            output[1][i] = pmat[1] * ir + pmat[3] * il;
        }
    }
    if (busSendStorage.level != 1.f)
    {
        auto lv = busSendStorage.level * busSendStorage.level * busSendStorage.level;
        mech::scale_by<blockSize>(lv, output[0], output[1]);
    }

    if ((busSendStorage.mute && !busSendStorage.solo) || mutedDueToSoloAway)
    {
        memset(output, 0, sizeof(output));
    }

    if (busSendStorage.supportsSends && busSendStorage.hasSends)
    {
        memcpy(auxoutputPostVCA, output, sizeof(output));
    }

    float a = vuFalloff;

    for (int c = 0; c < 2; ++c)
    {
        vuLevel[c] = std::min(2.f, a * vuLevel[c]);
        /* we use this as a proxy for a silent block without having to scan the entire
         * block. If the block isn't silent, we will get the next vlock if we just get
         * unlucky with sample 0 except in a super pathological case of an exactly
         * 16 sample sin wave or something.
         */
        if (std::fabs(output[c][0]) > 1e-15)
        {
            vuLevel[c] = std::max((float)vuLevel[c], mech::blockAbsMax<BLOCK_SIZE>(output[c]));
        }
    }
}

void Bus::sendBusEffectInfoToClient(const scxt::engine::Engine &e, int slot)
{
    std::array<datamodel::pmd, BusEffectStorage::maxBusEffectParams> pmds;

    int saz{0};
    if (busEffects[slot])
    {
        for (int i = 0; i < busEffects[slot]->numParams(); ++i)
        {
            pmds[i] = busEffects[slot]->paramAt(i);
        }
        saz = busEffects[slot]->numParams();
    }
    for (int i = saz; i < BusEffectStorage::maxBusEffectParams; ++i)
        pmds[i].type = sst::basic_blocks::params::ParamMetaData::NONE;

    messaging::client::serializationSendToClient(
        messaging::client::s2c_bus_effect_full_data,
        messaging::client::busEffectFullData_t{(int)address, slot, {pmds, busEffectStorage[slot]}},
        *(e.getMessageController()));
}

void Bus::sendBusSendStorageToClient(const scxt::engine::Engine &e)
{
    messaging::client::serializationSendToClient(
        messaging::client::s2c_bus_send_data,
        messaging::client::busSendData_t{(int)address, busSendStorage},
        *(e.getMessageController()));
}

void Bus::onSampleRateChanged()
{
    for (auto &fx : busEffects)
    {
        if (fx)
        {
            fx->onSampleRateChanged();
        }
    }
}

std::string
Bus::BusSendStorage::toStringAuxLocation(const scxt::engine::Bus::BusSendStorage::AuxLocation &p)
{
    switch (p)
    {
    case PRE_FX:
        return "pre_fx";
    case POST_FX_PRE_VCA:
        return "post_fx_pre_vca";
    case POST_VCA:
        return "post_vca";
    }
    return "post_vca";
}

Bus::BusSendStorage::AuxLocation Bus::BusSendStorage::fromStringAuxLocation(const std::string &s)
{
    static auto inverse =
        makeEnumInverse<Bus::BusSendStorage::AuxLocation, Bus::BusSendStorage::toStringAuxLocation>(
            Bus::BusSendStorage::AuxLocation::PRE_FX, Bus::BusSendStorage::AuxLocation::POST_VCA);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return POST_VCA;
    return p->second;
}
} // namespace scxt::engine
