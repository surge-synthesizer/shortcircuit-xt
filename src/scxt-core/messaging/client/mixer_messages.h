/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_MIXER_MESSAGES_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_MIXER_MESSAGES_H

#include "client_macros.h"
#include "engine/bus.h"
#include "engine/part.h"

namespace scxt::messaging::client
{

using busEffectFullData_t =
    std::tuple<int, int, int,
               std::pair<std::array<sst::basic_blocks::params::ParamMetaData,
                                    engine::BusEffectStorage::maxBusEffectParams>,
                         engine::BusEffectStorage>>;
// thats bus or -1, part or -1, slot, data

SERIAL_TO_CLIENT(SendBusEffectFullData, s2c_bus_effect_full_data, busEffectFullData_t,
                 onBusOrPartEffectFullData);

using busSendData_t = std::tuple<int, int, engine::Bus::BusSendStorage>; // bus/-1, part/-1, dat
SERIAL_TO_CLIENT(SendBusSendData, s2c_bus_send_data, busSendData_t, onBusOrPartSendData);

using setBusEffectToType_t = std::tuple<int, int, int, int>; // bus or -1, part of -1, fx, type
inline void setBusEffectToType(const setBusEffectToType_t &payload,
                               messaging::MessageController &cont)
{
    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [p = payload](auto &e) {
            auto [bus, part, fxslot, type] = p;
            if (bus >= 0)
            {
                e.getPatch()
                    ->busses.busByAddress((engine::BusAddress)bus)
                    .setBusEffectType(e, fxslot, (engine::AvailableBusEffects)type);
            }
            else
            {
                e.getPatch()->getPart(part)->setBusEffectType(e, fxslot,
                                                              (engine::AvailableBusEffects)type);
            }
        },
        [p = payload, &cont](auto &e) {
            auto [bus, part, fxslot, type] = p;
            if (bus >= 0)
            {
                e.getPatch()
                    ->busses.busByAddress((engine::BusAddress)bus)
                    .sendBusEffectInfoToClient(e, fxslot);
            }
            else
            {
                e.getPatch()->getPart(part)->sendBusEffectInfoToClient(e, fxslot);
            }
        });
}
CLIENT_TO_SERIAL(SetBusEffectToType, c2s_set_mixer_effect, setBusEffectToType_t,
                 setBusEffectToType(payload, cont));

using setBusEffectStorage_t =
    std::tuple<int, int, int, engine::BusEffectStorage>; // bus/-1, part/-1, fx, type
inline void setBusEffectStorage(const setBusEffectStorage_t &payload,
                                messaging::MessageController &cont)
{
    cont.scheduleAudioThreadCallback([p = payload](auto &e) {
        const auto &[bus, part, fxslot, bes] = p;
        if (bus >= 0)
            e.getPatch()->busses.busByAddress((engine::BusAddress)bus).busEffectStorage[fxslot] =
                bes;
        else
            e.getPatch()->getPart(part)->partEffectStorage[fxslot] = bes;
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
        e.getPatch()->busses.reconfigureSolo();
        e.getPatch()->busses.reconfigureOutputBusses();
    });
}
CLIENT_TO_SERIAL(SetBusSendStorage, c2s_set_mixer_send_storage, setBusSendStorage_t,
                 setBusSendStorage(payload, cont));

using busFxSwap_t = std::tuple<int16_t, int16_t, int16_t, int16_t, int16_t>;
inline void doBusSwapFX(const busFxSwap_t &payload, const engine::Engine &engine,
                        messaging::MessageController &cont)
{

    auto [bs1, sl1, bs2, sl2, smc] = payload;
    if (bs1 == bs2 && sl1 == sl2)
    {
        cont.reportErrorToClient("Cant move fx onto itself",
                                 "Bus Swap FX had same bus/slot location");
        return;
    }

    cont.scheduleAudioThreadCallback(
        [bs1, bs2, sl1, sl2](auto &engine) {
            auto &b1 = engine.getPatch()->busses.busByAddress((engine::BusAddress)bs1);
            auto &b2 = engine.getPatch()->busses.busByAddress((engine::BusAddress)bs2);
            auto fs = b1.busEffectStorage[sl1];
            auto ts = b2.busEffectStorage[sl2];

            b1.setBusEffectType(engine, sl1, ts.type);
            b2.setBusEffectType(engine, sl2, fs.type);

            b1.busEffectStorage[sl1] = ts;
            b2.busEffectStorage[sl2] = fs;
        },
        [bs1, sl1, bs2, sl2](const auto &engine) {
            engine.getPatch()
                ->busses.busByAddress((engine::BusAddress)bs1)
                .sendBusEffectInfoToClient(engine, sl1);
            engine.getPatch()
                ->busses.busByAddress((engine::BusAddress)bs2)
                .sendBusEffectInfoToClient(engine, sl2);
        });
}
CLIENT_TO_SERIAL(SwapBusFX, c2s_swap_bus_fx, busFxSwap_t, doBusSwapFX(payload, engine, cont));
} // namespace scxt::messaging::client
#endif // SHORTCIRCUITXT_MIXER_MESSAGES_H
