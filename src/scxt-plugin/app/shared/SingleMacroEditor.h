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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_SHARED_SINGLEMACROEDITOR_H
#define SCXT_SRC_SCXT_PLUGIN_APP_SHARED_SINGLEMACROEDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/style/StyleAndSettingsConsumer.h"
#include "sst/jucegui/util/VisibilityParentWatcher.h"
#include "app/HasEditor.h"
#include "utils.h"

namespace scxt::ui::app::shared
{
struct MacroValueAttachment;
struct SingleMacroEditor : HasEditor,
                           juce::Component,
                           juce::TextEditor::Listener,
                           sst::jucegui::style::StyleConsumer
{
    std::unique_ptr<MacroValueAttachment> valueAttachment;
    std::unique_ptr<sst::jucegui::components::Knob> knob;
    std::unique_ptr<juce::Component> menuButton;
    std::unique_ptr<juce::TextEditor> macroNameEditor;
    std::unique_ptr<sst::jucegui::components::Label> macroNameLabel;
    SingleMacroEditor(SCXTEditor *e, int part, int index, bool valueOnly);
    ~SingleMacroEditor();
    void resized() override;
    void paint(juce::Graphics &g) override;
    void showMenu();
    void updateFromEditorData();
    void changePart(int p);

    void textEditorReturnKeyPressed(juce::TextEditor &editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &editor) override;
    void textEditorFocusLost(juce::TextEditor &editor) override;

    void onStyleChanged() override;

    void visibilityChanged() override { updateFromEditorData(); }

  private:
    int16_t part{-1}, index{-1};
    bool valueOnly{false};
    std::unique_ptr<sst::jucegui::util::VisibilityParentWatcher> visibilityWatcher;
};
} // namespace scxt::ui::app::shared
#endif // SHORTCIRCUITXT_SINGLEMACROEDITOR_H
