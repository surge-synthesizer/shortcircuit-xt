/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#ifndef SCXT_SRC_MESSAGING_CLIENT_PART_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_PART_MESSAGES_H

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
    });
}
CLIENT_TO_SERIAL(UpdatePartFullConfig, c2s_send_full_part_config, partConfigurationPayload_t,
                 updatePartFullConfig(payload, engine, cont));
} // namespace scxt::messaging::client
#endif // SHORTCIRCUITXT_PART_MESSAGES_H
