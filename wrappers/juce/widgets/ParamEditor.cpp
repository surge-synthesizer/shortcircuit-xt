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

void FloatParamEditor::paint(juce::Graphics &g)
{
    switch (style)
    {
    case HSLIDER:
        paintHSlider(g);
        break;
    default:
    {
        g.fillAll(juce::Colours::orchid);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(9));
        auto b = getLocalBounds().reduced(2, 0);
        g.setColour(juce::Colours::white);
        g.drawText(param.label, b, juce::Justification::left);
        g.drawText(param.valueToStringWithUnits(), b, juce::Justification::right);
    }
    break;
    }
}

void FloatParamEditor::paintHSlider(juce::Graphics &g)
{
    auto b = getLocalBounds();

    // Draw the tray
    auto tray = b.reduced(3, 0).withHeight(2).translated(0, getHeight() / 2.0 - 1);
    g.setColour(juce::Colours::white);
    g.fillRect(tray);

    // draw the label
    auto lab = b.withHeight(getHeight() / 2 - 2).translated(0, getHeight() / 2 + 2).reduced(3, 0);
    g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
    g.setColour(juce::Colours::white);
    g.drawText(param.label, lab, juce::Justification::left);
    g.drawText(param.valueToStringWithUnits(), lab, juce::Justification::right);

    // Draw the handle
    auto edgeToEdge = getHeight() - 6;
    auto radius = edgeToEdge * 0.5;
    auto f01 = param.getValue01();

    auto ctr = f01 * (getWidth() - edgeToEdge) + radius;
    auto x0 = ctr - radius;
    auto y0 = getHeight() * 0.5 - radius;
    g.setColour(juce::Colours::green.withAlpha(0.7f));
    g.fillEllipse(x0, y0, edgeToEdge, edgeToEdge);
    g.setColour(juce::Colours::white);
    g.drawEllipse(x0, y0, edgeToEdge, edgeToEdge, 1.0);
    g.drawLine(x0 + radius - 0.5, y0, x0 + radius - 0.5, y0 + edgeToEdge, 1.0);
}

void FloatParamEditor::mouseDrag(const juce::MouseEvent &e)
{
    auto xf = std::clamp(e.position.x / getWidth(), 0.f, 1.f);
    param.sendValue01(xf, sender);
    repaint();
}

void FloatParamEditor::mouseUp(const juce::MouseEvent &e)
{
    auto xf = e.position.x / getWidth();
    param.sendValue01(xf, sender);
    repaint();
}

void IntParamEditor::paint(juce::Graphics &g)
{
    if (labels.size() == 0)
        return;

    auto bh = 20;
    auto r = getLocalBounds().withHeight(bh);
    for (const auto &[idx, l] : sst::cpputils::enumerate(labels))
    {
        if (param.val == idx)
            SCXTLookAndFeel::fillWithRaisedOutline(g, r, juce::Colour(0xFFAA1515), true);
        else
            SCXTLookAndFeel::fillWithRaisedOutline(g, r, juce::Colour(0xFF151515), false);
        g.setColour(juce::Colours::white);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
        g.drawText(l, r, juce::Justification::centred);
        r = r.translated(0, bh + 1);
    }
}

void IntParamEditor::mouseUp(const juce::MouseEvent &e)
{
    int midx = e.position.y / 20;
    if (midx >= 0 && midx < labels.size())
    {
        param.sendValue(midx, sender);
        repaint();
    }
}
} // namespace widgets
} // namespace scxt