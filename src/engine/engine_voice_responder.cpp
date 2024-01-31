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

        if (!z->samplePointers[0])
        {
            // SCLOG( "Skipping voice with missing sample data" );
        }
        else
        {
            auto v = engine.initiateVoice(path);
            if (v)
            {
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