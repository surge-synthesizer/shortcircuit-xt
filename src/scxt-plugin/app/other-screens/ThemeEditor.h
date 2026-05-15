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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_OTHER_SCREENS_THEMEEDITOR_H
#define SCXT_SRC_SCXT_PLUGIN_APP_OTHER_SCREENS_THEMEEDITOR_H

#include <memory>
#include <juce_gui_basics/juce_gui_basics.h>

#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/screens/ColorEditor.h"

#include "app/HasEditor.h"

namespace scxt::ui::app::other_screens
{
struct ThemeEditor : sst::jucegui::components::WindowPanel, HasEditor, juce::Timer
{
    ThemeEditor(SCXTEditor *e);
    ~ThemeEditor() override;

    void resized() override;

    // Rebuild the inner ColorEditor from editor->themeApplier.currentColorMap().
    // Call after a load-theme swaps the ColorMap under us.
    void rebuildFromThemeApplier();

    // Debounce: each color edit (re)starts the timer; on fire we serialize
    // the current ColorMap and send it to the engine so it lives on DAW
    // extra state for this session.
    void timerCallback() override;

    std::unique_ptr<sst::jucegui::screens::ColorEditor> colorEditor;
    std::unique_ptr<sst::jucegui::components::TextPushButton> loadButton, saveButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeEditor);
};

// Hosts a ThemeEditor in its own OS window so the user can edit colours while
// interacting with the plugin editor.
struct ThemeEditorWindow : juce::DocumentWindow
{
    ThemeEditorWindow(SCXTEditor *e);
    ~ThemeEditorWindow() override;

    void closeButtonPressed() override { setVisible(false); }

    void rebuildFromThemeApplier();

    std::unique_ptr<ThemeEditor> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeEditorWindow);
};
} // namespace scxt::ui::app::other_screens

#endif // SCXT_SRC_SCXT_PLUGIN_APP_OTHER_SCREENS_THEMEEDITOR_H
