//
// Created by Paul Walker on 1/5/22.
//

#ifndef SHORTCIRCUIT_HEADERPANEL_H
#define SHORTCIRCUIT_HEADERPANEL_H

#include "SCXTEditor.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "style/StyleSheet.h"

namespace scxt
{
namespace widgets
{
struct CompactVUMeter;
struct PolyphonyDisplay;
struct OutlinedTextButton;
} // namespace widgets
namespace components
{
struct HeaderPanel : public juce::Component,
                     public scxt::data::UIStateProxy::Invalidatable,
                     style::DOMParticipant
{
    HeaderPanel(SCXTEditor *ed);
    ~HeaderPanel();
    void paint(juce::Graphics &g) override;

    void resized() override;

    std::array<std::unique_ptr<widgets::OutlinedTextButton>, n_sampler_parts> partsButtons;

    std::unique_ptr<widgets::CompactVUMeter> vuMeter0;
    std::unique_ptr<widgets::PolyphonyDisplay> polyDisplay;

    std::unique_ptr<widgets::OutlinedTextButton> zonesButton, partButton, fxButton, menuButton,
        aboutButton;

    void onProxyUpdate() override;

    SCXTEditor *editor{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderPanel);
};
} // namespace components
} // namespace scxt
#endif // SHORTCIRCUIT_HEADERPANEL_H
