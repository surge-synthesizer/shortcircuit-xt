/*
** ShortCircuit3 is Free and Open Source Software
**
** ShortCircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** ShortCircuit was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made ShortCircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_DEBUGPANEL_H
#define SHORTCIRCUIT_DEBUGPANEL_H

#include <JuceHeader.h>
#include <string>
#include "version.h"
#include "scratchpad.h"

class SC3AudioProcessorEditor;

class ActionRunner : public Component,
                     public juce::Button::Listener,
                     public juce::ComboBox::Listener
{

    void buttonClicked(Button *b) override;
    void resized() override;
    void comboBoxChanged(ComboBox *comboBoxThatHasChanged) override;
    std::unique_ptr<juce::TextButton> mSendActionBtn;
    std::unique_ptr<juce::TextEditor> mParameters;
    std::unique_ptr<juce::ComboBox> mActionList;
    std::unique_ptr<juce::TextEditor> mDescription;
    std::vector<ScratchPadItem *> mItems;

  public:
    ActionRunner();

    SC3AudioProcessorEditor *mEditor;
};

class DebugPanel : public juce::Component
{
    void resized() override;

  public:
    DebugPanel();
    std::unique_ptr<ActionRunner> mActionRunner;
    SC3AudioProcessorEditor *mEditor;
    std::unique_ptr<juce::TextEditor> samplerT;
    std::unique_ptr<juce::TextEditor> logT;
};

class DebugPanelWindow : public juce::DocumentWindow
{
    DebugPanel *panel;

  public:
    DebugPanelWindow();

    void setSamplerText(const juce::String &s) { panel->samplerT->setText(s); }

    void setEditor(SC3AudioProcessorEditor *ed)
    {
        panel->mEditor = ed;
        panel->mActionRunner->mEditor = ed;
    }

    void appendLogText(const juce::String &s)
    {
        String str = s;
        str += "\n";
        panel->logT->setCaretPosition(panel->logT->getText().length());
        panel->logT->insertTextAtCaret(str);
    }
};

#endif // SHORTCIRCUIT_DEBUGPANEL_H
