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

#ifndef SCXT_SRC_MESSAGING_CLIENT_ENGINESTATUS_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_ENGINESTATUS_MESSAGES_H

#include "messaging/client/detail/client_json_details.h"
#include "json/engine_traits.h"
#include "json/datamodel_traits.h"
#include "selection/selection_manager.h"

namespace scxt::messaging::client
{
struct VoiceCountUpdate
{
    static constexpr SerializationToClientMessageIds s2c_id{s2c_voice_count};
    typedef uint32_t s2c_payload_t;
    template <typename Client> static void executeOnClient(Client *c, const s2c_payload_t &payload)
    {
        c->onVoiceCount(payload);
    }
};

template <> struct SerializationToClientType<VoiceCountUpdate::s2c_id>
{
    typedef VoiceCountUpdate T;
};

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_ENGINESTATUS_MESSAGES_H
