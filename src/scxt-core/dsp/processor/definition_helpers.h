/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_SCXT_CORE_DSP_PROCESSOR_DEFINITION_HELPERS_H
#define SCXT_SRC_SCXT_CORE_DSP_PROCESSOR_DEFINITION_HELPERS_H

// the varags at end are underlyer ctor args to the base type
#define DEFINE_PROC(procClass, procImpl, osProcImpl, procID, procDisplayName, procDisplayGroup,    \
                    ...)                                                                           \
    namespace procimpl                                                                             \
    {                                                                                              \
    struct alignas(16) procClass : SSTVoiceEffectShim<procImpl>                                    \
    {                                                                                              \
        static constexpr const char *processorName{procDisplayName};                               \
        static constexpr const char *processorStreamingName{procImpl::streamingName};              \
        static constexpr const char *processorDisplayGroup{procDisplayGroup};                      \
        procClass(engine::Engine *engine, engine::MemoryPool *mp, const ProcessorStorage &ps,      \
                  float *f, int *i, bool needsMD)                                                  \
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
        static constexpr const char *processorStreamingName{osProcImpl::streamingName};            \
        static constexpr const char *processorDisplayGroup{procDisplayGroup};                      \
        OS##procClass(engine::Engine *engine, engine::MemoryPool *mp, const ProcessorStorage &ps,  \
                      float *f, int *i, bool needsMD)                                              \
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

#define PROC_FOR_GROUP_ONLY(proct)                                                                 \
    template <> struct ProcessorForGroupOnly<proct>                                                \
    {                                                                                              \
        static constexpr bool isGroupOnly{true};                                                   \
    };

#endif // SHORTCIRCUITXT_DEFINITION_HELPERS_H
