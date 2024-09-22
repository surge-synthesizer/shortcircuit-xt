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

#ifndef SCXT_SRC_ENGINE_GROUP_AND_ZONE_IMPL_H
#define SCXT_SRC_ENGINE_GROUP_AND_ZONE_IMPL_H

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
    auto &pd = processorDescription[whichProcessor];
    ps.type = type;

    ps.mix = dsp::processor::getProcessorDefaultMix(type);

    uint8_t memory[dsp::processor::processorMemoryBufferSize];
    float pfp[dsp::processor::maxProcessorFloatParams];
    int ifp[dsp::processor::maxProcessorIntParams];

    memcpy(pfp, ps.floatParams.data(), sizeof(ps.floatParams));
    memcpy(ifp, ps.intParams.data(), sizeof(ps.intParams));

    // this processor is just used for params and the like so we can always assume
    // the non-oversampled case
    auto *tmpProcessor = spawnTempProcessor(whichProcessor, type, memory, pfp, ifp, true);

    setupProcessorControlDescriptions(whichProcessor, type, tmpProcessor);

    if (tmpProcessor)
    {
        ps.streamingVersion = tmpProcessor->getStreamingVersion();
        dsp::processor::unspawnProcessor(tmpProcessor);
    }
    else
    {
        ps.streamingVersion = 0;
    }

    SCLOG("STREAMING VERSION IS " << ps.streamingVersion);
    asT()->onProcessorTypeChanged(whichProcessor, type);
}

template <typename T>
void HasGroupZoneProcessors<T>::setupProcessorControlDescriptions(
    int whichProcessor, dsp::processor::ProcessorType type,
    dsp::processor::Processor *tmpProcessorFromAfar, bool reClampFloatValues)
{
    if (type == dsp::processor::proct_none)
    {
        processorDescription[whichProcessor] = {};
        processorDescription[whichProcessor].typeDisplayName = dsp::processor::noneDisplayName;
        return;
    }

    auto *tmpProcessor = tmpProcessorFromAfar;

    uint8_t memory[dsp::processor::processorMemoryBufferSize];
    float pfp[dsp::processor::maxProcessorFloatParams];
    int ifp[dsp::processor::maxProcessorIntParams];

    auto &ps = asT()->processorStorage[whichProcessor];

    // this is just used for params and stuff so assume no oversample
    if (!tmpProcessor)
    {
        memcpy(pfp, ps.floatParams.data(), sizeof(ps.floatParams));
        memcpy(ifp, ps.intParams.data(), sizeof(ps.intParams));
        tmpProcessor = spawnTempProcessor(whichProcessor, type, memory, pfp, ifp, false);
    }

    assert(tmpProcessor);

    tmpProcessor->setKeytrack(ps.isKeytracked);

    processorDescription[whichProcessor] = tmpProcessor->getControlDescription();

    if (forGroup)
    {
        processorDescription[whichProcessor].supportsKeytrack = false;
    }

    if (reClampFloatValues)
    {
        // Clamp floats to min/max here. This matters when, say, you toggle
        // keytrack and change ranges and so need to clamp inside the new range.
        auto &pd = processorDescription[whichProcessor];
        for (int i = 0; i < pd.numFloatParams; ++i)
        {
            ps.floatParams[i] = std::clamp(ps.floatParams[i], pd.floatControlDescriptions[i].minVal,
                                           pd.floatControlDescriptions[i].maxVal);
        }
    }

    if (!tmpProcessorFromAfar)
        dsp::processor::unspawnProcessor(tmpProcessor);
}

template <typename T>
dsp::processor::Processor *
HasGroupZoneProcessors<T>::spawnTempProcessor(int whichProcessor,
                                              dsp::processor::ProcessorType type, uint8_t *mem,
                                              float *pfp, int *ifp, bool initFromDefaults)
{
    auto &tc = asT()->getEngine()->getMessageController()->threadingChecker;

    dsp::processor::Processor *tmpProcessor{nullptr};

    if (type != dsp::processor::proct_none)
    {
        auto &ps = processorStorage[whichProcessor];
        tmpProcessor = dsp::processor::spawnProcessorInPlace(
            type, asT()->getEngine()->getMemoryPool().get(), mem,
            dsp::processor::processorMemoryBufferSize, ps, pfp, ifp, false, true);

        assert(tmpProcessor);
        assert(asT()->getEngine());
        if (asT()->getEngine()->isSampleRateSet())
        {
            tmpProcessor->setSampleRate(asT()->getEngine()->getSampleRate());
            tmpProcessor->setTempoPointer(&(asT()->getEngine()->transport.tempo));
            tmpProcessor->init();

            if (initFromDefaults)
            {
                // This is a no-op if you don't support keytrack
                ps.previousIsKeytracked = -1;
                ps.isKeytracked = tmpProcessor->getDefaultKeytrack();
                tmpProcessor->setKeytrack(ps.isKeytracked);
                tmpProcessor->init_params(); // thos blows out with default}

                if (tmpProcessor && tmpProcessor->supportsMakingParametersConsistent())
                {
                    tmpProcessor->makeParametersConsistent();
                }

                tmpProcessor->resetMetadata();
                memcpy(&(ps.floatParams[0]), pfp, sizeof(ps.floatParams));
                memcpy(&(ps.intParams[0]), ifp, sizeof(ps.intParams));
            }
        }
    }

    return tmpProcessor;
}

template <typename T>
bool HasGroupZoneProcessors<T>::checkOrAdjustIntConsistency(int whichProcessor)
{
    auto &pd = asT()->processorDescription[whichProcessor];

    if (pd.requiresConsistencyCheck)
    {
        auto &ps = asT()->processorStorage[whichProcessor];
        uint8_t memory[dsp::processor::processorMemoryBufferSize];
        float pfp[dsp::processor::maxProcessorFloatParams];
        int ifp[dsp::processor::maxProcessorIntParams];

        memcpy(pfp, ps.floatParams.data(), sizeof(ps.floatParams));
        memcpy(ifp, ps.intParams.data(), sizeof(ps.intParams));
        auto tmpProcessor = spawnTempProcessor(whichProcessor, ps.type, memory, pfp, ifp, false);

        auto restate = tmpProcessor->makeParametersConsistent();

        setupProcessorControlDescriptions(whichProcessor, ps.type, tmpProcessor, restate);

        if (restate)
        {
            memcpy(ps.floatParams.data(), pfp, sizeof(ps.floatParams));
            memcpy(ps.intParams.data(), ifp, sizeof(ps.intParams));
        }

        // TODO: This could be more parsimonious
        messaging::audio::AudioToSerialization updateProc;
        updateProc.id = messaging::audio::a2s_processor_refresh;
        updateProc.payloadType = messaging::audio::AudioToSerialization::INT;
        updateProc.payload.i[0] = forZone;
        updateProc.payload.i[1] = whichProcessor;
        asT()->getEngine()->getMessageController()->sendAudioToSerialization(updateProc);

        dsp::processor::unspawnProcessor(tmpProcessor);
        return true;
    }

    return false;
}

template <typename T>
bool HasGroupZoneProcessors<T>::checkOrAdjustBoolConsistency(int whichProcessor)
{
    auto &pd = asT()->processorDescription[whichProcessor];

    if (pd.supportsKeytrack)
    {
        auto &ps = asT()->processorStorage[whichProcessor];
        if (ps.previousIsKeytracked < 0 || ps.isKeytracked != ps.previousIsKeytracked)
        {
            ps.previousIsKeytracked = ps.isKeytracked ? 1 : 0;
            asT()->setupProcessorControlDescriptions(whichProcessor, ps.type, nullptr, true);

            messaging::audio::AudioToSerialization updateProc;
            updateProc.id = messaging::audio::a2s_processor_refresh;
            updateProc.payloadType = messaging::audio::AudioToSerialization::INT;
            updateProc.payload.i[0] = forZone;
            updateProc.payload.i[1] = whichProcessor;
            asT()->getEngine()->getMessageController()->sendAudioToSerialization(updateProc);
            return true;
        }
    }

    return false;
}

template <typename T>
std::string HasGroupZoneProcessors<T>::toStringProcRoutingPath(
    const scxt::engine::HasGroupZoneProcessors<T>::ProcRoutingPath &p)
{
    switch (p)
    {
    case procRoute_linear:
        return "procRoute_linear";
    case procRoute_ser2:
        return "procRoute_ser2";
    case procRoute_ser3:
        return "procRoute_ser3";
    case procRoute_par1:
        return "procRoete_par1";
    case procRoute_par2:
        return "procRoete_par2";
    case procRoute_par3:
        return "procRoete_par3";
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
        return "Ser 1 : Linear";
    case procRoute_ser2:
        return "Ser 2 : > { 1 | 2 } > { 3 | 4 } >";
    case procRoute_ser3:
        return "Ser 3 : > 1 > { 2 | 3 } > 4 >";
    case procRoute_par1:
        return "Par 1 : > { 1 | 2 | 3 |4 } >";
    case procRoute_par2:
        return "Par 2 : > { { 1>2 } | { 3 > 4 } } >";
    case procRoute_par3:
        return "Par 3 : > { 1 | 2 | 3 } > 4 >";
    case procRoute_bypass:
        return "Bypass";
    }
    return "ERROR";
}

template <typename T>
std::string HasGroupZoneProcessors<T>::getProcRoutingPathShortName(ProcRoutingPath p)
{
    switch (p)
    {
    case procRoute_linear:
        return "SER1";
    case procRoute_ser2:
        return "SER2";
    case procRoute_ser3:
        return "SER3";
    case procRoute_par1:
        return "PAR1";
    case procRoute_par2:
        return "PAR2";
    case procRoute_par3:
        return "PAR3";
    case procRoute_bypass:
        return "BYP";
    }
    return "ERROR";
}

template <typename T>
void HasGroupZoneProcessors<T>::updateRoutingTableAfterProcessorSwap(size_t f, size_t t)
{
    for (auto &r : asT()->routingTable.routes)
    {
        if (r.active && r.target.has_value())
        {
            if (r.target->isProcessorTarget(f))
                r.target->setProcessorTargetTo(t);
            else if (r.target->isProcessorTarget(t))
                r.target->setProcessorTargetTo(f);
        }
    }
}
} // namespace scxt::engine

#endif