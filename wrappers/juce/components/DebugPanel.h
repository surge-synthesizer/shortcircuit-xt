/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_DEBUGPANEL_H
#define SHORTCIRCUIT_DEBUGPANEL_H

#include "juce_gui_basics/juce_gui_basics.h"

#include <string>
#include "version.h"

class SCXTEditor;

class DebugPanel : public juce::Component
{
    void resized() override;

  public:
    DebugPanel();
    SCXTEditor *mEditor;
    std::unique_ptr<juce::TextEditor> samplerT;
    std::unique_ptr<juce::TextEditor> logT;
    std::unique_ptr<juce::Label> warning;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DebugPanel);
};

class DebugPanelWindow : public juce::DocumentWindow
{
    DebugPanel *panel;

  public:
    DebugPanelWindow();

    void setSamplerText(const juce::String &s) { panel->samplerT->setText(s); }

    void setEditor(SCXTEditor *ed) { panel->mEditor = ed; }

    void appendLogText(const juce::String &s, bool appendNewline = true)
    {
        if (appendNewline)
        {
            auto str = s;
            str += "\n";
            panel->logT->setCaretPosition(panel->logT->getText().length());
            panel->logT->insertTextAtCaret(str);
        }
        else
        {
            panel->logT->setCaretPosition(panel->logT->getText().length());
            panel->logT->insertTextAtCaret(s);
        }
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DebugPanelWindow);
};

#endif // SHORTCIRCUIT_DEBUGPANEL_H
