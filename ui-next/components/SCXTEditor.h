//
// Created by Paul Walker on 2/4/23.
//

#ifndef SCXT_UI_COMPONENTS_SCXTEDITOR_H
#define SCXT_UI_COMPONENTS_SCXTEDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "messaging/messaging.h"

namespace scxt::ui
{
struct SCXTEditor : juce::Component
{
    messaging::MessageController &msgCont;

    struct IdleTimer : juce::Timer
    {
        SCXTEditor *editor{nullptr};
        IdleTimer(SCXTEditor *ed) : editor(ed) {}
        void timerCallback() override { editor->idle(); }
    };
    std::unique_ptr<IdleTimer> idleTimer;

    SCXTEditor(messaging::MessageController &e) : msgCont(e)
    {
        idleTimer = std::make_unique<IdleTimer>(this);
        idleTimer->startTimer(1000 / 60);

        namespace cmsg = scxt::messaging::client;
        msgCont.registerClient("SCXTEditor", [this](auto &s) { onMessageCallback(s); });

        cmsg::clientSendMessage(cmsg::RefreshPatchRequest(), msgCont);
    }
    ~SCXTEditor()
    {
        idleTimer->stopTimer();
        msgCont.unregisterClient();
    }

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::yellow); }

    // This runs on the serialization thread and needs to toss to the UI thread
    void onMessageCallback(const std::string &s)
    {
        std::lock_guard<std::mutex> g(callbackMutex);
        callbackQueue.push(s);
    }

    void idle() { drainCallbackQueue(); }

    void drainCallbackQueue()
    {
        namespace cmsg = scxt::messaging::client;

        bool itemsToDrain{true};
        while (itemsToDrain)
        {
            itemsToDrain = false;
            std::string qmsg;
            {
                std::lock_guard<std::mutex> g(callbackMutex);
                if (!callbackQueue.empty())
                {
                    qmsg = callbackQueue.front();
                    itemsToDrain = true;
                    callbackQueue.pop();
                }
            }
            if (itemsToDrain)
            {
                cmsg::clientThreadExecuteSerializationMessage(qmsg, this);
            }
        }
    }

    void onPatchStreamed(const engine::Patch &p)
    {
        std::cout << "Patch Streamed "
                  << p.getPart(0)->getGroup(0)->getZone(0)->sampleID.to_string() << std::endl;
    }

    std::mutex callbackMutex;
    std::queue<std::string> callbackQueue;
};
} // namespace scxt::ui

#endif // SHORTCIRCUIT_SCXTEDITOR_H
