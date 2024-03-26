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
        const auto &z = engine.zoneByPath(path);
        auto nbSampleLoadedInZone = z->getNumSampleLoaded();
        auto remainingOptions = nbSampleLoadedInZone - z->usedVariants;
        int countToN = -1; // for finding the nth true bool in the variantRng state array

        if (nbSampleLoadedInZone == 1) // if there's only one variant
        {
            z->sampleIndex = 0; // you guessed it: just pick that one
        }
        else if (nbSampleLoadedInZone == 2) // if there's two, round-robin == non-repeat random
        {
            z->sampleIndex = (z->sampleIndex + 1) % 2;
            // but true random != those two, so...
            // TODO: if variant mode is true random do:
            // z->sampleIndex = (z->sampleIndex + std::rand()) % 2;
        }
        else
        {
            // TODO: if variant mode is round-robin do:
            // z->sampleIndex = (z->sampleIndex + 1) % nbSampleLoadedInZone;
            // if it's true random do:
            // z->sampleIndex = (z->sampleIndex + std::rand()) % nbSampleLoadedInZone;
            // for non-repeating random use the following:
            if (remainingOptions == 1) // if only one variant remains
            {
                for (int n = 0; n < nbSampleLoadedInZone;
                     ++n) // its index == that of last false flag here
                {
                    if (z->variantRngBits[n] == false) // so find it
                    {
                        z->sampleIndex = n;          // pick it to be played
                        for (int f = 0; f < 16; ++f) // and reset the used flags
                        {
                            if (f == n)
                                z->variantRngBits[f] = true; // by setting this last one to used
                            else
                                z->variantRngBits[f] = false; // and all the others to unused
                        }
                        break;
                    }
                }
                z->usedVariants = 1; // consequently this should = 1
            }
            else // ok so what if more than one remain?
            {
                int dieRoll = std::rand() % remainingOptions;  // roll a random number in range
                for (int n = 0; n < nbSampleLoadedInZone; ++n) // loop the range of variants
                {
                    if (z->variantRngBits[n] == false) // on encountering an unused one:
                    {
                        countToN = countToN + 1; // increment this counter
                        if (countToN == dieRoll) // the random number of times
                        {
                            z->sampleIndex = n;                    // that's our candidate, pick it!
                            z->variantRngBits[n] = true;           // set its flag to used
                            z->usedVariants = z->usedVariants + 1; // and increment its counter
                            break;
                        }
                    }
                }
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
                v->originalMidiKey = key;
                v->attack();
            }
            voiceInitWorkingBuffer[idx] = v;
            idx++;
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
