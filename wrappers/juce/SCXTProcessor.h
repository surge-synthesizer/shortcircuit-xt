/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#pragma once

#include "sampler.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "clap-juce-extensions/clap-juce-extensions.h"

//==============================================================================
/**
 */
class SCXTProcessor : public juce::AudioProcessor,
                      public scxt::log::LoggingCallback,
                      public clap_juce_extensions::clap_properties
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
