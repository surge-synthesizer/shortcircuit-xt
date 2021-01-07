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

class DebugPanel : public juce::Component
{
  public:
    DebugPanel() : Component( "Debug Panel" ) {
        int w = 500;
        int sH = 400;
        int lH = 200;
        setSize( w, sH + lH );

        samplerT = std::make_unique<juce::TextEditor>();
        samplerT->setBounds( 5, 5, w - 10, sH - 10 );
        samplerT->setMultiLine(true, false );
        addAndMakeVisible(samplerT.get());

        logT = std::make_unique<juce::TextEditor>();
        logT->setBounds( 5, 5 + sH, w - 10, lH - 10 );
        logT->setMultiLine(true, false);
        addAndMakeVisible(logT.get());
    }

    void setSamplerText( const juce::String &s )
    {
        samplerT->setText(s);
        

    }

    void appendLogText( const juce::String &s )
    {
        String str=s;
        str+= "\n";
        logT->setCaretPosition(logT->getText().length());
        logT->insertTextAtCaret(str);
    }

    std::unique_ptr<juce::TextEditor> samplerT;
    std::unique_ptr<juce::TextEditor> logT;
};

class DebugPanelWindow : public juce::DocumentWindow
{
  public:
    DebugPanelWindow()
        : juce::DocumentWindow("SC3 Debug Window", juce::Colour(0xFF000000),
                               juce::DocumentWindow::allButtons)
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
