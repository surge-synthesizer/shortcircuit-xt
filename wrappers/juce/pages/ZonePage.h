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

#ifndef SHORTCIRCUIT_ZONEPAGE_H
#define SHORTCIRCUIT_ZONEPAGE_H

#include "PageBase.h"

namespace scxt
{

namespace components
{
struct ZoneKeyboardDisplay;
struct WaveDisplay;
} // namespace components
namespace widgets
{
struct OutlinedTextButton;
}

namespace pages
{
namespace zone_contents
{
struct Sample;
struct NamesAndRanges;
struct Pitch;
struct Envelope;
struct Filters;
struct Routing;
struct LFO;
struct Outputs;
} // namespace zone_contents

struct ZonePage : public PageBase
{
    ZonePage(SCXTEditor *ed, SCXTEditor::Pages p);
    ~ZonePage();

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::black); }
    void resized() override;
    virtual void connectProxies() override;
    virtual void onProxyUpdate() override;

    std::unique_ptr<components::ZoneKeyboardDisplay> zoneKeyboardDisplay;
    std::unique_ptr<components::WaveDisplay> waveDisplay;
    std::array<std::unique_ptr<widgets::OutlinedTextButton>, num_layers> layerButtons;

    std::unique_ptr<zone_contents::Sample> sample;
    std::unique_ptr<zone_contents::NamesAndRanges> namesAndRanges;
    std::unique_ptr<zone_contents::Pitch> pitch;
    std::unique_ptr<zone_contents::Envelope> envelopes[2];
    std::unique_ptr<zone_contents::Filters> filters;
    std::unique_ptr<zone_contents::Routing> routing;
    std::unique_ptr<zone_contents::LFO> lfo;
    std::unique_ptr<zone_contents::Outputs> outputs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZonePage);
};
} // namespace pages
} // namespace scxt

#endif // SHORTCIRCUIT_ZONEPAGE_H
