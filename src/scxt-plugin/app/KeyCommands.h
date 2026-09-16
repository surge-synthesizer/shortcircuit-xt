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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_KEYCOMMANDS_H
#define SCXT_SRC_SCXT_PLUGIN_APP_KEYCOMMANDS_H

#include <cstdint>

namespace scxt::ui::app
{
// the order here is the order of the shortcut editor; the stream names are what persist
enum KeyCommands : uint32_t
{
    UNDO,
    REDO,
    SHOW_KEYBINDINGS_EDITOR,
    SHOW_TUNING_EDITOR,
    SHOW_LOG,
    SHOW_ABOUT,

    FOCUS_PLAY,
    FOCUS_PARTS,
    FOCUS_GROUPS,
    FOCUS_ZONES,
    FOCUS_MIXER,
    SWITCH_GROUP_ZONE_SELECTION,

    SELECT_ALL,
    SELECT_PREVIOUS,
    SELECT_NEXT,
    ACTIVATE,
    COLLAPSE,
    EXPAND,
    RENAME,
    COPY,
    CUT,
    PASTE,
    DUPLICATE,
    DELETE_SELECTED,

    numKeyCommands
};

enum struct KeyCommandScope
{
    // handled by the editor wherever focus is
    GLOBAL,
    // handled by whichever focused component knows the selection it applies to
    EDIT
};

struct KeyCommandInfo
{
    // persisted in the user's key map, so never change one
    const char *streamName;
    const char *displayName;
    const char *category;
    KeyCommandScope scope;
};

// the focused component and its parents are offered a command before the editor
struct KeyCommandTarget
{
    virtual ~KeyCommandTarget() = default;
    virtual bool handleKeyCommand(KeyCommands command) = 0;
};
} // namespace scxt::ui::app

#endif // SCXT_SRC_SCXT_PLUGIN_APP_KEYCOMMANDS_H
