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
int32_t Engine::VoiceManagerResponder::voiceCountForInitializationAction(
    uint16_t port, uint16_t channel, uint16_t key, int32_t noteId, float velocity)
{
    // TODO: We can optimize this so we don't have to find twice in the future
    auto useKey = engine.midikeyRetuner.remapKeyTo(channel, key);
    auto nts = engine.findZone(channel, useKey, noteId, std::clamp((int)(velocity * 128), 0, 127));

    return nts.size();
}

int32_t Engine::VoiceManagerResponder::initializeMultipleVoices(
    std::array<voice::Voice *, VMConfig::maxVoiceCount> &voiceInitWorkingBuffer, uint16_t port,
    uint16_t channel, uint16_t key, int32_t noteId, float velocity, float retune)
{
    auto useKey = engine.midikeyRetuner.remapKeyTo(channel, key);
    auto nts = engine.findZone(channel, useKey, noteId, std::clamp((int)(velocity * 128), 0, 127));

    int idx{0};
    for (const auto &path : nts)
    {
        auto &z = engine.zoneByPath(path);
        auto nbSampleLoadedInZone = z->getNumSampleLoaded();
        auto initVoice = [&](int sampleIndex) {
            if (!z->samplePointers[sampleIndex])
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
            idx++;
        };

        if (nbSampleLoadedInZone == 0)
        {
            z->sampleIndex = -1;
            initVoice(z->sampleIndex);
        }
        else if (nbSampleLoadedInZone == 1)
        {
            z->sampleIndex = 0;
            initVoice(z->sampleIndex);
        }
        else
        {
            int nextAvail{0};
            switch (z->sampleData.variantPlaybackMode)
            {
            case Zone::FORWARD_RR:
                z->sampleIndex = (z->sampleIndex + 1) % nbSampleLoadedInZone;
                initVoice(z->sampleIndex);
                break;
            case Zone::TRUE_RANDOM:
                z->sampleIndex = engine.rng.unifInt(0, nbSampleLoadedInZone);
                initVoice(z->sampleIndex);
                break;
            case Zone::RANDOM_CYCLE:
                if (nbSampleLoadedInZone == 2)
                {
                    z->sampleIndex = (z->sampleIndex + 1) % 2;
                }
                else
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
                            nextAvail = (nextAvail + engine.rng.unifInt(0, z->numAvail));
                        }
                    }
                    else
                    {
                        nextAvail = z->numAvail == 1 ? 0 : engine.rng.unifInt(0, z->numAvail);
                    }
                    auto voice = z->rrs[nextAvail];              // we've used it so its a gap
                    z->rrs[nextAvail] = z->rrs[z->numAvail - 1]; // fill the gap with the end point
                    z->numAvail--; // and move the endpoint back by one
                    z->lastPlayed = voice;
                    z->sampleIndex = voice;
                    initVoice(z->sampleIndex);
                }
                break;
            case Zone::UNISON:
                for (int uv = 0; uv < nbSampleLoadedInZone; ++uv)
                {
                    z->sampleIndex = uv;
                    initVoice(z->sampleIndex);
                }
                break;
            default:
                z->sampleIndex = -1;
                initVoice(z->sampleIndex);
                break;
            }
        }
    }
    engine.midiNoteStateCounter++;
    return idx;
}

void Engine::VoiceManagerResponder::releaseVoice(voice::Voice *v, float velocity) { v->release(); }

void Engine::VoiceManagerResponder::setVoiceMIDIPitchBend(voice::Voice *v, uint16_t pb14bit)
{
    auto fv = (pb14bit - 8192) / 8192.f;
    auto part = v->zone->parentGroup->parentPart;
    part->pitchBendSmoother.setTarget(fv);
}

void Engine::VoiceManagerResponder::setMIDI1CC(voice::Voice *v, int8_t controller, int8_t val)
{
    auto fv = val / 127.0;
    auto part = v->zone->parentGroup->parentPart;
    part->midiCCSmoothers[controller].setTarget(fv);
}
} // namespace scxt::engine
