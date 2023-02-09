/*
 * ShortCircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortCircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortCircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortCircuitXT is inspired by, and shares some code with, the
 * commercial product ShortCircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for ShortCircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortCircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "messaging/messaging.h"
#include "messaging/audio/audio_serial.h"
#include "client/client_serial.h"
#include "messaging/client/detail/client_serial_impl.h"
#include "client/client_messages.h"

namespace scxt::messaging
{
// Serialization Thread
void MessageController::parseAudioMessageOnSerializationThread(
    const audio::AudioToSerialization &as)
{
    // TODO - we could do the template shuffle later if this becomes too big
    switch (as.id)
    {
    case audio::a2s_voice_count:
        client::serializationSendToClient(client::s2c_voice_count, as.payload.u[0], *this);
        break;
    case audio::a2s_pointer_complete:
        returnAudioThreadCallback(static_cast<AudioThreadCallback *>(as.payload.p));
        break;
    case audio::a2s_note_on:
    case audio::a2s_note_off:
        throw std::logic_error("Implement this");
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
    if (cbStore.empty())
    {
        return new AudioThreadCallback();
    }
    auto res = cbStore.top();
    cbStore.pop();
    return res;
}

void MessageController::returnAudioThreadCallback(AudioThreadCallback *r) { cbStore.push(r); }

void MessageController::scheduleAudioThreadCallback(std::function<void(engine::Engine &)> cb)
{
    auto pt = getAudioThreadCallback();
    pt->f = std::move(cb);
    auto s2a = audio::SerializationToAudio();
    s2a.id = audio::s2a_dispatch_to_pointer;
    s2a.payload.p = (void *)pt;
    s2a.payloadType = audio::SerializationToAudio::VOID_STAR;

    serializationToAudioQueue.try_emplace(s2a);
}

void MessageController::runSerialization()
{
    while (shouldRun)
    {
        using namespace std::chrono_literals;

        clientToSerializationMessage_t inbound;
        bool receivedMessageFromClient{false};
        {
            std::unique_lock<std::mutex> lock(clientToSerializationMutex);
            while (shouldRun && clientToSerializationQueue.empty() &&
                   !(audioToSerializationQueue.peek()))
            {
                clientToSerializationConditionVar.wait_for(lock, 50ms);
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
                client::serializationThreadExecuteClientMessage(inbound, engine, *this);
            }

            // TODO: Drain SerToAudioQ if there's no audio thread

            audioToSerializationMessage_t msg;
            while (audioToSerializationQueue.try_dequeue(msg))
            {
                parseAudioMessageOnSerializationThread(msg);
            }
        }
        else
        {
            std::cout << "Serial thread complete" << std::endl;
        }
    }
}

void MessageController::registerClient(const std::string &nm, clientCallback_t &&f)
{
    std::lock_guard<std::mutex> g(clientToSerializationMutex);

    assert(!clientCallback);
    clientCallback = std::move(f);
    // TODO increase atomic
}
void MessageController::unregisterClient()
{
    std::lock_guard<std::mutex> g(clientToSerializationMutex);

    assert(clientCallback);
    clientCallback = nullptr;
    // TODO decrease atomic
}
void MessageController::sendRawFromClient(const clientToSerializationMessage_t &s)
{
    {
        std::lock_guard<std::mutex> g(clientToSerializationMutex);
        clientToSerializationQueue.push(s);
    }
    clientToSerializationConditionVar.notify_one();
}

} // namespace scxt::messaging