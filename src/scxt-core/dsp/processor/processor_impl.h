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

#ifndef SCXT_SRC_SCXT_CORE_DSP_PROCESSOR_PROCESSOR_IMPL_H
#define SCXT_SRC_SCXT_CORE_DSP_PROCESSOR_PROCESSOR_IMPL_H

#include <functional>

#include "configuration.h"
#include "processor.h"
#include "engine/memory_pool.h"

namespace scxt::dsp::processor
{
// oversample factor; 1, 2, 4 etc...
template <int OSFactor> struct SCXTVFXConfig
{
    using BaseClass = Processor;
    static constexpr int blockSize{scxt::blockSize * OSFactor};

    static constexpr int16_t oversamplingRatio{OSFactor};

    static void preReservePool(BaseClass *b, size_t s)
    {
        if (b->memoryPool)
        {
            b->memoryPool->preReservePool(s);
        }
        else
        {
            for (int i = 0; i < 16; ++i)
            {
                if (b->preReserveSize[i] == 0)
                {
                    b->preReserveSize[i] = s;
                    break;
                }
            }
        }
    }

    static void preReserveSingleInstancePool(BaseClass *b, size_t s)
    {
        if (b->memoryPool)
        {
            b->memoryPool->preReserveSingleInstancePool(s);
        }
        else
        {
            for (int i = 0; i < 16; ++i)
            {
                if (b->preReserveSingleInstanceSize[i] == 0)
                {
                    b->preReserveSingleInstanceSize[i] = s;
                    break;
                }
            }
        }
    }

    static uint8_t *checkoutBlock(BaseClass *b, size_t s)
    {
        assert(b->memoryPool);
        return b->memoryPool->checkoutBlock(s);
    }

    static void returnBlock(BaseClass *b, uint8_t *d, size_t s)
    {
        assert(b->memoryPool);
        b->memoryPool->returnBlock(d, s);
    }

    static void setFloatParam(BaseClass *b, size_t index, float val) { b->param[index] = val; }
    static float getFloatParam(const BaseClass *b, size_t index) { return b->param[index]; }

    static void setIntParam(BaseClass *b, size_t index, int val) { b->iparam[index] = val; }
    static int getIntParam(const BaseClass *b, size_t index) { return b->iparam[index]; }

    static bool isTemposync(const BaseClass *c)
    {
        auto ts = c->temposync;
        if (ts)
            return *ts;
        return false;
    }

    static bool isDeactivated(const BaseClass *c, int index)
    {
        auto ts = c->deactivated;
        if (ts)
            return ts[index];
        return false;
    }

    static double *getTempoPointer(const BaseClass *c)
    {
        assert(c->getTempoPointer());
        return c->getTempoPointer();
    }

    static float dbToLinear(BaseClass *b, float db) { return dsp::dbTable.dbToLinear(db); }

    // In voice.cpp and group.cpp we set the sample rate on the processor
    // to a sample rate scaled by the oversample factor so here we can just
    // return it.
    static float getSampleRate(const BaseClass *b) { return b->getSampleRate(); }
    static float getSampleRateInv(const BaseClass *b) { return b->getSampleRateInv(); }
    static float equalNoteToPitch(const BaseClass *b, float f)
    {
        return tuning::equalTuning.note_to_pitch(f);
    }
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
HAS_MEMFN(initVoiceEffect);
HAS_MEMFN(initVoiceEffectParams);
HAS_MEMFN(processStereo);
HAS_MEMFN(processMonoToMono);
HAS_MEMFN(processMonoToStereo);
HAS_MEMFN(silentSamplesLength);
HAS_MEMFN(enableKeytrack);
HAS_MEMFN(getKeytrackDefault);
HAS_MEMFN(getKeytrack);
HAS_MEMFN(checkParameterConsistency);
HAS_MEMFN(getMonoToStereoSetting);
HAS_MEMFN(remapParametersForStreamingVersion);

#undef HAS_MEMFN

template <typename T> struct SSTVoiceEffectShim : T
{
    // If you have both of the former you should also have the latter
    static_assert(!(HasMemFn_processMonoToMono<T>::value &&
                    HasMemFn_processMonoToStereo<T>::value) ||
                  HasMemFn_getMonoToStereoSetting<T>::value);
    template <class... Args> SSTVoiceEffectShim(Args &&...a) : T(std::forward<Args>(a)...)
    {
#if DEBUG_LOG_CONFIG
        SCLOG(T::effectName << " mono->mono=" << HasMemFn_processMonoToMono<T>::value
                            << " mono->stereo=" << HasMemFn_processMonoToStereo<T>::value
                            << " stereo->stereo=" << HasMemFn_processStereo<T>::value);
#endif

        static_assert(T::streamingVersion > 0,
                      "All template processors need independent streaming version");
        static_assert(HasMemFn_remapParametersForStreamingVersion<T>::value,
                      "All template processors need a stream change handler");

        static_assert(std::is_same_v<decltype(&T::processStereo),
                                     void (T::*)(const float *const, const float *const, float *,
                                                 float *, float)>);
        if constexpr (HasMemFn_processMonoToMono<T>::value)
        {
            static_assert(std::is_same_v<decltype(&T::processMonoToMono),
                                         void (T::*)(const float *const, float *, float)>);
        }

        if constexpr (HasMemFn_processMonoToStereo<T>::value)
        {
            static_assert(std::is_same_v<decltype(&T::processMonoToStereo),
                                         void (T::*)(const float *const, float *, float *, float)>);
        }
    }

    virtual ~SSTVoiceEffectShim() = default;

    void init() override
    {
        if constexpr (HasMemFn_initVoiceEffect<T>::value)
        {
            this->initVoiceEffect();
        }
        mInitCalledOnce = true; // this blocks sample rate changes pre-init from re-initing
    }

    void init_params() override
    {
        if constexpr (HasMemFn_initVoiceEffectParams<T>::value)
        {
            this->initVoiceEffectParams();
        }
    }

    bool canProcessMono() override
    {
        return HasMemFn_processMonoToMono<T>::value || HasMemFn_processMonoToStereo<T>::value;
    }
    bool monoInputCreatesStereoOutput() override
    {
        if constexpr (HasMemFn_getMonoToStereoSetting<T>::value)
        {
            return this->getMonoToStereoSetting();
        }
        return HasMemFn_processMonoToStereo<T>::value;
    }

    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch) override
    {
        this->assertSampleRateSet();
        static_assert(HasMemFn_processStereo<T>::value);
        this->processStereo(datainL, datainR, dataoutL, dataoutR, pitch);
    }

    virtual void process_monoToMono(float *datain, float *dataoutL, float pitch) override
    {
        this->assertSampleRateSet();
        if constexpr (HasMemFn_processMonoToMono<T>::value)
        {
            this->processMonoToMono(datain, dataoutL, pitch);
        }
        else
        {
            // static assert plus routing constraint on canMono should mean this is never hit
            assert(false);
        }
    }

    virtual void process_monoToStereo(float *datain, float *dataoutL, float *dataoutR,
                                      float pitch) override
    {
        this->assertSampleRateSet();
        if constexpr (HasMemFn_processMonoToStereo<T>::value)
        {
            this->processMonoToStereo(datain, dataoutL, dataoutR, pitch);
        }
        else
        {
            // static assert plus routing constraint on canMono should mean this is never hit
            assert(false);
        }
    }

    int16_t getStreamingVersion() const override { return T::streamingVersion; }
    void onSampleRateChanged() override
    {
        if (mInitCalledOnce)
            init();
    }
    bool mInitCalledOnce{false};

    int silence_length() override
    {
        if constexpr (HasMemFn_silentSamplesLength<T>::value)
        {
            auto res = this->silentSamplesLength();
            return res;
        }
        return 0;
    }

    ProcessorControlDescription getControlDescription() const override
    {
        ProcessorControlDescription res = T::getControlDescription();

        if constexpr (HasMemFn_enableKeytrack<T>::value)
        {
            static_assert(HasMemFn_getKeytrack<T>::value);
            res.supportsKeytrack = true;
        }

        res.supportsTemposync = false;
        for (int i = 0; i < res.numFloatParams; ++i)
        {
            res.supportsTemposync =
                res.supportsTemposync || res.floatControlDescriptions[i].canTemposync;
            if (!res.floatControlDescriptions[i].supportsStringConversion)
            {
                SCLOG("Warning: processor param " << i << " "
                                                  << res.floatControlDescriptions[i].name
                                                  << " does not support string conversion");
            }
        }

        if constexpr (HasMemFn_checkParameterConsistency<T>::value)
        {
            res.requiresConsistencyCheck = true;
        }

        return res;
    }

    virtual void resetMetadata() override
    {
        for (int i = 0; i < T::floatParameterCount; ++i)
            this->floatParameterMetaData[i] = T::paramAt(i);
        for (int i = T::floatParameterCount; i < maxProcessorFloatParams; ++i)
            this->floatParameterMetaData[i] = {};
        if constexpr (T::numIntParams > 0)
        {
            for (int i = 0; i < T::intParameterCount; ++i)
                this->intParameterMetaData[i] = T::intParamAt(i);
        }
        for (int i = T::intParameterCount; i < maxProcessorIntParams; ++i)
            this->intParameterMetaData[i] = {};
    }

    bool supportsMakingParametersConsistent() override
    {
        return HasMemFn_checkParameterConsistency<T>::value;
    }

    bool makeParametersConsistent() override
    {
        if constexpr (HasMemFn_checkParameterConsistency<T>::value)
        {
            return T::checkParameterConsistency();
        }
        return false;
    }

    bool isKeytracked() const override
    {
        if constexpr (HasMemFn_getKeytrack<T>::value)
        {
            return T::getKeytrack();
        }
        return false;
    }
    bool setKeytrack(bool b) override
    {
        if constexpr (HasMemFn_enableKeytrack<T>::value)
        {
            return T::enableKeytrack(b);
        }
        return false;
    }
    bool getDefaultKeytrack() override
    {
        if constexpr (HasMemFn_getKeytrackDefault<T>::value)
        {
            return T::getKeytrackDefault();
        }
        return false;
    }
};

template <typename T>
void Processor::setupProcessor(T *that, ProcessorType t, engine::MemoryPool *mp,
                               const ProcessorStorage &p, float *fp, int *ip, bool needsMetadata)
{
    myType = t;
    memoryPool = mp;
    param = fp;
    iparam = ip;
    temposync = &p.isTemposynced;
    deactivated = p.deactivated.data();

    setKeytrack(p.isKeytracked);

    floatParameterCount = T::numFloatParams;
    intParameterCount = T::numIntParams;

    if (needsMetadata)
    {
        resetMetadata();
    }

    for (int i = 0; i < 16; ++i)
    {
        if (preReserveSize[i] > 0)
        {
            assert(memoryPool);
            memoryPool->preReservePool(preReserveSize[i]);
        }
        if (preReserveSingleInstanceSize[i] > 0)
        {
            assert(memoryPool);
            memoryPool->preReserveSingleInstancePool(preReserveSingleInstanceSize[i]);
        }
    }
}

} // namespace scxt::dsp::processor
#endif // SHORTCIRCUITXT_PROCESSOR_IMPL_H
