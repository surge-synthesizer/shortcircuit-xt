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

    void mouseUp(const juce::MouseEvent &e) override;

    ActionSender *sender{nullptr};
};
} // namespace widgets
} // namespace scxt

#endif // SHORTCIRCUIT_PARAMEDITOR_H
