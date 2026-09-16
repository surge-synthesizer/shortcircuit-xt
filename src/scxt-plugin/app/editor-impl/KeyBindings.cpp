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
#include "sst/jucegui/screens/KeyBindingEditor.h"

namespace scxt::ui::app
{
namespace jscr = sst::jucegui::screens;

KeyBindings::KeyBindings(SCXTEditor *e) : HasEditor(e)
{
    manager = std::make_unique<manager_t>(
        e->browser.userDirectory, "ShortcircuitXT", [](auto f) { return commandToString(f); },
        [this](auto &t, auto &m) { editor->displayError(t, m); });
    manager->streamDefaultBindings = false;

    setupKeyBindings();

    manager->unstreamFromXML();
}
KeyBindings::~KeyBindings() {}

void KeyBindings::setupKeyBindings()
{
    using mod = manager_t::Modifiers;

    manager->addBinding(UNDO, {(uint32_t)mod::COMMAND, (int)'Z'});
#if MAC
    manager->addBinding(REDO, {(uint32_t)(mod::COMMAND | mod::SHIFT), (int)'Z'});
#else
    manager->addBinding(REDO, {(uint32_t)mod::COMMAND, (int)'Y'});
#endif
    manager->addBinding(SHOW_KEYBINDINGS_EDITOR, {(uint32_t)mod::ALT, (int)'B'});
    manager->addBinding(SHOW_TUNING_EDITOR, {(uint32_t)mod::ALT, (int)'T'});

    manager->addBinding(FOCUS_ZONES, {(uint32_t)mod::COMMAND, (int)'1'});
    manager->addBinding(FOCUS_GROUPS, {(uint32_t)mod::COMMAND, (int)'2'});
    manager->addBinding(FOCUS_PARTS, {(uint32_t)mod::COMMAND, (int)'3'});
    manager->addBinding(FOCUS_MIXER, {(uint32_t)mod::COMMAND, (int)'4'});
    manager->addBinding(FOCUS_PLAY, {(uint32_t)mod::COMMAND, (int)'5'});
    manager->addBinding(SWITCH_GROUP_ZONE_SELECTION, {(uint32_t)mod::ALT, (int)'G'});

    manager->addBinding(SELECT_ALL, {(uint32_t)mod::COMMAND, (int)'A'});
    manager->addBinding(SELECT_PREVIOUS, {juce::KeyPress::upKey});
    manager->addBinding(SELECT_NEXT, {juce::KeyPress::downKey});
    manager->addBinding(ACTIVATE, {juce::KeyPress::returnKey});
    manager->addBinding(COLLAPSE, {juce::KeyPress::leftKey});
    manager->addBinding(EXPAND, {juce::KeyPress::rightKey});
    manager->addBinding(RENAME, {(uint32_t)mod::COMMAND, (int)'R'});
    manager->addBinding(COPY, {(uint32_t)mod::COMMAND, (int)'C'});
    manager->addBinding(PASTE, {(uint32_t)mod::COMMAND, (int)'V'});
    manager->addBinding(DUPLICATE, {(uint32_t)mod::COMMAND, (int)'D'});
#if MAC
    // the key marked delete on a mac keyboard is backspace
    manager->addBinding(DELETE_SELECTED, {juce::KeyPress::backspaceKey});
#else
    manager->addBinding(DELETE_SELECTED, {juce::KeyPress::deleteKey});
#endif
}

KeyCommandInfo KeyBindings::commandInfo(KeyCommands c)
{
    static constexpr const char *general{"General"}, *screens{"Screens"},
        *edit{"Selection and Clipboard"};
    using sc = KeyCommandScope;

    switch (c)
    {
    case UNDO:
        return {"undo", "Undo", general, sc::GLOBAL};
    case REDO:
        return {"redo", "Redo", general, sc::GLOBAL};
    case SHOW_KEYBINDINGS_EDITOR:
        return {"showKeyBindingsEditor", "Keyboard Shortcuts", general, sc::GLOBAL};
    case SHOW_TUNING_EDITOR:
        return {"showTuningEditor", "Tuning Editor", general, sc::GLOBAL};
    case SHOW_LOG:
        return {"showLog", "Show Log", general, sc::GLOBAL};
    case SHOW_ABOUT:
        return {"showAbout", "About Shortcircuit XT", general, sc::GLOBAL};

    case FOCUS_PLAY:
        return {"focusPlay", "Play Screen", screens, sc::GLOBAL};
    case FOCUS_PARTS:
        return {"focusParts", "Edit Parts", screens, sc::GLOBAL};
    case FOCUS_GROUPS:
        return {"focusGroups", "Edit Groups", screens, sc::GLOBAL};
    case FOCUS_ZONES:
        return {"focusZones", "Edit Zones", screens, sc::GLOBAL};
    case FOCUS_MIXER:
        return {"focusMixer", "Mixer Screen", screens, sc::GLOBAL};
    case SWITCH_GROUP_ZONE_SELECTION:
        return {"switchGroupZoneSelection", "Swap Group and Zone Editing", screens, sc::GLOBAL};

    case SELECT_ALL:
        return {"selectAll", "Select All", edit, sc::EDIT};
    case SELECT_PREVIOUS:
        return {"selectPrevious", "Select Previous", edit, sc::EDIT};
    case SELECT_NEXT:
        return {"selectNext", "Select Next", edit, sc::EDIT};
    case ACTIVATE:
        return {"activate", "Open or Load", edit, sc::EDIT};
    case COLLAPSE:
        return {"collapse", "Collapse", edit, sc::EDIT};
    case EXPAND:
        return {"expand", "Expand", edit, sc::EDIT};
    case RENAME:
        return {"rename", "Rename", edit, sc::EDIT};
    case COPY:
        return {"copy", "Copy", edit, sc::EDIT};
    case PASTE:
        return {"paste", "Paste", edit, sc::EDIT};
    case DUPLICATE:
        return {"duplicate", "Duplicate", edit, sc::EDIT};
    case DELETE_SELECTED:
        return {"deleteSelected", "Delete", edit, sc::EDIT};

    case numKeyCommands:
        SCLOG_IF(warnings, "LOGIC ERROR Unstreaming Key Command");
        break;
    }
    return {"", "", "", sc::GLOBAL};
}

std::string KeyBindings::commandToString(KeyCommands c) { return commandInfo(c).streamName; }

std::vector<KeyCommands> KeyBindings::matchingCommands(const juce::KeyPress &key) const
{
    std::vector<KeyCommands> res, looseShift;
    for (const auto &[c, b] : manager->bindings)
    {
        if (!b.matches(key))
            continue;

        // linux matching lets shift slip when another modifier is held
        auto wantsShift = (b.modifier & manager_t::SHIFT) != 0;
        if (wantsShift == key.getModifiers().isShiftDown())
            res.push_back(c);
        else
            looseShift.push_back(c);
    }
    res.insert(res.end(), looseShift.begin(), looseShift.end());
    return res;
}

std::string KeyBindings::shortcutDescription(KeyCommands c) const
{
    auto b = manager->bindings.find(c);
    if (b == manager->bindings.end() || !b->second.active)
        return {};
    return jscr::KeyBindingEditor::fromKeyMapBinding<manager_t>(b->second).toDisplayString();
}

void KeyBindings::showEditor()
{
    if (editor->searchForOverlay<jscr::KeyBindingEditorModal>())
        return;

    auto entries = jscr::KeyBindingEditor::entriesFromKeyMapManager(
        *manager, [](int i) { return std::string(commandInfo((KeyCommands)i).displayName); },
        [](int i) { return std::string(commandInfo((KeyCommands)i).category); },
        [](int i) {
            return commandInfo((KeyCommands)i).scope == KeyCommandScope::GLOBAL ? std::string()
                                                                                : "edit";
        });

    auto modal = std::make_unique<jscr::KeyBindingEditorModal>(std::move(entries));
    // the scroll bar bakes its colour in at construction, so hand it the style now
    modal->setStyle(editor->style());
    modal->onOK = [w = juce::Component::SafePointer(editor)](const auto &result) {
        if (!w)
            return;
        auto &m = *w->keyBindings->manager;
        jscr::KeyBindingEditor::applyEntriesToKeyMapManager(m, result);
        m.streamToXML();
    };
    editor->displayModalOverlay(std::move(modal));
}

} // namespace scxt::ui::app
