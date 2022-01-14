//
// Created by Paul Walker on 1/9/22.
//

#ifndef SHORTCIRCUIT_PARAMEDITOR_H
#define SHORTCIRCUIT_PARAMEDITOR_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "DataInterfaces.h"
#include "SCXTLookAndFeel.h"

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

struct IntParamMultiSwitch : public juce::Component
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
    IntParamMultiSwitch(const Orientation &o, ParameterProxy<int> &p, ActionSender *snd)
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

struct IntParamSpinBox : public juce::Component
{
    ParameterProxy<int> &param;
    ActionSender *sender{nullptr};
    IntParamSpinBox(ParameterProxy<int> &p, ActionSender *snd) : param(p), sender(snd) {}

    void paint(juce::Graphics &g) override
    {
        SCXTLookAndFeel::fillWithRaisedOutline(g, getLocalBounds(), juce::Colour(0xFF333355), true);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(9));
        g.setColour(juce::Colours::white);
        g.drawText(param.value_to_string(), getLocalBounds(), juce::Justification::centred);
    }

    // HACK. Think about this
    std::function<void()> onSend = []() {};

    float dragY{0};
    void mouseDown(const juce::MouseEvent &e) override { dragY = e.position.y; }
    void mouseDrag(const juce::MouseEvent &e) override
    {
        auto dy = e.position.y - dragY;
        int mv = 0;
        if (dy > 10)
        {
            mv = -1;
            dragY = e.position.y;
        }
        if (dy < -10)
        {
            mv = 1;
            dragY = e.position.y;
        }
        if (mv)
        {
            param.sendValue(
                param.val + mv,
                sender); // std::clamp(param.val + param.step, param.min, param.max), sender);
            repaint();
            onSend();
        }
    }
};

} // namespace widgets
} // namespace scxt

#endif // SHORTCIRCUIT_PARAMEDITOR_H
