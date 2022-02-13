//
// Created by Paul Walker on 2/11/22.
//

#include "style/StyleSheet.h"

namespace scxt
{
namespace style
{
struct DefaultSheet : public Sheet
{
    juce::Colour resolveColour(const DOMParticipant &d, const ColourFeature &f) override
    {
        return juce::Colours::white;
    }
};

std::unique_ptr<Sheet> createDefaultSheet() { return std::make_unique<DefaultSheet>(); }
} // namespace style
} // namespace scxt