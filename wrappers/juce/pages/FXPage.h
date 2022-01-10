//
// Created by Paul Walker on 1/7/22.
//

#ifndef SHORTCIRCUIT_FXPAGE_H
#define SHORTCIRCUIT_FXPAGE_H

#include "PageBase.h"

namespace scxt
{
namespace components
{
struct SingleFX;
} // namespace components
namespace pages
{
struct FXPage : public PageBase, public UIStateProxy::Invalidatable
{
    FXPage(SCXTEditor *, SCXTEditor::Pages p);
    ~FXPage();

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::black); }
    void resized() override;

    void onProxyUpdate() override;
    std::array<std::unique_ptr<components::SingleFX>, num_fxunits> fxComponents;
};

} // namespace pages
} // namespace scxt

#endif // SHORTCIRCUIT_FXPAGE_H
