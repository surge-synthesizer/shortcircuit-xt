//
// Created by Paul Walker on 7/28/25.
//

#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

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
};     // namespace scxt::ui::app
#endif // KEYBINDINGS_H
