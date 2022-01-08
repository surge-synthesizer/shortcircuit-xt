//
// Created by Paul Walker on 1/7/22.
//

#ifndef SHORTCIRCUIT_SINGLEFX_H
#define SHORTCIRCUIT_SINGLEFX_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "DataInterfaces.h"
struct SC3Editor;

namespace SC3
{
namespace Components
{
struct SingleFX : public juce::Component, public UIStateProxy::Invalidatable
{
    SingleFX(SC3Editor *ed, int idx);
    ~SingleFX();

    void paint(juce::Graphics &g) override;
    void resized() override;
    void onProxyUpdate() override;

    SC3Editor *editor{nullptr};
    int idx{-1};

    void typeSelectorChanged();

    std::array<std::unique_ptr<juce::Label>, n_filter_parameters> fParams;
    std::array<std::unique_ptr<juce::Label>, n_filter_iparameters> iParams;

    std::unique_ptr<juce::ComboBox> typeSelector;
};

} // namespace Components
} // namespace SC3

#endif // SHORTCIRCUIT_SINGLEFX_H
