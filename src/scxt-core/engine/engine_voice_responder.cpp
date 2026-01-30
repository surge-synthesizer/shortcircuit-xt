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

#include "engine.h"
#include "voice/voice.h"

namespace scxt::engine
{
int32_t Engine::VoiceManagerResponder::beginVoiceCreationTransaction(
    sst::voicemanager::VoiceBeginBufferEntry<VMConfig>::buffer_t &buffer, uint16_t port,
    uint16_t channel, uint16_t key, int32_t noteId, float velocity)
{
    SCLOG_IF(voiceResponder, "begin voice transaction " << SCD(port) << SCD(channel) << SCD(key)
                                                        << SCD(noteId) << SCD(velocity));
    assert(!transactionValid);

    auto useKey = engine.midikeyRetuner.remapKeyTo(channel, key);
    auto nts = engine.findZone(channel, useKey, noteId, std::clamp((int)(velocity * 128), 0, 127),
                               findZoneWorkingBuffer);

    auto voicesCreated{0};
    for (auto idx = 0; idx < nts; ++idx)
    {
        const auto &path = findZoneWorkingBuffer[idx];
        auto &z = engine.zoneByPath(path);

        voiceCreationWorkingBuffer[voicesCreated] = {path, -1};
        if (z->parentGroup->outputInfo.hasIndependentPolyLimit ||
            z->parentGroup->outputInfo.vmPlayModeInt !=
                (uint32_t)Engine::voiceManager_t::PlayMode::POLY_VOICES)
        {
            SCLOG_IF(voiceResponder,
                     "-- Setting polyphony group to group basd " << (uint64_t)z->parentGroup);
            buffer[idx].polyphonyGroup = (uint64_t)z->parentGroup;
        }
        else if (z->parentGroup->parentPart->configuration.polyLimitVoices)
        {
            SCLOG_IF(voiceResponder,
                     "-- Setting polyphony group to part based " << z->parentGroup->parentPart)
            buffer[idx].polyphonyGroup = (uint64_t)z->parentGroup->parentPart;
        }
        else
        {
            buffer[idx].polyphonyGroup = 0;
        }
        SCLOG_IF(voiceResponder, "-- Created at " << voicesCreated << " - " << path.part << "/"
                                                  << path.group << "/" << path.zone
                                                  << "  zone handles variant");

        voicesCreated++;
    }
    transactionValid = true;
    transactionVoiceCount = voicesCreated;
    SCLOG_IF(voiceResponder, "beginTransaction returns " << voicesCreated << " voices");
    return voicesCreated;
}

int32_t Engine::VoiceManagerResponder::initializeMultipleVoices(
    int32_t voiceCount,
    const sst::voicemanager::VoiceInitInstructionsEntry<VMConfig>::buffer_t &voiceInstructionBuffer,
    sst::voicemanager::VoiceInitBufferEntry<VMConfig>::buffer_t &voiceInitWorkingBuffer,
    uint16_t port, uint16_t channel, uint16_t inkey, int32_t noteId, float velocity, float retune)
{
    assert(transactionValid);
    assert(transactionVoiceCount > 0);
    auto nts = transactionVoiceCount;
    assert(nts == voiceCount);
    SCLOG_IF(voiceResponder, "voice initiation of " << nts << " voices");
    int32_t actualCreated{0};
    int outIdx{0};
    for (auto idx = 0; idx < nts; ++idx)
    {
        if (voiceInstructionBuffer[idx].instruction ==
            sst::voicemanager::VoiceInitInstructionsEntry<
                scxt::engine::Engine::VMConfig>::Instruction::SKIP)
        {
            SCLOG_IF(voiceResponder, "Skipping voice at index " << idx);
            voiceInitWorkingBuffer[outIdx].voice = nullptr;
            outIdx++;
            continue;
        }
        const auto &[path, variantIndex] = voiceCreationWorkingBuffer[idx];
        auto &z = engine.zoneByPath(path);
        auto key = inkey + z->parentGroup->parentPart->configuration.transpose +
                   z->parentGroup->parentPart->getChannelBasedTransposition(channel);
        auto nbSampleLoadedInZone = z->getNumSampleLoaded();

        if (nbSampleLoadedInZone == 0)
        {
            z->sampleIndex = -1;
            auto v = engine.initiateVoice(path);
            if (v)
            {
                v->velocity = velocity;
                v->originalMidiKey = key;
                v->attack();
                actualCreated++;
            }
            voiceInitWorkingBuffer[outIdx].voice = v;
            outIdx++;
            SCLOG_IF(voiceResponder, "-- Created single voice for single zone ("
                                         << std::hex << v << std::dec << ")");
        }
        else
        {
            assert(variantIndex == -1);
            SCLOG_IF(voiceResponder, "-- Launching zone and using its RR tactic");
            int nextAvail{0};
            if (nbSampleLoadedInZone == 1 || z->variantData.variantPlaybackMode == Zone::UNISON)
            {
                z->sampleIndex = 0;
            }
            else if (nbSampleLoadedInZone == 2 &&
                     z->variantData.variantPlaybackMode != Zone::TRUE_RANDOM)
            {
                z->sampleIndex = (z->sampleIndex + 1) % 2;
            }
            else
            {
                if (z->variantData.variantPlaybackMode == Zone::FORWARD_RR)
                {
                    z->sampleIndex = (z->sampleIndex + 1) % nbSampleLoadedInZone;
                }
                else if (z->variantData.variantPlaybackMode == Zone::TRUE_RANDOM)
                {
                    z->sampleIndex = engine.rng.unifInt(0, nbSampleLoadedInZone);
                }
                else if (z->variantData.variantPlaybackMode == Zone::RANDOM_NOREPEAT)
                {
                    auto previdx = z->sampleIndex;
                    auto newidx = previdx;
                    while (newidx == previdx)
                        newidx = engine.rng.unifInt(0, nbSampleLoadedInZone);
                    z->sampleIndex = newidx;
                }
                else if (z->variantData.variantPlaybackMode == Zone::RANDOM_CYCLE)
                {
                    if (z->numAvail == 0 || z->setupFor != nbSampleLoadedInZone)
                    {
                        for (auto i = 0; i < nbSampleLoadedInZone; ++i)
                        {
                            z->rrs[i] = i;
                        }
                        z->numAvail = nbSampleLoadedInZone;
                        z->setupFor = nbSampleLoadedInZone;

                        nextAvail = engine.rng.unifInt(0, z->numAvail);
                        if (z->rrs[nextAvail] == z->lastPlayed)
                        {
                            // the -1 here makes sure we don't re-reach ourselves
                            nextAvail = (nextAvail + (engine.rng.unifInt(0, z->numAvail - 1))) %
                                        z->numAvail;
                        }
                    }
                    else
                    {
                        nextAvail = z->numAvail == 1 ? 0 : (engine.rng.unifInt(0, z->numAvail));
                    }
                    auto voice = z->rrs[nextAvail];              // we've used it so its a gap
                    z->rrs[nextAvail] = z->rrs[z->numAvail - 1]; // fill the gap with the end point
                    z->numAvail--; // and move the endpoint back by one
                    z->lastPlayed = voice;
                    z->sampleIndex = voice;
                }
            }

            if (!z->samplePointers[z->sampleIndex])
            {
                // Empty samples don't start a voice here
            }
            else
            {
                auto v = engine.initiateVoice(path);
                if (v)
                {
                    v->velocity = velocity;
                    v->velKeyFade = z->mapping.keyboardRange.fadeAmpltiudeAt(key);
                    v->velKeyFade *= z->mapping.velocityRange.fadeAmpltiudeAt(
                        (int16_t)std::clamp(velocity * 127.0, 0., 127.));

                    v->originalMidiKey = key;
                    v->attack();
                    actualCreated++;
                }
                voiceInitWorkingBuffer[outIdx].voice = v;
                outIdx++;
            }
        }
    }
    engine.midiNoteStateCounter++;
    SCLOG_IF(voiceResponder, "Completed voice initiation " << actualCreated << " of " << nts);
    return actualCreated;
}

void Engine::VoiceManagerResponder::endVoiceCreationTransaction(uint16_t port, uint16_t channel,
                                                                uint16_t key, int32_t noteId,
                                                                float velocity)
{
    SCLOG_IF(voiceResponder, "end voice transaction");
    assert(transactionValid);
    transactionValid = false;
    transactionVoiceCount = 0;
}

void Engine::VoiceManagerResponder::releaseVoice(voice::Voice *v, float velocity)
{
    v->releaseVelocity = velocity;
    v->release();
}

void Engine::VoiceManagerResponder::setNoteExpression(voice::Voice *v, int32_t expression,
                                                      double value)
{
    if (expression >= 0 && expression < voice::Voice::noteExpressionCount)
    {
        // bitwig shows timbre/brightness as bipolar. Correctly?
        if (expression == (int)voice::Voice::ExpressionIDs::BRIGHTNESS)
        {
            value = value * 2 - 1;
        }
        v->noteExpressionLags.setTarget(expression, value, &v->noteExpressions[expression]);
    }
}

void Engine::VoiceManagerResponder::setPolyphonicAftertouch(voice::Voice *v, int8_t pat)
{
    v->polyAT = pat * 1.0 / 127.0;
}

void Engine::VoiceManagerResponder::terminateVoice(voice::Voice *v)
{
    SCLOG_IF(voiceResponder, "terminateVoice " << v << " " << (int)v->key);

    if (!v->isVoicePlaying)
        return;
    if (v->isGated)
        v->release();
    v->beginTerminationSequence();
}

void Engine::VoiceManagerResponder::setVoiceMIDIMPEChannelPitchBend(voice::Voice *v,
                                                                    uint16_t pb14bit)
{
    assert(v);
    assert(v->zone->parentGroup->parentPart);
    assert(v->isVoiceAssigned);
    v->mpePitchBend = 1.f * (pb14bit - 8192) / 8192 *
                      v->zone->parentGroup->parentPart->configuration.mpePitchBendRange;
}

void Engine::VoiceManagerResponder::setVoiceMIDIMPEChannelPressure(voice::Voice *v, int8_t val)
{
    assert(v);
    assert(v->zone->parentGroup->parentPart);
    assert(v->isVoiceAssigned);
    v->mpePressure = val / 127.0;
}

void Engine::VoiceManagerResponder::setVoiceMIDIMPETimbre(voice::Voice *v, int8_t val)
{
    assert(v);
    assert(v->zone->parentGroup->parentPart);
    assert(v->isVoiceAssigned);
    v->mpeTimbre = val / 127.0;
}

void Engine::VoiceManagerResponder::moveVoice(VMConfig::voice_t *v, uint16_t port, uint16_t channel,
                                              uint16_t key, float vel)
{
    auto dkey = v->key - v->originalMidiKey;
    v->key = key;
    v->originalMidiKey = key - dkey;
    v->keyChangedInLegatoModeTrigger = 1.f;
    v->calculateVoicePitch();
}

void Engine::VoiceManagerResponder::moveAndRetriggerVoice(VMConfig::voice_t *v, uint16_t port,
                                                          uint16_t channel, uint16_t key, float vel)
{
    auto dkey = v->key - v->originalMidiKey;
    v->key = key;
    v->originalMidiKey = key - dkey;
    v->calculateVoicePitch();
    v->setIsGated(true);
    for (auto &eg : v->eg)
    {
        eg.attackFrom(eg.outBlock0);
    }
    for (auto &eg : v->egOS)
    {
        eg.attackFrom(eg.outBlock0);
    }
}

void Engine::MonoVoiceManagerResponder::setMIDIPitchBend(int16_t channel, int16_t pb14bit)
{
    auto fv = (pb14bit - 8192) / 8192.f;
    for (const auto &p : engine.getPatch()->getParts())
    {
        if (p->configuration.active && p->respondsToMIDIChannel(channel))
        {
            p->externalSignalLag.setTarget(Part::LagIndexes::pitchBend, fv, &p->pitchBendValue);
        }
    }
}
void Engine::MonoVoiceManagerResponder::setMIDI1CC(int16_t channel, int16_t cc, int8_t val)
{
    auto fv = val / 127.0;

    for (const auto &p : engine.getPatch()->getParts())
    {
        if (p->configuration.active && p->respondsToMIDIChannel(channel))
        {
            p->midiCCValues[cc] = fv;
        }
    }
}
void Engine::MonoVoiceManagerResponder::setMIDIChannelPressure(int16_t channel, int16_t pres)
{
    auto fv = pres / 127.0;

    for (const auto &p : engine.getPatch()->getParts())
    {
        if (p->configuration.active && p->respondsToMIDIChannel(channel))
        {
            p->channelAT = fv;
        }
    }
}

} // namespace scxt::engine
