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
#include "app/SCXTEditor.h"
#include "connectors/PayloadDataAttachment.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/component-adapters/ComponentTags.h"
#include "messaging/client/client_messages.h"

namespace scxt::ui::app::shared
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
    float getValue() const override { return macro().value; }
    void setValueFromGUI(const float &f) override
    {
        macro().setValueConstrained(f);
        sendToSerialization(scxt::messaging::client::SetMacroValue({part, index, macro().value}));
        editor->setTooltipContents(getLabel(), getValueAsString());
    }
    void setValueFromModel(const float &) override {}
    float getDefaultValue() const override { return 0; }
    bool isBipolar() const override { return macro().isBipolar; }
    float getMin() const override
    {
        if (macro().isBipolar)
            return -1;
        else
            return 0;
    }
    float getMax() const override { return 1; }
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
SingleMacroEditor::SingleMacroEditor(SCXTEditor *e, int p, int i, bool vo)
    : HasEditor(e),
      sst::jucegui::style::StyleConsumer(sst::jucegui::components::NamedPanel::Styles::styleClass),
      part(p), index(i), valueOnly(vo)
{
    knob = std::make_unique<sst::jucegui::components::Knob>();
    knob->setDrawLabel(false);
    addAndMakeVisible(*knob);

    knob->onBeginEdit = [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->editor->showTooltip(*(w->knob));
        w->editor->setTooltipContents(w->valueAttachment->getLabel(),
                                      w->valueAttachment->getValueAsString());
        w->sendToSerialization(messaging::client::MacroBeginEndEdit({true, w->part, w->index}));
    };
    knob->onEndEdit = [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->editor->hideTooltip();
        w->sendToSerialization(messaging::client::MacroBeginEndEdit({false, w->part, w->index}));
    };

    knob->onIdleHover = [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->editor->showTooltip(*(w->knob));
        w->editor->setTooltipContents(w->valueAttachment->getLabel(),
                                      w->valueAttachment->getValueAsString());
    };
    knob->onIdleHoverEnd = [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->editor->hideTooltip();
    };

    knob->onWheelEditOccurred = [w = juce::Component::SafePointer(knob.get())]() {
        if (w)
            w->immediatelyInitiateIdleAction(1000);
    };
    knob->onPopupMenu = [this, q = juce::Component::SafePointer(knob.get())](auto &mods) {
        editor->hideTooltip();
        editor->popupMenuForContinuous(q);
    };

    sst::jucegui::component_adapters::setClapParamId(knob.get(),
                                                     engine::Macro::partIndexToMacroID(p, i));

    valueAttachment = std::make_unique<MacroValueAttachment>(editor, part, index);
    knob->setSource(valueAttachment.get());

    if (valueOnly)
    {
        macroNameLabel = std::make_unique<sst::jucegui::components::Label>();
        addAndMakeVisible(*macroNameLabel);
    }
    else
    {
        auto mb = std::make_unique<NarrowVerticalMenu>(e);
        addAndMakeVisible(*mb);
        mb->cb = [w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showMenu();
        };
        menuButton = std::move(mb);

        macroNameEditor = std::make_unique<juce::TextEditor>("Name");
        macroNameEditor->setText("Macro");
        macroNameEditor->setJustification(juce::Justification::centred);
        macroNameEditor->addListener(this);
        addAndMakeVisible(*macroNameEditor);
    }

    visibilityWatcher = std::make_unique<sst::jucegui::util::VisibilityParentWatcher>(this);
}

SingleMacroEditor::~SingleMacroEditor() {}

void SingleMacroEditor::changePart(int p)
{
    valueAttachment->part = p;
    part = p;
    repaint();
}

void SingleMacroEditor::resized()
{
    if (valueOnly)
    {
        auto b = getLocalBounds().reduced(2);
        auto kw = b.getHeight() - 18;
        macroNameLabel->setBounds(getLocalBounds().withTrimmedTop(kw));

        auto kb = getLocalBounds().withHeight(kw - 3);
        if (kb.getWidth() > kb.getHeight())
        {
            auto overShoot = kb.getWidth() - kb.getHeight();
            kb = kb.reduced(overShoot / 2, 0);
        }
        knob->setBounds(kb);
    }
    else
    {
        auto b = getLocalBounds();
        knob->setBounds(b.withHeight(b.getWidth()).reduced(15));
        menuButton->setBounds(b.withWidth(12).withHeight(24).translated(0, 10));
        macroNameEditor->setBounds(
            b.withTrimmedTop(b.getWidth() - 12 + 3).withHeight(18).reduced(3, 0));
    }
}

void SingleMacroEditor::paint(juce::Graphics &g)
{
#if 0
    g.setFont(12);
    g.setColour(juce::Colours::red);
    auto txt = std::to_string(part) + "/" + std::to_string(index);
    g.drawText(txt, getLocalBounds(), juce::Justification::topRight);
    g.drawRect(getLocalBounds());
#endif
}

void SingleMacroEditor::showMenu()
{
    assert(part >= 0 && part < scxt::numParts);
    assert(index >= 0 && index < scxt::macrosPerPart);
    const auto &macro = editor->macroCache[part][index];
    juce::PopupMenu p;
    p.addSectionHeader(macro.name);
    p.addSeparator();
    p.addItem("Bipolar", true, macro.isBipolar, [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        auto &macro = w->editor->macroCache[w->part][w->index];
        macro.setIsBipolar(!macro.isBipolar);
        w->sendToSerialization(
            scxt::messaging::client::SetMacroFullState({w->part, w->index, macro}));
        w->repaint();
    });
    p.showMenuAsync(editor->defaultPopupMenuOptions(menuButton.get()));
}
void SingleMacroEditor::onStyleChanged()
{
    if (macroNameEditor)
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
        macroNameEditor->applyColourToAllText(
            editor->themeColor(theme::ColorMap::generic_content_high), true);
        macroNameEditor->applyFontToAllText(editor->themeApplier.interMediumFor(13), true);
    }
}

void SingleMacroEditor::updateFromEditorData()
{
    assert(part >= 0 && part < scxt::numParts);
    assert(index >= 0 && index < scxt::macrosPerPart);
    const auto &macro = editor->macroCache[part][index];
    if (macroNameEditor)
    {
        macroNameEditor->setText(macro.name, juce::NotificationType::dontSendNotification);
    }
    if (macroNameLabel)
    {
        macroNameLabel->setText(macro.name);
    }
    repaint();
}
void SingleMacroEditor::textEditorReturnKeyPressed(juce::TextEditor &e)
{
    auto &macro = editor->macroCache[part][index];
    macro.name = e.getText().toStdString();
    if (macro.name.empty())
    {
        macro.name = engine::Macro::defaultNameFor(index);
        e.setText(macro.name, juce::NotificationType::dontSendNotification);
    }
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
    if (macro.name.empty())
    {
        macro.name = engine::Macro::defaultNameFor(index);
        e.setText(macro.name, juce::NotificationType::dontSendNotification);
    }
    sendToSerialization(scxt::messaging::client::SetMacroFullState({part, index, macro}));
}

} // namespace scxt::ui::app::shared