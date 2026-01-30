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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_SHARED_MENUVALUETYPEIN_H
#define SCXT_SRC_SCXT_PLUGIN_APP_SHARED_MENUVALUETYPEIN_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <sst/jucegui/components/ContinuousParamEditor.h>
#include "app/HasEditor.h"

namespace scxt::ui::app::shared
{

struct MenuValueTypeinBase : HasEditor, juce::PopupMenu::CustomComponent, juce::TextEditor::Listener
{
    std::unique_ptr<juce::TextEditor> textEditor;
    MenuValueTypeinBase(SCXTEditor *editor);

    void getIdealSize(int &w, int &h) override
    {
        w = 180;
        h = 22;
    }
    void resized() override { textEditor->setBounds(getLocalBounds().reduced(3, 1)); }

    void visibilityChanged() override
    {
        juce::Timer::callAfterDelay(2, [this]() {
            if (textEditor->isVisible())
            {
                textEditor->setText(getInitialText(), juce::NotificationType::dontSendNotification);
                setupTextEditorStyle();
                textEditor->grabKeyboardFocus();
                textEditor->selectAll();
            }
        });
    }

    virtual std::string getInitialText() const = 0;
    virtual void setValueString(const std::string &) = 0;
    virtual juce::Colour getValueColour() const;

    void setupTextEditorStyle();

    void textEditorReturnKeyPressed(juce::TextEditor &ed) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &) override { triggerMenuItem(); }
};

struct MenuValueTypein : MenuValueTypeinBase
{
    juce::Component::SafePointer<sst::jucegui::components::ContinuousParamEditor> underComp;

    MenuValueTypein(
        SCXTEditor *editor,
        juce::Component::SafePointer<sst::jucegui::components::ContinuousParamEditor> under)
        : MenuValueTypeinBase(editor), underComp(under)
    {
    }

    std::string getInitialText() const override
    {
        return underComp->continuous()->getValueAsString();
    }

    void setValueString(const std::string &s) override
    {
        if (underComp && underComp->continuous())
        {
            if (s.empty())
            {
                underComp->continuous()->setValueFromGUI(
                    underComp->continuous()->getDefaultValue());
            }
            else
            {
                underComp->continuous()->setValueAsString(s);
            }
        }
    }

    juce::Colour getValueColour() const override;
};
} // namespace scxt::ui::app::shared
#endif // MENUVALUETYPEIN_H
