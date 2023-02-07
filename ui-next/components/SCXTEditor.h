//
// Created by Paul Walker on 2/4/23.
//

#ifndef SCXT_UI_COMPONENTS_SCXTEDITOR_H
#define SCXT_UI_COMPONENTS_SCXTEDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "messaging/messaging.h"
#include "sst/jucegui/components/HSlider.h"

namespace scxt::ui
{

struct HeaderRegion;
struct MultiScreen;
struct SendFXScreen;

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

    std::unique_ptr<HeaderRegion> headerRegion;
    std::unique_ptr<MultiScreen> multiScreen;
    std::unique_ptr<SendFXScreen> sendFxScreen;

    std::unique_ptr<sst::jucegui::components::HSlider> sliderHack;

    SCXTEditor(messaging::MessageController &e);
    virtual ~SCXTEditor() noexcept;

    enum ActiveScreen
    {
        MULTI,
        SEND_FX
    };
    void setActiveScreen(ActiveScreen s);

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::black); }
    void resized() override;

    void idle();
    void drainCallbackQueue();

    void onVoiceCount(const uint32_t &v)
    {
        std::cout << "SCXT EDITOR:: Vouce Count " << v << std::endl;
    }

    void onProcessorUpdated(const engine::Engine::processorAddress_t &,
                            const dsp::processor::ProcessorStorage &)
    {
        std::cout << "Processor udpated" << std::endl;
    }

    std::mutex callbackMutex;
    std::queue<std::string> callbackQueue;
};
} // namespace scxt::ui

#endif // SHORTCIRCUIT_SCXTEDITOR_H
