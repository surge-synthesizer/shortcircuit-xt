/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "SCXTProcessor.h"
#include "SCXTEditor.h"
#include <iostream>

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

    sc3->wrapperType = getWrapperTypeDescription(wrapperType);
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
    // There are obviously other options here
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}
#endif

void SCXTProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    int ons[127], offs[127];
    int onp = 0, offp = 0;

    auto midiIt = midiMessages.findNextSamplePosition(0);
    int nextMidi = -1;
    if (midiIt != midiMessages.cend())
    {
        nextMidi = (*midiIt).samplePosition;
    }

    auto mainInputOutput = getBusBuffer(buffer, false, 0);

    for (int i = 0; i < buffer.getNumSamples(); i++)
    {
        while (i == nextMidi)
        {
            applyMidi(*midiIt);
            midiIt++;
            if (midiIt == midiMessages.cend())
            {
                nextMidi = -1;
            }
            else
            {
                nextMidi = (*midiIt).samplePosition;
            }
        }

        auto outL = mainInputOutput.getWritePointer(0, i);
        auto outR = mainInputOutput.getWritePointer(1, i);

        if (blockPos == 0)
            sc3->process_audio();

        *outL = sc3->output[0][blockPos];
        *outR = sc3->output[1][blockPos];

        blockPos++;

        if (blockPos >= block_size)
            blockPos = 0;
    }

    // This should, in theory, never happen, but better safe than sorry
    while (midiIt != midiMessages.cend())
    {
        applyMidi(*midiIt);
        midiIt++;
    }
}

void SCXTProcessor::applyMidi(const juce::MidiMessageMetadata &msg)
{
    auto m = msg.getMessage();
    if (m.isNoteOn())
    {
        sc3->PlayNote(m.getChannel() - 1, m.getNoteNumber(), m.getVelocity());
    }
    else if (m.isNoteOff())
    {
        sc3->ReleaseNote(m.getChannel() - 1, m.getNoteNumber(), m.getVelocity());
    }
    else if (m.isPitchWheel())
    {
        sc3->PitchBend(m.getChannel() - 1, m.getPitchWheelValue() - 8192);
    }
    else if (m.isController())
    {
        sc3->ChannelController(m.getChannel() - 1, m.getControllerNumber(), m.getControllerValue());
    }
    else if (m.isAftertouch())
    {
        sc3->ChannelAftertouch(m.getChannel() - 1, m.getAfterTouchValue());
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        sc3->AllNotesOff();
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
