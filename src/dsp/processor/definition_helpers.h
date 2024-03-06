//
// Created by Paul Walker on 3/6/24.
//

#ifndef SHORTCIRCUITXT_DEFINITION_HELPERS_H
#define SHORTCIRCUITXT_DEFINITION_HELPERS_H

// the varags at end are underlyer ctor args to the base type
#define DEFINE_PROC(procClass, procImpl, procID, procDisplayName, procDisplayGroup,                \
                    procStreamingName, ...)                                                        \
    namespace procimpl                                                                             \
    {                                                                                              \
    struct alignas(16) procClass : SSTVoiceEffectShim<procImpl>                                    \
    {                                                                                              \
        static constexpr const char *processorName{procDisplayName};                               \
        static constexpr const char *processorStreamingName{procStreamingName};                    \
        static constexpr const char *processorDisplayGroup{procDisplayGroup};                      \
        procClass(engine::MemoryPool *mp, float *f, int32_t *i)                                    \
            : SSTVoiceEffectShim<procImpl>(__VA_ARGS__)                                            \
        {                                                                                          \
            assert(mp);                                                                            \
            setupProcessor(this, procID, mp, f, i);                                                \
        }                                                                                          \
        virtual ~procClass() = default;                                                            \
    };                                                                                             \
    }                                                                                              \
    template <> struct ProcessorImplementor<ProcessorType::procID>                                 \
    {                                                                                              \
        typedef procimpl::procClass T;                                                             \
        static_assert(sizeof(T) < processorMemoryBufferSize);                                      \
    };

#endif // SHORTCIRCUITXT_DEFINITION_HELPERS_H
