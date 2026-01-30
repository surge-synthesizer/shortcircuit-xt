/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "messaging/messaging.h"
#include "messaging/audio/audio_serial.h"
#include "client/client_serial.h"
#include "messaging/client/detail/client_serial_impl.h"
#include "client/client_messages.h"
#include "messaging/client/client_serial.h"

namespace scxt::messaging
{
// Serialization Thread
void MessageController::parseAudioMessageOnSerializationThread(
    const audio::AudioToSerialization &as)
{
    assert(threadingChecker.isSerialThread());
    // TODO - we could do the template shuffle later if this becomes too big
    switch (as.id)
    {
    case audio::a2s_pointer_complete:
        returnAudioThreadCallback(static_cast<AudioThreadCallback *>(as.payload.p));
        break;
    case audio::a2s_note_on:
    case audio::a2s_note_off:
        throw std::logic_error("Implement this");
    case audio::a2s_structure_refresh:
        // TODO: Factor this a bit better
        serializationSendToClient(client::s2c_send_pgz_structure,
                                  engine.getPartGroupZoneStructure(), *this);
        break;
    case audio::a2s_macro_updated:
    {
        int16_t pt = as.payload.i[0];
        int16_t idx = as.payload.i[1];

        macroSetValueCompressorUsed = true;
        macroSetValueCompressor[pt][idx] = true;
    }
    break;
    case audio::a2s_processor_refresh:
    {
        SCLOG_ONCE_IF(debug, "Processor Refresh Requestioned. TODO: Minimize this message "
                                 << (as.payload.i[0] ? "Zone" : "Group") << " slot "
                                 << as.payload.i[1]);
        engine.getSelectionManager()->sendClientDataForLeadSelectionState();
        break;
    }
    case audio::a2s_delete_this_pointer:
    {
        assert(as.payloadType == audio::AudioToSerialization::TO_BE_DELETED);
        switch (as.payload.delThis.type)
        {
        case audio::AudioToSerialization::ToBeDeleted::engine_Zone:
        {
            auto z = (engine::Zone *)(as.payload.delThis.ptr);
            delete z;
        }
        break;
        case audio::AudioToSerialization::ToBeDeleted::engine_Group:
        {
            auto g = (engine::Group *)(as.payload.delThis.ptr);
            delete g;
        }
        break;
        }
    }
    break;
    case audio::a2s_schedule_sample_purge:
    {
        engine.getSampleManager()->purgeUnreferencedSamples();
    }
    break;
    case audio::a2s_none:
        break;
    }
}

MessageController::~MessageController()
{
    while (!cbStore.empty())
    {
        auto res = cbStore.top();
        delete res;
        cbStore.pop();
    }
}

void MessageController::start()
{
    assert(!serializationThread);
    shouldRun = true;
    serializationThread = std::make_unique<std::thread>([this]() { this->runSerialization(); });
}

void MessageController::stop()
{
    assert(serializationThread);
    // TODO: Send queue goes away interrupt message
    shouldRun = false;
    clientToSerializationConditionVar.notify_all();

    serializationThread->join();
    serializationThread.reset(nullptr);
}
MessageController::AudioThreadCallback *MessageController::getAudioThreadCallback()
{
    assert(threadingChecker.isSerialThread());
    if (cbStore.empty())
    {
        return new AudioThreadCallback();
    }
    auto res = cbStore.top();
    cbStore.pop();
    return res;
}

void MessageController::returnAudioThreadCallback(AudioThreadCallback *r)
{
    assert(threadingChecker.isSerialThread());
    r->execCompleteOnSer(engine);
    cbStore.push(r);
}

void MessageController::scheduleAudioThreadFunctionCallback(
    audio::SerializationToAudioMessageId sid, std::function<void(engine::Engine &)> cb,
    std::function<void(const engine::Engine &)> sercb)
{
    assert(threadingChecker.isSerialThread());

    if (!localCopyOfIsAudioRunning)
    {
        // In this case our audio thread checks will be wrong.
        // We could elevate ourselves to audio thread for as econd or just...
        threadingChecker.bypassThreadChecks = true;
        if (cb)
            cb(engine);
        if (sercb)
            sercb(engine);

        threadingChecker.bypassThreadChecks = false;
    }
    else
    {
        auto pt = getAudioThreadCallback();
        pt->setFunction(cb);
        if (sercb)
            pt->setSerialCompleteFunction(sercb);
        else
            pt->nullSerialCompleteFunction();
        auto s2a = audio::SerializationToAudio();
        s2a.id = sid;
        s2a.payload.p = (void *)pt;
        s2a.payloadType = audio::SerializationToAudio::VOID_STAR;

        serializationToAudioQueue.push(s2a);
    }
}

void MessageController::stopAudioThreadThenRunOnSerial(
    std::function<void(const engine::Engine &)> f)
{
    assert(threadingChecker.isSerialThread());
    scheduleAudioThreadCallbackUnderStructureLock([](engine::Engine &e) { e.stopEngineRequests++; },
                                                  f);
}

void MessageController::restartAudioThreadFromSerial()
{
    assert(threadingChecker.isSerialThread());
    scheduleAudioThreadCallbackUnderStructureLock(
        [](engine::Engine &e) { e.stopEngineRequests--; });
}

void MessageController::runSerialization()
{
    threadingChecker.registerAsSerialThread();

    while (shouldRun)
    {
        using namespace std::chrono_literals;

        clientToSerializationMessage_t inbound;
        bool audioStateChanged{false};
        bool receivedMessageFromClient{false};
        {
            std::unique_lock<std::mutex> lock(clientToSerializationMutex);
            while (shouldRun && clientToSerializationQueue.empty() &&
                   (audioToSerializationQueue.empty()) && !audioStateChanged)
            {
                clientToSerializationConditionVar.wait_for(lock, 50ms);
                audioStateChanged = updateAudioRunning();
            }
            if (!clientToSerializationQueue.empty())
            {
                inbound = clientToSerializationQueue.front();
                clientToSerializationQueue.pop();
                receivedMessageFromClient = true;
            }
        }
        if (shouldRun)
        {
            if (receivedMessageFromClient)
            {
                std::lock_guard<std::mutex> g(engine.modifyStructureMutex);
                client::serializationThreadExecuteClientMessage(inbound, engine, *this);
                inboundClientMessageCount++;
#if BUILD_IS_DEBUG
                if (inboundClientMessageCount % 1000 == 0)
                {
                    SCLOG_IF(debug,
                             "Client -> Serial Message Count: " << inboundClientMessageCount);
                }
#endif
            }

            if (audioStateChanged && isClientConnected)
            {
                engine.sendEngineStatusToClient();
            }

            // TODO: Drain SerToAudioQ if there's no audio thread
            bool tryToDrain{true};
            prepareSerializationThreadForAudioQueueDrain();
            while (tryToDrain && !audioToSerializationQueue.empty())
            {
                auto msgopt = audioToSerializationQueue.pop();
                if (msgopt.has_value())
                {
                    std::lock_guard<std::mutex> g(engine.modifyStructureMutex);
                    parseAudioMessageOnSerializationThread(*msgopt);
                }
                else
                    tryToDrain = false;
            }
            serializationThreadPostAudioQueueDrain();
        }
        else
        {
        }
    }
}

bool MessageController::updateAudioRunning()
{
    assert(threadingChecker.isSerialThread());
    bool retval = false;
    if (!isAudioRunning && (localCopyOfEngineProcessRuns != engineProcessRuns))
    {
        isAudioRunning = true;
        retval = true;
    }
    else if (isAudioRunning && localCopyOfEngineProcessRuns == engineProcessRuns)
    {
        engineOffCountdown--;
        if (engineOffCountdown <= 0)
        {
            engineOffCountdown = engineOffCountdownInit;
            isAudioRunning = false;
            retval = true;
        }
    }
    else
    {
        engineOffCountdown = engineOffCountdownInit;
        if (localCopyOfIsAudioRunning != isAudioRunning)
        {
            retval = true;
        }
        localCopyOfEngineProcessRuns = engineProcessRuns;
    }
    localCopyOfIsAudioRunning = isAudioRunning;

    if (forceStatusUpdate)
    {
        retval = true;
        forceStatusUpdate = false;
    }
    return retval;
}

void MessageController::registerClient(const std::string &nm, clientCallback_t &&f)
{
    {
        auto lk = acquireClientCallbackMutex();

        assert(!clientCallback);
        clientCallback = std::move(f);
    }

    threadingChecker.addAsAClientThread();
    client::clientSendToSerialization(client::RegisterClient(true), *this);

    {
        auto lk = acquireClientCallbackMutex();

        for (const auto &pcc : preClientConnectionCache)
        {
            clientCallback(pcc);
        }

        preClientConnectionCache.clear();
    }

    isClientConnected = true;
}
void MessageController::unregisterClient()
{
    auto lk = acquireClientCallbackMutex();

    threadingChecker.removeAsAClientThread();

    assert(clientCallback);
    clientCallback = nullptr;
    isClientConnected = false;
}
void MessageController::sendRawFromClient(const clientToSerializationMessage_t &s)
{
    {
        std::lock_guard<std::mutex> g(clientToSerializationMutex);
        clientToSerializationQueue.push(s);

#if BUILD_IS_DEBUG
        c2sMessageCount++;
        c2sMessageBytes += s.size();
        if (c2sMessageCount % 100 == 0)
        {
            SCLOG_IF(debug, "Client -> Serial Message Count : "
                                << c2sMessageCount << " size " << c2sMessageBytes
                                << " avgmsg: " << 1.f * c2sMessageBytes / c2sMessageCount);
        }
#endif
    }
    clientToSerializationConditionVar.notify_one();
}

void MessageController::reportErrorToClient(const std::string &title, const std::string &body)
{
    SCLOG_IF(warnings, "Error: [" << title << "]");
    auto blog = body;
    auto pos{0};
    while ((pos = blog.find("\n", pos)) != std::string::npos)
    {
        blog[pos] = ' ';
    }
    SCLOG_IF(warnings, blog);
    client::serializationSendToClient(client::s2c_report_error, client::s2cError_t{title, body},
                                      *(this));
}

void MessageController::prepareSerializationThreadForAudioQueueDrain()
{
    assert(!macroSetValueCompressorUsed);
}
void MessageController::serializationThreadPostAudioQueueDrain()
{
    if (macroSetValueCompressorUsed)
    {
        for (int pt = 0; pt < numParts; ++pt)
        {
            for (int idx = 0; idx < macrosPerPart; ++idx)
            {
                if (macroSetValueCompressor[pt][idx])
                {
                    serializationSendToClient(
                        client::s2c_update_macro_value,
                        messaging::client::macroValue_t{
                            pt, idx, engine.getPatch()->getPart(pt)->macros[idx].value},
                        *this);
                    macroSetValueCompressor[pt][idx] = false;
                }
            }
        }

        macroSetValueCompressorUsed = false;
    }
}

void MessageController::updateClientActivityNotification(const std::string &msg, int idx)
{
    serializationSendToClient(client::s2c_send_activity_notification,
                              client::activityNotificationPayload_t{idx, msg},
                              *(engine.getMessageController()));
}

MessageController::ClientActivityNotificationGuard::ClientActivityNotificationGuard(
    const std::string &ms, MessageController &m)
    : man(m)
{
    man.updateClientActivityNotification(ms, 1);
}

MessageController::ClientActivityNotificationGuard::~ClientActivityNotificationGuard()
{
    man.updateClientActivityNotification("", 0);
}
} // namespace scxt::messaging