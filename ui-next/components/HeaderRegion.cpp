//
// Created by Paul Walker on 2/7/23.
//

#include "HeaderRegion.h"
#include "SCXTEditor.h"

namespace scxt::ui
{

HeaderRegion::HeaderRegion(SCXTEditor *e) : editor{e}
{
    multiPage = std::make_unique<juce::TextButton>("multi");
    multiPage->onClick = [this]() { editor->setActiveScreen(SCXTEditor::MULTI); };
    addAndMakeVisible(*multiPage);

    sendPage = std::make_unique<juce::TextButton>("send");
    sendPage->onClick = [this]() { editor->setActiveScreen(SCXTEditor::SEND_FX); };
    addAndMakeVisible(*sendPage);
}

} // namespace scxt::ui