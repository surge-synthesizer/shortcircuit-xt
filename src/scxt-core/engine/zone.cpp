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

#include "zone.h"
#include "bus.h"
#include "group.h"
#include "modulation/modulators/steplfo.h"
#include "part.h"
#include "engine.h"
#include "messaging/messaging.h"
#include "voice/voice.h"

#include "sst/basic-blocks/mechanics/block-ops.h"
#include "group_and_zone_impl.h"

#include "dsp/sample_analytics.h"

namespace scxt::engine
{
void Zone::process(Engine &e)
{
    if (parentGroup->outputInfo.oversample)
    {
        processWithOS<true>(e);
    }
    else
    {
        processWithOS<false>(e);
    }
}

template <bool OS> void Zone::processWithOS(scxt::engine::Engine &onto)
{
    if (terminateOnNextProcess)
    {
        terminateOnNextProcess = false;
        terminateAllVoices();
        return;
    }
    constexpr size_t osBlock{blockSize << (OS ? 1 : 0)};
    namespace blk = sst::basic_blocks::mechanics;
    // TODO these memsets are probably gratuitous
    memset(output, 0, sizeof(output));

    mUILag.process();

    std::array<voice::Voice *, maxVoices> toCleanUp;
    size_t cleanupIdx{0};
    gatedVoiceCount = 0;
    for (auto &v : voiceWeakPointers)
    {
        if (v && v->isVoiceAssigned)
        {
            if (v->process())
            {
                if (outputInfo.routeTo == DEFAULT_BUS)
                {
                    blk::accumulate_from_to<osBlock>(v->output[0], output[0]);
                    blk::accumulate_from_to<osBlock>(v->output[1], output[1]);
                }
                else if (outputInfo.routeTo >= 0)
                {
                    auto &tb = getEngine()->getPatch()->getBusForOutput(outputInfo.routeTo);
                    blk::accumulate_from_to<osBlock>(v->output[0],
                                                     OS ? tb.outputOS[0] : tb.output[0]);
                    blk::accumulate_from_to<osBlock>(v->output[1],
                                                     OS ? tb.outputOS[1] : tb.output[1]);
                    if constexpr (OS)
                    {
                        tb.hasOSSignal = true;
                    }
                }
            }
            if (!v->isVoicePlaying)
            {
                toCleanUp[cleanupIdx++] = v;
            }

            if (v->isGated)
            {
                gatedVoiceCount++;
            }
        }
    }

    for (int i = 0; i < cleanupIdx; ++i)
    {
        SCLOG_IF(voiceLifecycle, "Cleanup Voice at " << SCD((int)toCleanUp[i]->key));
        toCleanUp[i]->cleanupVoice();
    }
}

void Zone::addVoice(voice::Voice *v)
{
    if (activeVoices == 0)
    {
        parentGroup->addActiveZone(this);
    }

    activeVoices++;
    for (auto &nv : voiceWeakPointers)
    {
        if (nv == nullptr)
        {
            nv = v;
            return;
        }
    }

    assert(false);
}
void Zone::removeVoice(voice::Voice *v)
{
    for (auto &nv : voiceWeakPointers)
    {
        if (nv == v)
        {
            nv = nullptr;
            activeVoices--;
            if (activeVoices == 0)
            {
                mUILag.instantlySnap();
                parentGroup->removeActiveZone(this);
            }
            return;
        }
    }
    assert(false);
}

void Zone::setNormalizedSampleLevel(const bool usePeak, const int associatedSampleID)
{
    const auto startSample = (associatedSampleID < 0) ? 0 : associatedSampleID;
    const auto endSample = (associatedSampleID < 0) ? maxVariantsPerZone : associatedSampleID + 1;

    for (auto i = startSample; i < endSample; ++i)
    {
        if (variantData.variants[i].active && samplePointers[i])
        {
            auto normVal = usePeak ? dsp::sample_analytics::computePeak(samplePointers[i])
                                   : dsp::sample_analytics::computeMaxRMSInBlock(samplePointers[i]);
            // store this linear for better cpu
            variantData.variants[i].normalizationAmplitude = 1.f / normVal;
        }
    }
}

void Zone::clearNormalizedSampleLevel(const int associatedSampleID)
{
    const auto startSample = (associatedSampleID < 0) ? 0 : associatedSampleID;
    const auto endSample = (associatedSampleID < 0) ? maxVariantsPerZone : associatedSampleID + 1;

    for (auto i = startSample; i < endSample; ++i)
    {
        if (variantData.variants[i].active && samplePointers[i])
        {
            variantData.variants[i].normalizationAmplitude = 1.f;
        }
    }
}

void Zone::initialize()
{
    for (auto &v : voiceWeakPointers)
        v = nullptr;
}

void Zone::setupOnUnstream(const engine::Engine &e)
{
    auto nbSampleLoaded{getNumSampleLoaded()};
    for (auto i = 0; i < nbSampleLoaded; ++i)
    {
        auto oid = variantData.variants[i].sampleID;
        auto nid = e.getSampleManager()->resolveAlias(oid);

        if (nid != oid)
        {
            SCLOG_IF(sampleLoadAndPurge, getName() << " : Relabeling zone on change : "
                                                   << oid.to_string() << " -> " << nid.to_string());
            variantData.variants[i].sampleID = nid;
        }

        attachToSample(*(e.getSampleManager()), i, Zone::NONE);
    }
    for (int p = 0; p < processorCount; ++p)
    {
        setupProcessorControlDescriptions(p, processorStorage[p].type);
    }
}

bool Zone::attachToSample(const sample::SampleManager &manager, int index, int sir)
{
    auto &s = variantData.variants[index];
    if (s.sampleID.isValid())
    {
        samplePointers[index] = manager.getSample(s.sampleID);
    }
    else
    {
        samplePointers[index].reset();
    }

    if (sir & MAPPING)
    {
        if (samplePointers[index] && index == 0)
        {
            const auto &m = samplePointers[index]->meta;

            if (m.rootkey_present)
                mapping.rootKey = m.key_root;
            if (m.key_present)
            {
                mapping.keyboardRange = {m.key_low, m.key_high};
            }
            if (m.vel_present)
            {
                mapping.velocityRange = {m.vel_low, m.vel_high};
            }
        }
    }
    if (sir & (LOOP | ENDPOINTS))
    {
        if (samplePointers[index])
        {
            const auto &m = samplePointers[index]->meta;
            s.startSample = 0;
            s.endSample = samplePointers[index]->getSampleLength();
            if (sir & LOOP)
            {
                if (m.loop_present)
                {
                    s.startLoop = m.loop_start;
                    s.endLoop = m.loop_end;
                    s.loopActive = true;
                }
                else
                {
                    s.startLoop = 0;
                    s.endLoop = s.endSample;
                }
            }
        }
    }
    return samplePointers[index] != nullptr;
}

std::string Zone::toStringVariantPlaybackMode(const Zone::VariantPlaybackMode &p)
{
    switch (p)
    {
    case FORWARD_RR:
        return "frr";
    case TRUE_RANDOM:
        return "truerand";
    case RANDOM_CYCLE:
        return "randcyc";
    case RANDOM_NOREPEAT:
        return "truenorep";
    case UNISON:
        return "unison";
    }
    return "frr";
}

Zone::VariantPlaybackMode Zone::fromStringVariantPlaybackMode(const std::string &s)
{
    static auto inverse =
        makeEnumInverse<Zone::VariantPlaybackMode, Zone::toStringVariantPlaybackMode>(
            Zone::VariantPlaybackMode::FORWARD_RR, Zone::VariantPlaybackMode::UNISON);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return FORWARD_RR;
    return p->second;
}

std::string Zone::toStringPlayMode(const PlayMode &p)
{
    switch (p)
    {
    case NORMAL:
        return "normal";
    case ONE_SHOT:
        return "oneshot";
    case ON_RELEASE:
        return "onrelease";
    }
    return "normal";
}

Zone::PlayMode Zone::fromStringPlayMode(const std::string &s)
{
    static auto inverse = makeEnumInverse<Zone::PlayMode, Zone::toStringPlayMode>(
        Zone::PlayMode::NORMAL, Zone::PlayMode::ON_RELEASE);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return NORMAL;
    return p->second;
}

std::string Zone::toStringLoopMode(const LoopMode &p)
{
    switch (p)
    {
    case LOOP_DURING_VOICE:
        return "during_voice";
    case LOOP_WHILE_GATED:
        return "while_gated";
    case LOOP_COUNT:
        return "for_count";
    }
    return "during_voice";
}

Zone::LoopMode Zone::fromStringLoopMode(const std::string &s)
{
    static auto inverse = makeEnumInverse<Zone::LoopMode, Zone::toStringLoopMode>(
        Zone::LoopMode::LOOP_DURING_VOICE, Zone::LoopMode::LOOP_COUNT);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return LOOP_DURING_VOICE;
    return p->second;
}

std::string Zone::toStringLoopDirection(const LoopDirection &p)
{
    switch (p)
    {
    case FORWARD_ONLY:
        return "forward";
    case ALTERNATE_DIRECTIONS:
        return "alternate";
    }
    return "forward";
}

Zone::LoopDirection Zone::fromStringLoopDirection(const std::string &s)
{
    static auto inverse = makeEnumInverse<Zone::LoopDirection, Zone::toStringLoopDirection>(
        Zone::LoopDirection::FORWARD_ONLY, Zone::LoopDirection::ALTERNATE_DIRECTIONS);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return FORWARD_ONLY;
    return p->second;
}

void Zone::onSampleRateChanged() { mUILag.setRate(120, blockSize, sampleRate); }

void Zone::terminateAllVoices()
{
    std::array<voice::Voice *, maxVoices> toCleanUp{};
    size_t cleanupIdx{0};
    for (auto &v : voiceWeakPointers)
    {
        if (v && v->isVoiceAssigned)
        {
            toCleanUp[cleanupIdx++] = v;
        }
    }

    if constexpr (scxt::log::voiceLifecycle)
    {
        if (cleanupIdx)
        {
            SCLOG_IF(voiceLifecycle, "Early-terminating " << cleanupIdx << " voices");
        }
    }

    for (int i = 0; i < cleanupIdx; ++i)
    {
        toCleanUp[i]->cleanupVoice();
    }
}
void Zone::onRoutingChanged()
{
    voice::modulation::MatrixEndpoints::Sources usedForScanning(nullptr);
    std::fill(lfosActive.begin(), lfosActive.end(), false);
    auto pglfo = glfosActive;
    std::fill(glfosActive.begin(), glfosActive.end(), false);

    for (int i = 0; i < egsPerZone; ++i)
        egsActive[i] = false;
    egsActive[0] = true; // the AEG always runs
    phasorsActive = false;
    auto doCheck = [this, &usedForScanning](const voice::modulation::MatrixEndpoints::SR &src) {
        for (int i = 0; i < lfosPerZone; ++i)
        {
            if (src == usedForScanning.lfoSources.sources[i])
            {
                lfosActive[i] = true;
            }
        }

        for (int i = 0; i < lfosPerGroup; ++i)
        {
            if (src == usedForScanning.glfoSources.sources[i])
            {
                glfosActive[i] = true;
            }
        }

        for (int i = 0; i < egsPerZone; ++i)
        {
            if (src == usedForScanning.egSources[i])
            {
                egsActive[i] = true;
            }
        }

        for (int i = 0; i < phasorsPerGroupOrZone; ++i)
        {
            if (src == usedForScanning.transportSources.phasors[i])
            {
                phasorsActive = true;
            }
        }
    };
    for (auto &r : routingTable.routes)
    {
        // If we aren't mapped anywhere we don't care about this row
        if (!r.target.has_value())
            continue;

        // Similarly if we aren't active
        if (!r.active)
            continue;

        if (r.source.has_value())
            doCheck(*r.source);

        if (r.sourceVia.has_value())
            doCheck(*r.sourceVia);
    }

    if (pglfo != glfosActive)
    {
        parentGroup->rePrepareAndBindGroupMatrix();
    }
}

int16_t Zone::missingSampleCount() const
{
    int idx{0};
    int ct{0};
    for (auto &sv : variantData.variants)
    {
        if (sv.active)
        {
            auto smp = samplePointers[idx];

            if (smp && smp->isMissingPlaceholder)
            {
                ct++;
            }
        }
        idx++;
    }
    return ct;
}

void Zone::deleteVariant(int idx)
{
    assert(idx >= 0 && idx < maxVariantsPerZone);
    terminateOnNextProcess = true;

    for (int nv = idx + 1; nv < maxVariantsPerZone; ++nv)
    {
        variantData.variants[nv - 1] = variantData.variants[nv];
        samplePointers[nv - 1] = samplePointers[nv];
    }
    if (idx < maxVariantsPerZone - 1)
    {
        variantData.variants[maxVariantsPerZone - 1] = {};
        samplePointers[maxVariantsPerZone - 1] = {};
    }
}

template struct HasGroupZoneProcessors<Zone>;
} // namespace scxt::engine
