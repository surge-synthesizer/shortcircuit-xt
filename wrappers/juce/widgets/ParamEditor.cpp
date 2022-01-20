//
// Created by Paul Walker on 1/9/22.
//

#include "ParamEditor.h"
#include "SCXTLookAndFeel.h"
#include "sst/cpputils.h"

namespace scxt
{
namespace widgets
{

void FloatParamSlider::paint(juce::Graphics &g)
{
    if (param.get().hidden)
    {
        return;
    }

    assertParamRangesSet(param.get());
    switch (style)
    {
    case HSLIDER:
        paintHSlider(g);
        break;
    case VSLIDER:
        paintVSlider(g);
        break;
    default:
    {
        g.fillAll(juce::Colours::orchid);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(9));
        auto b = getLocalBounds().reduced(2, 0);
        g.setColour(juce::Colours::white);
        g.drawText(param.get().label, b, juce::Justification::left);
        g.drawText(param.get().valueToStringWithUnits(), b, juce::Justification::right);
    }
    break;
    }
}

void FloatParamSlider::paintHSlider(juce::Graphics &g)
{
    auto b = getLocalBounds();

    // Draw the tray
    auto tray = b.reduced(3, 0).withHeight(2).translated(0, getHeight() / 2.0 - 1);
    g.setColour(param.get().disabled ? juce::Colours::darkgrey : juce::Colours::white);
    g.fillRect(tray);

    // draw the label
    auto lab = b.withHeight(getHeight() / 2 - 2).translated(0, getHeight() / 2 + 2).reduced(3, 0);
    g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
    g.setColour(juce::Colours::white);
    g.drawText(param.get().label, lab, juce::Justification::left);
    g.drawText(param.get().valueToStringWithUnits(), lab, juce::Justification::right);

    // Draw the handle
    if (param.get().disabled)
        return;

    auto edgeToEdge = getHeight() - 6;
    auto radius = edgeToEdge * 0.5;
    auto f01 = param.get().getValue01();
    auto ctr = f01 * (getWidth() - edgeToEdge) + radius;
    auto x0 = ctr - radius;
    auto y0 = getHeight() * 0.5 - radius;
    g.setColour(juce::Colours::green.withAlpha(0.7f));
    g.fillEllipse(x0, y0, edgeToEdge, edgeToEdge);
    g.setColour(juce::Colours::white);
    g.drawEllipse(x0, y0, edgeToEdge, edgeToEdge, 1.0);
    g.drawLine(x0 + radius - 0.5, y0, x0 + radius - 0.5, y0 + edgeToEdge, 1.0);
}

void FloatParamSlider::paintVSlider(juce::Graphics &g)
{
    auto b = getLocalBounds();
    auto sb = b.withTrimmedBottom(8);
    auto tray = sb.reduced(0, 3).withWidth(2).translated(getWidth() / 2.0 - 1, 0);
    g.setColour(param.get().disabled ? juce::Colours::darkgrey : juce::Colours::white);
    g.fillRect(tray);

    g.setColour(juce::Colours::white);
    g.setFont(SCXTLookAndFeel::getMonoFontAt(9));
    auto lab = param.get().label;
    if (lab.empty())
        lab = fallbackLabel;
    g.drawText(lab, b, juce::Justification::centredBottom);

    // Draw the handle
    if (param.get().disabled)
        return;

    auto edgeToEdge = getWidth() - 6;
    auto radius = edgeToEdge * 0.5;
    auto f01 = param.get().getValue01();
    auto ctr = (1 - f01) * (getHeight() - edgeToEdge) + radius;
    auto y0 = ctr - radius;
    auto x0 = getWidth() * 0.5 - radius;
    g.setColour(juce::Colours::green.withAlpha(0.7f));
    g.fillEllipse(x0, y0, edgeToEdge, edgeToEdge);
    g.setColour(juce::Colours::white);
    g.drawEllipse(x0, y0, edgeToEdge, edgeToEdge, 1.0);
    g.drawLine(x0, y0 + radius - 0.5, x0 + edgeToEdge, y0 + radius - 0.5, 1.0);
}

void FloatParamSlider::mouseDrag(const juce::MouseEvent &e)
{
    if (param.get().disabled || param.get().hidden)
        return;

    if (style == HSLIDER)
    {
        auto xf = std::clamp(e.position.x / getWidth(), 0.f, 1.f);
        param.get().sendValue01(xf, sender);
    }
    else if (style == VSLIDER)
    {
        auto yf = std::clamp(1 - e.position.y / getHeight(), 0.f, 1.f);
        param.get().sendValue01(yf, sender);
    }
    repaint();
}

void FloatParamSlider::mouseUp(const juce::MouseEvent &e)
{
    if (param.get().disabled || param.get().hidden)
        return;

    if (style == HSLIDER)
    {
        auto xf = std::clamp(e.position.x / getWidth(), 0.f, 1.f);
        param.get().sendValue01(xf, sender);
    }
    else if (style == VSLIDER)
    {
        auto yf = std::clamp(1 - e.position.y / getHeight(), 0.f, 1.f);
        param.get().sendValue01(yf, sender);
    }
    repaint();
}

void IntParamMultiSwitch::paint(juce::Graphics &g)
{
    if (labels.size() == 0)
        return;
    if (param.get().hidden)
        return;

    for (const auto &[idx, l] : sst::cpputils::enumerate(labels))
    {
        auto r = hitRects[idx];
        if (param.get().disabled)
            SCXTLookAndFeel::fillWithRaisedOutline(g, r, juce::Colour(0xFF777777), true);
        else if (param.get().val == idx)
            SCXTLookAndFeel::fillWithRaisedOutline(g, r, juce::Colour(0xFFAA1515), true);
        else
            SCXTLookAndFeel::fillWithRaisedOutline(g, r, juce::Colour(0xFF151515), false);

        if (param.get().disabled)
            g.setColour(juce::Colour(0xFFAAAAAA));
        else
            g.setColour(juce::Colours::white);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
        g.drawText(l, r, juce::Justification::centred);
    }
}

void IntParamMultiSwitch::mouseUp(const juce::MouseEvent &e)
{
    if (param.get().disabled || param.get().hidden)
        return;

    for (const auto &[midx, r] : sst::cpputils::enumerate(hitRects))
    {
        if (r.toFloat().contains(e.position))
        {
            param.get().sendValue(midx, sender);
            repaint();
        }
    }
}
} // namespace widgets
} // namespace scxt