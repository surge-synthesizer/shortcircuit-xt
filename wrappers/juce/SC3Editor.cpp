/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "SC3Editor.h"
#include "SC3Processor.h"
#include "version.h"

//==============================================================================
SC3AudioProcessorEditor::SC3AudioProcessorEditor(SC3AudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p), manualPlaying(false)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(900, 600);

    loadButton = std::make_unique<juce::TextButton>("Load Sample");
    loadButton->setBounds(5, 5, 100, 20);
    loadButton->addListener(this);
    addAndMakeVisible(loadButton.get());

    manualButton = std::make_unique<juce::TextButton>("Play note");
    manualButton->setBounds(110, 5, 100, 20);
    manualButton->addListener(this);
    addAndMakeVisible(manualButton.get());

    zoneListBoxModel = std::make_unique<ZoneListBoxModel>(this);
    zoneList = std::make_unique<juce::TableListBox>();
    zoneList->setBounds( 5, 30, 800, 400 );
    zoneList->setModel( zoneListBoxModel.get() );
    int cid = 0;
    zoneList->getHeader().addColumn("Zone ID", cid++, 60 );
    zoneList->getHeader().addColumn( "Sample ID", cid++, 60 );
    zoneList->getHeader().addColumn( "Sample Name", cid++, 400 );
    zoneList->getHeader().addColumn( "midiStart", cid++, 60 );
    zoneList->getHeader().addColumn( "midiEnd", cid++, 60 );
    addAndMakeVisible(zoneList.get());

    rebuildUIState();
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
            audioProcessor.sc3->load_file(string_to_path(f.getFullPathName().toStdString()));
            rebuildUIState();
        }
    }
}

void SC3AudioProcessorEditor::buttonStateChanged(Button *b)
{
    if (b == manualButton.get())
    {
        switch (manualButton->getState())
        {
        case Button::buttonDown:
            audioProcessor.sc3->PlayNote(0, 60, 127);
            manualPlaying = true;
            break;
        case Button::buttonNormal:
            if (manualPlaying)
            {
                manualPlaying = false;
                audioProcessor.sc3->ReleaseNote(0, 60, 127);
            }
            break;
        case Button::buttonOver:
            break;
        }
        
    }
}

//==============================================================================
void SC3AudioProcessorEditor::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    auto bounds = getLocalBounds();
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    auto bottomLabel = bounds.removeFromTop(bounds.getHeight()).expanded(-2,0);
    g.drawFittedText("ShortCircuit3", bottomLabel, juce::Justification::bottomLeft, 1);
    g.drawFittedText(SC3::Build::FullVersionStr, bottomLabel, juce::Justification::bottomRight, 1);
}

void SC3AudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

bool SC3AudioProcessorEditor::isInterestedInFileDrag(const StringArray &files) {
    return true;
}
void SC3AudioProcessorEditor::filesDropped(const StringArray &files, int x, int y) {
    for( auto f : files )
    {
        audioProcessor.sc3->load_file(string_to_path(f.toStdString().c_str()));
    }
    rebuildUIState();
}

void SC3AudioProcessorEditor::rebuildUIState(){
    zoneList->updateContent();
}