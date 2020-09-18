/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "SC3Processor.h"
#include "SC3Editor.h"

//==============================================================================
SC3AudioProcessorEditor::SC3AudioProcessorEditor (SC3AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
}

SC3AudioProcessorEditor::~SC3AudioProcessorEditor()
{
}

//==============================================================================
void SC3AudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    juce::String s = "Harp Me";
    g.drawFittedText (s, getLocalBounds(), juce::Justification::centred, 1);
}

void SC3AudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
