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

#include "utils.h"
#include "part.h"
#include "group.h"
#include "zone.h"
#include "patch.h"

#include "configuration.h"

#include "sample/sample.h"
#include "sample/sample_manager.h"

#include <filesystem>
#include <set>
#include <cassert>

namespace scxt::voice
{
struct Voice;
}
namespace scxt::messaging
{
struct MessageController;
}
namespace scxt::engine
{
struct Engine : MoveableOnly<Engine>, SampleRateSupport
{
    Engine();
    ~Engine();

    EngineID id;

    const std::unique_ptr<Patch> &getPatch() const { return patch; }
    const std::unique_ptr<sample::SampleManager> &getSampleManager() const { return sampleManager; }

    /**
     * Audio processing
     */
    int getStereoOutputs() const { return 1; }
    void setStereoOutputs(int s) { assert(s == 1); }

    float output alignas(16)[maxOutputs][2][blockSize];

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
    std::vector<pathToZone_t> findZone(int16_t channel, int16_t key, int32_t noteId)
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
                        if (zone->keyboardRange.includes(key))
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
    void noteOn(int16_t channel, int16_t key, int32_t noteId, float velocity, float detune);
    void noteOff(int16_t channel, int16_t key, int32_t noteId, float velocity);

    const std::unique_ptr<Zone> &zoneByPath(const pathToZone_t &path) const
    {
        const auto &[p, g, z, c, k, n] = path;
        return patch->getPart(p)->getGroup(g)->getZone(z);
    }
    void initiateVoice(const pathToZone_t &path);
    void releaseVoice(const pathToZone_t &path);

    // TODO: All this gets ripped out when voice management is fixed
    uint32_t activeVoiceCount();

    const std::unique_ptr<messaging::MessageController> &getMessageController() const
    {
        return messageController;
    }

    /*
     * Data Query APIs
     */
    typedef std::tuple<int32_t, int32_t, int32_t, int32_t> processorAddress_t;
    const std::optional<dsp::processor::ProcessorStorage> getProcessorStorage(const processorAddress_t &addr) const;


  private:
    std::unique_ptr<Patch> patch;
    std::unique_ptr<sample::SampleManager> sampleManager;
    std::array<voice::Voice *, maxVoices> voices;
    std::unique_ptr<uint8_t[]> voiceInPlaceBuffer{nullptr};
    std::unique_ptr<messaging::MessageController> messageController;
};
} // namespace scxt::engine
#endif