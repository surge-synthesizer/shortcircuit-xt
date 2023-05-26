/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "SCXTProcessor.h"
#include "SCXTPluginEditor.h"
#include "sst/plugininfra/cpufeatures.h"

//==============================================================================
SCXTProcessor::SCXTProcessor()
    : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
                               .withOutput("Main Output", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output 01", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 02", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 03", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 04", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 05", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 06", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 07", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 08", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 09", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 10", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 11", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 12", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 13", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 14", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 15", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 16", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 17", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 18", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 19", juce::AudioChannelSet::stereo(), false)
                               .withOutput("Output 20", juce::AudioChannelSet::stereo(), false)),
      blockPos(0)
{
    engine = std::make_unique<scxt::engine::Engine>();

    std::string wrapperTypeString = getWrapperTypeDescription(wrapperType);
    if (wrapperType == wrapperType_Undefined && is_clap)
        wrapperTypeString = std::string("CLAP");

    std::string phd = juce::PluginHostType().getHostDescription();
    if (phd != "Unknown")
    {
        wrapperTypeString += std::string(" in ") + juce::PluginHostType().getHostDescription();
    }
    engine->runningEnvironment = wrapperTypeString;
}

SCXTProcessor::~SCXTProcessor()
{
    if (wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        // explicitly memory check on exit in the standalone
        engine.reset(nullptr);
        scxt::showLeakLog();
    }
}

//==============================================================================
const juce::String SCXTProcessor::getName() const { return JucePlugin_Name; }

bool SCXTProcessor::acceptsMidi() const { return true; }

bool SCXTProcessor::producesMidi() const { return false; }

bool SCXTProcessor::isMidiEffect() const { return false; }

double SCXTProcessor::getTailLengthSeconds() const
{
    // TODO FixMe
    return 0.0;
}

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
    engine->prepareToPlay(sampleRate);
}

void SCXTProcessor::releaseResources() {}

bool SCXTProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // OK three cases we support. 1 out, 17 out (stereo + parts) or 21 out (stereo + parts + aux)
    bool oneOut = true, partsOut = true, partsAndAuxOut = true;
    for (int i = 0; i < scxt::maxOutputs; ++i)
    {
        auto co = layouts.getNumChannels(false, i);
        auto isSt = (co == 2);
        auto isOf = (co == 0);
        oneOut = oneOut & (i == 0 ? isSt : isOf);
        partsOut = partsOut & (i < scxt::numParts + 1 ? isSt : isOf);
        partsAndAuxOut = partsAndAuxOut & isSt;
    }

    return oneOut || partsOut || partsAndAuxOut;
}

void SCXTProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    auto ftzGuard = sst::plugininfra::cpufeatures::FPUStateGuard();

    // TODO: TempoSync
    /*
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
    */

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

    // TODO FIX ME obviously
    if (activeBusCount == 0)
        return;

    // assert(activeBusCount == 1);
    if (activeBusCount > 1)
        activeBusCount = 1;

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
            engine->processAudio();

        for (int bus = 0; bus < activeBusCount; bus++)
        {
            auto iob = getBusBuffer(buffer, false, bus);

            auto outL = iob.getWritePointer(0, i);
            auto outR = iob.getWritePointer(1, i);

            if (bus == 0)
            {
                *outL = engine->getPatch()->busses.mainBus.output[0][blockPos];
                *outR = engine->getPatch()->busses.mainBus.output[1][blockPos];
            }
            else
            {
                assert(false);
            }
        }

        blockPos++;

        if (blockPos >= scxt::BLOCK_SIZE)
        {
            blockPos = 0;
            // TODO TIME DATA
            // sc3->time_data.ppqPos += (double)BLOCK_SIZE * sc3->time_data.tempo / (60. *
            // samplerate);
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
    // TODO: A clap version with note ids
    auto m = msg.getMessage();
    if (m.isNoteOn())
    {
        engine->noteOn(m.getChannel() - 1, m.getNoteNumber(), -1, m.getVelocity(), 0.f);
    }
    else if (m.isNoteOff())
    {
        engine->noteOff(m.getChannel() - 1, m.getNoteNumber(), -1, m.getVelocity());
    }
    else if (m.isPitchWheel())
    {
        engine->pitchBend(m.getChannel() - 1, m.getPitchWheelValue() - 8192);
    }
    else if (m.isController())
    {
        engine->midiCC(m.getChannel() - 1, m.getControllerNumber(), m.getControllerValue());
    }
    else if (m.isChannelPressure())
    {
        engine->channelAftertouch(m.getChannel() - 1, m.getChannelPressureValue());
    }
    else if (m.isAftertouch())
    {
        engine->polyAftertouch(m.getChannel() - 1, m.getNoteNumber(), m.getAfterTouchValue());
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        // sc3->AllNotesOff();
    }
}

//==============================================================================
bool SCXTProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *SCXTProcessor::createEditor()
{
    return new SCXTPluginEditor(*this, *(engine->getMessageController()), *(engine->defaults),
                                *(engine->getSampleManager()), engine->sharedUIMemoryState);
}

//==============================================================================
void SCXTProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto xml = scxt::json::streamEngineState(*engine);
    destData.replaceAll(xml.c_str(), xml.size() + 1);
}

void SCXTProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    const char *cd = (const char *)data;
    assert(cd[sizeInBytes - 1] == '\0');

    auto xml = std::string(cd);

    // TODO obviously fix this by pushing this xml to the serialization thread
    try
    {
        engine->getMessageController()->threadingChecker.bypassThreadChecks = true;
        std::lock_guard<std::mutex> g(engine->modifyStructureMutex);
        scxt::json::unstreamEngineState(*engine, xml);
    }
    catch (std::exception &e)
    {
        std::cerr << "Unstream exception " << e.what() << std::endl;
    }
    engine->getMessageController()->threadingChecker.bypassThreadChecks = false;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new SCXTProcessor(); }
