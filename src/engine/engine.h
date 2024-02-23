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
#ifndef SCXT_SRC_ENGINE_ENGINE_H
#define SCXT_SRC_ENGINE_ENGINE_H

#include "sst/voicemanager/voicemanager.h"

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
#include "infrastructure/rng_gen.h"

#include "modulation/voice_matrix.h"

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
namespace scxt::browser
{
struct Browser;
struct BrowserDB;
} // namespace scxt::browser

namespace scxt::engine
{
struct Engine : MoveableOnly<Engine>, SampleRateSupport
{
    Engine();
    ~Engine();

    EngineID id;

    const std::unique_ptr<Patch> &getPatch() const { return patch; }
    const std::unique_ptr<sample::SampleManager> &getSampleManager() const { return sampleManager; }
    const std::unique_ptr<browser::Browser> &getBrowser() const { return browser; }

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

    // new voice manager style
    struct VMConfig
    {
        static constexpr int maxVoiceCount{maxVoices};
        using voice_t = voice::Voice;
    };
    struct VoiceManagerResponder
    {
        Engine &engine;
        VoiceManagerResponder(Engine &e) : engine(e) {}

        std::function<void(voice::Voice *)> voiceEndCallback{nullptr};
        void setVoiceEndCallback(std::function<void(voice::Voice *)> vec)
        {
            voiceEndCallback = vec;
        }
        void doVoiceEndCallback(voice::Voice *v)
        {
            if (voiceEndCallback)
            {
                voiceEndCallback(v);
            }
        }

        int32_t voiceCountForInitializationAction(uint16_t port, uint16_t channel, uint16_t key,
                                                  int32_t noteId, float velocity);

        int32_t initializeMultipleVoices(
            std::array<voice::Voice *, VMConfig::maxVoiceCount> &voiceInitWorkingBuffer,
            uint16_t port, uint16_t channel, uint16_t key, int32_t noteId, float velocity,
            float retune);

        void releaseVoice(voice::Voice *v, float velocity);
        void retriggerVoiceWithNewNoteID(voice::Voice *v, int32_t noteid, float velocity)
        {
            SCLOG("Retrigger Voice Unimplemented")
        }

        void setVoiceMIDIPitchBend(voice::Voice *v, uint16_t pb14bit);

        void setVoiceMIDIMPEChannelPitchBend(voice::Voice *v, uint16_t pb14bit) {}

        void setVoicePolyphonicParameterModulation(voice::Voice *v, uint32_t parameter,
                                                   double value)
        {
        }

        void setNoteExpression(voice::Voice *v, int32_t expression, double value) {}
        void setPolyphonicAftertouch(voice::Voice *v, int8_t pat) {}
        void setChannelPressure(voice::Voice *v, int8_t pres) {}
        void setMIDI1CC(voice::Voice *v, int8_t cc, int8_t val);

    } voiceManagerResponder{*this};
    using voiceManager_t = sst::voicemanager::VoiceManager<VMConfig, VoiceManagerResponder>;
    voiceManager_t voiceManager{voiceManagerResponder};

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
        sharedUIMemoryState.voiceDisplayStateWriteCounter = 0;

        const double updateFrequencyHz{15.0};

        updateVoiceDisplayStateEvery = (int)std::floor(sampleRate / updateFrequencyHz / blockSize);
        lastUpdateVoiceDisplayState = updateVoiceDisplayStateEvery;

        auto vuFalloff = exp(-2 * M_PI * (60.f / samplerate));

        // TODO this is a bit hacky
        auto &bs = getPatch()->busses;
        bs.mainBus.vuFalloff = vuFalloff;
        for (auto &a : bs.auxBusses)
            a.vuFalloff = vuFalloff;
        for (auto &a : bs.partBusses)
            a.vuFalloff = vuFalloff;
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

    /*
     * The serialization technique described in messaging.h works
     * wonderfully especially for
     * bulk edits and so forth. But the practiczlities of writing an
     * in process UI makes us want to, for some rapidly changing
     * engine to ui only properties, want to really have some
     * std::atomic<blah> which are writable in the engine and
     * which are const & readable in the ui. The primary use cases
     * for this today are things like vu meters, sample-play-positions
     * and the like.
     *
     * While we could indeed have lots of messages for these, instead
     * we have a engine shared UI state object which the ui gets as a
     * const & and which contains atomic members.
     *
     * This is a cheat. But I think its a practical cheat. And it means
     * we can minimize the requirements of making streaming types just
     * for display things. By encapsulating it in one place we also know
     * what would need streaming if we went to a full out of process UI
     * (or what display features we would lose).
     */
    struct SharedUIMemoryState
    {
        std::array<std::array<std::atomic<float>, 2>, Patch::Busses::busCount> busVULevels;

        std::atomic<int64_t> voiceDisplayStateWriteCounter{0};

        struct VoiceDisplayStateItem
        {
            std::atomic<bool> active{false};
            std::atomic<size_t> part, group, zone;

            std::atomic<int64_t> samplePos{}, midiNote{-1}, midiChannel{-1};
            std::atomic<bool> gated{false};
        };
        std::atomic<int32_t> voiceCount;
        std::array<VoiceDisplayStateItem, maxVoices> voiceDisplayItems;
    } sharedUIMemoryState;

    /**
     * The VoiceDisplayState structure is updated at some relatively low (like 30hz)
     * frequency by the engine for communication to a potential client if the message
     * controller has a client connected. Updates will occur only if the read pointer and
     * write pointer are the same so the client needs to update the write pointer.
     */
    int32_t updateVoiceDisplayStateEvery{10000000};
    int32_t lastUpdateVoiceDisplayState{0};
    int64_t midiNoteStateCounter{0}, lastMidiNoteStateCounter{0};
    bool sendSamplePosition{true};

    /*
     * Random Number support
     */
    infrastructure::RNGGen rngGen;

    /*
     * Serialization thread originated mutation apis
     */
    void loadSampleIntoSelectedPartAndGroup(const fs::path &, int16_t rootKey = 60,
                                            KeyboardRange krange = {48, 72},
                                            VelocityRange vrange = {0, 127});

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

    const std::unique_ptr<MemoryPool> &getMemoryPool()
    {
        assert(memoryPool);
        return memoryPool;
    }

    std::atomic<int32_t> stopEngineRequests{0};

    /*
     * Metadata for the various voice group and so on matrices is generated
     * by a set of registered targets and sources with the engine
     */
    using vmodTgtStrFn_t = std::function<std::string(
        const Zone &, const voice::modulation::MatrixConfig::TargetIdentifier &)>;
    using gmodTgtStrFn_t = std::function<std::string(
        const Group &, const modulation::GroupMatrixConfig::TargetIdentifier &)>;

    using vmodSrcStrFn_t = std::function<std::string(
        const Zone &, const voice::modulation::MatrixConfig::SourceIdentifier &)>;
    using gmodSrcStrFn_t = std::function<std::string(
        const Group &, const voice::modulation::MatrixConfig::SourceIdentifier &)>;

    std::unordered_map<voice::modulation::MatrixConfig::TargetIdentifier,
                       std::pair<vmodTgtStrFn_t, vmodTgtStrFn_t>>
        voiceModTargets;
    std::unordered_map<modulation::GroupMatrixConfig::TargetIdentifier,
                       std::pair<gmodTgtStrFn_t, gmodTgtStrFn_t>>
        groupModTargets;

    std::unordered_map<voice::modulation::MatrixConfig::SourceIdentifier,
                       std::pair<vmodSrcStrFn_t, vmodSrcStrFn_t>>
        voiceModSources;
    std::unordered_map<modulation::GroupMatrixConfig::SourceIdentifier,
                       std::pair<gmodSrcStrFn_t, gmodSrcStrFn_t>>
        groupModSources;

    void registerVoiceModTarget(const voice::modulation::MatrixConfig::TargetIdentifier &,
                                vmodTgtStrFn_t pathFn, vmodTgtStrFn_t nameFn);
    void registerVoiceModSource(const voice::modulation::MatrixConfig::SourceIdentifier &,
                                vmodSrcStrFn_t pathFn, vmodSrcStrFn_t nameFn);
    void registerGroupModTarget(const modulation::GroupMatrixConfig::TargetIdentifier &,
                                gmodTgtStrFn_t pathFn, gmodTgtStrFn_t nameFn);
    void registerGroupModSource(const modulation::GroupMatrixConfig::SourceIdentifier &,
                                gmodSrcStrFn_t pathFn, gmodSrcStrFn_t nameFn);

  private:
    std::unique_ptr<Patch> patch;
    std::unique_ptr<MemoryPool> memoryPool;
    std::unique_ptr<sample::SampleManager> sampleManager;
    std::unique_ptr<browser::BrowserDB> browserDb;
    std::unique_ptr<browser::Browser> browser;
    std::array<voice::Voice *, maxVoices> voices;
    std::array<std::unique_ptr<voice::modulation::MatrixEndpoints>, maxVoices> allEndpoints;
    std::unique_ptr<uint8_t[]> voiceInPlaceBuffer{nullptr};
    std::unique_ptr<messaging::MessageController> messageController;
    std::unique_ptr<selection::SelectionManager> selectionManager;
};
} // namespace scxt::engine
#endif
