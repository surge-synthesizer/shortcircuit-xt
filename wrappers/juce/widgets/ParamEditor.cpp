//
// Created by Paul Walker on 1/9/22.
//

#include "ParamEditor.h"
#include "SCXTLookAndFeel.h"

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
        g.drawText(std::to_string(param.val), b, juce::Justification::centred);
        g.drawText(param.units, b, juce::Justification::right);
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

    auto vl = std::to_string(param.val) + " " + param.units;
    g.drawText(vl, lab, juce::Justification::right);

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

void FloatParamEditor::mouseUp(const juce::MouseEvent &e)
{
    auto xf = e.position.x / getWidth();
    param.sendValue01(xf, sender);
    repaint();
}
} // namespace widgets
} // namespace scxt