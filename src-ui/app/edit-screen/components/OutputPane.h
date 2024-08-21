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

#ifndef SCXT_SRC_UI_COMPONENTS_MULTI_OUTPUTPANE_H
#define SCXT_SRC_UI_COMPONENTS_MULTI_OUTPUTPANE_H

#include "sst/jucegui/components/NamedPanel.h"
#include "app/HasEditor.h"
#include "engine/zone.h"
#include "engine/group.h"
#include "messaging/messaging.h"

namespace scxt::ui::app::edit_screen
{

struct ProcessorPane;
struct OutPaneZoneTraits
{
    static constexpr bool forZone{true};
    static constexpr const char *defaultRoutingLocationName{"Group Output"};
    using info_t = engine::Zone::ZoneOutputInfo;
    using route_t = engine::Zone::ProcRoutingPath;

    using floatMsg_t = scxt::messaging::client::UpdateZoneOutputFloatValue;
    using int16Msg_t = scxt::messaging::client::UpdateZoneOutputInt16TValue;
};

struct OutPaneGroupTraits
{
    static constexpr bool forZone{false};
    static constexpr const char *defaultRoutingLocationName{"Part Output"};
    using info_t = engine::Group::GroupOutputInfo;
    using route_t = engine::Group::ProcRoutingPath;

    using floatMsg_t = scxt::messaging::client::UpdateGroupOutputFloatValue;
    using int16Msg_t = scxt::messaging::client::UpdateGroupOutputInt16TValue;
};
template <typename OTTraits> struct OutputTab;
template <typename OTTraits> struct ProcTab;

template <typename OTTraits> struct OutputPane : sst::jucegui::components::NamedPanel, HasEditor
{
    OutputPane(SCXTEditor *e);
    ~OutputPane();

    void resized() override;
    void setActive(bool);
    void setOutputData(const typename OTTraits::info_t &);

    std::unique_ptr<OutputTab<OTTraits>> output;
    std::unique_ptr<ProcTab<OTTraits>> proc;
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

    void setSelectedTab(int i);
};
} // namespace scxt::ui::app::edit_screen
#endif // SHORTCIRCUIT_MAPPINGPANE_H
