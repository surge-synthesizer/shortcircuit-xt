/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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
#include "sst/voicemanager/midi1_to_voicemanager.h"

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

    auto playhead = getPlayHead();
    if (playhead)
    {
        auto posinfo = playhead->getPosition();
        engine->transport.tempo = posinfo->getBpm().orFallback(120);

        // isRecording should always imply isPlaying but better safe than sorry
        if (posinfo->getIsPlaying())
        {
            engine->transport.hostTimeInBeats =
                posinfo->getPpqPosition().orFallback(engine->transport.timeInBeats);
            engine->transport.timeInBeats = engine->transport.hostTimeInBeats;
            engine->transport.lastBarStartInBeats =
                posinfo->getPpqPositionOfLastBarStart().orFallback(
                    engine->transport.lastBarStartInBeats);
        }

        engine->transport.status = scxt::engine::Transport::Status::STOPPED;
        if (posinfo->getIsPlaying())
            engine->transport.status &= scxt::engine::Transport::Status::PLAYING;
        if (posinfo->getIsRecording())
            engine->transport.status &= scxt::engine::Transport::Status::RECORDING;
        if (posinfo->getIsLooping())
            engine->transport.status &= scxt::engine::Transport::Status::LOOPING;

        auto ts = posinfo->getTimeSignature().orFallback(juce::AudioPlayHead::TimeSignature());
        engine->transport.signature.numerator = ts.numerator;
        engine->transport.signature.denominator = ts.denominator;
        engine->onTransportUpdated();
    }
    else
    {
        engine->transport.tempo = 120;
        engine->transport.signature.numerator = 4;
        engine->transport.signature.denominator = 4;
    }

    auto midiIt = midiMessages.findNextSamplePosition(0);
    int nextMidi = -1;
    if (midiIt != midiMessages.cend())
    {
        nextMidi = (*midiIt).samplePosition;
    }

    int activeBusCount = 0;
    for (int i = 0; i < scxt::maxOutputs; ++i)
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

        {
            auto iob = getBusBuffer(buffer, false, 0);

            auto outL = iob.getWritePointer(0, i);
            auto outR = iob.getWritePointer(1, i);

            if (outL && outR)
            {
                *outL = engine->getPatch()->busses.mainBus.output[0][blockPos];
                *outR = engine->getPatch()->busses.mainBus.output[1][blockPos];
            }
        }
        for (int bus = 1; bus < activeBusCount; bus++)
        {
            auto iob = getBusBuffer(buffer, false, bus);

            auto outL = iob.getWritePointer(0, i);
            auto outR = iob.getWritePointer(1, i);

            if (outL && outR)
            {
                if (!engine->getPatch()->usesOutputBus(bus))
                {
                    *outL = 0.f;
                    *outR = 0.f;
                }
                else
                {
                    *outL = engine->getPatch()->busses.pluginNonMainOutputs[bus - 1][0][blockPos];
                    *outR = engine->getPatch()->busses.pluginNonMainOutputs[bus - 1][1][blockPos];
                }
            }
        }

        blockPos++;

        if (blockPos >= scxt::BLOCK_SIZE)
        {
            blockPos = 0;
            engine->transport.timeInBeats += (double)scxt::blockSize * engine->transport.tempo *
                                             engine->getSampleRateInv() / 60.f;
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

    uint8_t data[3]{0, 0, 0};
    if (m.getRawDataSize() <= 3)
    {
        memcpy(data, m.getRawData(), std::max(m.getRawDataSize(), 3));
        sst::voicemanager::applyMidi1Message(engine->voiceManager, 0, data);
    }
    else
    {
        // What to do with these sysex and other messages? For now drop them
        // since eventually we will clap first this sucker anyway
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
                                *(engine->getSampleManager()), *(engine->getBrowser()),
                                engine->sharedUIMemoryState);
}

//==============================================================================
void SCXTProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    engine->getSampleManager()->purgeUnreferencedSamples();
    try
    {
        auto xml = scxt::json::streamEngineState(*engine);
        SCLOG("Streaming State Information: " << xml.size() << " bytes");
        destData.replaceAll(xml.c_str(), xml.size() + 1);
    }
    catch (const std::runtime_error &err)
    {
        SCLOG("Unable to stream [" << err.what() << "]");
    }
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
    catch (std::exception &err)
    {
        SCLOG("Unable to unstream [" << err.what() << "]");
    }
    engine->getMessageController()->threadingChecker.bypassThreadChecks = false;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new SCXTProcessor(); }
