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
struct OutlinedTextButton;
} // namespace Widgets
namespace Components
{
struct HeaderPanel : public juce::Component, public UIStateProxy::Invalidatable
{
    HeaderPanel(SC3Editor *ed);
    ~HeaderPanel();
    void paint(juce::Graphics &g) override;

    void resized() override;

    std::array<std::unique_ptr<Widgets::OutlinedTextButton>, n_sampler_parts> partsButtons;

    std::unique_ptr<Widgets::CompactVUMeter> vuMeter0;

    std::unique_ptr<Widgets::OutlinedTextButton> zonesButton, partButton, fxButton, configButton,
        aboutButton;
    void onProxyUpdate() override;

    SC3Editor *editor{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderPanel);
};
} // namespace Components
} // namespace SC3
#endif // SHORTCIRCUIT_HEADERPANEL_H
