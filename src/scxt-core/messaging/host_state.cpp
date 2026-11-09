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

#include "host_state.h"

#include <cctype>

#include "messaging/messaging.h"
#include "messaging/client/client_serial.h"
#include "messaging/client/enginestatus_messages.h"
#include "json/stream.h"

namespace scxt::messaging
{
bool looksLikeEngineState(const std::string &blob)
{
    auto isPad = [](char c) { return c == 0 || std::isspace((unsigned char)c); };

    auto first = blob.find_first_not_of(" \t\r\n");
    if (first == std::string::npos || blob[first] != '{')
        return false;

    // a truncated write loses the closing brace
    auto last = blob.size();
    while (last > first && isPad(blob[last - 1]))
        last--;
    return last > first + 1 && blob[last - 1] == '}';
}

bool hostStateLoad(engine::Engine &e, std::string &&blob)
{
    if (!looksLikeEngineState(blob))
    {
        SCLOG_IF(warnings, "Host state is not an engine state; ignoring it");
        return false;
    }

    auto &cont = *e.getMessageController();
    auto generation = cont.hostStateSlot.offer(std::move(blob));
    if (cont.hostStateSlot.onLoadBegin)
        cont.hostStateSlot.onLoadBegin(generation);

    auto bypass = cont.threadingChecker.bypassChecksInScope();
    client::clientSendToSerialization(client::LoadHostState(generation), cont);
    return true;
}

bool hostStateSave(engine::Engine &e, std::string &out)
{
    if (auto held = e.getMessageController()->hostStateSlot.heldBlob())
    {
        out = *held;
        return true;
    }

    // serial holds this for each message and structure change, so the engine sits still
    std::lock_guard<std::mutex> g(e.modifyStructureMutex);
    e.prepareToStream();
    e.prepareDawExtraState();
    try
    {
        auto sg = engine::Engine::StreamGuard(engine::Engine::FOR_DAW);
        out = json::streamEngineState(e);
    }
    catch (const std::runtime_error &err)
    {
        SCLOG_IF(warnings, "Streaming exception [" << err.what() << "]");
        return false;
    }
    return true;
}

const engine::Macro *HostLoadMacros::find(engine::Engine &e, int part, int index)
{
    using slot_t = MessageController::HostStateSlot;
    auto &slot = e.getMessageController()->hostStateSlot;
    auto [phase, g] = slot.status();
    if (phase != slot_t::PENDING && phase != slot_t::LOADING)
    {
        patch.reset();
        return nullptr;
    }

    // offers come from this thread, so a held blob is this generation's
    if (g != generation)
    {
        generation = g;
        patch.reset();
        if (auto blob = slot.heldBlob())
        {
            try
            {
                auto p = std::make_unique<engine::Patch>();
                json::unstreamMultiStructure(*p, *blob);
                patch = std::move(p);
            }
            catch (const std::exception &err)
            {
                SCLOG_IF(warnings, "Host state macros unreadable [" << err.what() << "]");
            }
        }
    }

    if (!patch || part < 0 || part >= numParts || index < 0 || index >= macrosPerPart)
        return nullptr;
    return &patch->getPart(part)->macros[index];
}
} // namespace scxt::messaging
