//
// Created by Paul Walker on 1/5/22.
//

#ifndef SHORTCIRCUIT_HEADERPANEL_H
#define SHORTCIRCUIT_HEADERPANEL_H

#include "SC3Editor.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace SC3
{
namespace Widgets
{
struct CompactVUMeter;
}
} // namespace SC3
struct HeaderPanel : public juce::Component, public UIStateProxy::Invalidatable
{
    HeaderPanel(SC3Editor *ed);
    ~HeaderPanel();

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::darkgoldenrod); }

    void resized() override;

    std::array<std::unique_ptr<juce::Button>, n_sampler_parts> partsButtons;

    std::unique_ptr<SC3::Widgets::CompactVUMeter> vuMeter0;

    std::unique_ptr<juce::Button> zonesButton, partButton, fxButton, configButton, menuButton;
    void onProxyUpdate() override;
    SC3Editor *editor{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderPanel);
};
#endif // SHORTCIRCUIT_HEADERPANEL_H
