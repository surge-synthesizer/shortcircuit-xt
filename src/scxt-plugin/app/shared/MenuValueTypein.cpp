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

#include "MenuValueTypein.h"
#include "app/SCXTEditor.h"

namespace scxt::ui::app::shared
{

MenuValueTypeinBase::MenuValueTypeinBase(SCXTEditor *editor)
    : HasEditor(editor), juce::PopupMenu::CustomComponent(false)
{
    textEditor = std::make_unique<juce::TextEditor>();
    textEditor->setWantsKeyboardFocus(true);
    textEditor->addListener(this);
    textEditor->setIndents(2, 0);

    addAndMakeVisible(*textEditor);
}

void MenuValueTypeinBase::textEditorReturnKeyPressed(juce::TextEditor &ed)
{
    auto s = ed.getText().toStdString();
    setValueString(s);
    triggerMenuItem();
}

juce::Colour MenuValueTypeinBase::getValueColour() const
{
    auto valCol = editor->style()->getColour(
        sst::jucegui::components::ContinuousParamEditor::Styles::styleClass,
        sst::jucegui::components::ContinuousParamEditor::Styles::value);

    return valCol;
}

void MenuValueTypeinBase::setupTextEditorStyle()
{
    auto valCol = getValueColour();

    textEditor->setColour(juce::TextEditor::ColourIds::backgroundColourId, valCol.withAlpha(0.2f));
    textEditor->setColour(juce::TextEditor::ColourIds::highlightColourId, valCol.withAlpha(0.32f));
    textEditor->setJustification(juce::Justification::centredLeft);
    textEditor->setColour(juce::TextEditor::ColourIds::outlineColourId,
                          juce::Colours::black.withAlpha(0.f));
    textEditor->setColour(juce::TextEditor::ColourIds::focusedOutlineColourId,
                          juce::Colours::black.withAlpha(0.f));
    textEditor->setBorder(juce::BorderSize<int>(3));
    textEditor->applyColourToAllText(editor->themeColor(theme::ColorMap::generic_content_high),
                                     true);
    textEditor->applyFontToAllText(editor->themeApplier.interMediumFor(13), true);
}

juce::Colour MenuValueTypein::getValueColour() const
{
    auto valCol = MenuValueTypeinBase::getValueColour();
    if (auto styleCon =
            dynamic_cast<sst::jucegui::style::StyleConsumer *>(underComp.getComponent()))
    {
        valCol =
            styleCon->getColour(sst::jucegui::components::ContinuousParamEditor::Styles::value);
    }
    return valCol;
}

} // namespace scxt::ui::app::shared