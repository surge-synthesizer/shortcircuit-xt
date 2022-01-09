//
// Created by Paul Walker on 1/9/22.
//

#include "ParamEditor.h"
#include "SCXTLookAndFeel.h"

namespace SC3
{
namespace Widgets
{

void FloatParamEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::orchid);
    g.setFont(SCXTLookAndFeel::getMonoFontAt(9));
    auto b = getLocalBounds().reduced(2, 0);
    g.setColour(juce::Colours::white);
    g.drawText(param.label, b, juce::Justification::left);
    g.drawText(std::to_string(param.val), b, juce::Justification::centred);
    g.drawText(param.units, b, juce::Justification::right);
}
} // namespace Widgets
} // namespace SC3