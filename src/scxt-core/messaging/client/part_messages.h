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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_PART_MESSAGES_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_PART_MESSAGES_H

#include "client_macros.h"
#include "engine/part.h"
#include "selection/selection_manager.h"

namespace scxt::messaging::client
{
SERIAL_TO_CLIENT(SelectedPart, s2c_send_selected_part, int16_t, onSelectedPart);
SERIAL_TO_CLIENT(SelectedGroupZoneMappingSummary, s2c_send_selected_group_zone_mapping_summary,
                 engine::Part::zoneMappingSummary_t, onGroupZoneMappingSummary);

using partConfigurationPayload_t = std::pair<int16_t, scxt::engine::Part::PartConfiguration>;
SERIAL_TO_CLIENT(SendPartConfiguration, s2c_send_part_configuration, partConfigurationPayload_t,
                 onPartConfiguration);

inline void updatePartFullConfig(const partConfigurationPayload_t &p, const engine::Engine &e,
                                 messaging::MessageController &cont)
{
    auto [pt, conf] = p;
    cont.scheduleAudioThreadCallback([part = pt, configuration = conf](auto &eng) {
        eng.getPatch()->getPart(part)->configuration = configuration;
        eng.onPartConfigurationUpdated();
    });
}
CLIENT_TO_SERIAL(UpdatePartFullConfig, c2s_send_full_part_config, partConfigurationPayload_t,
                 updatePartFullConfig(payload, engine, cont));

// part, b1, part2 b2, swap (0), move (1) or copy (2)
using partFxSwap_t = std::tuple<int16_t, int16_t, int16_t, int16_t, int16_t>;
inline void doPartSwapFX(const partFxSwap_t &payload, const engine::Engine &engine,
                         messaging::MessageController &cont)
{
    auto [p1, b1, p2, b2, smc] = payload;
    if (p1 != p2)
    {
        cont.reportErrorToClient("Cross Part Moves not supported yet",
                                 "Part Swap FX must be within the same part");
        return;
    }
    if (b1 == b2)
    {
        cont.reportErrorToClient("Cant move part onto itself",
                                 "Part Swap FX had same bus location");
        return;
    }

    cont.scheduleAudioThreadCallback(
        [pti = p1, b1, b2](auto &engine) {
            const auto &pt = engine.getPatch()->getPart(pti);
            auto fs = pt->partEffectStorage[b1];
            auto ts = pt->partEffectStorage[b2];

            pt->setBusEffectType(engine, b1, ts.type);
            pt->setBusEffectType(engine, b2, fs.type);
            pt->partEffectStorage[b1] = ts;
            pt->partEffectStorage[b2] = fs;
        },
        [pti = p1, b1, b2](const auto &engine) {
            engine.getPatch()->getPart(pti)->sendBusEffectInfoToClient(engine, b1);
            engine.getPatch()->getPart(pti)->sendBusEffectInfoToClient(engine, b2);
        });
}
CLIENT_TO_SERIAL(SwapPartFX, c2s_swap_part_fx, partFxSwap_t, doPartSwapFX(payload, engine, cont));
} // namespace scxt::messaging::client
#endif // SHORTCIRCUITXT_PART_MESSAGES_H
