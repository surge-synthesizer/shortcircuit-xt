//
// Created by Paul Walker on 3/7/23.
//

#ifndef SHORTCIRCUIT_GROUP_MESSAGES_H
#define SHORTCIRCUIT_GROUP_MESSAGES_H

#include "client_macros.h"
#include "engine/group.h"

namespace scxt::messaging::client
{
SERIAL_TO_CLIENT(SelectedGroupZoneMappingSummary, s2c_send_selected_group_zone_mapping_summary,
                 engine::Group::zoneMappingSummary_t , onGroupZoneMappingSummary);

}
#endif // SHORTCIRCUIT_GROUP_MESSAGES_H
