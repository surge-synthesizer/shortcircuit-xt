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
class SC3AudioProcessorEditor;

class DebugPanel : public juce::Component, public juce::Button::Listener
{
  public:
    DebugPanel() : Component( "Debug Panel" ) {
        int w = 500;
        int sH = 400;
        int lH = 200;
        setSize( w, sH + lH );

        loadButton = std::make_unique<juce::TextButton>("Load Sample");
        loadButton->setBounds(5, 2, 100, 20);
        loadButton->addListener(this);
        addAndMakeVisible(loadButton.get());

        manualButton = std::make_unique<juce::TextButton>("Play note");
        manualButton->setBounds(110, 2, 100, 20);
        manualButton->addListener(this);
        addAndMakeVisible(manualButton.get());

        noteNumber = std::make_unique<juce::TextEditor>("number");
        noteNumber->setText(std::to_string(playingNote));
        noteNumber->setBounds(215, 2, 60, 20);
        addAndMakeVisible(noteNumber.get());

        samplerT = std::make_unique<juce::TextEditor>();
        samplerT->setBounds(5, 25, w - 10, sH - 30);
        samplerT->setMultiLine(true, false );
        addAndMakeVisible(samplerT.get());

        logT = std::make_unique<juce::TextEditor>();
        logT->setBounds(5, 5 + sH, w - 10, lH - 30);
        logT->setMultiLine(true, false);
        addAndMakeVisible(logT.get());
    }

    void setSamplerText( const juce::String &s )
    {
        samplerT->setText(s);
    }

    void setEditor(SC3AudioProcessorEditor *ed) { this->ed = ed; }

    void appendLogText( const juce::String &s )
    {
        String str=s;
        str+= "\n";
        logT->setCaretPosition(logT->getText().length());
        logT->insertTextAtCaret(str);
    }

    virtual void buttonClicked(Button *b);
    virtual void buttonStateChanged(Button *b);

    std::unique_ptr<juce::TextEditor> samplerT;
    std::unique_ptr<juce::TextEditor> logT;

    std::unique_ptr<juce::Button> loadButton;
    std::unique_ptr<juce::Button> manualButton;
    std::unique_ptr<juce::TextEditor> noteNumber;
    bool manualPlaying = false;
    int playingNote = 60;

    SC3AudioProcessorEditor *ed;
};

class DebugPanelWindow : public juce::DocumentWindow
{
  public:
    DebugPanelWindow()
        : juce::DocumentWindow(juce::String("SC3 Debug - ") + SC3::Build::GitHash + "/" +
                                   SC3::Build::GitBranch,
                               juce::Colour(0xFF000000), juce::DocumentWindow::allButtons)
    {
        panel = new DebugPanel();
        setContentOwned(panel, true); // DebugPanelWindow takes ownership
        
        int baroff = 25;
        juce::Rectangle<int> area( 100, 100+baroff,  panel->getWidth(), panel->getHeight() + baroff );

        setBounds (area);
        panel->setBounds( 0, baroff, panel->getWidth(), panel->getHeight() );

        setResizable( false, false );
        setUsingNativeTitleBar( false );
    }

    DebugPanel *panel;
};

#endif // SHORTCIRCUIT_DEBUGPANEL_H
