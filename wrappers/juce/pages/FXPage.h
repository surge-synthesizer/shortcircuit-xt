//
// Created by Paul Walker on 1/7/22.
//

#ifndef SHORTCIRCUIT_FXPAGE_H
#define SHORTCIRCUIT_FXPAGE_H

#include "PageBase.h"

namespace SC3
{
namespace Components
{
struct SingleFX;
} // namespace Components
namespace Pages
{
struct FXPage : public PageBase, public UIStateProxy::Invalidatable
{
    FXPage(SC3Editor *, SC3Editor::Pages p);
    ~FXPage();

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::black); }
    void resized() override;

    void onProxyUpdate() override;
    std::array<std::unique_ptr<Components::SingleFX>, num_fxunits> fxComponents;
};

} // namespace Pages
} // namespace SC3

#endif // SHORTCIRCUIT_FXPAGE_H
