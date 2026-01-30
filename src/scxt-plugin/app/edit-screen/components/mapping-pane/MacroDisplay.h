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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_MACRODISPLAY_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_MACRODISPLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "configuration.h"
#include "app/shared/SingleMacroEditor.h"
#include "app/HasEditor.h"

namespace scxt::ui::app::edit_screen
{

struct MacroDisplay : HasEditor, juce::Component
{
    std::array<std::unique_ptr<shared::SingleMacroEditor>, scxt::macrosPerPart> macros;
    MacroDisplay(SCXTEditor *e);
    void resized() override;
    void selectedPartChanged();

    void macroDataChanged(int part, int index);
};
} // namespace scxt::ui::app::edit_screen

#endif // SHORTCIRCUITXT_MACRODISPLAY_H
