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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_ROUTINGPANE_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_ROUTINGPANE_H

#include "sst/jucegui/components/NamedPanel.h"
#include "app/HasEditor.h"
#include "engine/zone.h"
#include "engine/group.h"
#include "messaging/messaging.h"

namespace scxt::ui::app::edit_screen
{

struct ProcessorPane;
struct RoutingPaneZoneTraits
{
    static constexpr bool forZone{true};
    static constexpr const char *defaultRoutingLocationName{"Group Output"};
    using info_t = engine::Zone::ZoneOutputInfo;
    using route_t = engine::Zone::ProcRoutingPath;

    using floatMsg_t = scxt::messaging::client::UpdateZoneOutputFloatValue;
    using int16Msg_t = scxt::messaging::client::UpdateZoneOutputInt16TValue;
    using int16RefreshMsg_t = scxt::messaging::client::UpdateZoneOutputInt16TValueThenRefresh;

    static engine::Zone::ZoneOutputInfo &outputInfo(SCXTEditor *e);
};

struct RoutingPaneGroupTraits
{
    static constexpr bool forZone{false};
    static constexpr const char *defaultRoutingLocationName{"Part Output"};
    using info_t = engine::Group::GroupOutputInfo;
    using route_t = engine::Group::ProcRoutingPath;

    using floatMsg_t = scxt::messaging::client::UpdateGroupOutputFloatValue;
    using int16Msg_t = scxt::messaging::client::UpdateGroupOutputInt16TValue;
    using int16RefreshMsg_t = int16Msg_t; // for now.

    static engine::Group::GroupOutputInfo &outputInfo(SCXTEditor *e);
};

template <typename RPTraits> struct RoutingPaneContents;

template <typename RPTraits> struct RoutingPane : sst::jucegui::components::NamedPanel, HasEditor
{
    RoutingPane(SCXTEditor *e);
    ~RoutingPane();

    void resized() override;
    void setActive(bool);

    void updateFromOutputInfo();

    typedef connectors::BooleanPayloadDataAttachment<typename RPTraits::info_t> bool_attachment_t;
    std::unique_ptr<bool_attachment_t> oversampleAttachment;
    std::unique_ptr<sst::jucegui::components::ToggleButton> oversampleButton;
    std::unique_ptr<RoutingPaneContents<RPTraits>> contents;
    bool active{false};

    /* Since we contain the processor output info here, we need
     * the processor data path. We could dup it or we could just
     * have a reference to the processor pane here so
     */
    void addWeakProcessorPaneReference(int idx,
                                       const juce::Component::SafePointer<ProcessorPane> &p)
    {
        procWeakRefs[idx] = p;
    }
    void updateFromProcessorPanes();
    std::array<juce::Component::SafePointer<ProcessorPane>, scxt::processorsPerZoneAndGroup>
        procWeakRefs;
};
} // namespace scxt::ui::app::edit_screen
#endif // SHORTCIRCUIT_MAPPINGPANE_H
