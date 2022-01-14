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
#include <sstream>
#include "wrapper_msg_to_string.h"

namespace scxt
{
namespace widgets
{
template <typename P> inline void assertParamRangesSet(const P &p)
{
    if (p.id < 0)
        return;

    jassert(p.paramRangesSet);
    if (!p.paramRangesSet)
    {
        std::ostringstream oss;
        oss << "Param Ranges unset " << debug_wrapper_ip_to_string(p.id);
        DBG(oss.str());
    }
}
struct FloatParamSlider : public juce::Component
{
    enum Style
    {
        HSLIDER,
        VSLIDER
    } style{HSLIDER};
    std::reference_wrapper<ParameterProxy<float>> param;

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
    std::reference_wrapper<ParameterProxy<int>> param;
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
    std::reference_wrapper<ParameterProxy<T>> param;
    ActionSender *sender{nullptr};
    TParamSpinBox(ParameterProxy<T> &p, ActionSender *snd) : param(p), sender(snd) {}

    void paint(juce::Graphics &g) override
    {
        assertParamRangesSet(param.get());
        SCXTLookAndFeel::fillWithRaisedOutline(g, getLocalBounds(), juce::Colour(0xFF333355), true);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(9));
        g.setColour(juce::Colours::white);
        g.drawText(param.get().value_to_string(), getLocalBounds(), juce::Justification::centred);
    }

    // HACK. Think about this
    std::function<void()> onSend = []() {};

    float dragY{0};
    T valueJoggedBy(int dir)
    {
        if (param.get().paramRangesSet)
        {
            return std::clamp(param.get().val + dir * param.get().step, param.get().min,
                              param.get().max);
        }
        jassertfalse;
        return param.get().val + dir;
    }
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
            param.get().sendValue(valueJoggedBy(mv), sender);
            repaint();
            onSend();
        }
    }
};

template <> inline float TParamSpinBox<float>::valueJoggedBy(int dir)
{
    return std::clamp(param.get().val + dir * param.get().step, param.get().min, param.get().max);
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
    std::reference_wrapper<ParameterProxy<int>> param;
    ActionSender *sender{nullptr};
    const NameList &labels;

    IntParamComboBox(ParameterProxy<int> &p, ActionSender *snd, const NameList &labelRef)
        : param(p), sender(snd), labels(labelRef)
    {
        updateFromLabels();
        onChange = [this]() { sendChange(); };
    }
    ~IntParamComboBox() = default;

    void replaceParam(ParameterProxy<int> &p) { param = p; }

    static constexpr int idOff = 8675309;

    uint64_t lastUpdateCount{0};
    void updateFromLabels()
    {
        if (labels.update_count != lastUpdateCount)
        {
            clear(juce::dontSendNotification);
            for (const auto &[fidx, t] : sst::cpputils::enumerate(labels.data))
            {
                addItem(t, fidx + idOff);
            }
            lastUpdateCount = labels.update_count;
        }
        setSelectedId(param.get().val + idOff, juce::dontSendNotification);
        repaint();
    }

    void sendChange()
    {
        auto cidx = getSelectedId();
        auto ftype = cidx - idOff;
        param.get().sendValue(ftype, sender);
    }
};

} // namespace widgets
} // namespace scxt

#endif // SHORTCIRCUIT_PARAMEDITOR_H
