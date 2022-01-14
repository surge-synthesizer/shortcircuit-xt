//
// Created by Paul Walker on 1/9/22.
//

#ifndef SHORTCIRCUIT_PARAMEDITOR_H
#define SHORTCIRCUIT_PARAMEDITOR_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "sst/cpputils.h"
#include "DataInterfaces.h"
#include "SCXTLookAndFeel.h"
#include "ComboBox.h"

namespace scxt
{
namespace widgets
{
struct FloatParamSlider : public juce::Component
{
    enum Style
    {
        HSLIDER,
        VSLIDER
    } style{HSLIDER};
    ParameterProxy<float> &param;

    FloatParamSlider(const Style &s, ParameterProxy<float> &p, ActionSender *snd)
        : juce::Component(), style(s), param(p), sender(snd)
    {
    }

    std::string fallbackLabel;

    void paint(juce::Graphics &g) override;
    void paintHSlider(juce::Graphics &g);
    void paintVSlider(juce::Graphics &g);

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

template <typename T> struct TParamSpinBox : public juce::Component
{
    ParameterProxy<T> &param;
    ActionSender *sender{nullptr};
    TParamSpinBox(ParameterProxy<T> &p, ActionSender *snd) : param(p), sender(snd) {}

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
    T valueJoggedBy(int dir) { return param.val + dir; }
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
            param.sendValue(valueJoggedBy(mv), sender);
            repaint();
            onSend();
        }
    }
};

template <> inline float TParamSpinBox<float>::valueJoggedBy(int dir)
{
    return std::clamp(param.val + dir * param.step, param.min, param.max);
}

struct IntParamSpinBox : public TParamSpinBox<int>
{
    IntParamSpinBox(ParameterProxy<int> &p, ActionSender *snd) : TParamSpinBox<int>(p, snd) {}
};

struct FloatParamSpinBox : public TParamSpinBox<float>
{
    FloatParamSpinBox(ParameterProxy<float> &p, ActionSender *snd) : TParamSpinBox<float>(p, snd) {}
};

struct IntParamComboBox : public ComboBox
{
    ParameterProxy<int> &param;
    ActionSender *sender{nullptr};
    const std::vector<std::string> &labels;

    IntParamComboBox(ParameterProxy<int> &p, ActionSender *snd,
                     const std::vector<std::string> &labelRef)
        : param(p), sender(snd), labels(labelRef)
    {
        updateFromLabels();
        onChange = [this]() { sendChange(); };
    }
    ~IntParamComboBox() = default;

    static constexpr int idOff = 8675309;

    void updateFromLabels()
    {
        clear(juce::dontSendNotification);
        for (const auto &[fidx, t] : sst::cpputils::enumerate(labels))
        {
            addItem(t, fidx + idOff);
        }
        setSelectedId(param.val + idOff, juce::dontSendNotification);
        repaint();
    }

    void sendChange()
    {
        auto cidx = getSelectedId();
        auto ftype = cidx - idOff;
        param.sendValue(ftype, sender);
    }
};

} // namespace widgets
} // namespace scxt

#endif // SHORTCIRCUIT_PARAMEDITOR_H
