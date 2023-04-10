//
// Created by Paul Walker on 4/11/23.
//

#ifndef SHORTCIRCUITXT_PART_MESSAGES_H
#define SHORTCIRCUITXT_PART_MESSAGES_H

#include "client_macros.h"
#include "engine/part.h"
#include "selection/selection_manager.h"

namespace scxt::messaging::client
{
SERIAL_TO_CLIENT(SelectedGroupZoneMappingSummary, s2c_send_selected_group_zone_mapping_summary,
                 engine::Part::zoneMappingSummary_t, onGroupZoneMappingSummary);
} // namespace scxt::messaging::client
#endif // SHORTCIRCUITXT_PART_MESSAGES_H
