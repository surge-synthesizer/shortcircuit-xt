//
// Created by Paul Walker on 3/6/24.
//

#ifndef SHORTCIRCUITXT_DEFINITION_HELPERS_H
#define SHORTCIRCUITXT_DEFINITION_HELPERS_H

// the varags at end are underlyer ctor args to the base type
#define DEFINE_PROC(procClass, procImpl, osProcImpl, procID, procDisplayName, procDisplayGroup,    \
                    procStreamingName, ...)                                                        \
    namespace procimpl                                                                             \
    {                                                                                              \
    struct alignas(16) procClass : SSTVoiceEffectShim<procImpl>                                    \
    {                                                                                              \
        static constexpr const char *processorName{procDisplayName};                               \
        static constexpr const char *processorStreamingName{procStreamingName};                    \
        static constexpr const char *processorDisplayGroup{procDisplayGroup};                      \
        procClass(engine::MemoryPool *mp, const ProcessorStorage &ps, float *f, int *i,            \
                  bool needsMD)                                                                    \
            : SSTVoiceEffectShim<procImpl>(__VA_ARGS__)                                            \
        {                                                                                          \
            assert(mp);                                                                            \
            setupProcessor(this, procID, mp, ps, f, i, needsMD);                                   \
        }                                                                                          \
        virtual ~procClass() = default;                                                            \
    };                                                                                             \
    struct alignas(16) OS##procClass : SSTVoiceEffectShim<osProcImpl>                              \
    {                                                                                              \
        static constexpr const char *processorName{procDisplayName};                               \
        static constexpr const char *processorStreamingName{procStreamingName};                    \
        static constexpr const char *processorDisplayGroup{procDisplayGroup};                      \
        OS##procClass(engine::MemoryPool *mp, const ProcessorStorage &ps, float *f, int *i,        \
                      bool needsMD)                                                                \
            : SSTVoiceEffectShim<osProcImpl>(__VA_ARGS__)                                          \
        {                                                                                          \
            assert(mp);                                                                            \
            setupProcessor(this, procID, mp, ps, f, i, needsMD);                                   \
        }                                                                                          \
        virtual ~OS##procClass() = default;                                                        \
    };                                                                                             \
    }                                                                                              \
    template <> struct ProcessorImplementor<ProcessorType::procID>                                 \
    {                                                                                              \
        typedef procimpl::procClass T;                                                             \
        typedef procimpl::OS##procClass TOS;                                                       \
        static_assert(sizeof(T) < processorMemoryBufferSize);                                      \
        static_assert(sizeof(TOS) < processorMemoryBufferSize);                                    \
    };

#define PROC_DEFAULT_MIX(proct, mix)                                                               \
    template <> struct ProcessorDefaultMix<proct>                                                  \
    {                                                                                              \
        static constexpr float defaultMix{mix};                                                    \
    };

#endif // SHORTCIRCUITXT_DEFINITION_HELPERS_H
