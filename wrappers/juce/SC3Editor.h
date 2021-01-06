/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "SC3Processor.h"
#include <JuceHeader.h>
#include "sample.h"
#include "components/DebugPanel.h"

//==============================================================================
/**
 */
class SC3AudioProcessorEditor : public juce::AudioProcessorEditor,
                                public juce::Button::Listener,
                                public juce::FileDragAndDropTarget
{
  public:
    SC3AudioProcessorEditor(SC3AudioProcessor &);
    ~SC3AudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

    // juce::Button::Listener interface
    virtual void buttonClicked(Button *) override;
    virtual void buttonStateChanged(Button *) override;

    // juce::FileDragAndDrop interface
    bool isInterestedInFileDrag(const StringArray &files) override;
    void filesDropped(const StringArray &files, int x, int y) override;

    // Fixme - obviously this is done with no thought of threading or anything else
    void rebuildUIState();

  private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SC3AudioProcessor &audioProcessor;

    std::unique_ptr<juce::Button> loadButton;
    std::unique_ptr<juce::Button> manualButton;
    bool manualPlaying;

    std::unique_ptr<DebugPanelWindow> debugWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SC3AudioProcessorEditor)
};
