//
// Created by Paul Walker on 1/5/22.
//

#ifndef SHORTCIRCUIT_HEADERPANEL_H
#define SHORTCIRCUIT_HEADERPANEL_H

#include "SC3Editor.h"
#include "juce_gui_basics/juce_gui_basics.h"

struct HeaderPanel : public juce::Component, public UIStateProxy::Invalidatable
{
    HeaderPanel(SC3Editor *ed);

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::green); }

    void resized() override;

    std::array<std::unique_ptr<juce::Button>, n_sampler_parts> partsButtons;
    void onProxyUpdate() override;
    SC3Editor *editor{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderPanel);
};
#endif // SHORTCIRCUIT_HEADERPANEL_H
