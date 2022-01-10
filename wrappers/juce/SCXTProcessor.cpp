/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "SCXTProcessor.h"
#include "SCXTEditor.h"
#include <iostream>

void *hInstance = 0;

//==============================================================================
SCXTProcessor::SCXTProcessor()
    : mLogger(this),
      AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      blockPos(0)
{
    // This is a good place for VS mem leak debugging:
    // _CrtSetBreakAlloc(<id>);

    // determine where config file should be stored
    auto configLoc = juce::File::getSpecialLocation(
                         juce::File::SpecialLocationType::userApplicationDataDirectory)
                         .getFullPathName();
    fs::path configFile(string_to_path(static_cast<const char *>(configLoc.toUTF8())));
    configFile.append(SCXT_CONFIG_DIRECTORY);
    configFile.append("config.xml");
    mConfigFileName = configFile;

    sc3 = std::make_unique<sampler>(nullptr, 2, nullptr, this);
    if (!sc3->loadUserConfiguration(mConfigFileName))
    {
        LOGINFO(mLogger) << "Configuration file did not load" << std::flush;
    }
}

SCXTProcessor::~SCXTProcessor()
{
    // save config so it's recalled next time they load
    if (sc3)
    {
        if (!sc3->saveUserConfiguration(mConfigFileName))
        {
            // not that anyone will probably see it, but...
            LOGINFO(mLogger) << "Configuration file did not save" << std::flush;
        }
    }
}

//==============================================================================
const juce::String SCXTProcessor::getName() const { return JucePlugin_Name; }

bool SCXTProcessor::acceptsMidi() const { return true; }

bool SCXTProcessor::producesMidi() const { return false; }

bool SCXTProcessor::isMidiEffect() const { return false; }

double SCXTProcessor::getTailLengthSeconds() const { return 0.0; }

int SCXTProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int SCXTProcessor::getCurrentProgram() { return 0; }

void SCXTProcessor::setCurrentProgram(int index) {}

const juce::String SCXTProcessor::getProgramName(int index) { return {}; }

void SCXTProcessor::changeProgramName(int index, const juce::String &newName) {}

//==============================================================================
void SCXTProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    sc3->set_samplerate(sampleRate);
    sc3->AudioHalted = false;
}

void SCXTProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SCXTProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // FIXME - we can go mono -> stereo in the future
    // if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    // return false;

    return true;
}
#endif

void SCXTProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    int ons[127], offs[127];
    int onp = 0, offp = 0;

    for (const auto it : midiMessages)
    {
        auto m = it.getMessage();
        if (m.isNoteOn())
        {
            sc3->PlayNote(0, m.getNoteNumber(), m.getVelocity());
        }
        else if (m.isNoteOff())
        {
            sc3->ReleaseNote(0, m.getNoteNumber(), m.getVelocity());
        }
    }

    auto mainInputOutput = getBusBuffer(buffer, false, 0);

    for (int i = 0; i < buffer.getNumSamples(); i += BUFFER_COPY_CHUNK)
    {
        auto outL = mainInputOutput.getWritePointer(0, i);
        auto outR = mainInputOutput.getWritePointer(1, i);

        if (blockPos == 0)
            sc3->process_audio();

        memcpy(outL, &(sc3->output[0][blockPos]), BUFFER_COPY_CHUNK * sizeof(float));
        memcpy(outR, &(sc3->output[1][blockPos]), BUFFER_COPY_CHUNK * sizeof(float));

        blockPos += BUFFER_COPY_CHUNK;

        if (blockPos >= block_size)
            blockPos = 0;
    }
}

//==============================================================================
bool SCXTProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *SCXTProcessor::createEditor() { return new SCXTEditor(*this); }

//==============================================================================
void SCXTProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    void *data = 0; // I think this leaks
    auto sz = sc3->SaveAllAsRIFF(&data);

    if (sz > 0 && data)
    {
        destData.append(data, sz);
    }
}

void SCXTProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    sc3->LoadAllFromRIFF(data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new SCXTProcessor(); }
