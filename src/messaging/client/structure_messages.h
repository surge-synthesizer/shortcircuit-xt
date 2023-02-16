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

#ifndef SCXT_SRC_MESSAGING_CLIENT_STRUCTURE_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_STRUCTURE_MESSAGES_H

#include "messaging/client/detail/client_json_details.h"
#include "json/selection_traits.h"
#include "selection/selection_manager.h"
#include "engine/engine.h"

namespace scxt::messaging::client
{
/*
 * Send the full structure of all groups and zones or for a single part
 * in a single message
 */
struct PartGroupZoneStructure
{
    static constexpr ClientToSerializationMessagesIds c2s_id{c2s_request_pgz_structure};
    static constexpr SerializationToClientMessageIds s2c_id{s2c_send_pgz_structure};

    typedef int32_t c2s_payload_t; // the part number, or -1 for all parts

    // -1 group means empty part; -1 zone means empty group
    typedef engine::Engine::pgzStructure_t s2c_payload_t;

    c2s_payload_t payload{-1};

    PartGroupZoneStructure(const c2s_payload_t &p = -1) : payload(p) {}

    static void executeOnSerialization(const c2s_payload_t &which, const engine::Engine &engine,
                                       MessageController &cont)
    {
        serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(which),
                                  cont);
    }

    template <typename Client> static void executeOnClient(Client *c, const s2c_payload_t &payload)
    {
        c->onStructureUpdated(payload);
    }
};

template <> struct ClientToSerializationType<PartGroupZoneStructure::c2s_id>
{
    typedef PartGroupZoneStructure T;
};

template <> struct SerializationToClientType<PartGroupZoneStructure::s2c_id>
{
    typedef PartGroupZoneStructure T;
};

/*
 * A message the client auto-sends when it registers just so we can respond
 */
struct OnRegister
{
    static constexpr ClientToSerializationMessagesIds c2s_id{c2s_on_register};
    typedef bool c2s_payload_t; // the part number, or -1 for all parts
    c2s_payload_t payload{true};

    OnRegister(const c2s_payload_t &p = true) : payload(p) {}

    static void executeOnSerialization(const c2s_payload_t &which, const engine::Engine &engine,
                                       MessageController &cont)
    {
        PartGroupZoneStructure::executeOnSerialization(-1, engine, cont);
        engine.getSelectionManager()->singleSelect({});
    }
};

template <> struct ClientToSerializationType<OnRegister::c2s_id>
{
    typedef OnRegister T;
};

/*
 * A message the client auto-sends when it registers just so we can respond
 */
struct AddSample
{
    static constexpr ClientToSerializationMessagesIds c2s_id{c2s_add_sample};
    typedef std::string c2s_payload_t; // the part number, or -1 for all parts
    c2s_payload_t payload{};

    AddSample(const fs::path &p) : payload(p.u8string()) {}

    static void executeOnSerialization(const c2s_payload_t &pathu8, engine::Engine &engine,
                                       MessageController &cont)
    {
        auto p = fs::path{pathu8};
        engine.loadSampleIntoSelectedPartAndGroup(p);
    }
};

template <> struct ClientToSerializationType<AddSample::c2s_id>
{
    typedef AddSample T;
};

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_STRUCTURE_MESSAGES_H
