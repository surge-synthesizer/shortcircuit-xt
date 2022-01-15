//
// Created by Paul Walker on 1/14/22.
//

#ifndef SHORTCIRCUIT_PAGECOMPONENTBASE_H
#define SHORTCIRCUIT_PAGECOMPONENTBASE_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "widgets/ParamEditor.h"
#include "data/SCXTData.h"
#include "SCXTLookAndFeel.h"
#include "widgets/OutlinedTextButton.h"

namespace scxt
{
namespace pages
{
namespace contents
{

struct RowDivider
{
    juce::Rectangle<int> row;
    float howFar{0.f};
    int rx, ry;
    RowDivider(const juce::Rectangle<int> &r, int rx = 1, int ry = 1) : row(r), rx(rx), ry(ry) {}
    juce::Rectangle<int> next(float q)
    {
        auto res = row.withWidth(q * row.getWidth())
                       .translated(howFar * row.getWidth(), 0)
                       .reduced(rx, ry);
        howFar += q;
        return res;
    }

    juce::Rectangle<int> rest() { return next(1.0 - howFar); }
};

template <typename T>
struct PageContentBase : public juce::Component, scxt::data::UIStateProxy::Invalidatable
{
    PageContentBase(const T &p, const std::string &header, const juce::Colour &col)
        : parentPage(p), header(header), col(col)
    {
    }
    static constexpr int headerSize = 16;
    void paint(juce::Graphics &g) override
    {
        g.fillAll(col);
        auto h = getLocalBounds().withHeight(headerSize);
        SCXTLookAndFeel::fillWithGradientHeaderBand(g, h, col);
        g.setColour(juce::Colours::white);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
        g.drawText(header, h, juce::Justification::centred);

        auto b = getLocalBounds().withTrimmedBottom(headerSize);
        auto gs = juce::Graphics::ScopedSaveState(g);
        g.addTransform(juce::AffineTransform().translated(0, headerSize));
        paintContentInto(g, b);
    }
    juce::Rectangle<int> getContentsBounds() { return getLocalBounds().withTrimmedTop(headerSize); }
    virtual void paintContentInto(juce::Graphics &g, const juce::Rectangle<int> &bounds) {}
    void onProxyUpdate() override
    {
        for (auto q : comboWeakRefs)
            q->updateFromLabels();
    }
    std::string header;
    juce::Colour col;

    std::vector<std::unique_ptr<widgets::OutlinedTextButton>> tabButtons;
    void activateTabs()
    {
        if (getTabCount() <= 1)
            return;
        for (int i = 0; i < getTabCount(); ++i)
        {
            auto q = std::make_unique<widgets::OutlinedTextButton>(getTabLabel(i));
            q->onClick = [this, i]() {
                if (tabButtons[i]->getToggleState())
                    switchToTab(i);
            };
            q->setRadioGroupId(842, juce::NotificationType::dontSendNotification);
            q->setClickingTogglesState(true);
            q->setToggleState(i == 0, juce::NotificationType::dontSendNotification);
            addAndMakeVisible(*q);
            tabButtons.push_back(std::move(q));
        }
    }
    void resized() override
    {
        if (getTabCount() <= 1)
            return;

        auto bw = 40;
        auto hb = getLocalBounds().withHeight(headerSize).withWidth(bw);
        hb = hb.translated(getWidth() - (getTabCount()) * bw - 5, 0);
        for (const auto &t : tabButtons)
        {
            t->setBounds(hb.reduced(1, 1));
            hb = hb.translated(bw, 0);
        }
    }
    virtual int getTabCount() const { return 1; }
    virtual std::string getTabLabel(int i) const { return ""; }
    virtual void switchToTab(int i) {}

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

    template <typename W, typename Q> auto rebind(const W &wid, Q &par)
    {
        wid->param = par;
        wid->onRebind();
        wid->repaint();
    }

    std::vector<widgets::IntParamComboBox *> comboWeakRefs;
    template <typename P> auto bindIntComboBox(P &param, const scxt::data::NameList &lab)
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
