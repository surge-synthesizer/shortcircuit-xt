//
// Created by Paul Walker on 1/9/22.
//

#ifndef SHORTCIRCUIT_PARAMEDITOR_H
#define SHORTCIRCUIT_PARAMEDITOR_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "sst/cpputils.h"
#include "data/SCXTData.h"
#include "SCXTLookAndFeel.h"
#include "ComboBox.h"
#include <sstream>
#include "wrapper_msg_to_string.h"
#include "widgets/OutlinedTextButton.h"

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

template <typename T> struct ParamRefMixin
{
    typedef data::ParameterProxy<T> param_t;
    std::reference_wrapper<data::ParameterProxy<T>> param;
    data::ActionSender *sender{nullptr};

    ParamRefMixin(param_t &p, data::ActionSender *s) : param(p), sender(s) {}

    virtual void onRebind() {}
};

struct FloatParamSlider : public juce::Component, ParamRefMixin<float>
{
    enum Style
    {
        HSLIDER,
        VSLIDER
    } style{HSLIDER};

    FloatParamSlider(const Style &s, data::ParameterProxy<float> &p, data::ActionSender *snd)
        : juce::Component(), style(s), ParamRefMixin<float>(p, snd)
    {
    }

    std::string fallbackLabel;

    void paint(juce::Graphics &g) override;
    void paintHSlider(juce::Graphics &g);
    void paintVSlider(juce::Graphics &g);

    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
};

struct IntParamMultiSwitch : public juce::Component,
                             ParamRefMixin<int>,
                             data::UIStateProxy::Invalidatable
{
    enum Orientation
    {
        HORIZ,
        VERT
    } orientation{VERT};

    int maxValue{0};
    std::vector<std::string> labels;
    IntParamMultiSwitch(const Orientation &o, data::ParameterProxy<int> &p, data::ActionSender *snd)
        : juce::Component(), orientation(o), ParamRefMixin<int>(p, snd)
    {
        setLabelsFromParam();
    }

    void setLabels(const std::vector<std::string> &l)
    {
        jassertfalse;
        labels = l;
        maxValue = labels.size();
        repaint();
    }
    void paint(juce::Graphics &g) override;
    void mouseUp(const juce::MouseEvent &e) override;

    void onProxyUpdate() override { setLabelsFromParam(); }
    void setLabelsFromParam()
    {
        labels.clear();
        auto lb = param.get().label;
        auto p = lb.find(";");
        if (p != std::string::npos)
        {
            while (p != std::string::npos)
            {
                labels.push_back(lb.substr(0, p));
                lb = lb.substr(p + 1);
                p = lb.find(";");
            }
            labels.push_back(lb);
        }
        maxValue = labels.size();

        resetHitRects();
    }

    void resized() override { resetHitRects(); }

    void resetHitRects()
    {
        hitRects.clear();
        if (labels.empty())
            return;
        float tranx, trany;
        auto r = getLocalBounds();

        if (orientation == VERT)
        {
            auto bh = std::min(getHeight() * 1.f / labels.size(), 15.f);
            r = getLocalBounds().withHeight(bh);
            tranx = 0;
            trany = bh + 1;
        }
        else
        {
            auto bh = getWidth() * 1.f / labels.size();
            r = getLocalBounds().withWidth(bh);
            tranx = bh;
            trany = 0;
        }

        for (int i = 0; i < labels.size(); ++i)
        {
            hitRects.push_back(r);
            r = r.translated(tranx, trany);
        }
    }

    std::vector<juce::Rectangle<int>> hitRects;
};

template <typename T> struct TParamSpinBox : public juce::Component, ParamRefMixin<T>
{
    TParamSpinBox(data::ParameterProxy<T> &p, data::ActionSender *snd) : ParamRefMixin<T>(p, snd) {}

    void paint(juce::Graphics &g) override
    {
        assertParamRangesSet(ParamRefMixin<T>::param.get());
        SCXTLookAndFeel::fillWithRaisedOutline(g, getLocalBounds(), juce::Colour(0xFF333355), true);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(9));
        g.setColour(juce::Colours::white);
        g.drawText(ParamRefMixin<T>::param.get().value_to_string(), getLocalBounds(),
                   juce::Justification::centred);
    }

    // HACK. Think about this
    std::function<void()> onSend = []() {};

    float dragY{0};
    T valueJoggedBy(int dir)
    {
        if (ParamRefMixin<T>::param.get().paramRangesSet)
        {
            return std::clamp(ParamRefMixin<T>::param.get().val +
                                  dir * ParamRefMixin<T>::param.get().step,
                              ParamRefMixin<T>::param.get().min, ParamRefMixin<T>::param.get().max);
        }
        jassertfalse;
        return ParamRefMixin<T>::param.get().val + dir;
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
            ParamRefMixin<T>::param.get().sendValue(valueJoggedBy(mv), ParamRefMixin<T>::sender);
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
    IntParamSpinBox(data::ParameterProxy<int> &p, data::ActionSender *snd)
        : TParamSpinBox<int>(p, snd)
    {
    }
};

struct FloatParamSpinBox : public TParamSpinBox<float>
{
    FloatParamSpinBox(data::ParameterProxy<float> &p, data::ActionSender *snd)
        : TParamSpinBox<float>(p, snd)
    {
    }
};

struct IntParamComboBox : public ComboBox,
                          ParamRefMixin<int>,
                          scxt::data::UIStateProxy::Invalidatable
{
    const data::NameList &labels;
    IntParamComboBox(data::ParameterProxy<int> &p, const data::NameList &labelRef,
                     data::ActionSender *snd)
        : ParamRefMixin<int>(p, snd), labels(labelRef)
    {
        updateFromLabels();
        onChange = [this]() { sendChange(); };
    }
    ~IntParamComboBox() = default;

    void replaceParam(data::ParameterProxy<int> &p) { param = p; }

    static constexpr int idOff = 8675309;

    uint64_t lastUpdateCount{0};
    void updateFromLabels()
    {
        if (labels.update_count != lastUpdateCount)
        {
            clear(juce::dontSendNotification);
            for (const auto &[fidx, t] : sst::cpputils::enumerate(labels.data))
            {
                if (!t.empty())
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

    void onProxyUpdate() override { updateFromLabels(); }
    void onRebind() override { updateFromLabels(); }
};

struct IntParamToggleButton : public OutlinedTextButton,
                              ParamRefMixin<int>,
                              scxt::data::UIStateProxy::Invalidatable
{
    IntParamToggleButton(data::ParameterProxy<int> &p, const std::string &label,
                         data::ActionSender *snd)
        : OutlinedTextButton(label), ParamRefMixin<int>(p, snd)
    {
        setClickingTogglesState(true);
        setToggleState(param.get().val, juce::dontSendNotification);
        onStateChange = [this]() { sendChange(); };
    }

    void onProxyUpdate() override { setToggleState(param.get().val, juce::dontSendNotification); }
    void sendChange()
    {
        if (getToggleState())
            param.get().sendValue(1, sender);
        else
            param.get().sendValue(0, sender);
    }
};

struct SingleLineTextEditor : public juce::TextEditor,
                              ParamRefMixin<std::string>,
                              scxt::data::UIStateProxy::Invalidatable
{
    SingleLineTextEditor(data::ParameterProxy<std::string> &p, data::ActionSender *snd)
        : juce::TextEditor(p.val), ParamRefMixin<std::string>(p, snd)
    {
        setText(p.val, juce::dontSendNotification);
        setFont(SCXTLookAndFeel::getMonoFontAt(10));
        setColour(juce::TextEditor::textColourId, juce::Colours::white);
        onReturnKey = [this]() { sendUpdate(); };
    }

    void sendUpdate() { param.get().sendValue(getText().toStdString(), sender); }
    void onProxyUpdate() override { setText(param.get().val, juce::dontSendNotification); }
};

} // namespace widgets
} // namespace scxt

#endif // SHORTCIRCUIT_PARAMEDITOR_H
