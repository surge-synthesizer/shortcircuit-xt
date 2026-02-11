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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_PARTGROUPSIDEBAR_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_PARTGROUPSIDEBAR_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/NamedPanel.h"
#include "app/HasEditor.h"
#include "engine/engine.h"

namespace scxt::ui::app::edit_screen
{
struct GroupSidebar;
struct PartSidebar;
struct ZoneSidebar;

struct PartGroupSidebar : sst::jucegui::components::NamedPanel, HasEditor
{
    PartGroupSidebar(SCXTEditor *);
    ~PartGroupSidebar();

    engine::Engine::pgzStructure_t pgzStructure;
    void setPartGroupZoneStructure(const engine::Engine::pgzStructure_t &p);
    void editorSelectionChanged();
    void selectedPartChanged();

    void selectParts() {}
    void selectGroups() {}

    void setSelectedTab(int t); // part / group / zone

    std::unique_ptr<ZoneSidebar> zoneSidebar;
    std::unique_ptr<GroupSidebar> groupSidebar;
    std::unique_ptr<PartSidebar> partSidebar;

    void partConfigurationChanged(int i);
    void groupTriggerConditionChanged(const scxt::engine::GroupTriggerConditions &);

    void setAllToOmniFlavor(int f);
    void updateMidiMenuLabel();
    void setMpeBendRange(int r);

    void showHamburgerMenu();

    void resized() override;
};
} // namespace scxt::ui::app::edit_screen

#endif // SHORTCIRCUIT_PARTGROUPSIDEBAR_H
