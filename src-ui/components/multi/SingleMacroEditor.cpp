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

#include "SingleMacroEditor.h"
#include "components/SCXTEditor.h"
#include "connectors/PayloadDataAttachment.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "messaging/client/client_messages.h"

namespace scxt::ui::multi
{
struct MacroValueAttachment : HasEditor, sst::jucegui::data::Continuous
{
    int part{-1}, index{-1};
    MacroValueAttachment(SCXTEditor *e, int p, int i) : HasEditor(e), part(p), index(i) {}

    const engine::Macro &macro() const
    {
        assert(part >= 0 && part < scxt::numParts);
        assert(index >= 0 && index < scxt::macrosPerPart);
        return editor->macroCache[part][index];
    }

    engine::Macro &macro()
    {
        assert(part >= 0 && part < scxt::numParts);
        assert(index >= 0 && index < scxt::macrosPerPart);
        return editor->macroCache[part][index];
    }

    std::string getLabel() const override { return macro().name; }
    float getValue() const override { return macro().normalizedValue; }
    void setValueFromGUI(const float &f) override
    {
        macro().normalizedValue = f;
        sendToSerialization(scxt::messaging::client::SetMacroValue({part, index, f}));
        editor->setTooltipContents(getLabel(), getValueAsString());
    }
    void setValueFromModel(const float &) override {}
    float getDefaultValue() const override { return 0; }
};

struct NarrowVerticalMenu : HasEditor, juce::Component
{
    bool isHovered{false};
    NarrowVerticalMenu(SCXTEditor *e) : HasEditor(e) {}

    void mouseEnter(const juce::MouseEvent &event) override
    {
        isHovered = true;
        repaint();
    }
    void mouseExit(const juce::MouseEvent &event) override
    {
        isHovered = false;
        repaint();
    }
    void mouseDown(const juce::MouseEvent &event) override
    {
        if (cb)
            cb();
    }
    void paint(juce::Graphics &g) override
    {
        auto b = getLocalBounds();
        b.expand((getHeight() - getWidth()) / 2, 0);
        auto col = isHovered ? theme::ColorMap::generic_content_highest
                             : theme::ColorMap::generic_content_high;
        sst::jucegui::components::GlyphPainter::paintGlyph(
            g, b, sst::jucegui::components::GlyphPainter::ELLIPSIS_V, editor->themeColor(col));
    }
    std::function<void()> cb;
};
SingleMacroEditor::SingleMacroEditor(SCXTEditor *e, int p, int i)
    : HasEditor(e),
      sst::jucegui::style::StyleConsumer(sst::jucegui::components::NamedPanel::Styles::styleClass),
      part(p), index(i)
{
    auto mb = std::make_unique<NarrowVerticalMenu>(e);
    addAndMakeVisible(*mb);
    mb->cb = [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showMenu();
    };
    menuButton = std::move(mb);
    knob = connectors::makeConnectedToDummy<sst::jucegui::components::Knob>('mcro');
    addAndMakeVisible(*knob);

    knob->onBeginEdit = [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->editor->showTooltip(*(w->knob));
        w->editor->setTooltipContents(w->valueAttachment->getLabel(),
                                      w->valueAttachment->getValueAsString());
    };
    knob->onEndEdit = [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->editor->hideTooltip();
    };
    knob->onIdleHover = knob->onBeginEdit;
    knob->onIdleHoverEnd = knob->onEndEdit;

    valueAttachment = std::make_unique<MacroValueAttachment>(editor, part, index);
    knob->setSource(valueAttachment.get());

    macroNameEditor = std::make_unique<juce::TextEditor>("Name");
    macroNameEditor->setText("Macro");
    macroNameEditor->setJustification(juce::Justification::centred);
    macroNameEditor->addListener(this);
    addAndMakeVisible(*macroNameEditor);
}

SingleMacroEditor::~SingleMacroEditor() {}

void SingleMacroEditor::changePart(int p)
{
    valueAttachment->part = p;
    repaint();
}

void SingleMacroEditor::resized()
{
    auto b = getLocalBounds();
    knob->setBounds(b.withHeight(b.getWidth()).reduced(15));
    menuButton->setBounds(b.withWidth(12).withHeight(24).translated(0, 10));
    macroNameEditor->setBounds(
        b.withTrimmedTop(b.getWidth() - 12 + 3).withHeight(18).reduced(3, 0));
}

void SingleMacroEditor::paint(juce::Graphics &g)
{
#if 0
    g.setFont(12);
    g.setColour(juce::Colours::red);
    auto txt = std::to_string(part) + "/" + std::to_string(index);
    g.drawText(txt, getLocalBounds(), juce::Justification::topRight);
#endif
}

void SingleMacroEditor::showMenu() { SCLOG("Show Menu"); }
void SingleMacroEditor::onStyleChanged()
{
    // Update the text editor
    macroNameEditor->setFont(editor->themeApplier.interMediumFor(12));
    macroNameEditor->setColour(juce::TextEditor::ColourIds::textColourId,
                               editor->themeColor(theme::ColorMap::generic_content_high));
    macroNameEditor->setColour(juce::TextEditor::ColourIds::backgroundColourId,
                               editor->themeColor(theme::ColorMap::bg_1));
    macroNameEditor->setColour(juce::TextEditor::ColourIds::outlineColourId,
                               juce::Colours::black.withAlpha(0.f));
    macroNameEditor->setColour(juce::TextEditor::ColourIds::focusedOutlineColourId,
                               juce::Colours::black.withAlpha(0.f));
    macroNameEditor->applyColourToAllText(editor->themeColor(theme::ColorMap::generic_content_high),
                                          true);
    macroNameEditor->applyFontToAllText(editor->themeApplier.interMediumFor(13), true);
}

void SingleMacroEditor::updateFromEditorData()
{
    assert(part >= 0 && part < scxt::numParts);
    assert(index >= 0 && index < scxt::macrosPerPart);
    const auto &macro = editor->macroCache[part][index];
    macroNameEditor->setText(macro.name, juce::NotificationType::dontSendNotification);
}
void SingleMacroEditor::textEditorReturnKeyPressed(juce::TextEditor &e)
{
    auto &macro = editor->macroCache[part][index];
    macro.name = e.getText().toStdString();
    sendToSerialization(scxt::messaging::client::SetMacroFullState({part, index, macro}));
}

void SingleMacroEditor::textEditorEscapeKeyPressed(juce::TextEditor &e)
{
    const auto &macro = editor->macroCache[part][index];
    macroNameEditor->setText(macro.name, juce::NotificationType::dontSendNotification);
}

void SingleMacroEditor::textEditorFocusLost(juce::TextEditor &e)
{
    auto &macro = editor->macroCache[part][index];
    macro.name = e.getText().toStdString();
    sendToSerialization(scxt::messaging::client::SetMacroFullState({part, index, macro}));
}

} // namespace scxt::ui::multi