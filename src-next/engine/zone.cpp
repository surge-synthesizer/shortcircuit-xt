//
// Created by Paul Walker on 2/1/23.
//

#include "zone.h"
#include "group.h"
#include "voice/voice.h"

#include "sst/basic-blocks/mechanics/block-ops.h"

namespace scxt::engine
{
void Zone::process()
{
    namespace blk = sst::basic_blocks::mechanics;
    // TODO these memsets are probably gratuitous
    memset(output, 0, sizeof(output));

    std::array<voice::Voice *, maxVoices> toCleanUp;
    size_t cleanupIdx{0};
    for (auto &v : voiceWeakPointers)
    {
        if (v && v->playState != voice::Voice::OFF)
        {
            if (v->process())
            {
                blk::accumulate_from_to<blockSize>(v->output[0], output[0][0]);
                blk::accumulate_from_to<blockSize>(v->output[1], output[0][1]);
            }
            if (v->playState == voice::Voice::CLEANUP)
            {
                toCleanUp[cleanupIdx++] = v;
            }
        }
    }
    for (int i = 0; i < cleanupIdx; ++i)
    {
        toCleanUp[i]->cleanupVoice();
    }
}

void Zone::addVoice(voice::Voice *v)
{
    if (activeVoices == 0)
    {
        parentGroup->addActiveZone();
    }
    activeVoices++;
    for (auto &nv : voiceWeakPointers)
    {
        if (nv == nullptr)
        {
            nv = v;
            return;
        }
    }
    assert(false);
}
void Zone::removeVoice(voice::Voice *v)
{
    for (auto &nv : voiceWeakPointers)
    {
        if (nv == v)
        {
            nv = nullptr;
            activeVoices--;
            if (activeVoices == 0)
            {
                parentGroup->removeActiveZone();
            }
            return;
        }
    }
    assert(false);
}
} // namespace scxt::engine