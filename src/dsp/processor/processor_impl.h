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

#ifndef SCXT_SRC_DSP_PROCESSOR_PROCESSOR_IMPL_H
#define SCXT_SRC_DSP_PROCESSOR_PROCESSOR_IMPL_H

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
HAS_MEMFN(tailLength);
HAS_MEMFN(enableKeytrack);
HAS_MEMFN(getKeytrack);
HAS_MEMFN(checkParameterConsistency);

#undef HAS_MEMFN

template <typename T> struct SSTVoiceEffectShim : T
{
    // You can have neither or one, but you can't have both
    static_assert(!(HasMemFn_processMonoToMono<T>::value &&
                    HasMemFn_processMonoToStereo<T>::value));
    template <class... Args> SSTVoiceEffectShim(Args &&...a) : T(std::forward<Args>(a)...)
    {
#if DEBUG_LOG_CONFIG
        SCLOG(T::effectName << " mono->mono=" << HasMemFn_processMonoToMono<T>::value
                            << " mono->stereo=" << HasMemFn_processMonoToStereo<T>::value
                            << " stereo->stereo=" << HasMemFn_processStereo<T>::value);
#endif
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

    virtual bool canProcessMono() override
    {
        return HasMemFn_processMonoToMono<T>::value || HasMemFn_processMonoToStereo<T>::value;
    }
    virtual bool monoInputCreatesStereoOutput() override
    {
        return HasMemFn_processMonoToStereo<T>::value;
    }

    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch) override
    {
        this->assertSampleRateSet();
        static_assert(HasMemFn_processStereo<T>::value);
        this->processStereo(datainL, datainR, dataoutL, dataoutR, pitch);
    }

    virtual void process_mono(float *datain, float *dataoutL, float *dataoutR, float pitch) override
    {
        this->assertSampleRateSet();
        if constexpr (HasMemFn_processMonoToMono<T>::value)
        {
            this->processMonoToMono(datain, dataoutL, pitch);
        }
        else if constexpr (HasMemFn_processMonoToStereo<T>::value)
        {
            this->processMonoToStereo(datain, dataoutL, dataoutR, pitch);
        }
        else
        {
            // static assert plus routing constraint on canMono should mean this is never hit
            assert(false);
        }
    }

    void onSampleRateChanged() override
    {
        if (mInitCalledOnce)
            init();
    }
    bool mInitCalledOnce{false};

    int tail_length() override
    {
        if constexpr (HasMemFn_tailLength<T>::value)
        {
            return this->tailLength();
        }
        return 0;
    }

    ProcessorControlDescription getControlDescription() const override
    {
        auto res = T::getControlDescription();

        if constexpr (HasMemFn_enableKeytrack<T>::value)
        {
            static_assert(HasMemFn_getKeytrack<T>::value);
            res.supportsKeytrack = true;
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
};

template <typename T>
void Processor::setupProcessor(T *that, ProcessorType t, engine::MemoryPool *mp,
                               const ProcessorStorage &p, float *fp, int *ip, bool needsMetadata)
{
    myType = t;
    memoryPool = mp;
    param = fp;
    iparam = ip;

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
    }
}

} // namespace scxt::dsp::processor
#endif // SHORTCIRCUITXT_PROCESSOR_IMPL_H
