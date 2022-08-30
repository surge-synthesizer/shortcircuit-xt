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

#include "SCXTProcessor.h"
#include "SCXTEditor.h"
#include <iostream>
#include <string>
#include "sst/plugininfra/cpufeatures.h"

//==============================================================================
SCXTProcessor::SCXTProcessor()
    : mLogger(this),
      AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Out 02", juce::AudioChannelSet::stereo(), false)
                         .withOutput("Out 03", juce::AudioChannelSet::stereo(), false)
                         .withOutput("Out 04", juce::AudioChannelSet::stereo(), false)
                         .withOutput("Out 05", juce::AudioChannelSet::stereo(), false)
                         .withOutput("Out 06", juce::AudioChannelSet::stereo(), false)
                         .withOutput("Out 07", juce::AudioChannelSet::stereo(), false)
                         .withOutput("Out 08", juce::AudioChannelSet::stereo(), false)),
      blockPos(0)
{
    // This is a good place for VS mem leak debugging:
    // _CrtSetBreakAlloc(<id>);

    sc3 = std::make_unique<sampler>(nullptr, 8, nullptr, this);
    sc3->wrapperType = getWrapperTypeDescription(wrapperType);
    if (wrapperType == wrapperType_Undefined && is_clap)
    {
        sc3->wrapperType = std::string("CLAP");
    }
}

SCXTProcessor::~SCXTProcessor()
{
    // save config so it's recalled next time they load
    if (sc3)
    {
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

bool SCXTProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // OK three cases we support. 1 out, 4 out, 8 out
    bool oneOut = true, fourOut = true, eightOut = true;
    for (int i = 0; i < 8; ++i)
    {
        auto co = layouts.getNumChannels(false, i);
        auto isSt = (co == 2);
        auto isOf = (co == 0);
        oneOut = oneOut & (i == 0 ? isSt : isOf);
        fourOut = fourOut & (i < 4 ? isSt : isOf);
        eightOut = eightOut & isSt;
    }

    return oneOut || eightOut;
}

void SCXTProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    auto ftzGuard = sst::plugininfra::cpufeatures::FPUStateGuard();

    auto playhead = getPlayHead();
    if (playhead)
    {
        auto posinfo = playhead->getPosition();
        sc3->time_data.tempo = posinfo->getBpm().orFallback(120);

        // isRecording should always imply isPlaying but better safe than sorry
        if (posinfo->getIsPlaying())
            sc3->time_data.ppqPos = posinfo->getPpqPosition().orFallback(sc3->time_data.ppqPos);

        auto ts = posinfo->getTimeSignature().orFallback(juce::AudioPlayHead::TimeSignature());
        sc3->time_data.timeSigNumerator = ts.numerator;
        sc3->time_data.timeSigDenominator = ts.denominator;
        sc3->resetStateFromTimeData();
    }
    else
    {
        sc3->time_data.tempo = 120;
        sc3->time_data.timeSigNumerator = 4;
        sc3->time_data.timeSigDenominator = 4;
    }

    auto midiIt = midiMessages.findNextSamplePosition(0);
    int nextMidi = -1;
    if (midiIt != midiMessages.cend())
    {
        nextMidi = (*midiIt).samplePosition;
    }

    int activeBusCount = 0;
    for (int i = 0; i < 8; ++i)
    {
        auto iob = getBusBuffer(buffer, false, i);
        if (iob.getNumChannels() != 2)
        {
            break;
        }

        auto outL = iob.getWritePointer(0, 0);
        auto outR = iob.getWritePointer(1, 0);
        if (!(outL && outR))
        {
            break;
        }
        activeBusCount++;
    }

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

        if (blockPos == 0)
            sc3->process_audio();

        for (int bus = 0; bus < activeBusCount; bus++)
        {
            auto iob = getBusBuffer(buffer, false, bus);

            auto outL = iob.getWritePointer(0, i);
            auto outR = iob.getWritePointer(1, i);

            *outL = sc3->output[2 * bus][blockPos];
            *outR = sc3->output[2 * bus + 1][blockPos];
        }

        blockPos++;

        if (blockPos >= BLOCK_SIZE)
        {
            blockPos = 0;
            sc3->time_data.ppqPos += (double)BLOCK_SIZE * sc3->time_data.tempo / (60. * samplerate);
        }
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
