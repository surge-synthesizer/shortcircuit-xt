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

#include "engine.h"

#include "sst/plugininfra/version_information.h"

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
#include "sample/sfz_support/sfz_import.h"
#include "sample/exs_support/exs_import.h"
#include "sample/sf2_support/sf2_import.h"
#include "sample/gig_support/gig_import.h"
#include "sample/multisample_support/multisample_import.h"
#include "infrastructure/user_defaults.h"
#include "infrastructure/md5support.h"
#include "browser/browser.h"
#include "browser/browser_db.h"

#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/plugininfra/paths.h"

#include "gig.h"
#include "SF.h"

#include <filesystem>
#include <mutex>
#include "messaging/client/client_serial.h"
#include "feature_enums.h"
#include "missing_resolution.h"
#include "sst/voicemanager/midi1_to_voicemanager.h"
#include "voice/preview_voice.h"

namespace scxt::engine
{

Engine::Engine()
{
    SCLOG("Shortcircuit XT : Constructing Engine");
    SCLOG("    Version   = " << sst::plugininfra::VersionInformation::git_implied_display_version
                             << " / "
                             << sst::plugininfra::VersionInformation::project_version_and_hash);
    SCLOG("    Stream V  = " << humanReadableVersion(scxt::currentStreamingVersion));

    memset(cpuAverages, 0, sizeof(cpuAverages));

    id.id = rng.unifU32() % 1024;

    messageController = std::make_unique<messaging::MessageController>(*this);
    dsp::sincTable.init();
    dsp::dbTable.init();
    dsp::twoToTheXTable.init();
    tuning::equalTuning.init();

    sampleManager = std::make_unique<sample::SampleManager>(messageController->threadingChecker);
    patch = std::make_unique<Patch>();
    patch->parentEngine = this;

    auto tdp = setupUserStorageDirectory();
    fs::path useTDP;

    if (tdp.has_value())
    {
        useTDP = *tdp;
    }
    else
    {
        useTDP = fs::temp_directory_path();
        messageController->reportErrorToClient("Cannot create documents dir",
                                               "Unable to create documents directory. Using " +
                                                   useTDP.u8string());
    }

    defaults = std::make_unique<infrastructure::DefaultsProvider>(
        useTDP, "ShortcircuitXT",
        [](auto e) { return scxt::infrastructure::defaultKeyToString(e); },
        [](auto em, auto t) { SCLOG("Defaults Parse Error :" << em << " " << t << std::endl); });

    browserDb = std::make_unique<browser::BrowserDB>(*tdp, *messageController);
    browser = std::make_unique<browser::Browser>(
        *browserDb, *defaults, useTDP,
        [this](const auto &a, const auto &b) { messageController->reportErrorToClient(a, b); });

    for (auto &v : voices)
        v = nullptr;

    voiceInPlaceBuffer.reset(new uint8_t[sizeof(scxt::voice::Voice) * maxVoices]);

    setStereoOutputs(1);
    selectionManager = std::make_unique<selection::SelectionManager>(*this);

    memoryPool = std::make_unique<MemoryPool>();

    voice::Voice::ahdsrenv_t::initializeLuts();

    messageController->start();

    browserDb->writeDebugMessage(std::string("SCXT Startup ") +
                                 sst::plugininfra::VersionInformation::project_version_and_hash);

    // This forces metadata init of the mod matrix
    modulation::ModulationCurves::initializeCurves();
    voice::modulation::MatrixEndpoints usedForInit(this);
    modulation::GroupMatrixEndpoints usedForGroupInit(this);

    // Zone->voice endpoints are pre-allocated. Group endpoints are part of the group since they
    // are monophonic
    for (auto &ep : allEndpoints)
    {
        ep = std::make_unique<voice::modulation::MatrixEndpoints>(nullptr);
    }

    previewVoice = std::make_unique<voice::PreviewVoice>();

    onPartConfigurationUpdated();
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
    sampleManager->purgeUnreferencedSamples();

    /*
     * We want to now clear all the parts to make sure we return
     * any memory to the memory pool before it shuts down (since that
     * would debug assert). We could also do this by making the MP
     * a shared ptr but.... lets not for now.
     */
    for (auto &part : *getPatch())
    {
        part->clearGroups();
    }
}

thread_local bool Engine::isFullEngineUnstream{false};
thread_local Engine::StreamReason Engine::streamReason{StreamReason::IN_PROCESS};
thread_local uint64_t Engine::fullEngineUnstreamStreamingVersion{0};

voice::Voice *Engine::initiateVoice(const pathToZone_t &path)
{
    SCLOG_IF(voiceResponder, "Initializing Voice at " << SCD((int)path.key));

    assert(zoneByPath(path));
    for (const auto &[idx, v] : sst::cpputils::enumerate(voices))
    {
        if (!v || !v->isVoiceAssigned)
        {
            std::unique_ptr<voice::modulation::MatrixEndpoints> mp;
            if (v)
            {
                mp = std::move(voices[idx]->endpoints);
                voices[idx]->~Voice();
            }
            else
            {
                mp = std::move(allEndpoints[idx]);
            }
            auto *dp = voiceInPlaceBuffer.get() + idx * sizeof(voice::Voice);
            const auto &z = zoneByPath(path);
            voices[idx] = new (dp) voice::Voice(this, z.get());
            voices[idx]->zonePath = path;
            voices[idx]->channel = path.channel;
            voices[idx]->key = path.key;
            voices[idx]->noteId = path.noteid;
            voices[idx]->setSampleRate(sampleRate, sampleRateInv);
            voices[idx]->endpoints = std::move(mp);
            activeVoices++;
            return voices[idx];
        }
    }

    SCLOG_IF(voiceResponder, "Fallthrough - looking for early termination voices");
    for (const auto &[idx, v] : sst::cpputils::enumerate(voices))
    {
        if (v && v->terminationSequence > 0)
        {
            voices[idx]->cleanupVoice();
            std::unique_ptr<voice::modulation::MatrixEndpoints> mp;
            mp = std::move(voices[idx]->endpoints);
            voices[idx]->~Voice();

            auto *dp = voiceInPlaceBuffer.get() + idx * sizeof(voice::Voice);
            const auto &z = zoneByPath(path);
            voices[idx] = new (dp) voice::Voice(this, z.get());
            voices[idx]->zonePath = path;
            voices[idx]->channel = path.channel;
            voices[idx]->key = path.key;
            voices[idx]->noteId = path.noteid;
            voices[idx]->setSampleRate(sampleRate, sampleRateInv);
            voices[idx]->endpoints = std::move(mp);
            activeVoices++;
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

void Engine::immediatelyTerminateAllVoices()
{
    std::array<voice::Voice *, maxVoices> toCleanUp{};
    int idx{0};
    for (auto &v : voices)
    {
        if (v && v->isVoiceAssigned && v->isVoicePlaying)
            toCleanUp[idx++] = v;
    }

    for (auto tc : toCleanUp)
    {
        if (tc)
        {
            tc->cleanupVoice();
        }
    }
    forceVoiceUpdate = true;
}

bool Engine::processAudio()
{
    auto processingStartTime = std::chrono::high_resolution_clock::now();

    namespace mech = sst::basic_blocks::mechanics;
#if BUILD_IS_DEBUG
    messageController->threadingChecker.registerAsAudioThread();
#endif
    messageController->engineProcessRuns++;
    messageController->isAudioRunning = true;
    auto av = (uint32_t)activeVoices;

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
        case messaging::audio::s2a_param_beginendedit:
        case messaging::audio::s2a_param_set_value:
        case messaging::audio::s2a_param_refresh:
        {
            if (messageController->passWrapperEventsToWrapperQueue)
                messageController->engineToPluginWrapperQueue.push(*msgopt);
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

    updateTransportPhasors();

    getPatch()->process(*this);

    if (previewVoice->isActive)
    {
        previewVoice->processBlock();

        auto &main = getPatch()->busses.mainBus.output;
        mech::accumulate_from_to<blockSize>(previewVoice->output[0], main[0]);
        mech::accumulate_from_to<blockSize>(previewVoice->output[1], main[1]);
    }

    if (previewVoice->schedulePurge)
    {
        previewVoice->schedulePurge = false;

        scxt::messaging::audio::AudioToSerialization a2s;
        a2s.id = messaging::audio::a2s_schedule_sample_purge;
        a2s.payloadType = scxt::messaging::audio::AudioToSerialization::NONE;
        getMessageController()->sendAudioToSerialization(a2s);
    }

    auto &bl = sharedUIMemoryState.busVULevels;
    const auto &bs = getPatch()->busses;
    for (int c = 0; c < 2; ++c)
    {
        int idx = 0;
        bl[idx++][c] = bs.mainBus.vuLevel[c];
        for (const auto &a : bs.partBusses)
            bl[idx++][c] = a.vuLevel[c];
        for (const auto &a : bs.auxBusses)
            bl[idx++][c] = a.vuLevel[c];
    }

    auto pav = (uint32_t)activeVoices;
#if BUILD_IS_DEBUG
    if (pav != av)
        assertActiveVoiceCount();
#endif

    bool doUpdate = messageController->isClientConnected &&
                    (forceVoiceUpdate ||
                     // voice count changed
                     (pav != av) ||
                     // voices are playing and timer ticked by
                     ((pav != 0) && sendSamplePosition &&
                      (lastUpdateVoiceDisplayState >= updateVoiceDisplayStateEvery)) ||
                     // Midi has arrived
                     (midiNoteStateCounter != lastMidiNoteStateCounter));
    if (doUpdate)
    {
        forceVoiceUpdate = false;
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
                itm.sample = v->sampleIndex;
                itm.samplePos = v->GD[0].samplePos;
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

    auto processingEndTime = std::chrono::high_resolution_clock::now();

    auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(processingEndTime -
                                                                               processingStartTime);
    // auto maxtime = blockSize * sampleRateInv;
    // auto pct = time_span.count / maxtime;
    //  or...
    auto pct = time_span.count() * sampleRate * blockSizeInv * 100.0;
    auto ppct = cpuAverages[cpuWP];
    cpuAverages[cpuWP] = pct;
    cpuWP = (cpuWP + 1) & (cpuAverageObservation - 1);
    cpuAvg += (pct - ppct) / cpuAverageObservation;
    sharedUIMemoryState.cpuLevel = cpuAvg;
    return true;
}

void Engine::assertActiveVoiceCount()
{
    uint32_t res{0};
    for (const auto v : voices)
    {
        if (v)
            res += (v->isVoiceAssigned && v->isVoicePlaying);
    }
    assert(res == activeVoices);
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

    if (which < processorCount)
        return zref->processorStorage[which];

    return {};
}

Engine::pgzStructure_t Engine::getPartGroupZoneStructure() const
{
    Engine::pgzStructure_t res;
    int32_t partidx{0};
    for (const auto &part : *patch)
    {
        res.push_back({{partidx, -1, -1}, part->getName()});

        int32_t groupidx{0};
        for (const auto &group : *part)
        {
            res.push_back({{partidx, groupidx, -1}, group->getName()});
            int32_t zoneidx{0};
            for (const auto &zone : *group)
            {
                int32_t zoneFeatures{0};
                if (zone->missingSampleCount() > 0)
                {
                    zoneFeatures |= ZoneFeatures::MISSING_SAMPLE;
                }

                res.push_back({{partidx, groupidx, zoneidx}, zone->getName(), zoneFeatures});
                zoneidx++;
            }

            groupidx++;
        }

        partidx++;
    }
    if constexpr (scxt::log::uiStructure)
    {
        SCLOG("Returning partgroup structure size " << res.size());

        for (const auto &pg : res)
            SCLOG("   " << pg.address << " @ " << pg.name);
    }

    return res;
}

void Engine::loadCompoundElementIntoZone(const sample::compound::CompoundElement &p, int16_t partID,
                                         int16_t groupID, int16_t zoneID, int variantID)
{
    assert(messageController->threadingChecker.isSerialThread());
    assert(variantID < maxVariantsPerZone);

    auto sz = getSelectionManager()->currentLeadZone(*this);

    if (!sz.has_value())
    {
        messageController->reportErrorToClient("Unable to load Sample",
                                               "There is no currentLeadZone");
        return;
    }

    assert((*sz).part == partID);
    assert((*sz).group == groupID);
    assert((*sz).zone == zoneID);

    auto osid = SampleID();
    auto sid = sampleManager->loadSampleByFileAddress(p.sampleAddress, osid);

    if (!sid.has_value())
    {
        messageController->reportErrorToClient(
            "Unable to load Compoint Sample",
            "Sample load failed:\n\n" + p.sampleAddress.path.u8string() + "\n\n" +
                "More information may be available in the log file (menu/log)");
        return;
    }

    messageController->scheduleAudioThreadCallbackUnderStructureLock(
        [p = partID, g = groupID, z = zoneID, sID = variantID, sample = *sid](auto &e) {
            auto &zone = e.getPatch()->getPart(p)->getGroup(g)->getZone(z);
            zone->terminateAllVoices();
            zone->variantData.variants[sID].sampleID = sample;
            zone->variantData.variants[sID].active = true;
            zone->attachToSample(*e.getSampleManager(), sID,
                                 (Zone::SampleInformationRead)(Zone::LOOP | Zone::ENDPOINTS));
        },
        [p = partID, g = groupID, z = zoneID](auto &e) {
            e.getSelectionManager()->selectAction({p, g, z, true, true, true});
        });
}

void Engine::loadSampleIntoZone(const fs::path &p, int16_t partID, int16_t groupID, int16_t zoneID,
                                int variantID)
{
    assert(messageController->threadingChecker.isSerialThread());
    assert(variantID < maxVariantsPerZone);

    // TODO: Deal with compound types more comprehensively
    // If you add a type here add it to Browser::isLoadableFile also
    if (extensionMatches(p, ".multisample") || extensionMatches(p, ".sf2") ||
        extensionMatches(p, ".sfz"))
    {
        assert(false);
        return;
    }

    auto sz = getSelectionManager()->currentLeadZone(*this);

    if (!sz.has_value())
    {
        messageController->reportErrorToClient("Unable to load Sample",
                                               "There is no currentLeadZone");
        return;
    }

    assert((*sz).part == partID);
    assert((*sz).group == groupID);
    assert((*sz).zone == zoneID);

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

    messageController->scheduleAudioThreadCallbackUnderStructureLock(
        [p = partID, g = groupID, z = zoneID, sID = variantID, sample = *sid](auto &e) {
            auto &zone = e.getPatch()->getPart(p)->getGroup(g)->getZone(z);
            zone->terminateAllVoices();
            zone->variantData.variants[sID].sampleID = sample;
            zone->variantData.variants[sID].active = true;
            zone->attachToSample(*e.getSampleManager(), sID,
                                 (Zone::SampleInformationRead)(Zone::LOOP | Zone::ENDPOINTS));
        },
        [p = partID, g = groupID, z = zoneID](auto &e) {
            e.getSelectionManager()->selectAction({p, g, z, true, true, true});
        });
}

void Engine::loadCompoundElementIntoSelectedPartAndGroup(const sample::compound::CompoundElement &p,
                                                         int16_t rootKey, KeyboardRange krange,
                                                         VelocityRange vrange)
{
    if (p.type == sample::compound::CompoundElement::INSTRUMENT)
    {
        if (extensionMatches(p.sampleAddress.path, ".sf2"))
        {
            // TODO ok this refresh and restart is a bit unsatisfactory
            messageController->stopAudioThreadThenRunOnSerial([this, p](const auto &) {
                auto res =
                    sf2_support::importSF2(p.sampleAddress.path, *this, p.sampleAddress.preset);
                messageController->restartAudioThreadFromSerial();
                serializationSendToClient(messaging::client::s2c_send_pgz_structure,
                                          getPartGroupZoneStructure(), *messageController);
            });
            return;
        }
        if (extensionMatches(p.sampleAddress.path, ".sfz"))
        {
            messageController->stopAudioThreadThenRunOnSerial([this, p](const auto &) {
                auto res = sfz_support::importSFZ(p.sampleAddress.path, *this);
                if (!res)
                    messageController->reportErrorToClient("SFZ Import Failed", "Dunno why");
                messageController->restartAudioThreadFromSerial();
                serializationSendToClient(messaging::client::s2c_send_pgz_structure,
                                          getPartGroupZoneStructure(), *messageController);
            });
            return;
        }
        if (extensionMatches(p.sampleAddress.path, ".gig"))
        {
            messageController->stopAudioThreadThenRunOnSerial([this, p](const auto &) {
                auto res =
                    gig_support::importGIG(p.sampleAddress.path, *this, p.sampleAddress.preset);
                messageController->restartAudioThreadFromSerial();
                serializationSendToClient(messaging::client::s2c_send_pgz_structure,
                                          getPartGroupZoneStructure(), *messageController);
            });
            return;
        }
        return;
    }

    auto osid = SampleID();
    auto sid = sampleManager->loadSampleByFileAddress(p.sampleAddress, osid);

    if (!sid.has_value())
    {
        messageController->reportErrorToClient(
            "Unable to load Compound Sample Element",
            "Sample load failed:\n\n" + p.sampleAddress.path.u8string() + "\n\n" +
                "More information may be available in the log file (menu/log)");
        return;
    }

    // 2. Create a zone object on this thread but don't add it
    auto zptr = std::make_unique<Zone>(*sid);
    // TODO fixme
    zptr->mapping.keyboardRange = krange;
    zptr->mapping.velocityRange = vrange;
    zptr->mapping.rootKey = rootKey;
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

void Engine::loadSampleIntoSelectedPartAndGroup(const fs::path &p, int16_t rootKey,
                                                KeyboardRange krange, VelocityRange vrange)
{
    assert(messageController->threadingChecker.isSerialThread());

    // TODO: Deal with compound types more comprehensively
    // If you add a type here add it to Browser::isLoadableFile also
    if (extensionMatches(p, ".sf2"))
    {
        // TODO ok this refresh and restart is a bit unsatisfactory
        messageController->stopAudioThreadThenRunOnSerial([this, p](const auto &) {
            sf2_support::importSF2(p, *this, -1);
            messageController->restartAudioThreadFromSerial();
            serializationSendToClient(messaging::client::s2c_send_pgz_structure,
                                      getPartGroupZoneStructure(), *messageController);
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
                                      getPartGroupZoneStructure(), *messageController);
        });
        return;
    }
    else if (extensionMatches(p, ".exs"))
    {
        // TODO ok this refresh and restart is a bit unsatisfactory
        messageController->stopAudioThreadThenRunOnSerial([this, p](const auto &) {
            auto res = exs_support::importEXS(p, *this);
            if (!res)
                messageController->reportErrorToClient("EXS Import Failed", "Dunno why");
            messageController->restartAudioThreadFromSerial();
            serializationSendToClient(messaging::client::s2c_send_pgz_structure,
                                      getPartGroupZoneStructure(), *messageController);
        });
        return;
    }
    else if (extensionMatches(p, ".multisample"))
    {
        messageController->stopAudioThreadThenRunOnSerial([this, p](const auto &) {
            auto res = multisample_support::importMultisample(p, *this);
            if (!res)
                messageController->reportErrorToClient("SFZ Import Failed", "Dunno why");
            messageController->restartAudioThreadFromSerial();
            serializationSendToClient(messaging::client::s2c_send_pgz_structure,
                                      getPartGroupZoneStructure(), *messageController);
        });
        return;
    }
    if (extensionMatches(p, ".sf2"))
    {
        // TODO ok this refresh and restart is a bit unsatisfactory
        messageController->stopAudioThreadThenRunOnSerial([this, p](const auto &) {
            gig_support::importGIG(p, *this, -1);
            messageController->restartAudioThreadFromSerial();
            serializationSendToClient(messaging::client::s2c_send_pgz_structure,
                                      getPartGroupZoneStructure(), *messageController);
        });
        return;
    }
    if (extensionMatches(p, ".scm"))
    {
        patch_io::loadMulti(p, *this);
        return;
    }
    if (extensionMatches(p, ".scp"))
    {
        patch_io::loadPartInto(p, *this, getSelectionManager()->selectedPart);
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
    zptr->mapping.keyboardRange = krange;
    zptr->mapping.velocityRange = vrange;
    zptr->mapping.rootKey = rootKey;
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

void Engine::createEmptyZone(scxt::engine::KeyboardRange krange, scxt::engine::VelocityRange vrange)
{
    assert(messageController->threadingChecker.isSerialThread());

    // 2. Create a zone object on this thread but don't add it
    auto zptr = std::make_unique<Zone>();
    zptr->mapping.keyboardRange = krange;
    zptr->mapping.velocityRange = vrange;
    zptr->mapping.rootKey = (krange.keyStart + krange.keyEnd) / 2;
    zptr->givenName = "Empty Zone (" + std::to_string(zptr->id.id) + ")";

    // give it a name

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

void Engine::copyZone(const selection::SelectionManager::ZoneAddress &s)
{
    auto &zoneO = getPatch()->getPart(s.part)->getGroup(s.group)->getZone(s.zone);
    auto v = json::scxt_value(*zoneO);
    zoneClipboard = tao::json::msgpack::to_string(v);
}

void Engine::pasteZone(const selection::SelectionManager::ZoneAddress &a)
{
    if (zoneClipboard.empty())
        return;
    tao::json::events::transformer<tao::json::events::to_basic_value<json::scxt_traits>> consumer;
    tao::json::msgpack::events::from_string(consumer, zoneClipboard);
    auto v = std::move(consumer.value);
    auto zptr = std::make_unique<Zone>();
    v.to(*zptr);

    zptr->setupOnUnstream(*this);

    auto sp = a.part;
    ;
    auto sg = a.group;
    ;

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

void Engine::duplicateZone(const selection::SelectionManager::ZoneAddress &s)
{
    assert(messageController->threadingChecker.isSerialThread());

    // 2. Create a zone object on this thread but don't add it
    auto &zoneO = getPatch()->getPart(s.part)->getGroup(s.group)->getZone(s.zone);
    auto v = json::scxt_value(*zoneO);
    auto zptr = std::make_unique<Zone>();
    v.to(*zptr);

    zptr->setupOnUnstream(*this);

    // give it a name
    zptr->givenName = zoneO->getName() + " (copy)";

    // Drop into selected group logic goes here
    auto sp = s.part;
    ;
    auto sg = s.group;
    ;

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

void Engine::onSampleRateChanged()
{
    patch->setSampleRate(sampleRate);
    previewVoice->setSampleRate(sampleRate);

    messageController->forceStatusUpdate = true;
}

void Engine::registerVoiceModTarget(const voice::modulation::MatrixConfig::TargetIdentifier &t,
                                    vmodTgtStrFn_t pathFn, vmodTgtStrFn_t nameFn,
                                    vmodTgtBoolFn_t additiveFn, vmodTgtBoolFn_t enabledFn)
{
    voiceModTargets.emplace(t, std::make_tuple(pathFn, nameFn, additiveFn, enabledFn));
}

void Engine::registerVoiceModSource(const voice::modulation::MatrixConfig::SourceIdentifier &t,
                                    vmodSrcStrFn_t pathFn, vmodSrcStrFn_t nameFn)
{
    voiceModSources.emplace(t, std::make_pair(pathFn, nameFn));
}

void Engine::registerGroupModTarget(const modulation::GroupMatrixConfig::TargetIdentifier &t,
                                    gmodTgtStrFn_t pathFn, gmodTgtStrFn_t nameFn,
                                    gmodTgtBoolFn_t additiveFn, gmodTgtBoolFn_t enabledFn)
{
    groupModTargets.emplace(t, std::make_tuple(pathFn, nameFn, additiveFn, enabledFn));
}

void Engine::registerGroupModSource(const modulation::GroupMatrixConfig::SourceIdentifier &t,
                                    gmodSrcStrFn_t pathFn, gmodSrcStrFn_t nameFn)
{
    groupModSources.emplace(t, std::make_pair(pathFn, nameFn));
}

void Engine::terminateVoicesForZone(scxt::engine::Zone &z) { z.terminateAllVoices(); }

void Engine::terminateVoicesForGroup(scxt::engine::Group &g)
{
    for (auto &z : g)
    {
        terminateVoicesForZone(*z);
    }
}

void Engine::onTransportUpdated()
{
    sharedUIMemoryState.transportDisplay.tempo = transport.tempo;
    sharedUIMemoryState.transportDisplay.tsden = transport.signature.denominator;
    sharedUIMemoryState.transportDisplay.tsnum = transport.signature.numerator;
    sharedUIMemoryState.transportDisplay.hostpos = transport.hostTimeInBeats;
    sharedUIMemoryState.transportDisplay.timepos = transport.timeInBeats;

    updateTransportPhasors();
}

void Engine::updateTransportPhasors()
{
#if 0
    float mul = 1 << ((numTransportPhasors - 1) / 2);
    for (int i = 0; i < numTransportPhasors; ++i)
    {
        float rawBeat;
        transportPhasors[i] = std::modf((float)(transport.timeInBeats) * mul, &rawBeat);
        mul = mul / 2;
    }
#endif
}

std::optional<fs::path> Engine::setupUserStorageDirectory()
{
    auto installPath = sst::plugininfra::paths::sharedLibraryBinaryPath().parent_path();
    const std::string productName = "Shortcircuit XT";
    const std::string portableName = "ShortcircuitXTUserData";

    try
    {
        auto cp = installPath;

        while (cp.has_parent_path() && cp != cp.parent_path())
        {
            auto portable = cp / portableName;

            if (fs::is_directory(portable))
            {
                SCLOG("Using portable user directory: " << portable.u8string());
                return portable;
            }

            cp = cp.parent_path();
        }
    }
    catch (const fs::filesystem_error &e)
    {
    }

    try
    {
        auto res = sst::plugininfra::paths::bestDocumentsFolderPathFor(productName);
        if (!fs::is_directory(res))
            fs::create_directories(res);
        if (fs::is_directory(res))
        {
            SCLOG("Using system user directory: " << res.u8string());
            return res;
        }
    }
    catch (const fs::filesystem_error &e)
    {
    }

    try
    {
        auto cp = installPath;

        auto portable = installPath / portableName;
        if (!fs::is_directory(portable))
        {
            SCLOG("Creating user portable directory :" << portable.u8string());
            fs::create_directories(portable);
        }
        if (fs::is_directory(portable))
        {
            SCLOG("Using newly minted portable directory " << portable.u8string());
            messageController->reportErrorToClient(
                "Creating default portable directory",
                "Unable to find a user dir, short circuit created a portable directory at " +
                    portable.u8string());
            return portable;
        }
    }
    catch (fs::filesystem_error &e)
    {
    }

    SCLOG("Unable to determine user directory");
    messageController->reportErrorToClient(
        "Unable to make user directory",
        "Both regular and portable methods failed to make a user dir");
    return std::nullopt;
}

void Engine::sendFullRefreshToClient() const
{
    auto &cont = getMessageController();
    assert(cont->threadingChecker.isSerialThread());
    sendMetadataToClient();
    serializationSendToClient(messaging::client::s2c_send_pgz_structure,
                              getPartGroupZoneStructure(), *cont);
    if (getSelectionManager()->currentLeadZone(*this).has_value())
    {
        // We want to re-send the action as if a lead was selected.
        // TODO - make this API less gross
        selection::SelectionManager::SelectActionContents sac{
            *(getSelectionManager()->currentLeadZone(*this))};
        sac.distinct = false;
        getSelectionManager()->selectAction(sac);
    }
    for (int p = 0; p < numParts; ++p)
    {
        serializationSendToClient(
            messaging::client::s2c_send_part_configuration,
            messaging::client::partConfigurationPayload_t{p, getPatch()->getPart(p)->configuration},
            *(getMessageController()));
        for (int i = 0; i < macrosPerPart; ++i)
        {
            serializationSendToClient(
                messaging::client::s2c_update_macro_full_state,
                messaging::client::macroFullState_t{p, i, getPatch()->getPart(p)->macros[i]},
                *(getMessageController()));
        }
    }

    getSelectionManager()->sendGroupZoneMappingForSelectedPart();
    getSelectionManager()->sendClientDataForLeadSelectionState();
    getSelectionManager()->sendSelectedZonesToClient();
    getSelectionManager()->sendSelectedPartMacrosToClient();
    getSelectionManager()->sendOtherTabsSelectionToClient();

    auto missing = collectMissingResolutionWorkItems(*this);
    // send missing even if empty. An empty missing flags dont show dialog
    serializationSendToClient(messaging::client::s2c_send_missing_resolution_workitem_list, missing,
                              *(getMessageController()));
}

void Engine::clearAll()
{
    selectionManager = std::make_unique<selection::SelectionManager>(*this);
    for (auto &part : *patch)
    {
        for (auto &group : *part)
        {
            std::vector<ZoneID> zids;
            for (const auto &z : *group)
                zids.push_back(z->id);
            for (const auto &zid : zids)
            {
                group->removeZone(zid);
            }
        }

        std::vector<GroupID> gids;
        for (const auto &g : *part)
            gids.push_back(g->id);
        for (const auto &gid : gids)
        {
            part->removeGroup(gid);
        }
    }

    sampleManager->purgeUnreferencedSamples();
}

void Engine::setMacro01ValueFromPlugin(int part, int index, float value01)
{
    // Open Question: What about with paramFlush
    assert(getMessageController()->threadingChecker.isAudioThread());
    assert(part >= 0 && part < scxt::numParts);
    assert(index >= 0 && index < scxt::macrosPerPart);
    getPatch()->getPart(part)->macros[index].setValue01(value01);

    scxt::messaging::audio::AudioToSerialization a2s;
    a2s.id = messaging::audio::a2s_macro_updated;
    a2s.payloadType = scxt::messaging::audio::AudioToSerialization::INT;
    a2s.payload.i[0] = part;
    a2s.payload.i[1] = index;
    getMessageController()->sendAudioToSerialization(a2s);
}

void Engine::processMIDI1Event(uint16_t idx, const uint8_t data[3])
{
    sst::voicemanager::applyMidi1Message(voiceManager, idx, data);
}

void Engine::processNoteOnEvent(int16_t port, int16_t channel, int16_t key, int32_t note_id,
                                double velocity, float retune)
{
    voiceManager.processNoteOnEvent(port, channel, key, note_id, velocity, retune);
}
void Engine::processNoteOffEvent(int16_t port, int16_t channel, int16_t key, int32_t note_id,
                                 double velocity)
{
    voiceManager.processNoteOffEvent(port, channel, key, note_id, velocity);
}

void Engine::onPartConfigurationUpdated()
{
    auto midiM = voiceManager_t::MIDI1Dialect::MIDI1;
    auto anySolo = false;
    for (auto &p : *getPatch())
    {
        if (p->configuration.channel == Part::PartConfiguration::mpeChannel)
            midiM = voiceManager_t::MIDI1Dialect::MIDI1_MPE;
        if (p->configuration.solo)
            anySolo = true;
        p->configuration.muteDueToSolo = false;

        voiceManager.setPolyphonyGroupVoiceLimit((uint64_t)p.get(),
                                                 p->configuration.polyLimitVoices);
    }

    voiceManager.dialect = midiM;

    if (anySolo)
    {
        for (auto &p : *getPatch())
        {
            p->configuration.muteDueToSolo = !p->configuration.solo;
        }
    }
}
} // namespace scxt::engine
