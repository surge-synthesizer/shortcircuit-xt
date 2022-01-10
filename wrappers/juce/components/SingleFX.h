//
// Created by Paul Walker on 1/7/22.
//

#ifndef SHORTCIRCUIT_SINGLEFX_H
#define SHORTCIRCUIT_SINGLEFX_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "DataInterfaces.h"
struct SCXTEditor;

namespace scxt
{
namespace widgets
{
struct FloatParamEditor;
}
namespace components
{
struct SingleFX : public juce::Component, public UIStateProxy::Invalidatable
{
    SingleFX(SCXTEditor *ed, int idx);
    ~SingleFX();

    void paint(juce::Graphics &g) override;
    void resized() override;
    void onProxyUpdate() override;

    SCXTEditor *editor{nullptr};
    int idx{-1};

    void typeSelectorChanged();

    std::array<std::unique_ptr<widgets::FloatParamEditor>, n_filter_parameters> fParams;
    std::array<std::unique_ptr<juce::Label>, n_filter_iparameters> iParams;

    std::unique_ptr<juce::ComboBox> typeSelector;
};

} // namespace components
} // namespace scxt

#endif // SHORTCIRCUIT_SINGLEFX_H
