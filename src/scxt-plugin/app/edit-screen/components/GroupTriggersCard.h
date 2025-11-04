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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_GROUPTRIGGERSCARD_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_GROUPTRIGGERSCARD_H

#include <array>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "engine/group_triggers.h"

#include "configuration.h"
#include "app/HasEditor.h"

namespace scxt::ui::app::edit_screen
{
struct GroupTriggersCard : juce::Component, HasEditor
{
    struct ConditionRow;
    std::array<std::unique_ptr<ConditionRow>, scxt::triggerConditionsPerGroup> rows;
    GroupTriggersCard(SCXTEditor *e);
    ~GroupTriggersCard();
    void paint(juce::Graphics &g) override;
    void resized() override;

    void setGroupTriggerConditions(const scxt::engine::GroupTriggerConditions &);
    void pushUpdate();
    scxt::engine::GroupTriggerConditions cond;
};
} // namespace scxt::ui::app::edit_screen
#endif // GROUPTRIGGERSCARD_H
