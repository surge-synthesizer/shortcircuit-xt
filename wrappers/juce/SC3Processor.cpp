/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "SC3Processor.h"
#include "SC3Editor.h"
#include <iostream>

void *hInstance = 0;

//==============================================================================
SC3AudioProcessor::SC3AudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // This is a good place for VS mem leak debugging:
    // _CrtSetBreakAlloc(<id>);
    sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
}

SC3AudioProcessor::~SC3AudioProcessor() {}

//==============================================================================
const juce::String SC3AudioProcessor::getName() const { return JucePlugin_Name; }

bool SC3AudioProcessor::acceptsMidi() const { return true; }

bool SC3AudioProcessor::producesMidi() const { return false; }

bool SC3AudioProcessor::isMidiEffect() const { return false; }

double SC3AudioProcessor::getTailLengthSeconds() const { return 0.0; }

int SC3AudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int SC3AudioProcessor::getCurrentProgram() { return 0; }

void SC3AudioProcessor::setCurrentProgram(int index) {}

const juce::String SC3AudioProcessor::getProgramName(int index) { return {}; }

void SC3AudioProcessor::changeProgramName(int index, const juce::String &newName) {}

//==============================================================================
void SC3AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    sc3->set_samplerate(sampleRate);
}

void SC3AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SC3AudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
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

void SC3AudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                     juce::MidiBuffer &midiMessages)
{
    MidiBuffer::Iterator midiIterator(midiMessages);
    midiIterator.setNextSamplePosition(0);
    int midiEventPos;
    MidiMessage m;

    int ons[127], offs[127];
    int onp = 0, offp = 0;

    while (midiIterator.getNextEvent(m, midiEventPos))
    {
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

    for (int outPos = 0; outPos < buffer.getNumSamples(); outPos += block_size)
    {
        auto outL = mainInputOutput.getWritePointer(0, outPos);
        auto outR = mainInputOutput.getWritePointer(1, outPos);

        sc3->process_audio();

        memcpy(outL, &(sc3->output[0]), block_size * sizeof(float));
        memcpy(outR, &(sc3->output[1]), block_size * sizeof(float));
    }
}

//==============================================================================
bool SC3AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *SC3AudioProcessor::createEditor()
{
    return new SC3AudioProcessorEditor(*this);
}

//==============================================================================
void SC3AudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SC3AudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new SC3AudioProcessor(); }
