//
// Created by Paul Walker on 8/25/22.
//

#ifndef SHORTCIRCUIT_SSTJUCEGUIDATAADAPTERS_H
#define SHORTCIRCUIT_SSTJUCEGUIDATAADAPTERS_H

#include <sst/jucegui/data/Continuous.h>
#include <sst/jucegui/data/Discrete.h>
#include "ParameterProxy.h"

namespace scxt::data::juceguiadapters
{
struct ContinuousFloat : public sst::jucegui::data::ContinunousModulatable,
                         data::ParameterProxy<float>::Listener
{
    data::ParameterProxy<float> &p;
    data::ActionSender *sender{nullptr};
    ContinuousFloat(data::ParameterProxy<float> &in, data::ActionSender *snd) : p(in), sender(snd)
    {
    }

    std::string getLabel() const override { return p.getLabel(); }
    bool isHidden() const override { return p.hidden; }
    float v = 0.2;
    float getValue() const override { return p.getValue01(); }
    std::string getValueAsString() const override { return p.valueToStringWithUnits(); }
    void setValueFromGUI(const float &f) override { p.sendValue01(f, sender); }
    void setValueFromModel(const float &f) override { v = f; }

    void onChanged() override {}

    float getModulationValuePM1() const override { return 0; }
    void setModulationValuePM1(const float &f) override {}
    bool isModulationBipolar() override { return false; }
};

struct Discrete : public sst::jucegui::data::Discrete, data::ParameterProxy<int>::Listener
{
    data::ParameterProxy<int> &p;
    data::ActionSender *sender{nullptr};
    Discrete(data::ParameterProxy<int> &in, data::ActionSender *snd) : p(in), sender(snd) {}

    void onChanged() override
    {
        labels.clear();
        min = 0;
        max = 1;
        if (p.label.empty())
            return;

        auto lb = p.label;
        auto ps = lb.find(";");
        if (ps != std::string::npos)
        {
            while (ps != std::string::npos)
            {
                labels.push_back(lb.substr(0, ps));
                lb = lb.substr(ps + 1);
                ps = lb.find(";");
            }
            labels.push_back(lb);
        }
        max = labels.size() - 1;
        if (max == 0 && !p.hidden && !p.getName().empty())
        {
            DBG("No information in the labels from " << p.getName());
        }
    }
    int min{0}, max{1};
    std::vector<std::string> labels;

    std::string getLabel() const override { return p.getLabel(); }
    bool isHidden() const override { return p.hidden; }
    int getValue() const override { return p.val; }
    std::string getValueAsStringFor(int i) const override
    {
        if (i >= 0 && i < labels.size())
            return labels[i];
        return "<err>";
    }
    void setValueFromGUI(const int &f) override { p.sendValue(f, sender); }
    void setValueFromModel(const int &f) override {}

    int getMin() const override { return min; }
    int getMax() const override { return max; }
};

struct BinaryDiscrete : sst::jucegui::data::BinaryDiscrete
{
    data::ParameterProxy<int> &p;
    data::ActionSender *sender{nullptr};
    BinaryDiscrete(data::ParameterProxy<int> &in, data::ActionSender *snd) : p(in), sender(snd) {}

    std::string getLabel() const override
    {
        std::cout << "LABEL IS " << p.getLabel() << std::endl;
        return p.getLabel();
    }
    bool isHidden() const override { return p.hidden; }
    int getValue() const override { return p.val; }
    void setValueFromGUI(const int &f) override { p.sendValue(f, sender); }
    void setValueFromModel(const int &f) override {}
};

} // namespace scxt::data::juceguiadapters

#endif // SHORTCIRCUIT_SSTJUCEGUIDATAADAPTERS_H
