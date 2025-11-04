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