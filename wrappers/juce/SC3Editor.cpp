/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "SC3Editor.h"
#include "SC3Processor.h"

//==============================================================================
SC3AudioProcessorEditor::SC3AudioProcessorEditor(SC3AudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(400, 300);

    loadButton = std::make_unique<juce::TextButton>("Load Sample");
    loadButton->setBounds(20, 20, 100, 20);
    loadButton->addListener(this);
    addAndMakeVisible(loadButton.get());
}

SC3AudioProcessorEditor::~SC3AudioProcessorEditor() {}

void SC3AudioProcessorEditor::buttonClicked(Button *b)
{
    if (b == loadButton.get())
    {
        juce::FileChooser sampleChooser(
            "Please choose a sample file",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory));
        if (sampleChooser.browseForFileToOpen())
        {
            auto f = sampleChooser.getResult();
            std::cout << f.getFullPathName() << std::endl;
            audioProcessor.sc3->load_file(f.getFullPathName().toStdString().c_str());
        }
    }
}

//==============================================================================
void SC3AudioProcessorEditor::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    juce::String s = "Harp Me";
    g.drawFittedText(s, getLocalBounds(), juce::Justification::centred, 1);
}

void SC3AudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
