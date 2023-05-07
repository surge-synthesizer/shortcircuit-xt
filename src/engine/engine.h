/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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
#ifndef SCXT_SRC_ENGINE_ENGINE_H
#define SCXT_SRC_ENGINE_ENGINE_H

#include "engine/bus.h"
#include "utils.h"
#include "part.h"
#include "group.h"
#include "zone.h"
#include "patch.h"

#include "configuration.h"

#include "sample/sample.h"
#include "sample/sample_manager.h"

#include <filesystem>
#include <memory>
#include <set>
#include <cassert>
#include <thread>

#include "selection/selection_manager.h"
#include "memory_pool.h"
#include "tuning/midikey_retuner.h"

#define DEBUG_VOICE_LIFECYCLE 0

namespace scxt::voice
{
struct Voice;
}
namespace scxt::messaging
{
struct MessageController;
}
namespace scxt::infrastructure
{
struct DefaultsProvider;
}
namespace scxt::engine
{
struct Engine : MoveableOnly<Engine>, SampleRateSupport
{
    Engine();
    ~Engine();

    EngineID id;

    struct Busses
    {
        Busses()
        {
            std::fill(partToVSTRouting.begin(), partToVSTRouting.end(), 0);
            std::fill(auxToVSTRouting.begin(), auxToVSTRouting.end(), 0);
        }
        Bus mainBus;

        std::array<Bus, numParts> partBusses;
        // here '0' means main bus
        std::array<int16_t, numParts> partToVSTRouting{};

        std::array<Bus, numAux> auxBusses;
        std::array<int16_t, numParts> auxToVSTRouting{};
    } busses;

    const std::unique_ptr<Patch> &getPatch() const { return patch; }
    const std::unique_ptr<sample::SampleManager> &getSampleManager() const { return sampleManager; }

    std::unique_ptr<infrastructure::DefaultsProvider> defaults;

    /**
     * An option to add a description of your running environment, like "VST3 in Reaper"
     */
    std::string runningEnvironment{"Unknown"};

    /**
     * Audio processing
     */
    int getStereoOutputs() const { return 1; }
    void setStereoOutputs(int s) { assert(s == 1); }

    /**
     * Process into an array of size stereoOutputs * 2 * blocksize.
     * Returns true if processing should continue
     */
    bool processAudio();

    /**
     * Midi-style events. Each event is assumed to be at the top of the
     * blockSize sample block
     */

    // TODO: This is obviusly a pretty inefficient search method. We can definitel
    // pre-cache some of this lookup when the patch mutates
    struct pathToZone_t
    {
        size_t part{0};
        size_t group{0};
        size_t zone{0};

        int16_t channel{-1};
        int16_t key{-1};
        int32_t noteid{-1};
    };
    std::vector<pathToZone_t> findZone(int16_t channel, int16_t key, int32_t noteId,
                                       int16_t velocity)
    {
        // TODO: This allocates which is a bummer
        std::vector<pathToZone_t> res;
        for (const auto &[pidx, part] : sst::cpputils::enumerate(*patch))
        {
            if (part->channel == channel || part->channel == Part::omniChannel)
            {
                for (const auto &[gidx, group] : sst::cpputils::enumerate(*part))
                {
                    for (const auto &[zidx, zone] : sst::cpputils::enumerate(*group))
                    {
                        if (zone->mapping.keyboardRange.includes(key) &&
                            zone->mapping.velocityRange.includes(velocity))
                        {
                            res.push_back(
                                {(size_t)pidx, (size_t)gidx, (size_t)zidx, channel, key, noteId});
                        }
                    }
                }
            }
        }
        return res;
    }

    tuning::MidikeyRetuner midikeyRetuner;

    void noteOn(int16_t channel, int16_t key, int32_t noteId, int32_t velocity, float detune);
    void noteOff(int16_t channel, int16_t key, int32_t noteId, int32_t velocity);

    void pitchBend(int16_t channel, int16_t value);
    void midiCC(int16_t channel, int16_t controller, int16_t value);
    void channelAftertouch(int16_t channel, int16_t value);
    void polyAftertouch(int16_t channel, int16_t noteNumber, int16_t value);

    void onSampleRateChanged() override;

    const std::unique_ptr<Zone> &zoneByPath(const pathToZone_t &path) const
    {
        const auto &[p, g, z, c, k, n] = path;
        return patch->getPart(p)->getGroup(g)->getZone(z);
    }
    voice::Voice *initiateVoice(const pathToZone_t &path);
    void releaseVoice(int16_t channel, int16_t key, int32_t noteid, int32_t releaseVelocity);

    // TODO: All this gets ripped out when voice management is fixed
    uint32_t activeVoiceCount();

    const std::unique_ptr<messaging::MessageController> &getMessageController() const
    {
        return messageController;
    }
    const std::unique_ptr<selection::SelectionManager> &getSelectionManager() const
    {
        return selectionManager;
    }

    /**
     * Prepare to play sets up the internal state to prepare us to play. Call it in
     * your interfaces pre-playing once.
     */
    void prepareToPlay(double sampleRate)
    {
        setSampleRate(sampleRate);
        voiceDisplayStateReadCounter = 0;
        voiceDisplayStateWriteCounter = 0;

        const double updateFrequencyHz{15.0};

        updateVoiceDisplayStateEvery = (int)std::floor(sampleRate / updateFrequencyHz / blockSize);
        lastUpdateVoiceDisplayState = updateVoiceDisplayStateEvery;
    }

    /**
     * This is a mutex which we lock when modifying the structure in the engine.
     * The structure will only be modified in one of two situations
     * 1. On the audio thread (wav load for instance)
     * 2. On the serialization thread after the audio thread has been paused
     *    (which is how we do SFZ)
     *
     * As a result this mutex needs to be locked when serialization reads the structure
     * or when engine changes it but not when engine traverses it so note on and the
     * like can avoid a mutex lock.
     */
    std::mutex modifyStructureMutex;

    /**
     * The VoiceDisplayState structure is updated at some relatively low (like 30hz)
     * frequency by the engine for communication to a potential client if the message
     * controller has a client connected. Updates will occur only if the read pointer and
     * write pointer are the same so the client needs to update the write pointer.
     */
    std::atomic<int64_t> voiceDisplayStateReadCounter{0}, voiceDisplayStateWriteCounter{0};
    int32_t updateVoiceDisplayStateEvery{10000000};
    int32_t lastUpdateVoiceDisplayState{0};
    int64_t midiNoteStateCounter{0}, lastMidiNoteStateCounter{0};
    bool sendSamplePosition{false}; // right now turning this on sends too many messages
    struct VoiceDisplayStateItem
    {
        bool active{false};
        pathToZone_t zonePath{};
        int64_t samplePos{};
        int16_t midiNote{-1};
        int16_t midiChannel{-1};
        bool gated{false};
    };
    // Mark this with a type so we can do more efficient streaming
    struct VoiceDisplayState
    {
        int32_t voiceCount;
        std::array<VoiceDisplayStateItem, maxVoices> items;
    } voiceDisplayState;

    /*
     * Serialization thread originated mutation apis
     */
    void loadSampleIntoSelectedPartAndGroup(const fs::path &);

    void loadSf2MultiSampleIntoSelectedPart(const fs::path &);

    /*
     * OnRegister generate and send all the metdata the client needs
     */
    void sendMetadataToClient() const;

    /*
     * Update the audio playing state
     */
    void sendEngineStatusToClient() const;

    struct EngineStatusMessage
    {
        bool isAudioRunning;
        double sampleRate;
        std::string runningEnvironment;
    };

    /*
     * Data Query APIs
     */
    typedef std::tuple<int32_t, int32_t, int32_t, int32_t> processorAddress_t;
    const std::optional<dsp::processor::ProcessorStorage>
    getProcessorStorage(const processorAddress_t &addr) const;

    typedef std::vector<std::pair<selection::SelectionManager::ZoneAddress, std::string>>
        pgzStructure_t;
    /**
     * Get the Part/Group/Zone structure as a set o fzone addreses. A part with
     * no groups will be (p,-1,-1); a group with no zones will be (p,g,-1).
     *
     * @param partFilter -1 for all parts; 0-16 for a specific part
     * @return The vector of zones matching the filter in the running engine
     * independent of selection.
     */
    pgzStructure_t getPartGroupZoneStructure(int partFilter) const;

    const std::unique_ptr<MemoryPool> &getMemoryPool() { return memoryPool; }

    std::atomic<int32_t> stopEngineRequests{0};

  private:
    std::unique_ptr<Patch> patch;
    std::unique_ptr<MemoryPool> memoryPool;
    std::unique_ptr<sample::SampleManager> sampleManager;
    std::array<voice::Voice *, maxVoices> voices;
    std::unique_ptr<uint8_t[]> voiceInPlaceBuffer{nullptr};
    std::unique_ptr<messaging::MessageController> messageController;
    std::unique_ptr<selection::SelectionManager> selectionManager;
};
} // namespace scxt::engine
#endif