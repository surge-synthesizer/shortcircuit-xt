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
struct SCXTVFXConfig
{
    using BaseClass = Processor;
    static constexpr int blockSize{scxt::blockSize};

    static void preReservePool(BaseClass *b, size_t s)
    {
        if (b->memoryPool)
        {
            b->memoryPool->preReservePool(s);
        }
        else
        {
            b->preReserveSize = s;
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
#undef HAS_MEMFN

template <typename T> struct SSTVoiceEffectShim : T
{
    // You can have neither or one, but you can't have both
    static_assert(!(HasMemFn_processMonoToMono<T>::value &&
                    HasMemFn_processMonoToStereo<T>::value));
    template <class... Args> SSTVoiceEffectShim(Args &&...a) : T(std::forward<Args>(a)...)
    {
        static_assert(T::numIntParams <= maxProcessorIntParams);
        if constexpr (T::numIntParams > 0)
        {
            for (int i = 0; i < T::numIntParams; ++i)
            {
                intParamMetadata[i] = this->intParamAt(i);
            }
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

    virtual bool canProcessMono() override
    {
        return HasMemFn_processMonoToMono<T>::value || HasMemFn_processMonoToStereo<T>::value;
    }
    virtual bool monoInputCreatesStereoOutput() override
    {
        return HasMemFn_processMonoToStereo<T>::value;
    }

    size_t getIntParameterCount() const override { return T::numIntParams; }
    std::string getIntParameterLabel(int ip_id) const override
    {
        return intParamMetadata[ip_id].name;
    }
    size_t getIntParameterChoicesCount(int ip_id) const override
    {
        return intParamMetadata[ip_id].maxVal - intParamMetadata[ip_id].minVal + 1;
    }
    std::string getIntParameterChoicesLabel(int ip_id, int c_id) const override
    {
        auto vs = intParamMetadata[ip_id].valueToString(c_id);
        if (vs.has_value())
            return *vs;
        return "Error";
    }

    void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                        float pitch) override
    {
        static_assert(HasMemFn_processStereo<T>::value);
        this->processStereo(datainL, datainR, dataoutL, dataoutR, pitch);
    }

    virtual void process_mono(float *datain, float *dataoutL, float *dataoutR, float pitch) override
    {
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

    int tail_length() override { return tailInfinite; }

    sst::basic_blocks::params::ParamMetaData intParamMetadata[maxProcessorIntParams];
};

template <typename T>
void Processor::setupProcessor(T *that, ProcessorType t, engine::MemoryPool *mp, float *fp, int *ip)
{
    myType = t;
    memoryPool = mp;
    param = fp;
    iparam = ip;

    parameter_count = T::numFloatParams;
    for (int i = 0; i < parameter_count; ++i)
        this->ctrlmode_desc[i] = that->paramAt(i);

    if (preReserveSize > 0)
    {
        assert(memoryPool);
        memoryPool->preReservePool(preReserveSize);
    }
}

} // namespace scxt::dsp::processor
#endif // SHORTCIRCUITXT_PROCESSOR_IMPL_H
