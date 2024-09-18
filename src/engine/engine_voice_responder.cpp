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

#include "engine.h"
#include "voice/voice.h"

namespace scxt::engine
{
int32_t Engine::VoiceManagerResponder::beginVoiceCreationTransaction(uint16_t port,
                                                                     uint16_t channel, uint16_t key,
                                                                     int32_t noteId, float velocity)
{
    SCLOG_IF(voiceResponder, "begin voice transaction " << SCD(port) << SCD(channel) << SCD(key)
                                                        << SCD(noteId) << SCD(velocity));
    assert(!transactionValid);
    // TODO: We can optimize this so we don't have to find twice in the future
    auto useKey = engine.midikeyRetuner.remapKeyTo(channel, key);
    auto nts = engine.findZone(channel, useKey, noteId, std::clamp((int)(velocity * 128), 0, 127),
                               findZoneWorkingBuffer);

    auto voicesCreated{0};
    for (auto idx = 0; idx < nts; ++idx)
    {
        const auto &path = findZoneWorkingBuffer[idx];
        auto &z = engine.zoneByPath(path);
        if (z->variantData.variantPlaybackMode == Zone::UNISON)
        {
            for (int i = 0; i < maxVariantsPerZone; ++i)
            {
                if (z->variantData.variants[i].active)
                {
                    voiceCreationWorkingBuffer[voicesCreated] = {path, i};
                    SCLOG_IF(voiceResponder, "-- Created at " << voicesCreated << " - " << path.part
                                                              << "/" << path.group << "/"
                                                              << path.zone << " variant=" << i);
                    voicesCreated++;
                }
            }
        }
        else
        {
            voiceCreationWorkingBuffer[voicesCreated] = {path, -1};
            SCLOG_IF(voiceResponder, "-- Created at " << voicesCreated << " - " << path.part << "/"
                                                      << path.group << "/" << path.zone
                                                      << "  zone handles variant");

            voicesCreated++;
        }
    }
    transactionValid = true;
    transactionVoiceCount = voicesCreated;
    SCLOG_IF(voiceResponder, "beginTransaction returns " << voicesCreated << " voices");
    return voicesCreated;
}

int32_t Engine::VoiceManagerResponder::initializeMultipleVoices(
    std::array<voice::Voice *, VMConfig::maxVoiceCount> &voiceInitWorkingBuffer, uint16_t port,
    uint16_t channel, uint16_t key, int32_t noteId, float velocity, float retune)
{
    assert(transactionValid);
    assert(transactionVoiceCount > 0);
    auto useKey = engine.midikeyRetuner.remapKeyTo(channel, key);
    auto nts = transactionVoiceCount;
    SCLOG_IF(voiceResponder, "voice initiation of " << nts << " voices");
    for (auto idx = 0; idx < nts; ++idx)
    {
        const auto &[path, variantIndex] = voiceCreationWorkingBuffer[idx];
        auto &z = engine.zoneByPath(path);
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
            }
            voiceInitWorkingBuffer[idx] = v;
            SCLOG_IF(voiceResponder, "-- Created single voice for single zone");
        }
        else if (z->variantData.variantPlaybackMode == Zone::UNISON)
        {
            assert(variantIndex >= 0);
            z->sampleIndex = variantIndex;
            auto v = engine.initiateVoice(path);
            if (v)
            {
                v->velocity = velocity;
                v->originalMidiKey = key;
                v->attack();
            }
            voiceInitWorkingBuffer[idx] = v;
            SCLOG_IF(voiceResponder,
                     "-- Created one of variants for zone (" << variantIndex << ")");
        }
        else
        {
            assert(variantIndex == -1);
            SCLOG_IF(voiceResponder, "-- Launching zone and using its RR tactic");
            int nextAvail{0};
            if (nbSampleLoadedInZone == 1)
            {
                z->sampleIndex = 0;
            }
            if (nbSampleLoadedInZone == 2 &&
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
                // SCLOG( "Skipping voice with missing sample data" );
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
                }
                voiceInitWorkingBuffer[idx] = v;
            }
        }
    }
    engine.midiNoteStateCounter++;
    SCLOG_IF(voiceResponder, "Completed voice initiation");
    return nts;
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

void Engine::VoiceManagerResponder::releaseVoice(voice::Voice *v, float velocity) { v->release(); }

void Engine::VoiceManagerResponder::setVoiceMIDIPitchBend(voice::Voice *v, uint16_t pb14bit)
{
    auto fv = (pb14bit - 8192) / 8192.f;
    auto part = v->zone->parentGroup->parentPart;
    part->pitchBendValue = fv;
}

void Engine::VoiceManagerResponder::setMIDI1CC(voice::Voice *v, int8_t controller, int8_t val)
{
    assert(controller >= 0);
    auto fv = val / 127.0;
    auto part = v->zone->parentGroup->parentPart;
    part->midiCCValues[controller] = fv;
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
        v->noteExpressions[expression] = value;
    }
}

void Engine::VoiceManagerResponder::setPolyphonicAftertouch(voice::Voice *v, int8_t pat)
{
    v->polyAT = pat * 1.0 / 127.0;
}

} // namespace scxt::engine
