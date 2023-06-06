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

#ifndef SCXT_SRC_MESSAGING_CLIENT_MIXER_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_MIXER_MESSAGES_H

#include "client_macros.h"
#include "engine/bus.h"
#include "engine/part.h"

namespace scxt::messaging::client
{

using busEffectFullData_t =
    std::tuple<int, int,
               std::pair<std::array<sst::basic_blocks::params::ParamMetaData,
                                    engine::BusEffectStorage::maxBusEffectParams>,
                         engine::BusEffectStorage>>;

SERIAL_TO_CLIENT(SendBusEffectFullData, s2c_bus_effect_full_data, busEffectFullData_t,
                 onMixerBusEffectFullData);

using busSendData_t = std::tuple<int, engine::Bus::BusSendStorage>;
SERIAL_TO_CLIENT(SendBusSendData, s2c_bus_send_data, busSendData_t, onMixerBusSendData);

using setBusEffectToType_t = std::tuple<int, int, int>; // bus, fx, type
inline void setBusEffectToType(const setBusEffectToType_t &payload,
                               messaging::MessageController &cont)
{
    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [p = payload](auto &e) {
            auto [bus, fxslot, type] = p;
            e.getPatch()
                ->busses.busByAddress((engine::BusAddress)bus)
                .setBusEffectType(e, fxslot, (engine::AvailableBusEffects)type);
        },
        [p = payload, &cont](auto &e) {
            auto [bus, fxslot, type] = p;
            // ToDo: Send Metadata Blast back
            e.getPatch()
                ->busses.busByAddress((engine::BusAddress)bus)
                .sendBusEffectInfoToClient(e, fxslot);
        });
}
CLIENT_TO_SERIAL(SetBusEffectToType, c2s_set_mixer_effect, setBusEffectToType_t,
                 setBusEffectToType(payload, cont));

using setBusEffectStorage_t = std::tuple<int, int, engine::BusEffectStorage>; // bus, fx, type
inline void setBusEffectStorage(const setBusEffectStorage_t &payload,
                                messaging::MessageController &cont)
{
    cont.scheduleAudioThreadCallback([p = payload](auto &e) {
        auto [bus, fxslot, bes] = p;
        e.getPatch()->busses.busByAddress((engine::BusAddress)bus).busEffectStorage[fxslot] = bes;
    });
}
CLIENT_TO_SERIAL(SetBusEffectStorage, c2s_set_mixer_effect_storage, setBusEffectStorage_t,
                 setBusEffectStorage(payload, cont));

using setBusSendStorage_t = std::tuple<int, engine::Bus::BusSendStorage>;
inline void setBusSendStorage(const setBusSendStorage_t &payload,
                              messaging::MessageController &cont)
{
    cont.scheduleAudioThreadCallback([p = payload](auto &e) {
        auto [bus, bss] = p;
        e.getPatch()->busses.busByAddress((engine::BusAddress)bus).busSendStorage = bss;
        e.getPatch()->busses.busByAddress((engine::BusAddress)bus).resetSendState();
    });
}
CLIENT_TO_SERIAL(SetBusSendStorage, c2s_set_mixer_send_storage, setBusSendStorage_t,
                 setBusSendStorage(payload, cont));
} // namespace scxt::messaging::client
#endif // SHORTCIRCUITXT_MIXER_MESSAGES_H
