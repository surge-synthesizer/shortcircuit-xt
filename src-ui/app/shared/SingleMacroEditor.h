/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_UI_COMPONENTS_MULTI_SINGLEMACROEDITOR_H
#define SCXT_SRC_UI_COMPONENTS_MULTI_SINGLEMACROEDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/style/StyleAndSettingsConsumer.h"
#include "app/HasEditor.h"

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

  private:
    int16_t part{-1}, index{-1};
    bool valueOnly{false};
};
} // namespace scxt::ui::app::shared
#endif // SHORTCIRCUITXT_SINGLEMACROEDITOR_H
