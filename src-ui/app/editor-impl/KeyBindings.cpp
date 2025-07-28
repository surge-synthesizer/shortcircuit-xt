//
// Created by Paul Walker on 7/28/25.
//

#include "KeyBindings.h"

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
KeyBindings::~KeyBindings(){};

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
        SCLOG("LOGIC ERROR Unstreaming Key Command");
    }
    return "LOGIC ERROR";
}

} // namespace scxt::ui::app