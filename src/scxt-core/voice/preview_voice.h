/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_SCXT_CORE_VOICE_PREVIEW_VOICE_H
#define SCXT_SRC_SCXT_CORE_VOICE_PREVIEW_VOICE_H

#include <memory>

#include "utils.h"
#include "configuration.h"
#include "sample/sample.h"

namespace scxt::voice
{
struct alignas(16) PreviewVoice : SampleRateSupport
{
    float output alignas(16)[2][blockSize];

    struct Details;
    PreviewVoice();
    ~PreviewVoice();

    /***
     * Given this sample, play it with this voice unless this vice is already playing
     * exactly this sample, in which case, stop and release the reference
     */
    bool attachAndStartUnlessPlaying(const std::shared_ptr<sample::Sample> &, float amplitude);

    /***
     * Stop playing the sample and release the reference to the sample
     */
    bool detatchAndStop();

    /***
     * Populate the output field with the sample data
     */
    void processBlock();

    bool isActive{false};
    bool schedulePurge{false};
    std::unique_ptr<Details> details;
};
} // namespace scxt::voice

#endif // PREVIEW_VOICE_H
