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
#include "part.h"
#include "voice/voice.h"
#include "dsp/data_tables.h"
#include "tuning/equal.h"
#include "messaging/messaging.h"
#include "messaging/audio/audio_messages.h"
#include "selection/selection_manager.h"
#include "sst/basic-blocks/mechanics/block-ops.h"

#include "gig.h"
#include "SF.h"

namespace scxt::engine
{

Engine::Engine()
{
    id.id = rand() % 1024;

    messageController = std::make_unique<messaging::MessageController>(*this);
    dsp::sincTable.init();
    dsp::dbTable.init();
    tuning::equalTuning.init();

    sampleManager = std::make_unique<sample::SampleManager>(messageController->threadingChecker);
    patch = std::make_unique<Patch>();
    patch->parentEngine = this;

    for (auto &v : voices)
        v = nullptr;

    voiceInPlaceBuffer.reset(new uint8_t[sizeof(scxt::voice::Voice) * maxVoices]);

    setStereoOutputs(1);
    selectionManager = std::make_unique<selection::SelectionManager>(*this);

    memoryPool = std::make_unique<MemoryPool>();

    messageController->start();
}

Engine::~Engine()
{
    for (auto &v : voices)
    {
        if (v)
        {
            if (v->playState != voice::Voice::OFF)
                v->release();

            v->~Voice();
            v = nullptr;
        }
    }
    messageController->stop();
}

void Engine::initiateVoice(const pathToZone_t &path)
{
    assert(zoneByPath(path));
    for (const auto &[idx, v] : sst::cpputils::enumerate(voices))
    {
        if (!v || v->playState == voice::Voice::OFF)
        {
            if (v)
            {
                voices[idx]->~Voice();
            }
            auto *dp = voiceInPlaceBuffer.get() + idx * sizeof(voice::Voice);
            const auto &z = zoneByPath(path);
            voices[idx] = new (dp) voice::Voice(z.get());

            voices[idx]->channel = path.channel;
            voices[idx]->key = path.key;
            voices[idx]->noteId = path.noteid;
            voices[idx]->setSampleRate(sampleRate, sampleRateInv);
            voices[idx]->attack();
            return;
        }
    }
}
void Engine::releaseVoice(const pathToZone_t &path)
{
    assert(zoneByPath(path));
    auto targetId = zoneByPath(path)->id;
    for (auto &v : voices)
    {
        if (v && v->playState != voice::Voice::OFF && v->zone->id == targetId && v->key == path.key)
        {
            v->release();
        }
    }
}

bool Engine::processAudio()
{
    namespace blk = sst::basic_blocks::mechanics;
#if BUILD_IS_DEBUG
    messageController->threadingChecker.registerAsAudioThread();
#endif

    messaging::MessageController::serializationToAudioMessage_t msg;
    while (messageController->serializationToAudioQueue.try_dequeue(msg))
    {
        switch (msg.id)
        {
        case messaging::audio::s2a_dispatch_to_pointer:
        {
            auto cb =
                static_cast<messaging::MessageController::AudioThreadCallback *>(msg.payload.p);
            cb->exec(*this);

            messaging::audio::AudioToSerialization rt;
            rt.id = messaging::audio::a2s_pointer_complete;
            rt.payloadType = messaging::audio::AudioToSerialization::VOID_STAR;
            rt.payload.p = (void *)cb;
            messageController->audioToSerializationQueue.try_emplace(rt);
        }
        break;
        case messaging::audio::s2a_none:
            break;
        }
    }

    // TODO these memsets are probably gratuitous
    memset(output, 0, sizeof(output));
    if (stopEngineRequests > 0)
    {
        return true;
    }

    // TODO This gets ripped out when voice management imporves
    auto av = activeVoiceCount();

    for (const auto &part : *patch)
    {
        if (part->isActive())
        {
            part->process();
            for (int i = 0; i < part->getNumOutputs(); ++i)
            {
                blk::accumulate_from_to<blockSize>(part->output[i][0], output[i][0]);
                blk::accumulate_from_to<blockSize>(part->output[i][1], output[i][1]);
            }
        }
    }

    auto pav = activeVoiceCount();
    if (pav != av)
    {
        messaging::audio::sendVoiceCount(pav, *messageController);
    }

    return true;
}

void Engine::noteOn(int16_t channel, int16_t key, int32_t noteId, float velocity, float detune)
{
    // std::cout << "Engine::noteOn c=" << channel << " k=" << key << " nid=" << noteId
    //           << " v=" << velocity << std::endl;
    for (const auto &path : findZone(channel, key, noteId))
    {
        initiateVoice(path);
    }
    messaging::audio::sendVoiceCount(activeVoiceCount(), *messageController);
}
void Engine::noteOff(int16_t channel, int16_t key, int32_t noteId, float velocity)
{
    // std::cout << "Engine::noteOff c=" << channel << " k=" << key << " nid=" << noteId <<
    // std::endl;

    for (const auto &path : findZone(channel, key, noteId))
    {
        releaseVoice(path);
    }
    messaging::audio::sendVoiceCount(activeVoiceCount(), *messageController);
}

uint32_t Engine::activeVoiceCount()
{
    uint32_t res{0};
    for (const auto v : voices)
    {
        if (v)
            res += (v->playState != voice::Voice::OFF);
    }
    return res;
}

const std::optional<dsp::processor::ProcessorStorage>
Engine::getProcessorStorage(const processorAddress_t &addr) const
{
    auto [part, group, zone, which] = addr;
    assert(part >= 0 && part < Patch::numParts);
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
    return res;
}

void Engine::loadSampleIntoSelectedPartAndGroup(const fs::path &p)
{
    assert(messageController->threadingChecker.isSerialThread());

    auto e = p.extension();
    // TODO: Deal with compound types more comprehensively
    if (e == ".sf2")
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

    // OK so what we want to do now is
    // 1. Load this sample on this thread
    auto sid = sampleManager->loadSampleByPath(p);

    if (!sid.has_value())
    {
        SCDBGCOUT << "Unable to load sample " << p.u8string() << std::endl;
        return;
    }

    // 2. Create a zone object on this thread but don't add it
    auto zptr = std::make_unique<Zone>(*sid);
    // TODO fixme
    zptr->mapping.keyboardRange = {48, 72};
    zptr->mapping.rootKey = 60;
    zptr->attachToSample(*sampleManager);

    auto [sp, sg, sz] =
        selectionManager->getSelectedZone().value_or(selection::SelectionManager::ZoneAddress());
    if (sp < 0)
        sp = 0;
    if (sg < 0)
        sg = 0;

    // 3. Send a message to the audio thread saying to add that zone and
    messageController->scheduleAudioThreadCallback(
        [sp = sp, sg = sg, zone = zptr.release()](auto &e) {
            std::unique_ptr<Zone> zptr;
            zptr.reset(zone);
            e.getPatch()->getPart(sp)->guaranteeGroupCount(sg + 1);
            e.getPatch()->getPart(sp)->getGroup(sg)->addZone(zptr);

            // 4. have the audio thread message back here to refresh the ui
            messaging::audio::sendStructureRefresh(*(e.getMessageController()));
        });
}

void Engine::sendMetadataToClient() const
{
    // On register send metadata
    messaging::client::serializationSendToClient(
        messaging::client::s2c_send_all_processor_descriptions,
        dsp::processor::getAllProcessorDescriptions(), *messageController);
}

void Engine::loadSf2MultiSampleIntoSelectedPart(const fs::path &p)
{
    assert(messageController->threadingChecker.isSerialThread());
    // TODO: Make this (obvioosly) do something else
    // TODO: Make this stop the engine as well duh
    SCDBGCOUT << "SF2 Load " << p.u8string() << std::endl;

    try
    {
        auto riff = std::make_unique<RIFF::File>(p.u8string());
        auto sf = std::make_unique<sf2::File>(riff.get());

        auto sz = getSelectionManager()->getSelectedZone();
        auto pt = 0;
        if (sz.has_value())
            pt = sz->part;

        auto &part = getPatch()->getPart(pt);
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

                using namespace std;
                auto s = sfsamp;
                auto reg = region;

                zn->mapping.pitchOffset += reg->coarseTune + reg->fineTune * 0.01;



                if (reg->GetEG1PreAttackDelay() > 0.001)
                {
                    std::cout << "ERROR: PreAttach Delay which we don't support" << std::endl;
                }
                auto s2a = [](double s)
                {
                    auto l2s = log2(s);
                    auto scs = l2s - sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMin;
                    auto ncs = scs / (sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMax - sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMin);
                    return std::clamp(ncs, 0., 1.);
                };

                auto sus2l = [](double s)
                {
                    auto db = -s / 10;
                    return pow(10.0, db * 0.05);
                };

                zn->aegStorage.a = s2a(reg->GetEG1Attack());
                zn->aegStorage.h = s2a(reg->GetEG1Hold());
                zn->aegStorage.d = s2a(reg->GetEG1Decay());
                zn->aegStorage.s = sus2l(reg->GetEG1Sustain());
                zn->aegStorage.r = s2a(reg->GetEG1Release());

                auto GetValue = [](const auto &val)
                {
                    if (val == sf2::NONE) return ToString("NONE");
                    return ToString(val);
                };

                zn->eg2Storage.a = s2a(reg->GetEG2Attack());
                zn->eg2Storage.h = s2a(reg->GetEG2Hold());
                zn->eg2Storage.d = s2a(reg->GetEG2Decay());
                zn->eg2Storage.s = sus2l(reg->GetEG2Sustain());
                zn->eg2Storage.r = s2a(reg->GetEG2Release());

                std::cout << "STUFF I HAVEN'T DEALT WITH YET" << std::endl;
                cout << "\t\t    Modulation Envelope Generator" << endl;
                cout << "\t\t\tPitch=" << GetValue(reg->modEnvToPitch) << "cents, Cutoff=";
                cout << GetValue(reg->modEnvToFilterFc) << "cents" << endl << endl;

                cout << "\t\t    Modulation LFO: Delay=" << ::sf2::ToSeconds(reg->delayModLfo) << "s, Frequency=";
                cout << ::sf2::ToHz(reg->freqModLfo) << "Hz, LFO to Volume=" << (reg->modLfoToVolume / 10) << "dB";
                cout << ", LFO to Filter Cutoff=" << reg->modLfoToFilterFc;
                cout << ", LFO to Pitch=" << reg->modLfoToPitch << endl;

                cout << "\t\t    Vibrato LFO:    Delay=" << ::sf2::ToSeconds(reg->delayVibLfo) << "s, Frequency=";
                cout << ::sf2::ToHz(reg->freqVibLfo) << "Hz, LFO to Pitch=" << reg->vibLfoToPitch << endl;

                cout << "\t\t\tModulators (" << reg->modulators.size() << ")" << endl;

                for (int i = 0; i < reg->modulators.size(); i++) {
                    cout << "\t\t\tModulator " << i << endl;
                    // PrintModulatorItem(&reg->modulators[i]);
                }

                cout << "\t" << s->Name << " (Depth: " << ((s->GetFrameSize() / s->GetChannelCount()) * 8);
                cout << ", SampleRate: " << s->SampleRate;
                cout << ", Pitch: " << ((int)s->OriginalPitch);
                cout << ", Pitch Correction: " << ((int)s->PitchCorrection) << endl;
                cout << "\t\tStart: " << s->Start << ", End: " << s->End;
                cout << ", Start Loop: " << s->StartLoop << ", End Loop: " << s->EndLoop << endl;
                cout << "\t\tSample Type: " << s->SampleType << ", Sample Link: " << s->SampleLink << ")"
                     << endl;

                if (s != NULL) {
                    if (reg->HasLoop) {
                        cout << ", Loop Start: " << reg->LoopStart << ", Loop End: " << reg->LoopEnd;
                    }
                    cout << endl;
                }
                cout << "\t\t    Key range=";

                cout << "\t\t    Initial cutoff frequency=";
                if (reg->initialFilterFc == ::sf2::NONE) cout << "None" << endl;
                else cout << reg->initialFilterFc << "cents" << endl;

                cout << "\t\t    Initial resonance=";
                if (reg->initialFilterQ == ::sf2::NONE) cout << "None" << endl;
                else cout << (reg->initialFilterQ / 10.0) << "dB" << endl;

                if (reg->exclusiveClass) cout << ", Exclusive group=" << reg->exclusiveClass;
                cout << endl;


                grp->addZone(zn);
            }
        }
    }
    catch (RIFF::Exception e)
    {
        messaging::client::serializationSendToClient(
            messaging::client::s2c_report_error,
            messaging::client::s2cError_t{"SF2 Load Error", e.Message}, *messageController);
        return;
    }
    catch (...)
    {
        return;
    }
}
} // namespace scxt::engine