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
    engine = std::make_unique<scxt::engine::Engine>();

    engine->getMessageController()->threadingChecker.bypassThreadChecks = true;
    temporaryInitPatch();
    engine->getMessageController()->threadingChecker.bypassThreadChecks = false;
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

bool SCXTProcessor::acceptsMidi() const { return false; }

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
    engine->setSampleRate(sampleRate);
}

void SCXTProcessor::releaseResources() {}

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
    assert(activeBusCount == 1);
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

            *outL = engine->output[bus][0][blockPos];
            *outR = engine->output[bus][1][blockPos];
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
    // TODO: All these messages
    // TODO: A clap version with note ids
    // TODO: Poly AT
    auto m = msg.getMessage();
    if (m.isNoteOn())
    {
        engine->noteOn(m.getChannel() - 1, m.getNoteNumber(), -1, m.getVelocity() / 127.f, 0.f);
    }
    else if (m.isNoteOff())
    {
        engine->noteOff(m.getChannel() - 1, m.getNoteNumber(), -1, m.getVelocity() / 127.f);
    }
    else if (m.isPitchWheel())
    {
        // sc3->PitchBend(m.getChannel() - 1, m.getPitchWheelValue() - 8192);
    }
    else if (m.isController())
    {
        // sc3->ChannelController(m.getChannel() - 1, m.getControllerNumber(),
        // m.getControllerValue());
    }
    else if (m.isAftertouch())
    {
        // sc3->ChannelAftertouch(m.getChannel() - 1, m.getAfterTouchValue());
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
    return new SCXTPluginEditor(*this, *(engine->getMessageController()));
}

//==============================================================================
void SCXTProcessor::getStateInformation(juce::MemoryBlock &destData) {}

void SCXTProcessor::setStateInformation(const void *data, int sizeInBytes) {}

void SCXTProcessor::temporaryInitPatch()
{
    auto samplePath = fs::current_path();

    auto rm = fs::path{"resources/test_samples/next/README.md"};

    while (!samplePath.empty() && samplePath.has_parent_path() &&
           samplePath.parent_path() != samplePath && !(fs::exists(samplePath / rm)))
    {
        samplePath = samplePath.parent_path();
    }
    if (fs::exists(samplePath / rm))
    {
        {
            auto sid = engine->getSampleManager()->loadSampleByPath(
                samplePath / "resources/test_samples/next/PulseSaw.wav");
            auto zptr = std::make_unique<scxt::engine::Zone>(*sid);
            zptr->keyboardRange = {48, 72};
            zptr->rootKey = 60;
            zptr->attachToSample(*(engine->getSampleManager()));

            zptr->processorStorage[0].type = scxt::dsp::processor::proct_SuperSVF;
            zptr->processorStorage[0].mix = 1.0;

            zptr->aegStorage.a = 0.7;

            engine->getPatch()->getPart(0)->guaranteeGroupCount(1);
            engine->getPatch()->getPart(0)->getGroup(0)->addZone(zptr);
        }
        {
            auto sid = engine->getSampleManager()->loadSampleByPath(
                samplePath / "resources/test_samples/next/Beep.wav");
            auto zptr = std::make_unique<scxt::engine::Zone>(*sid);
            zptr->keyboardRange = {60, 72};
            zptr->rootKey = 60;
            zptr->attachToSample(*(engine->getSampleManager()));

            zptr->aegStorage.a = 0.1;

            engine->getPatch()->getPart(0)->guaranteeGroupCount(1);
            engine->getPatch()->getPart(0)->getGroup(0)->addZone(zptr);
        }
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon, "Can't find test samples",
            "Please run this from the source directory for now.");
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new SCXTProcessor(); }
