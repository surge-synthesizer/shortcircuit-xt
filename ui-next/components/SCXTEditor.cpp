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

#include "SCXTEditor.h"

#include "sst/jucegui/style/StyleSheet.h"

#include "HeaderRegion.h"
#include "MultiScreen.h"
#include "SendFXScreen.h"

namespace scxt::ui
{

struct dc : sst::jucegui::data::ContinunousModulatable
{
    /*
  auto leakThis = new dc(msgCont);
  sliderHack = std::make_unique<sst::jucegui::components::HSlider>();
  sliderHack->setBounds(10, 10, 300, 40);
  sliderHack->setSource(leakThis);
  addAndMakeVisible(*sliderHack);
*/
    messaging::MessageController &mc;
    dc(messaging::MessageController &m) : mc(m) {}

    std::string getLabel() const override { return "Filter 1 Mix"; }
    float value{0.f};
    float getValue() const override { return value; }
    void setValueFromGUI(const float &f) override
    {
        //messaging::client::clientSendToSerialization(
        //    messaging::client::TemporarySetZone0Filter1Mix(f), mc);
        value = f;
    }
    void setValueFromModel(const float &f) override { value = f; }

    float getModulationValuePM1() const override { return 0; }
    void setModulationValuePM1(const float &f) override {}
    bool isModulationBipolar() const override { return false; }
};

SCXTEditor::SCXTEditor(messaging::MessageController &e) : msgCont(e)
{
    sst::jucegui::style::StyleSheet::initializeStyleSheets([]() {});

    idleTimer = std::make_unique<IdleTimer>(this);
    idleTimer->startTimer(1000 / 60);

    namespace cmsg = scxt::messaging::client;
    msgCont.registerClient("SCXTEditor", [this](auto &s) {
        // Remember this runs on the serialization thread so needs to be thread safe
        std::lock_guard<std::mutex> g(callbackMutex);
        callbackQueue.push(s);
    });

    headerRegion = std::make_unique<HeaderRegion>(this);
    addAndMakeVisible(*headerRegion);

    multiScreen = std::make_unique<MultiScreen>();
    addAndMakeVisible(*multiScreen);

    sendFxScreen = std::make_unique<SendFXScreen>();
    addChildComponent(*sendFxScreen);

    // TODO: This is les garbage but still a bit odd
    cmsg::clientSendToSerialization(cmsg::ProcessorDataRequest({0, 0, 0, 1}), msgCont);
}

SCXTEditor::~SCXTEditor() noexcept
{
    idleTimer->stopTimer();
    msgCont.unregisterClient();
}

void SCXTEditor::setActiveScreen(ActiveScreen s)
{
    switch (s)
    {
    case MULTI:
        multiScreen->setVisible(true);
        sendFxScreen->setVisible(false);
        resized();
        break;

    case SEND_FX:
        multiScreen->setVisible(false);
        sendFxScreen->setVisible(true);
        resized();
        break;
    }
}

void SCXTEditor::resized()
{
    static constexpr uint32_t headerHeight{34};
    headerRegion->setBounds(0, 0, getWidth(), headerHeight);

    if (multiScreen->isVisible())
        multiScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
    if (sendFxScreen->isVisible())
        sendFxScreen->setBounds(0, headerHeight, getWidth(), getHeight() - headerHeight);
}

void SCXTEditor::idle() { drainCallbackQueue(); }

void SCXTEditor::drainCallbackQueue()
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

} // namespace scxt::ui