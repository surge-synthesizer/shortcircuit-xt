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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_GROUPTRIGGERSCARD_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_GROUPTRIGGERSCARD_H

#include <array>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include <sst/jucegui/components/Label.h>
#include <sst/jucegui/components/TextPushButton.h>

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

    // the keyswitch row waiting on a played key, or -1
    int learningRow{-1};
    void setLearningRow(int row);
    void noteLearned(int16_t key);

    /*
     * The release trigger sits above the conditions rather than among them: it says which note
     * event asks them, not whether they hold. The widgets want bools and the engine carries an
     * enum, so the two are kept in step by hand.
     */
    struct ReleaseRow;
    std::unique_ptr<ReleaseRow> releaseRow;
    bool releaseTriggerOn{false}, pedalTriggerOn{false};

    // the release row and the rule under it, above the condition stack
    static constexpr int releaseBlockHeight{26};

    /*
     * A multi-group selection whose conditions are shaped differently row for row. The card stays
     * live on the lead - editing one group of a mixed selection is a fair thing to want - but an
     * edit only reaches the others where the rows line up, so the strip says so and offers to
     * flatten them. It costs height only while it is showing, hence the query for the sidebar.
     */
    std::unique_ptr<sst::jucegui::components::Label> mixedLabel;
    std::unique_ptr<sst::jucegui::components::TextPushButton> makeConsistentButton;
    bool structureMixed() const { return !cond.structureConsistent; }
    static constexpr int warningStripHeight{22};
};
} // namespace scxt::ui::app::edit_screen
#endif // GROUPTRIGGERSCARD_H
