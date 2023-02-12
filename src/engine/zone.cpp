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