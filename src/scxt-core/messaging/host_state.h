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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_HOST_STATE_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_HOST_STATE_H

#include <memory>
#include <string>

#include "engine/engine.h"
#include "engine/patch.h"

namespace scxt::messaging
{
// cheap shape check the host thread can afford; the real parse happens on serial
bool looksLikeEngineState(const std::string &blob);

/*
 * Host main thread. Queues the blob for serial to unstream and returns at once; false
 * only when the blob fails the shape check, and then nothing changes.
 */
bool hostStateLoad(engine::Engine &e, std::string &&blob);

// host main thread; the held blob while a load is pending or failed, else the engine
bool hostStateSave(engine::Engine &e, std::string &out);

// host main thread; while a host load runs, params answer with the macros it carries
struct HostLoadMacros
{
    // null unless a load is pending or loading and its blob reads
    const engine::Macro *find(engine::Engine &e, int part, int index);

  private:
    uint64_t generation{0};
    std::unique_ptr<engine::Patch> patch;
};
} // namespace scxt::messaging

#endif // SCXT_SRC_SCXT_CORE_MESSAGING_HOST_STATE_H
