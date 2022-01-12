//
// Created by Paul Walker on 1/11/22.
//

#ifndef SHORTCIRCUIT_PARTPAGE_H
#define SHORTCIRCUIT_PARTPAGE_H
#include "PageBase.h"

namespace scxt
{
namespace pages
{
namespace part_contents
{
struct Main;
struct Polymode;
struct LayerRanges;
struct VelocitySplit;
struct ModulationRouting;
struct Controllers;
struct Output;
struct Effects;
}
struct PartPage : public PageBase, public UIStateProxy::Invalidatable
{
    PartPage(SCXTEditor *, SCXTEditor::Pages p);
    ~PartPage();

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::black); }
    void resized() override;

    void onProxyUpdate() override;


    std::unique_ptr<part_contents::Main> main;
    std::unique_ptr<part_contents::Polymode> polyMode;
    std::unique_ptr<part_contents::LayerRanges> layerRanges;
    std::unique_ptr<part_contents::VelocitySplit> velocitySplit;
    std::unique_ptr<part_contents::ModulationRouting> modulationRouting;
    std::unique_ptr<part_contents::Controllers> controllers;
    std::unique_ptr<part_contents::Output> output;
    std::unique_ptr<part_contents::Effects> effects;
};

} // namespace pages
} // namespace scxt
#endif // SHORTCIRCUIT_PARTPAGE_H
