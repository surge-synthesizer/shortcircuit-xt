/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "SC3Processor.h"
#include <JuceHeader.h>
#include "sample.h"

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

    struct ZoneListBoxModel : public TableListBoxModel
    {
        ZoneListBoxModel(SC3AudioProcessorEditor *ed ) : ed(ed) {}

        SC3AudioProcessorEditor *ed;

        std::vector<int> zoneByRow;
        int getNumRows() override {
            // This sucks obviously
            int rows = 0;
            zoneByRow.clear();
            for( int i=0; i<max_zones; ++i )
                if( ed->audioProcessor.sc3->zone_exist(i))
                    zoneByRow.push_back(i);
            return zoneByRow.size();
        }
        void paintRowBackground(Graphics &graphics, int rowNumber, int width, int height,
                                bool rowIsSelected) override
        {
            auto alternateColour = ed->getLookAndFeel().findColour (ListBox::backgroundColourId)
                .interpolatedWith (ed->getLookAndFeel().findColour (ListBox::textColourId), 0.03f);
            if (rowIsSelected)
                graphics.fillAll (Colours::lightblue);
            else if (rowNumber % 2)
                graphics.fillAll (alternateColour);
        }
        void paintCell(Graphics &g, int rowNumber, int columnId, int width, int height,
                       bool rowIsSelected) override
        {
            g.setColour (ed->getLookAndFeel().findColour (ListBox::textColourId));

            auto zoneN = zoneByRow[rowNumber];
            auto zone = ed->audioProcessor.sc3->zones[zoneN];
            std::string txt = "-";
            switch( columnId )
            {
            case 0:
                txt = std::to_string(zoneN);
                break;
            case 1:
                txt = std::to_string(zone.sample_id);
                break;
            case 2:
            {
                fs::path p;
                if (ed->audioProcessor.sc3->samples[zone.sample_id]->get_filename(&p))
                    txt = path_to_string(p);
                break;
            }
            case 3:
                txt = std::to_string(zone.key_low);
                break;
            case 4:
                txt = std::to_string(zone.key_high);
                break;
            }

            g.drawText (txt.c_str(), 2, 0, width - 4, height, Justification::centredLeft, true);
        }
    };
    std::unique_ptr<ZoneListBoxModel> zoneListBoxModel;

  private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SC3AudioProcessor &audioProcessor;

    std::unique_ptr<juce::Button> loadButton;
    std::unique_ptr<juce::Button> manualButton;
    bool manualPlaying;

    std::unique_ptr<juce::TableListBox> zoneList;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SC3AudioProcessorEditor)
};
