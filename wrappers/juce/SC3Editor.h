/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SC3Processor.h"

//==============================================================================
/**
*/
class SC3AudioProcessorEditor  : public juce::AudioProcessorEditor, juce::Button::Listener
{
public:
    SC3AudioProcessorEditor (SC3AudioProcessor&);
    ~SC3AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    virtual void buttonClicked( Button * ) override;
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SC3AudioProcessor& audioProcessor;

    std::unique_ptr<juce::Button> loadButton;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SC3AudioProcessorEditor)
};
