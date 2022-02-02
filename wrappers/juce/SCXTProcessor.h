/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "sampler.h"
#include "juce_audio_processors/juce_audio_processors.h"

//==============================================================================
/**
 */
class SCXTProcessor : public juce::AudioProcessor, public scxt::log::LoggingCallback
{
    scxt::log::StreamLogger mLogger;
    fs::path mConfigFileName;

  public:
    //==============================================================================
    SCXTProcessor();
    ~SCXTProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    std::unordered_set<scxt::log::LoggingCallback *> logDispList;
    void addLogDisplayListener(scxt::log::LoggingCallback *e) { logDispList.insert(e); }
    void removeLogDisplayListener(scxt::log::LoggingCallback *e) { logDispList.erase(e); }

    // implement logger callback
    scxt::log::Level getLevel() override { return scxt::log::Level::Debug; }
    void message(scxt::log::Level lev, const std::string &msg) override
    {
        for (auto l : logDispList)
        {
            if (l->getLevel() >= lev)
                l->message(lev, msg);
        }
    }

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    void applyMidi(const juce::MidiMessageMetadata &msg);

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
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SCXTProcessor)
};
