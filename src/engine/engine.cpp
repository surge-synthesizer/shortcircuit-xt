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

#include "engine.h"
#include "configuration.h"
#include "messaging/audio/audio_serial.h"
#include "part.h"
#include "sst/cpputils/iterators.h"
#include "voice/voice.h"
#include "dsp/data_tables.h"
#include "tuning/equal.h"
#include "messaging/messaging.h"
#include "messaging/audio/audio_messages.h"
#include "selection/selection_manager.h"
#include "sfz_support/sfz_import.h"
#include "infrastructure/user_defaults.h"

#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/plugininfra/paths.h"

#include "gig.h"
#include "SF.h"

#include "version.h"
#include <filesystem>
#include <mutex>
namespace scxt::engine
{

Engine::Engine()
{
    SCLOG("Shortcircuit XT : Constructing Engine - Version " << scxt::build::FullVersionStr);

    id.id = rngGen.randU32() % 1024;

    messageController = std::make_unique<messaging::MessageController>(*this);
    dsp::sincTable.init();
    dsp::dbTable.init();
    tuning::equalTuning.init();

    sampleManager = std::make_unique<sample::SampleManager>(messageController->threadingChecker);
    patch = std::make_unique<Patch>();
    patch->parentEngine = this;

    auto docpath = sst::plugininfra::paths::bestDocumentsFolderPathFor("Shortcircuit XT");
    try
    {
        if (!fs::is_directory(docpath))
            fs::create_directory(docpath);
    }
    catch (std::exception &e)
    {
    }

    defaults = std::make_unique<infrastructure::DefaultsProvider>(
        docpath, "ShortcircuitXT",
        [](auto e) { return scxt::infrastructure::defaultKeyToString(e); },
        [](auto em, auto t) { std::cerr << "PARSE ERROR FIXME " << em << t << std::endl; });

    for (auto &v : voices)
        v = nullptr;

    voiceInPlaceBuffer.reset(new uint8_t[sizeof(scxt::voice::Voice) * maxVoices]);

    setStereoOutputs(1);
    selectionManager = std::make_unique<selection::SelectionManager>(*this);

    memoryPool = std::make_unique<MemoryPool>();

    voice::Voice::ahdsrenv_t::initializeLuts();

    messageController->start();
}

Engine::~Engine()
{
    for (auto &v : voices)
    {
        if (v)
        {
            v->~Voice();
            v = nullptr;
        }
    }
    messageController->stop();
}

voice::Voice *Engine::initiateVoice(const pathToZone_t &path)
{
#if DEBUG_VOICE_LIFECYCLE
    SCLOG("Initializing Voice at " << SCDBGV((int)path.key));
    ;
#endif

    assert(zoneByPath(path));
    for (const auto &[idx, v] : sst::cpputils::enumerate(voices))
    {
        if (!v || !v->isVoiceAssigned)
        {
            if (v)
            {
                voices[idx]->~Voice();
            }
            auto *dp = voiceInPlaceBuffer.get() + idx * sizeof(voice::Voice);
            const auto &z = zoneByPath(path);
            voices[idx] = new (dp) voice::Voice(this, z.get());
            voices[idx]->zonePath = path;
            voices[idx]->channel = path.channel;
            voices[idx]->key = path.key;
            voices[idx]->noteId = path.noteid;
            voices[idx]->setSampleRate(sampleRate, sampleRateInv);
            return voices[idx];
        }
    }
    return nullptr;
}
void Engine::releaseVoice(int16_t channel, int16_t key, int32_t noteId, int32_t releaseVelocity)
{
    for (auto &v : voices)
    {
        if (v && v->isVoiceAssigned && (v->originalMidiKey == key || key == -1) &&
            (v->channel == channel || channel == -1 || v->channel == -1) &&
            (v->noteId == noteId || v->noteId == -1 || noteId == -1))
        {
            v->release();
#if DEBUG_VOICE_LIFECYCLE
            SCLOG("Release Voice at " << SCDBGV(key));
#endif
        }
    }

#if DEBUG_VOICE_LIFECYCLE
    for (auto &v : voices)
    {
        if (v && v->isVoiceAssigned)
        {
            SCLOG("     PostRelease Voice at " << SCDBGV((int)v->key));
        }
    }
#endif
}

bool Engine::processAudio()
{
    namespace mech = sst::basic_blocks::mechanics;
#if BUILD_IS_DEBUG
    messageController->threadingChecker.registerAsAudioThread();
#endif
    messageController->engineProcessRuns++;
    messageController->isAudioRunning = true;
    auto av = activeVoiceCount();

    bool tryToDrain{true};
    while (tryToDrain && !messageController->serializationToAudioQueue.empty())
    {
        auto msgopt = messageController->serializationToAudioQueue.pop();
        if (!msgopt.has_value())
        {
            tryToDrain = false;
            break;
        }
        switch (msgopt->id)
        {
        case messaging::audio::s2a_dispatch_to_pointer:
        {
            auto cb =
                static_cast<messaging::MessageController::AudioThreadCallback *>(msgopt->payload.p);
            cb->exec(*this);

            messaging::audio::AudioToSerialization rt;
            rt.id = messaging::audio::a2s_pointer_complete;
            rt.payloadType = messaging::audio::AudioToSerialization::VOID_STAR;
            rt.payload.p = (void *)cb;
            messageController->audioToSerializationQueue.push(rt);
        }
        break;
        case messaging::audio::s2a_dispatch_to_pointer_under_structurelock:
        {
            std::lock_guard<std::mutex> structG(modifyStructureMutex);
            auto cb =
                static_cast<messaging::MessageController::AudioThreadCallback *>(msgopt->payload.p);
            cb->exec(*this);

            messaging::audio::AudioToSerialization rt;
            rt.id = messaging::audio::a2s_pointer_complete;
            rt.payloadType = messaging::audio::AudioToSerialization::VOID_STAR;
            rt.payload.p = (void *)cb;
            messageController->audioToSerializationQueue.push(rt);
        }
        break;
        case messaging::audio::s2a_none:
            break;
        }
    }

    getPatch()->busses.clear();

    if (stopEngineRequests > 0)
    {
        return true;
    }

    getPatch()->process(*this);

    // VU Update. Very crude for now.
    float a = vuFalloff;
    sharedUIMemoryState.busVULevels[0][0] =
        std::min(2.f, a * sharedUIMemoryState.busVULevels[0][0]);
    sharedUIMemoryState.busVULevels[0][1] =
        std::min(2.f, a * sharedUIMemoryState.busVULevels[0][1]);
    sharedUIMemoryState.busVULevels[0][0] =
        std::max((float)sharedUIMemoryState.busVULevels[0][0],
                 mech::blockAbsMax<BLOCK_SIZE>(getPatch()->busses.mainBus.output[0]));
    sharedUIMemoryState.busVULevels[0][1] =
        std::max((float)sharedUIMemoryState.busVULevels[0][1],
                 mech::blockAbsMax<BLOCK_SIZE>(getPatch()->busses.mainBus.output[1]));

    auto ml = getPatch()->busses.mainBus.output[0][0] + getPatch()->busses.mainBus.output[1][0];

    auto pav = activeVoiceCount();

    bool doUpdate = messageController->isClientConnected &&
                    (
                        // voice count changed
                        (pav != av) ||
                        // voices are playing and timer ticked by
                        ((pav != 0) && sendSamplePosition &&
                         (lastUpdateVoiceDisplayState >= updateVoiceDisplayStateEvery)) ||
                        // Midi has arrived
                        (midiNoteStateCounter != lastMidiNoteStateCounter));
    if (doUpdate)
    {
        lastUpdateVoiceDisplayState = 0;
        lastMidiNoteStateCounter = midiNoteStateCounter;

        int i{0};
        for (const auto *v : voices)
        {
            auto &itm = sharedUIMemoryState.voiceDisplayItems[i];

            if (v && (v->isVoiceAssigned && v->isVoicePlaying))
            {
                itm.active = true;
                itm.part = v->zonePath.part;
                itm.group = v->zonePath.group;
                itm.zone = v->zonePath.zone;
                itm.samplePos = v->GD.samplePos;
                itm.midiNote = v->originalMidiKey;
                itm.midiChannel = v->channel;
                itm.gated = v->isGated;
            }
            else
            {
                itm.active = false;
            }
            i++;
        }
        sharedUIMemoryState.voiceCount = pav;
        sharedUIMemoryState.voiceDisplayStateWriteCounter++;
    }
    lastUpdateVoiceDisplayState++;

    return true;
}

void Engine::noteOn(int16_t channel, int16_t key, int32_t noteId, int32_t velocity, float detune)
{
    auto useKey = midikeyRetuner.remapKeyTo(channel, key);
    // SCLOG_WFUNC( SCDBGV(channel) << SCDBGV(key) << SCDBGV(useKey):

    for (const auto &path : findZone(channel, useKey, noteId, velocity))
    {
        const auto &z = zoneByPath(path);
        if (!z->samplePointers[0])
        {
            // SCLOG( "Skipping voice with missing sample data" );
        }
        else
        {
            auto v = initiateVoice(path);
            if (v)
            {
                v->originalMidiKey = key;
                v->attack();
            }
        }
    }
    midiNoteStateCounter++;
}
void Engine::noteOff(int16_t channel, int16_t key, int32_t noteId, int32_t velocity)
{
    releaseVoice(channel, key, noteId, velocity);
    midiNoteStateCounter++;
}

void Engine::pitchBend(int16_t channel, int16_t value)
{
    // SCLOG( __func__ << " " << SCDBGV(channel) << SCDBGV(value) );
    auto fv = value / 8192.f;
    for (const auto &part : *patch)
    {
        if (part->channel == -1 || part->channel == channel)
        {
            part->pitchBendSmoother.setTarget(fv);
        }
    }
}
void Engine::midiCC(int16_t channel, int16_t controller, int16_t value)
{
    assert(controller >= 0 && controller < 128);
    auto fv = value / 127.0;
    for (const auto &part : *patch)
    {
        if (part->channel == -1 || part->channel == channel)
        {
            part->midiCCSmoothers[controller].setTarget(fv);
        }
    }
}
void Engine::channelAftertouch(int16_t channel, int16_t value)
{
    // SCLOG(  SCDBGV(channel) << SCDBGV(value) );
}
void Engine::polyAftertouch(int16_t channel, int16_t noteNumber, int16_t value)
{
    // SCLOG_WFUNC(SCDBGV(channel) << SCDBGV(noteNumber) << SCDBGV(value));
}

uint32_t Engine::activeVoiceCount()
{
    uint32_t res{0};
    for (const auto v : voices)
    {
        if (v)
            res += (v->isVoiceAssigned && v->isVoicePlaying);
    }
    return res;
}

const std::optional<dsp::processor::ProcessorStorage>
Engine::getProcessorStorage(const processorAddress_t &addr) const
{
    auto [part, group, zone, which] = addr;
    assert(part >= 0 && part < numParts);
    const auto &pref = getPatch()->getPart(part);

    if (!pref || group < 0)
        return {};

    const auto &gref = pref->getGroup(group);

    if (!gref || zone < 0)
        return {};

    const auto &zref = gref->getZone(zone);
    if (!zref || which < 0)
        return {};

    if (which < processorsPerZone)
        return zref->processorStorage[which];

    return {};
}

Engine::pgzStructure_t Engine::getPartGroupZoneStructure(int partFilter) const
{
    Engine::pgzStructure_t res;
    int32_t partidx{0};
    for (const auto &part : *patch)
    {
        if (partFilter >= 0 && partFilter != partidx)
            continue;

        res.push_back({{partidx, -1, -1}, part->getName()});

        int32_t groupidx{0};
        for (const auto &group : *part)
        {
            res.push_back({{partidx, groupidx, -1}, group->getName()});
            int32_t zoneidx{0};
            for (const auto &zone : *group)
            {
                res.push_back({{partidx, groupidx, zoneidx}, zone->getName()});
                zoneidx++;
            }

            groupidx++;
        }

        partidx++;
    }
    /*
    SCLOG( "Returning partgroup structure size " << res.size() );;
    for (const auto &pg : res)
        SCLOG( "   " << pg.first );;
        */
    return res;
}

void Engine::loadSampleIntoSelectedPartAndGroup(const fs::path &p)
{
    assert(messageController->threadingChecker.isSerialThread());

    // TODO: Deal with compound types more comprehensively
    if (extensionMatches(p, ".sf2"))
    {
        // TODO ok this refresh and restart is a bit unsatisfactory
        messageController->stopAudioThreadThenRunOnSerial([this, p](const auto &) {
            loadSf2MultiSampleIntoSelectedPart(p);
            messageController->restartAudioThreadFromSerial();
            serializationSendToClient(messaging::client::s2c_send_pgz_structure,
                                      getPartGroupZoneStructure(-1), *messageController);
        });
        return;
    }
    else if (extensionMatches(p, ".sfz"))
    {
        // TODO ok this refresh and restart is a bit unsatisfactory
        messageController->stopAudioThreadThenRunOnSerial([this, p](const auto &) {
            auto res = sfz_support::importSFZ(p, *this);
            if (!res)
                messageController->reportErrorToClient("SFZ Import Failed", "Dunno why");
            messageController->restartAudioThreadFromSerial();
            serializationSendToClient(messaging::client::s2c_send_pgz_structure,
                                      getPartGroupZoneStructure(-1), *messageController);
        });
        return;
    }

    // OK so what we want to do now is
    // 1. Load this sample on this thread
    auto sid = sampleManager->loadSampleByPath(p);

    if (!sid.has_value())
    {
        messageController->reportErrorToClient(
            "Unable to load Sample",
            "Sample load failed:\n\n" + p.u8string() + "\n\n" +
                "It is either an unsupported format or invalid file. "
                "More information may be available in the log file (menu/log)");
        return;
    }

    // 2. Create a zone object on this thread but don't add it
    auto zptr = std::make_unique<Zone>(*sid);
    // TODO fixme
    zptr->mapping.keyboardRange = {48, 72};
    zptr->mapping.rootKey = 60;
    zptr->attachToSample(*sampleManager);

    // Drop into selected group logic goes here
    auto [sp, sg] = selectionManager->bestPartGroupForNewSample(*this);

    // 3. Send a message to the audio thread saying to add that zone and
    messageController->scheduleAudioThreadCallbackUnderStructureLock(
        [sp = sp, sg = sg, zone = zptr.release()](auto &e) {
            std::unique_ptr<Zone> zptr;
            zptr.reset(zone);
            e.getPatch()->getPart(sp)->guaranteeGroupCount(sg + 1);
            e.getPatch()->getPart(sp)->getGroup(sg)->addZone(zptr);

            // 4. have the audio thread message back here to refresh the ui
            messaging::audio::sendStructureRefresh(*(e.getMessageController()));
        },
        [sp = sp, sg = sg](auto &e) {
            auto &g = e.getPatch()->getPart(sp)->getGroup(sg);
            int32_t zi = g->getZones().size() - 1;
            e.getSelectionManager()->selectAction({sp, sg, zi, true, true, true});
        });
}

void Engine::sendMetadataToClient() const
{
    // On register send metadata
    messaging::client::serializationSendToClient(
        messaging::client::s2c_send_all_processor_descriptions,
        dsp::processor::getAllProcessorDescriptions(), *messageController);

    sendEngineStatusToClient();

    getPatch()->busses.sendInitialBusInfo(*this);
}

void Engine::sendEngineStatusToClient() const
{
    EngineStatusMessage ec;
    ec.isAudioRunning = messageController->isAudioRunning;
    ec.sampleRate = sampleRate;
    ec.runningEnvironment = runningEnvironment;
    messaging::client::serializationSendToClient(messaging::client::s2c_engine_status, ec,
                                                 *messageController);
}
void Engine::loadSf2MultiSampleIntoSelectedPart(const fs::path &p)
{
    assert(messageController->threadingChecker.isSerialThread());

    try
    {
        auto riff = std::make_unique<RIFF::File>(p.u8string());
        auto sf = std::make_unique<sf2::File>(riff.get());

        auto sz = getSelectionManager()->currentLeadZone(*this);
        auto pt = 0;
        if (sz.has_value())
            pt = sz->part;
        if (pt < 0 || pt >= numParts)
            pt = 0;

        SCLOG("Adding SF2 to part " << SCD(pt));
        auto &part = getPatch()->getPart(pt);
        int firstGroup = -1;
        for (int i = 0; i < sf->GetInstrumentCount(); ++i)
        {
            sf2::Instrument *instr = sf->GetInstrument(i);

            auto grpnum = part->addGroup() - 1;
            auto &grp = part->getGroup(grpnum);
            grp->name = instr->GetName();

            if (instr->pGlobalRegion)
            {
                // TODO: Global Region
            }
            for (int j = 0; j < instr->GetRegionCount(); ++j)
            {
                auto region = instr->GetRegion(j);
                auto sfsamp = region->GetSample();
                if (sfsamp == nullptr)
                    continue;

                auto sid = sampleManager->loadSampleFromSF2(p, sf.get(), i, j);
                if (!sid.has_value())
                    continue;

                if (firstGroup < 0)
                    firstGroup = grpnum;
                auto zn = std::make_unique<engine::Zone>(*sid);
                if (region->overridingRootKey >= 0)
                    zn->mapping.rootKey = region->overridingRootKey;
                zn->mapping.rootKey += sfsamp->OriginalPitch - 60;

                auto lk = region->loKey;
                auto hk = region->hiKey;
                if (lk == sf2::NONE)
                    lk = sfsamp->OriginalPitch;
                if (hk == sf2::NONE)
                    hk = sfsamp->OriginalPitch;
                zn->mapping.keyboardRange = {lk, hk};

                auto lv = region->minVel;
                auto hv = region->maxVel;
                if (lv == sf2::NONE)
                    lv = 0;
                if (hv == sf2::NONE)
                    hv = 127;
                zn->mapping.velocityRange = {lv, hv};

                // TODO check this 256
                zn->mapping.pitchOffset = 1.0 * sfsamp->PitchCorrection / 256;
                zn->attachToSample(*sampleManager);

                auto p = region->GetPan();
                // pan in SF2 is -64 to 63 so hackety hack a bit
                if (p < 0)
                    zn->mapping.pan = p / 64.f;
                else
                    zn->mapping.pan = p / 63.f;

                using namespace std;
                auto s = sfsamp;
                auto reg = region;

                zn->mapping.pitchOffset += reg->coarseTune + reg->fineTune * 0.01;

                if (reg->GetEG1PreAttackDelay() > 0.001)
                {
                    std::cout << "ERROR: PreAttach Delay which we don't support" << std::endl;
                }
                auto s2a = [](double s) {
                    auto l2s = log2(s);
                    auto scs = l2s - sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMin;
                    auto ncs = scs / (sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMax -
                                      sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMin);
                    return std::clamp(ncs, 0., 1.);
                };

                auto sus2l = [](double s) {
                    auto db = -s / 10;
                    return pow(10.0, db * 0.05);
                };

                zn->aegStorage.a = s2a(reg->GetEG1Attack());
                zn->aegStorage.h = s2a(reg->GetEG1Hold());
                zn->aegStorage.d = s2a(reg->GetEG1Decay());
                zn->aegStorage.s = sus2l(reg->GetEG1Sustain());
                zn->aegStorage.r = s2a(reg->GetEG1Release());

                auto GetValue = [](const auto &val) {
                    if (val == sf2::NONE)
                        return ToString("NONE");
                    return ToString(val);
                };

                zn->eg2Storage.a = s2a(reg->GetEG2Attack());
                zn->eg2Storage.h = s2a(reg->GetEG2Hold());
                zn->eg2Storage.d = s2a(reg->GetEG2Decay());
                zn->eg2Storage.s = sus2l(reg->GetEG2Sustain());
                zn->eg2Storage.r = s2a(reg->GetEG2Release());
#if 0
                std::cout << "STUFF I HAVEN'T DEALT WITH YET" << std::endl;
                cout << "\t\t    Modulation Envelope Generator" << endl;
                cout << "\t\t\tPitch=" << GetValue(reg->modEnvToPitch) << "cents, Cutoff=";
                cout << GetValue(reg->modEnvToFilterFc) << "cents" << endl << endl;

                cout << "\t\t    Modulation LFO: Delay=" << ::sf2::ToSeconds(reg->delayModLfo)
                     << "s, Frequency=";
                cout << ::sf2::ToHz(reg->freqModLfo)
                     << "Hz, LFO to Volume=" << (reg->modLfoToVolume / 10) << "dB";
                cout << ", LFO to Filter Cutoff=" << reg->modLfoToFilterFc;
                cout << ", LFO to Pitch=" << reg->modLfoToPitch << endl;

                cout << "\t\t    Vibrato LFO:    Delay=" << ::sf2::ToSeconds(reg->delayVibLfo)
                     << "s, Frequency=";
                cout << ::sf2::ToHz(reg->freqVibLfo) << "Hz, LFO to Pitch=" << reg->vibLfoToPitch
                     << endl;

                cout << "\t\t\tModulators (" << reg->modulators.size() << ")" << endl;

                for (int i = 0; i < reg->modulators.size(); i++)
                {
                    cout << "\t\t\tModulator " << i << endl;
                    // PrintModulatorItem(&reg->modulators[i]);
                }

                cout << "\t" << s->Name
                     << " (Depth: " << ((s->GetFrameSize() / s->GetChannelCount()) * 8);
                cout << ", SampleRate: " << s->SampleRate;
                cout << ", Pitch: " << ((int)s->OriginalPitch);
                cout << ", Pitch Correction: " << ((int)s->PitchCorrection) << endl;
                cout << "\t\tStart: " << s->Start << ", End: " << s->End;
                cout << ", Start Loop: " << s->StartLoop << ", End Loop: " << s->EndLoop << endl;
                cout << "\t\tSample Type: " << s->SampleType << ", Sample Link: " << s->SampleLink
                     << ")" << endl;

                if (s != NULL)
                {
                    if (reg->HasLoop)
                    {
                        cout << ", Loop Start: " << reg->LoopStart
                             << ", Loop End: " << reg->LoopEnd;
                    }
                    cout << endl;
                }
                cout << "\t\t    Key range=";

                cout << "\t\t    Initial cutoff frequency=";
                if (reg->initialFilterFc == ::sf2::NONE)
                    cout << "None" << endl;
                else
                    cout << reg->initialFilterFc << "cents" << endl;

                cout << "\t\t    Initial resonance=";
                if (reg->initialFilterQ == ::sf2::NONE)
                    cout << "None" << endl;
                else
                    cout << (reg->initialFilterQ / 10.0) << "dB" << endl;

                if (reg->exclusiveClass)
                    cout << ", Exclusive group=" << reg->exclusiveClass;
                cout << endl;
#endif

                grp->addZone(zn);
            }
        }
        selectionManager->selectAction(
            {pt, firstGroup, firstGroup >= 0 ? 0 : -1, true, true, true});
    }
    catch (RIFF::Exception e)
    {
        messageController->reportErrorToClient("SF2 Load Error", e.Message);
        return;
    }
    catch (const SCXTError &e)
    {
        messageController->reportErrorToClient("SF2 Load Error", e.what());
        return;
    }
    catch (...)
    {
        return;
    }
}

void Engine::onSampleRateChanged()
{
    for (const auto &part : *patch)
        part->setSampleRate(samplerate);

    messageController->forceStatusUpdate = true;
}
} // namespace scxt::engine