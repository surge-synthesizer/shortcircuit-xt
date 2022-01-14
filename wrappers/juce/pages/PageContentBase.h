//
// Created by Paul Walker on 1/14/22.
//

#ifndef SHORTCIRCUIT_PAGECOMPONENTBASE_H
#define SHORTCIRCUIT_PAGECOMPONENTBASE_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "widgets/ParamEditor.h"
#include "DataInterfaces.h"
#include "SCXTLookAndFeel.h"

namespace scxt
{
namespace pages
{
namespace contents
{

template <typename T> struct PageContentBase : public juce::Component, UIStateProxy::Invalidatable
{
    PageContentBase(const T &p, const std::string &header, const juce::Colour &col)
        : parentPage(p), header(header), col(col)
    {
    }
    void paint(juce::Graphics &g) override
    {
        g.fillAll(col);
        auto h = getLocalBounds().withHeight(16);
        SCXTLookAndFeel::fillWithGradientHeaderBand(g, h, col);
        g.setColour(juce::Colours::white);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
        g.drawText(header, h, juce::Justification::centred);

        auto b = getLocalBounds().withTrimmedBottom(16);
        auto gs = juce::Graphics::ScopedSaveState(g);
        g.addTransform(juce::AffineTransform().translated(0, 16));
        paintContentInto(g, b);
    }
    juce::Rectangle<int> getContentsBounds() { return getLocalBounds().withTrimmedTop(16); }
    virtual void paintContentInto(juce::Graphics &g, const juce::Rectangle<int> &bounds) {}
    void onProxyUpdate() override
    {
        for (auto q : comboWeakRefs)
            q->updateFromLabels();
    }
    std::string header;
    juce::Colour col;

    template <typename W, typename... Args> auto bind(Args &&...args)
    {
        auto q = std::make_unique<W>(std::forward<Args>(args)..., parentPage.editor);
        addAndMakeVisible(*q);
        return q;
    }

    template <typename P> auto bindFloatSpinBox(P &param)
    {
        return bind<widgets::FloatParamSpinBox>(param);
    }
    template <typename P> auto bindFloatSlider(P &param, widgets::FloatParamSlider::Style o)
    {
        return bind<widgets::FloatParamSlider>(o, param);
    }
    template <typename P> auto bindFloatVSlider(P &param, const std::string &fallbackLabel)
    {
        auto res = bind<widgets::FloatParamSlider>(widgets::FloatParamSlider::VSLIDER, param);
        res->fallbackLabel = fallbackLabel;
        return res;
    }
    template <typename P> auto bindFloatHSlider(P &param)
    {
        return bind<widgets::FloatParamSlider>(widgets::FloatParamSlider::HSLIDER, param);
    }

    template <typename P> auto bindIntSpinBox(P &param)
    {
        return bind<widgets::IntParamSpinBox>(param);
    }

    std::vector<widgets::IntParamComboBox *> comboWeakRefs;
    template <typename P> auto bindIntComboBox(P &param, const std::vector<std::string> &lab)
    {
        auto q = std::make_unique<widgets::IntParamComboBox>(param, parentPage.editor, lab);
        comboWeakRefs.push_back(q.get());
        addAndMakeVisible(*q);
        return q;
    }

    auto whiteLabel(const std::string &txt)
    {
        auto q = std::make_unique<juce::Label>(txt);
        q->setText(txt, juce::dontSendNotification);
        q->setColour(juce::Label::textColourId, juce::Colours::white);
        q->setJustificationType(juce::Justification::centred);
        q->setFont(SCXTLookAndFeel::getMonoFontAt(9));
        addAndMakeVisible(*q);
        return q;
    }

    struct StandIn : public juce::Component
    {
        StandIn(const std::string &lab) : label(lab) {}
        void paint(juce::Graphics &g) override
        {
            g.fillAll(juce::Colours::yellow);
            g.setColour(juce::Colours::black);
            g.setFont(SCXTLookAndFeel::getMonoFontAt(9));
            g.drawText(label, getLocalBounds(), juce::Justification::centred);
        }
        std::string label;
    };

    auto standIn(const std::string &txt)
    {
        auto q = std::make_unique<StandIn>(txt);
        addAndMakeVisible(*q);
        return q;
    }
    const T &parentPage;
};
} // namespace contents
} // namespace pages
} // namespace scxt

#endif // SHORTCIRCUIT_PAGECOMPONENTBASE_H
