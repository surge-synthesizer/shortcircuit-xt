/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

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
#include "style/StyleSheet.h"

namespace scxt
{
namespace widgets
{
template <typename P> inline void assertParamRangesSet(P &p)
{
    if (p.id < 0)
        return;

    if (!p.complainedAboutParamRangesSet)
    {
        // jassert(p.paramRangesSet);
        if (!p.paramRangesSet)
        {
            std::ostringstream oss;
            oss << "Param Ranges unset " << debug_wrapper_ip_to_string(p.id);
            DBG(oss.str());
        }
        p.complainedAboutParamRangesSet = true;
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

struct FloatParamSlider : public juce::Component,
                          ParamRefMixin<float>,
                          style::DOMParticipant,
                          data::UIStateProxy::Invalidatable
{
    enum Style
    {
        HSLIDER,
        VSLIDER
    } style{HSLIDER};

    FloatParamSlider(const Style &s, data::ParameterProxy<float> &p, data::ActionSender *snd)
        : juce::Component(), style(s), ParamRefMixin<float>(p, snd), DOMParticipant("float_slider")
    {
        setIsDOMContainer(false);
    }

    std::string fallbackLabel;

    void paint(juce::Graphics &g) override;
    void paintHSlider(juce::Graphics &g);
    void paintVSlider(juce::Graphics &g);

    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;

    void onRebind() override { repaint(); }
    void onProxyUpdate() override { repaint(); }
};

struct IntParamMultiSwitch : public juce::Component,
                             ParamRefMixin<int>,
                             style::DOMParticipant,
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
        : juce::Component(), orientation(o), ParamRefMixin<int>(p, snd),
          DOMParticipant("multiswitch")
    {
        setEnabled(!param.get().disabled);
        setLabelsFromParam();
        setIsDOMContainer(false);
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
        setEnabled(!param.get().disabled);
        labels.clear();
        auto lb = param.get().getLabel();
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
        if (maxValue == 0 && !param.get().hidden && !param.get().getName().empty())
        {
            DBG("No information in the labels from " << param.get().getName());
        }

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

template <typename T>
struct TParamSpinBox : public juce::Component, ParamRefMixin<T>, style::DOMParticipant
{
    TParamSpinBox(data::ParameterProxy<T> &p, const style::Selector &sel, data::ActionSender *snd)
        : ParamRefMixin<T>(p, snd), DOMParticipant(sel)
    {
        textEd = std::make_unique<juce::TextEditor>();
        textEd->setFont(SCXTLookAndFeel::getMonoFontAt(9));
        textEd->onReturnKey = [this]() { sendTextEd(); };
        textEd->onEscapeKey = [this]() {
            textEd->setVisible(false);
            repaint();
        };
        textEd->onFocusLost = [this]() {
            textEd->setVisible(false);
            repaint();
        };
        addChildComponent(*textEd);
        setIsDOMContainer(false);
    }

    std::unique_ptr<juce::TextEditor> textEd;

    void resized() override { textEd->setBounds(getLocalBounds()); }

    void paint(juce::Graphics &g) override
    {
        assertParamRangesSet(ParamRefMixin<T>::param.get());
        if (textEd->isVisible())
            return;

        auto colFil = juce::Colour(0xFF333355);
        auto colText = juce::Colours::white;
        if (ParamRefMixin<T>::param.get().disabled)
        {
            colFil = juce::Colour(0xFF888899);
            colText = juce::Colour(0xFFCCCCDD);
        }
        SCXTLookAndFeel::fillWithRaisedOutline(g, getLocalBounds(), colFil, true);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(9));
        g.setColour(colText);
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
        // jassertfalse;
        return ParamRefMixin<T>::param.get().val + dir;
    }
    void mouseDown(const juce::MouseEvent &e) override { dragY = e.position.y; }
    void mouseDrag(const juce::MouseEvent &e) override
    {
        if (this->param.get().disabled || this->param.get().hidden)
            return;

        auto dy = e.position.y - dragY;
        int mv = 0;
        if (dy > 3)
        {
            mv = -1;
            dragY = e.position.y;
        }
        if (dy < -3)
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

    void mouseDoubleClick(const juce::MouseEvent &e) override
    {
        textEd->setVisible(true);
        textEd->setText(ParamRefMixin<T>::param.get().value_to_string(),
                        juce::dontSendNotification);
        textEd->grabKeyboardFocus();
        textEd->selectAll();
    }

    void sendTextEd()
    {
        auto newVal =
            std::clamp(stringToVal(textEd->getText().toStdString()),
                       ParamRefMixin<T>::param.get().min, ParamRefMixin<T>::param.get().max);
        ParamRefMixin<T>::param.get().sendValue(newVal, ParamRefMixin<T>::sender);
        onSend();
        textEd->setVisible(false);
        repaint();
    }

    T stringToVal(const std::string &s) { jassertfalse; }
};

template <> inline float TParamSpinBox<float>::stringToVal(const std::string &s)
{
    return std::atof(s.c_str());
}

template <> inline int TParamSpinBox<int>::stringToVal(const std::string &s)
{
    return std::atoi(s.c_str());
}

template <> inline float TParamSpinBox<float>::valueJoggedBy(int dir)
{
    return std::clamp(param.get().val + dir * param.get().step, param.get().min, param.get().max);
}

struct IntParamSpinBox : public TParamSpinBox<int>
{
    IntParamSpinBox(data::ParameterProxy<int> &p, data::ActionSender *snd)
        : TParamSpinBox<int>(p, style::Selector("int_spinbox"), snd)
    {
    }
};

struct FloatParamSpinBox : public TParamSpinBox<float>
{
    FloatParamSpinBox(data::ParameterProxy<float> &p, data::ActionSender *snd)
        : TParamSpinBox<float>(p, style::Selector("float_spinbox"), snd)
    {
    }
};

struct IntParamComboBox : public ComboBox,
                          ParamRefMixin<int>,
                          scxt::data::UIStateProxy::Invalidatable,
                          style::DOMParticipant
{
    const data::NameList &labels;
    IntParamComboBox(data::ParameterProxy<int> &p, const data::NameList &labelRef,
                     data::ActionSender *snd)
        : ParamRefMixin<int>(p, snd), labels(labelRef), DOMParticipant("combobox")
    {
        updateFromLabels();
        setEnabled(!param.get().disabled);
        onChange = [this]() { sendChange(); };
        setIsDOMContainer(false);
        setScrollWheelEnabled(true);
    }
    ~IntParamComboBox() = default;

    void replaceParam(data::ParameterProxy<int> &p) { param = p; }

    static constexpr int idOff = 8675309;

    uint64_t lastUpdateCount{0};
    void updateFromLabels()
    {
        setEnabled(!param.get().disabled);
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
        setEnabled(!param.get().disabled);
        setClickingTogglesState(true);
        setToggleState(param.get().val, juce::dontSendNotification);
        onStateChange = [this]() {
            bool ts = getToggleState();
            bool pv = param.get().val != 0;

            if (ts != pv)
                sendChange();
        };
        resetSelector("int_toggle");
    }

    void onProxyUpdate() override { updateState(); }
    void onRebind() override { updateState(); }
    void updateState()
    {
        setEnabled(!param.get().disabled);
        setToggleState(param.get().val, juce::dontSendNotification);
    }
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
                              scxt::data::UIStateProxy::Invalidatable,
                              style::DOMParticipant
{
    SingleLineTextEditor(data::ParameterProxy<std::string> &p, data::ActionSender *snd)
        : juce::TextEditor(p.val), ParamRefMixin<std::string>(p, snd), DOMParticipant("textfield")
    {
        setText(p.val, juce::dontSendNotification);
        setFont(SCXTLookAndFeel::getMonoFontAt(10));
        setColour(juce::TextEditor::textColourId, juce::Colours::white);
        onReturnKey = [this]() { sendUpdate(); };
        setIsDOMContainer(false);
    }

    void sendUpdate() { param.get().sendValue(getText().toStdString(), sender); }
    void updateText()
    {
        setText(param.get().val, juce::dontSendNotification);
        repaint();
    }
    void onProxyUpdate() override { updateText(); }
    void onRebind() override { updateText(); }
};

} // namespace widgets
} // namespace scxt

#endif // SHORTCIRCUIT_PARAMEDITOR_H
