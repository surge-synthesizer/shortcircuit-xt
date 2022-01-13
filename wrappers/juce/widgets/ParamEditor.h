//
// Created by Paul Walker on 1/9/22.
//

#ifndef SHORTCIRCUIT_PARAMEDITOR_H
#define SHORTCIRCUIT_PARAMEDITOR_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "DataInterfaces.h"

namespace scxt
{
namespace widgets
{
struct FloatParamEditor : public juce::Component
{
    enum Style
    {
        HSLIDER,
        VSLIDER,
        SPINBOX
    } style{HSLIDER};
    ParameterProxy<float> &param;

    FloatParamEditor(const Style &s, ParameterProxy<float> &p, ActionSender *snd)
        : juce::Component(), style(s), param(p), sender(snd)
    {
    }

    void paint(juce::Graphics &g) override;
    void paintHSlider(juce::Graphics &g);

    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;

    ActionSender *sender{nullptr};
};

struct IntParamEditor : public juce::Component
{
    enum Orientation
    {
        HORIZ,
        VERT
    } orientation{VERT};
    ParameterProxy<int> &param;
    ActionSender *sender{nullptr};
    int maxValue{0};

    std::vector<std::string> labels;
    IntParamEditor(const Orientation &o, ParameterProxy<int> &p, ActionSender *snd)
        : juce::Component(), orientation(o), param(p), sender(snd)
    {
    }

    void setLabels(const std::vector<std::string> &l)
    {
        labels = l;
        maxValue = labels.size();
        repaint();
    }
    void paint(juce::Graphics &g) override;
    void mouseUp(const juce::MouseEvent &e) override;
};
} // namespace widgets
} // namespace scxt

#endif // SHORTCIRCUIT_PARAMEDITOR_H
