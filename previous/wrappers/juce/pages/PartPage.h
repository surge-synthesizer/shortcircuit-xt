/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

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
} // namespace part_contents
struct PartPage : public PageBase
{
    PartPage(SCXTEditor *, SCXTEditor::Pages p);
    ~PartPage();

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::black); }
    void resized() override;

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
