//
// Created by Paul Walker on 2/4/23.
//

#ifndef SHORTCIRCUIT_MESSAGING_H
#define SHORTCIRCUIT_MESSAGING_H
#include "engine/engine.h"
#include "utils.h"

#include <thread>
#include <queue>
#include <chrono>

#include "readerwriterqueue.h"

namespace scxt::messaging
{
/**
 * The SCXT Next messaging model
 *
 * We want SCXT "engine" to be able to provide a JSON interface at its edge to
 * a consumer, but obviously we don't want those messages on the audio thread (all
 * that string allocation!! all that parsing time) but also we don't want to require
 * a UI thread to run (what if we want a headless server mode, or an HTTP interface!)
 *
 * So our messaging strategy ends up with three thread
 *
 * Engine Audio Thread
 *    - Runs ::process
 *    - Can't allocate memory
 *    - Cant use anything other than lock free data structures
 *
 * Engine Serialization Thread
 *     - Runs its own ::run
 *     - Can allocate
 *     - Can do read-only access to Engine memrory (same process as the audio thread)
 *     - Can use system mutex/cond var etc.. to communicate with anything other than audio
 *
 * Client Thread (aka UI Thread)
 *      - Can do wahtever it wants except...
 *      - Talk to the engine audio thread or engine data structures (separate memory)
 *
 * So that results in the following data structures
 *
 * Audio <-> Serialization
 *    - A pair of lock free queues called "serializationToAudio" and "audioToSeralization"
 *    - Fixed data structures only in audioToSerialization (no alloc)
 *    - If serializatToAudio has allocated structues, the engine hands them back for future freeing
 *    - Engine has an atomic bool (or perhaps int) of client connected (or client count)
 *
 *    in Engine::process
 *       - drain serializationToAudio and respond. These will almost exclusively be
 *         state mutation operators
 *       - send updates back. These will be things like noteon-off display indicates,
 *         parameter change displays and so on. Only do this if a client is connected.
 *
 * Client <-> Serialization
 *    - A queue locked by a mutex probably
 *    - A condition variable for queue mutation notifications
 *    - A callback function to send a response to the client which can be called
 *      from the serialization thread. (In the out-of-process model this would be
 *      a socket write most likely).
 *
 * Serialization::run
 *    - basically wait on the condition variable if the queues are emtpy
 *    - then drain the audioToSerialization queue and generate the associated outbound
 *      messages by calling the ui callback (but don't hold the lock while doing this)
 *    - The serialization thread blocks for inbound from the UI and
 *      does a check of the outbound thread every 50ms or so. If you
 *      want a faster outbound process send an idle wakeup message
 *
 * We encapsulate this in a few objects here.
 *
 * The MessageController is an object owned by the engine, constructed with an engine & blah
 *
 */

struct MessageController : MoveableOnly<MessageController>
{
  private:
    engine::Engine &engine;

  public:
    MessageController(engine::Engine &e) : engine(e) {}

    /**
     * start / stop
     * called from thread which constructs the Engine once.
     */
    void start()
    {
        shouldRun = true;
        serializationThread = std::make_unique<std::thread>([this]() { this->runSerialization(); });
    }
    void stop()
    {
        // TODO: Send queue goes away interrupt message
        shouldRun = false;
        clientToSerializationConditionVar.notify_all();

        serializationThread->join();
        serializationThread.reset(nullptr);
    }

    typedef std::function<void(const std::string &)> clientCallback_t;
    clientCallback_t clientCallback{nullptr};
    void registerClient(const std::string &nm, clientCallback_t &&f)
    {
        std::lock_guard<std::mutex> g(clientToSerializationMutex);

        assert(!clientCallback);
        clientCallback = std::move(f);
        // TODO increase atomic
    }
    void unregisterClient()
    {
        std::lock_guard<std::mutex> g(clientToSerializationMutex);

        assert(clientCallback);
        clientCallback = nullptr;
        // TODO decrease atomic
    }
    void sendFromClient(const std::string &s)
    {
        {
            std::lock_guard<std::mutex> g(clientToSerializationMutex);
            clientToSerializationQueue.push(s);
        }
        clientToSerializationConditionVar.notify_one();
    }

    void runSerialization()
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
                    std::cout << "Got an inbound message " << inbound << std::endl;
                    serializationToAudioQueue.try_enqueue(17);
                    auto res = "backatcha " + inbound;
                    clientCallback(res);
                    // TODO: handle the inbounds
                }

                // TODO: Drain audioToSerialization
                audioToSerializationMessage_t msg;
                while (audioToSerializationQueue.try_dequeue(msg))
                {
                    std::cout << "INBOUND " << msg << std::endl;
                }
            }
            else
            {
                std::cout << "Goodnight from the serial thread!" << std::endl;
            }
        }
    }

    friend class engine::Engine;

  private:
    // TODO: Fix these types
    typedef int serializationToAudioMessage_t;
    typedef int audioToSerializationMessage_t;
    moodycamel::ReaderWriterQueue<serializationToAudioMessage_t, 1024> serializationToAudioQueue{
        128};
    moodycamel::ReaderWriterQueue<audioToSerializationMessage_t, 1024> audioToSerializationQueue{
        128};

    typedef std::string clientToSerializationMessage_t;
    std::queue<clientToSerializationMessage_t> clientToSerializationQueue;
    std::mutex clientToSerializationMutex;
    std::condition_variable clientToSerializationConditionVar;

    int serializationToClientCallback;

    std::atomic<int> clientCount{0};
    std::atomic<bool> shouldRun{false};

    std::unique_ptr<std::thread> serializationThread;
};
} // namespace scxt::messaging
#endif // SHORTCIRCUIT_MESSAGING_H
