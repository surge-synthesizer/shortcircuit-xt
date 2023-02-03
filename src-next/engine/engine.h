#ifndef __SCXT_ENGINE_ENGINE_H
#define __SCXT_ENGINE_ENGINE_H

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
namespace scxt::engine
{
struct Engine : MoveableOnly<Engine>, SampleRateSupport
{
    Engine();
    ~Engine();

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
    void noteOn(int16_t channel, int16_t key, int32_t noteId, float velocity, float detune)
    {
        std::cout << "Engine::noteOn c=" << channel << " k=" << key << " nid=" << noteId
                  << " v=" << velocity << std::endl;
        for (const auto &path : findZone(channel, key, noteId))
        {
            initiateVoice(path);
        }
    }
    void noteOff(int16_t channel, int16_t key, int32_t noteId, float velocity)
    {
        std::cout << "Engine::noteOff c=" << channel << " k=" << key << " nid=" << noteId
                  << std::endl;

        for (const auto &path : findZone(channel, key, noteId))
        {
            releaseVoice(path);
        }
    }

    const std::unique_ptr<Zone> &zoneByPath(const pathToZone_t &path) const
    {
        const auto &[p, g, z, c, k, n] = path;
        return patch->getPart(p)->getGroup(g)->getZone(z);
    }
    void initiateVoice(const pathToZone_t &path);
    void releaseVoice(const pathToZone_t &path);

  private:
    std::unique_ptr<Patch> patch;
    std::unique_ptr<sample::SampleManager> sampleManager;
    std::array<voice::Voice *, maxVoices> voices;
    std::unique_ptr<uint8_t[]> voiceInPlaceBuffer{nullptr};
};
} // namespace scxt::engine
#endif