//
// Created by Paul Walker on 2/11/23.
//

#ifndef SHORTCIRCUIT_ADSRPANE_H
#define SHORTCIRCUIT_ADSRPANE_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/data/Continuous.h"
#include "datamodel/adsr_storage.h"
#include "components/HasEditor.h"

namespace scxt::ui::multi
{
struct AdsrPane : sst::jucegui::components::NamedPanel, HasEditor
{
    // TODO Factor this better obviously
    struct attachment : sst::jucegui::data::ContinunousModulatable
    {
        float &value;
        std::string label;
        AdsrPane *parent{nullptr};
        attachment(AdsrPane *p, const std::string &l, float &v) : parent(p), value(v), label(l) {}

        std::string getLabel() const override { return label; }
        float getValue() const override { return value; }
        void setValueFromGUI(const float &f) override
        {
            value = f;
            parent->adsrChangedFromGui();
        }
        void setValueFromModel(const float &f) override { value = f; }

        float getModulationValuePM1() const override { return 0; }
        void setModulationValuePM1(const float &f) override {}
        bool isModulationBipolar() const override { return false; }
    };

    // ToDo: shapes of course
    std::unique_ptr<attachment> atA, atD, atS, atR;
    std::unique_ptr<sst::jucegui::components::VSlider> slA, slD, slS, slR;

    datamodel::AdsrStorage adsrView;
    int index{0};
    AdsrPane(SCXTEditor *, int index);

    void resized() override;

    void adsrChangedFromModel(const datamodel::AdsrStorage &);
    void adsrChangedFromGui();
};
} // namespace scxt::ui::multi

#endif // SHORTCIRCUIT_ADSRPANE_H
