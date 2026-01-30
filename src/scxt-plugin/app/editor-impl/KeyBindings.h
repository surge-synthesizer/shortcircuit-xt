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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDITOR_IMPL_KEYBINDINGS_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDITOR_IMPL_KEYBINDINGS_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "app/HasEditor.h"
#include "sst/plugininfra/keybindings.h"

namespace scxt::ui::app
{
enum KeyCommands : uint32_t
{
    SWITCH_GROUP_ZONE_SELECTION,

    FOCUS_PARTS,
    FOCUS_GROUPS,
    FOCUS_ZONES,
    FOCUS_MIXER,
    FOCUS_PLAY,

    numKeyCommands
};
struct KeyBindings : HasEditor
{
    using manager_t = sst::plugininfra::KeyMapManager<KeyCommands, (int)KeyCommands::numKeyCommands,
                                                      juce::KeyPress>;
    KeyBindings(SCXTEditor *);
    ~KeyBindings();

    void setupKeyBindings();

    std::string commandToString(KeyCommands);

    std::unique_ptr<manager_t> manager;
};
} // namespace scxt::ui::app
#endif // KEYBINDINGS_H
