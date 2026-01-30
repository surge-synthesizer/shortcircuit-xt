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

#include "KeyBindings.h"
#include "app/SCXTEditor.h"

namespace scxt::ui::app
{
KeyBindings::KeyBindings(SCXTEditor *e) : HasEditor(e)
{
    manager = std::make_unique<manager_t>(
        e->browser.userDirectory, "ShortcircuitXT", [this](auto f) { return commandToString(f); },
        [this](auto &t, auto &m) { editor->displayError(t, m); });

    setupKeyBindings();

    manager->unstreamFromXML();
}
KeyBindings::~KeyBindings() {}

void KeyBindings::setupKeyBindings()
{
    manager->addBinding(SWITCH_GROUP_ZONE_SELECTION,
                        {(uint32_t)manager_t::Modifiers::ALT, (int)'G'});

    manager->addBinding(FOCUS_ZONES, {(uint32_t)manager_t::Modifiers::COMMAND, (int)'1'});
    manager->addBinding(FOCUS_GROUPS, {(uint32_t)manager_t::Modifiers::COMMAND, (int)'2'});
    manager->addBinding(FOCUS_PARTS, {(uint32_t)manager_t::Modifiers::COMMAND, (int)'3'});
    manager->addBinding(FOCUS_MIXER, {(uint32_t)manager_t::Modifiers::COMMAND, (int)'4'});
    manager->addBinding(FOCUS_PLAY, {(uint32_t)manager_t::Modifiers::COMMAND, (int)'5'});
}

std::string KeyBindings::commandToString(KeyCommands c)
{
    switch (c)
    {
    case SWITCH_GROUP_ZONE_SELECTION:
        return "switchGroupZoneSelection";
    case FOCUS_PARTS:
        return "focusParts";
    case FOCUS_GROUPS:
        return "focusGroups";
    case FOCUS_ZONES:
        return "focusZones";
    case FOCUS_PLAY:
        return "focusPlay";
    case FOCUS_MIXER:
        return "focusMixer";
    case numKeyCommands:
        SCLOG_IF(warnings, "LOGIC ERROR Unstreaming Key Command");
    }
    return "LOGIC ERROR";
}

} // namespace scxt::ui::app
