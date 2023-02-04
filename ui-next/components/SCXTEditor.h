//
// Created by Paul Walker on 2/4/23.
//

#ifndef SHORTCIRCUIT_SCXTEDITOR_H
#define SHORTCIRCUIT_SCXTEDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "messaging/messaging.h"

namespace scxt::ui
{
struct SCXTEditor : juce::Component
{
    messaging::MessageController &msgCont;
    std::unique_ptr<juce::Button> button;

    struct IdleTimer : juce::Timer
    {
        SCXTEditor *editor{nullptr};
        IdleTimer(SCXTEditor *ed) : editor(ed) {}
        void timerCallback() override {
            editor->idle();
        }
    };
    std::unique_ptr<IdleTimer> idleTimer;

    SCXTEditor(messaging::MessageController &e) : msgCont(e)
    {
        msgCont.registerClient("SCXTEditor", [this](auto &s) { onCallback(s);});

        button = std::make_unique<juce::TextButton>("PressMe");
        button->setBounds(10, 10, 100, 30);
        button->onClick = [this]() { this->goForIt(); };
        addAndMakeVisible(*button);

        idleTimer = std::make_unique<IdleTimer>(this);
        idleTimer->startTimer(1000/60);
    }
    ~SCXTEditor() { msgCont.unregisterClient(); }

    void goForIt()
    {
        std::cout << "Going for it " << std::endl;
        msgCont.sendFromClient("hi " + std::to_string(rand() % 100));
    }
    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::yellow); }

    void onCallback(const std::string &s)
    {
        std::lock_guard<std::mutex> g(callbackMutex);
        callbackQueue.push(s);
    }

    void idle()
    {
        {
            std::lock_guard<std::mutex> g(callbackMutex);
            while (!callbackQueue.empty())
            {
                std::cout << "Callback " << callbackQueue.front() << std::endl;
                callbackQueue.pop();
            }
        }
    }

    std::mutex callbackMutex;
    std::queue<std::string> callbackQueue;
};
} // namespace scxt::ui

#endif // SHORTCIRCUIT_SCXTEDITOR_H
