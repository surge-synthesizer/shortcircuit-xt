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

#ifndef SCXT_SRC_MESSAGING_CLIENT_CLIENT_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_CLIENT_MESSAGES_H

#include "messaging/client/detail/client_json_details.h"
#include "json/engine_traits.h"
#include "json/datamodel_traits.h"
#include "selection/SelectionManager.h"

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

struct AdsrSelectedZoneView
{
    static constexpr ClientToSerializationMessagesIds c2s_id{c2s_request_zone_adsr_view};
    static constexpr SerializationToClientMessageIds s2c_id{s2c_respond_zone_adsr_view};

    typedef int c2s_payload_t;
    typedef std::tuple<int, bool, datamodel::AdsrStorage> s2c_payload_t;

    c2s_payload_t payload;

    AdsrSelectedZoneView(int whichEnv) : payload(whichEnv) {}

    static void executeOnSerialization(const c2s_payload_t &which, const engine::Engine &engine,
                                       MessageController &cont)
    {
        // TODO Selected Zone State
        // const auto &selectedZone = engine.getPatch()->getPart(0)->getGroup(0)->getZone(0);
        auto addr = engine.getSelectionManager()->getSelectedZone();

        // Temporary hack
        addr = {0, 0, 0};
        if (addr.has_value())
        {
            const auto &selectedZone =
                engine.getPatch()->getPart(addr->part)->getGroup(addr->group)->getZone(addr->zone);
            if (which == 0)
                serializationSendToClient(s2c_respond_zone_adsr_view,
                                          s2c_payload_t{which, true, selectedZone->aegStorage},
                                          cont);
            if (which == 1)
                serializationSendToClient(s2c_respond_zone_adsr_view,
                                          s2c_payload_t{which, true, selectedZone->eg2Storage},
                                          cont);
        }
        else
        {
            // It is a wee bit wasteful re-sending a default ADSR here but easier
            // than two messages
            serializationSendToClient(s2c_respond_zone_adsr_view, s2c_payload_t{which, false, {}},
                                      cont);
        }
    }

    template <typename Client> static void executeOnClient(Client *c, const s2c_payload_t &payload)
    {
        const auto &[which, active, env] = payload;
        c->onEnvelopeUpdated(which, active, env);
    }
};

template <> struct ClientToSerializationType<AdsrSelectedZoneView::c2s_id>
{
    typedef AdsrSelectedZoneView T;
};

template <> struct SerializationToClientType<AdsrSelectedZoneView::s2c_id>
{
    typedef AdsrSelectedZoneView T;
};

struct AdsrSelectedZoneUpdateRequest
{
    static constexpr ClientToSerializationMessagesIds c2s_id{c2s_update_zone_adsr_view};
    static constexpr bool hasC2SPayload{true};
    typedef std::tuple<int, datamodel::AdsrStorage> c2s_payload_t;

    c2s_payload_t payload;

    AdsrSelectedZoneUpdateRequest(int whichEnv, const datamodel::AdsrStorage &a)
        : payload{whichEnv, a}
    {
    }

    static void executeOnSerialization(const c2s_payload_t &payload, const engine::Engine &engine,
                                       MessageController &cont)
    {
        // TODO Selected Zone State
        const auto &[e, adsr] = payload;
        cont.scheduleAudioThreadCallback([ew = e, adsrv = adsr](auto &eng) {
            if (ew == 0)
                eng.getPatch()->getPart(0)->getGroup(0)->getZone(0)->aegStorage = adsrv;
        });
    }
};

template <> struct ClientToSerializationType<AdsrSelectedZoneUpdateRequest::c2s_id>
{
    typedef AdsrSelectedZoneUpdateRequest T;
};

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_CLIENT_MESSAGE_IMPLS_H
