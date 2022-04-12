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

#ifndef SHORTCIRCUIT_FXPAGE_H
#define SHORTCIRCUIT_FXPAGE_H

#include "PageBase.h"

namespace scxt
{
namespace pages
{
namespace fx_contents
{
struct SingleFX;
} // namespace fx_contents
struct FXPage : public PageBase
{
    FXPage(SCXTEditor *, SCXTEditor::Pages p);
    ~FXPage();

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::black); }
    void resized() override;

    std::array<std::unique_ptr<fx_contents::SingleFX>, num_fxunits> fxComponents;
};

} // namespace pages
} // namespace scxt

#endif // SHORTCIRCUIT_FXPAGE_H
