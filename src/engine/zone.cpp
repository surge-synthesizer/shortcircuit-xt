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
                    auto &bs = getEngine()->getPatch()->busses;
                    auto &tb = bs.busByAddress(outputInfo.routeTo);
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
#if DEBUG_VOICE_LIFECYCLE
        SCLOG("Cleanup Voice at " << SCDBGV((int)toCleanUp[i]->key));
#endif
        toCleanUp[i]->cleanupVoice();
    }

    for (int i = 0; i < osBlock; i += 4)
    {
        // SCLOG(i << " " << output[0][i] << " " << output[0][i + 1] << " " << output[0][i + 2] << "
        // "
        //         << output[0][i + 3])
    }
}

void Zone::addVoice(voice::Voice *v)
{
    if (activeVoices == 0)
    {
        parentGroup->addActiveZone();
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
                parentGroup->removeActiveZone();
            }
            return;
        }
    }
    assert(false);
}

engine::Engine *Zone::getEngine()
{
    if (parentGroup && parentGroup->parentPart && parentGroup->parentPart->parentPatch)
        return parentGroup->parentPart->parentPatch->parentEngine;
    return nullptr;
}

const engine::Engine *Zone::getEngine() const
{
    if (parentGroup && parentGroup->parentPart && parentGroup->parentPart->parentPatch)
        return parentGroup->parentPart->parentPatch->parentEngine;
    return nullptr;
}

void Zone::initialize()
{
    for (auto &v : voiceWeakPointers)
        v = nullptr;

    for (auto &l : modulatorStorage)
    {
        modulation::modulators::clear_lfo(l);
    }
}

void Zone::setupOnUnstream(const engine::Engine &e)
{
    auto nbSampleLoaded{getNumSampleLoaded()};
    for (auto i = 0; i < nbSampleLoaded; ++i)
    {
        attachToSample(*(e.getSampleManager()), i, Zone::NONE);
    }
    for (int p = 0; p < processorCount; ++p)
    {
        setupProcessorControlDescriptions(p, processorStorage[p].type);
    }
}

bool Zone::attachToSample(const sample::SampleManager &manager, int index,
                          SampleInformationRead sir)
{
    auto &s = sampleData.samples[index];
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
    case LOOP_FOR_COUNT:
        return "for_count";
    }
    return "during_voice";
}

Zone::LoopMode Zone::fromStringLoopMode(const std::string &s)
{
    static auto inverse = makeEnumInverse<Zone::LoopMode, Zone::toStringLoopMode>(
        Zone::LoopMode::LOOP_DURING_VOICE, Zone::LoopMode::LOOP_FOR_COUNT);
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
    if (cleanupIdx)
    {
        SCLOG("Early-terminating " << cleanupIdx << " voices");
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
    egsActive[0] = true; // the AEG always runs
    egsActive[1] = false;
    auto doCheck = [this, &usedForScanning](const voice::modulation::MatrixEndpoints::SR &src) {
        for (int i = 0; i < lfosPerZone; ++i)
        {
            if (src == usedForScanning.lfoSources.sources[i])
            {
                lfosActive[i] = true;
            }
        }
        if (src == usedForScanning.aegSource)
        {
            egsActive[0] = true;
        }

        if (src == usedForScanning.eg2Source)
        {
            egsActive[1] = true;
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
    // SCLOG("Post check: " << SCD(lfosActive[0]) << SCD(lfosActive[1]) << SCD(lfosActive[2])
    //                      << SCD(lfosActive[3]) << SCD(egsActive[0]) << SCD(egsActive[1]))
}

template struct HasGroupZoneProcessors<Zone>;
} // namespace scxt::engine
