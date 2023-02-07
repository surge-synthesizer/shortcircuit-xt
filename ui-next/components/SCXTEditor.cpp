//
// Created by Paul Walker on 2/4/23.
//

#include "SCXTEditor.h"

#include "sst/jucegui/style/StyleSheet.h"

namespace scxt::ui
{

struct dc : sst::jucegui::data::ContinunousModulatable
{
    messaging::MessageController &mc;
    dc(messaging::MessageController &m) : mc(m) {}

    std::string getLabel() const override { return "Filter 1 Mix"; }
    float value{0.f};
    float getValue() const override { return value; }
    void setValueFromGUI(const float &f) override {
        messaging::client::clientSendMessage(messaging::client::TemporarySetZone0Filter1Mix(f),
                                             mc);
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
    msgCont.registerClient("SCXTEditor", [this](auto &s) { onMessageCallback(s); });

    auto leakThis = new dc(msgCont);
    sliderHack = std::make_unique<sst::jucegui::components::HSlider>();
    sliderHack->setBounds(10, 10, 300, 40);
    sliderHack->setSource(leakThis);
    addAndMakeVisible(*sliderHack);

    cmsg::clientSendMessage(cmsg::RefreshPatchRequest(), msgCont);
}
} // namespace scxt::ui