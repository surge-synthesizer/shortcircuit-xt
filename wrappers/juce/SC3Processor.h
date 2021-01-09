/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "sampler.h"
#include <JuceHeader.h>

class SC3ProcessorLogger;
// temporary
// the proper way to do this would probably be with a fifo between processor and editor
class EditorNotify {
  public:
    virtual void setLogText(const std::string &txt)=0;
};


//==============================================================================
/**
 */
class SC3AudioProcessor : public juce::AudioProcessor
{
  
  public:
  // editor will set and clear this...
  // shitty i know, but it's temporary
  EditorNotify *mNotify;

    //==============================================================================
    SC3AudioProcessor();
    ~SC3AudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

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

    std::unique_ptr<SC3ProcessorLogger>mLogger; // must destroy before sc3 because flush in logger needs its cb
    std::unique_ptr<sampler> sc3;
    

  private:
    size_t blockpos;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SC3AudioProcessor)
};

class SC3ProcessorLogger : public SC3::Log::LoggingCallback {

    SC3AudioProcessor *mParent;
    SC3::Log::Level getLevel() override {
        return SC3::Log::Level::Debug;
    }
    void message(SC3::Log::Level lev, const std::string &msg) override {
        if(mParent->mNotify) {
          std::string t = SC3::Log::getShortLevelStr(lev);
          t += msg;
          mParent->mNotify->setLogText(t);
        }
    }
public:
    SC3ProcessorLogger(SC3AudioProcessor *p) : mParent(p) {}
    ~SC3ProcessorLogger() {}
};
