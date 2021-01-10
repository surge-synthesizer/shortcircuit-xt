/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "sampler.h"
#include <JuceHeader.h>

class LogDisplayListener
{
  public:
    virtual void handleLogMessage(SC3::Log::Level lev, const std::string &txt) = 0;
};

//==============================================================================
/**
 */
class SC3AudioProcessor : public juce::AudioProcessor, public SC3::Log::LoggingCallback
{
  
  public:
    //==============================================================================
    SC3AudioProcessor();
    ~SC3AudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    std::unordered_set<LogDisplayListener *> logDispList;
    void addLogDisplayListener(LogDisplayListener *e) { logDispList.insert(e); }
    void removeLogDisplayListener(LogDisplayListener *e) { logDispList.erase(e); }
    SC3::Log::Level getLevel() override { return SC3::Log::Level::Debug; }

    void message(SC3::Log::Level lev, const std::string &msg) override
    {
        for (auto l : logDispList)
        {
            l->handleLogMessage(lev, msg);
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

