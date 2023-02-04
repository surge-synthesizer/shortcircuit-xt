//
// Created by Paul Walker on 2/4/23.
//

#ifndef SHORTCIRCUIT_JUCE_APP_H
#define SHORTCIRCUIT_JUCE_APP_H

#include <cstdint>
#include <string>
#include "engine/engine.h"

struct RtMidiIn;

namespace scxt::juce_app
{

struct PlaybackState
{
    RtMidiIn *midiIn{nullptr};
    uint64_t samplesPassed{0};
    int16_t prevKey{-1}, nextKey{60};
    bool autoPlay{false};
    scxt::engine::Engine *engine{nullptr};

    // This is a cruddy thread safe queue to get midi over
    std::atomic<int> readPoint{0}, writePoint{0};
    static constexpr int maxMsg{2048};
    uint8_t msgs[maxMsg][3];

    size_t outpos{0};
};

bool startMIDI(PlaybackState *s, const std::string &device);
bool stopMIDI(PlaybackState *s);

bool startAudioThread(PlaybackState *s, bool selfPlay);
bool stopAudioThread(PlaybackState *s);
} // namespace scxt::juce_app
#endif // SHORTCIRCUIT_APP_H
