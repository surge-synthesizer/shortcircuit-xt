/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "sampler.h"
#include <JuceHeader.h>

//==============================================================================
/**
 */
class SC3AudioProcessor : public juce::AudioProcessor, public SC3::Log::LoggingCallback
{
    SC3::Log::StreamLogger mLogger; 
    fs::path mConfigFileName;
  public:
    //==============================================================================
    SC3AudioProcessor();
    ~SC3AudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    std::unordered_set<SC3::Log::LoggingCallback *> logDispList;
    void addLogDisplayListener(SC3::Log::LoggingCallback *e) { logDispList.insert(e); }
    void removeLogDisplayListener(SC3::Log::LoggingCallback *e) { logDispList.erase(e); }

    // implement logger callback
    SC3::Log::Level getLevel() override { return SC3::Log::Level::Debug; }
    void message(SC3::Log::Level lev, const std::string &msg) override
    {
        for (auto l : logDispList)
        {
            if(l->getLevel() >= lev)
                l->message(lev, msg);
        }
    }

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    //==============================================================================
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    std::unique_ptr<sampler> sc3;
    

  private:
    size_t blockPos;
    static const uint32 BUFFER_COPY_CHUNK = 4; // sample copy grain size
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SC3AudioProcessor)
};

