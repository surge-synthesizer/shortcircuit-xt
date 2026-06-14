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

#ifndef SCXT_SRC_SCXT_PLUGIN_CONNECTORS_BEGINEDITTRAITS_H
#define SCXT_SRC_SCXT_PLUGIN_CONNECTORS_BEGINEDITTRAITS_H

/*
 * Maps a c2s update message type to the undo subtree its gesture should
 * snapshot. configureUpdater consults this so every attachment created
 * from one of these messages automatically sends the tagged begin-edit
 * from its widget's onBeginEdit, making a whole drag one undo entry.
 */

#include "messaging/client/client_serial.h"
#include "messaging/client/selection_messages.h"
#include "messaging/client/zone_messages.h"
#include "messaging/client/group_messages.h"
#include "messaging/client/group_or_zone_messages.h"
#include "messaging/client/modulation_messages.h"
#include "messaging/client/processor_messages.h"

namespace scxt::ui::connectors
{
namespace cmsg = scxt::messaging::client;

template <typename M> struct BeginEditTraits
{
    static constexpr bool hasSubtree{false};
};

// fixed-scope zone messages
template <> struct BeginEditTraits<cmsg::UpdateZoneMappingFloatValue>
{
    static constexpr bool hasSubtree{true};
    static constexpr auto subtree{cmsg::EditSubtree::zone_mapping};
    static constexpr bool fixedForZone{true};
};
template <> struct BeginEditTraits<cmsg::UpdateZoneMappingInt16TValue>
{
    static constexpr bool hasSubtree{true};
    static constexpr auto subtree{cmsg::EditSubtree::zone_mapping};
    static constexpr bool fixedForZone{true};
};
template <> struct BeginEditTraits<cmsg::UpdateZoneOutputFloatValue>
{
    static constexpr bool hasSubtree{true};
    static constexpr auto subtree{cmsg::EditSubtree::output_info};
    static constexpr bool fixedForZone{true};
};
template <> struct BeginEditTraits<cmsg::UpdateGroupOutputFloatValue>
{
    static constexpr bool hasSubtree{true};
    static constexpr auto subtree{cmsg::EditSubtree::output_info};
    static constexpr bool fixedForZone{false};
};

// zone-or-group messages; forZone (and index) ride in the updater args
template <> struct BeginEditTraits<cmsg::UpdateZoneOrGroupEGFloatValue>
{
    static constexpr bool hasSubtree{true};
    static constexpr auto subtree{cmsg::EditSubtree::eg};
};
template <> struct BeginEditTraits<cmsg::UpdateZoneOrGroupModStorageFloatValue>
{
    static constexpr bool hasSubtree{true};
    static constexpr auto subtree{cmsg::EditSubtree::mod_storage};
};
template <> struct BeginEditTraits<cmsg::UpdateZoneOrGroupProcessorFloatValue>
{
    static constexpr bool hasSubtree{true};
    static constexpr auto subtree{cmsg::EditSubtree::processor};
};
template <> struct BeginEditTraits<cmsg::UpdateAudiomodStorageElement>
{
    static constexpr bool hasSubtree{true};
    static constexpr auto subtree{cmsg::EditSubtree::audio_mod};
};

} // namespace scxt::ui::connectors

#endif // SCXT_SRC_SCXT_PLUGIN_CONNECTORS_BEGINEDITTRAITS_H
