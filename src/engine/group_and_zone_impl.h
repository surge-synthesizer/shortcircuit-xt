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

#include "group_and_zone.h"

/*
 * For why this is a .h, see the comment in group_and_zone.h
 */

namespace scxt::engine
{

template <typename T>
void HasGroupZoneProcessors<T>::setProcessorType(int whichProcessor,
                                                 dsp::processor::ProcessorType type)
{
    assert(asT()->getEngine() &&
           asT()->getEngine()->getMessageController()->threadingChecker.isAudioThread());

    auto &ps = processorStorage[whichProcessor];
    ps.type = type;
    ps.mix = 1.0;

    uint8_t memory[dsp::processor::processorMemoryBufferSize];
    dsp::processor::Processor *tmpProcessor{nullptr};
    float pfp[dsp::processor::maxProcessorFloatParams];
    int ifp[dsp::processor::maxProcessorIntParams];

    // this processor is just used for params and the like so we can always assume
    // the non-oversampled case
    tmpProcessor = dsp::processor::spawnProcessorInPlace(
        type, asT()->getEngine()->getMemoryPool().get(), memory,
        dsp::processor::processorMemoryBufferSize, pfp, ifp, false);

    if (type != dsp::processor::proct_none)
    {
        assert(tmpProcessor);
        assert(asT()->getEngine());
        tmpProcessor->setSampleRate(asT()->getEngine()->getSampleRate());
        tmpProcessor->init();
        tmpProcessor->init_params();

        memcpy(&(ps.floatParams[0]), pfp, sizeof(ps.floatParams));
        memcpy(&(ps.intParams[0]), ifp, sizeof(ps.intParams));
        setupProcessorControlDescriptions(whichProcessor, type, tmpProcessor);

        // TODO: The control descriptions get populated now also
        dsp::processor::unspawnProcessor(tmpProcessor);
    }
    else
    {
        assert(!tmpProcessor);
        setupProcessorControlDescriptions(whichProcessor, type, tmpProcessor);
    }

    asT()->onProcessorTypeChanged(whichProcessor, type);
}

template <typename T>
void HasGroupZoneProcessors<T>::setupProcessorControlDescriptions(
    int whichProcessor, dsp::processor::ProcessorType type,
    dsp::processor::Processor *tmpProcessorFromAfar)
{
    if (type == dsp::processor::proct_none)
    {
        processorDescription[whichProcessor] = {};
        processorDescription[whichProcessor].typeDisplayName = "Off";
        return;
    }

    auto *tmpProcessor = tmpProcessorFromAfar;

    uint8_t memory[dsp::processor::processorMemoryBufferSize];
    float pfp[dsp::processor::maxProcessorFloatParams];
    int ifp[dsp::processor::maxProcessorIntParams];

    // this is just used for params and stuff so assume no oversample
    if (!tmpProcessor)
    {
        tmpProcessor = dsp::processor::spawnProcessorInPlace(
            type, asT()->getEngine()->getMemoryPool().get(), memory,
            dsp::processor::processorMemoryBufferSize, pfp, ifp, false);
    }

    assert(tmpProcessor);

    processorDescription[whichProcessor] = tmpProcessor->getControlDescription();

    if (!tmpProcessorFromAfar)
        dsp::processor::unspawnProcessor(tmpProcessor);
}

template <typename T>
std::string HasGroupZoneProcessors<T>::toStringProcRoutingPath(
    const scxt::engine::HasGroupZoneProcessors<T>::ProcRoutingPath &p)
{
    switch (p)
    {
    case procRoute_linear:
        return "procRoute_linear";
    case procRoute_bypass:
        return "procRoute_bypass";
    }
    return "procRoute_linear";
}

template <typename T>
typename HasGroupZoneProcessors<T>::ProcRoutingPath
HasGroupZoneProcessors<T>::fromStringProcRoutingPath(const std::string &s)
{
    static auto inverse =
        makeEnumInverse<ProcRoutingPath, HasGroupZoneProcessors<T>::toStringProcRoutingPath>(
            procRoute_linear, procRoute_bypass);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return procRoute_linear;
    return p->second;
}

template <typename T>
std::string HasGroupZoneProcessors<T>::getProcRoutingPathDisplayName(
    scxt::engine::HasGroupZoneProcessors<T>::ProcRoutingPath p)
{
    switch (p)
    {
    case procRoute_linear:
        return "Linear";
    case procRoute_bypass:
        return "Bypass";
    }
    return "ERROR";
}
} // namespace scxt::engine