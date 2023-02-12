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

#ifndef SHORTCIRCUIT_HEADERPANEL_H
#define SHORTCIRCUIT_HEADERPANEL_H

#include "SCXTPluginEditor.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "style/StyleSheet.h"

namespace scxt
{
namespace widgets
{
struct CompactVUMeter;
struct PolyphonyDisplay;
struct OutlinedTextButton;
} // namespace widgets
namespace components
{
struct HeaderPanel : public juce::Component,
                     public scxt::data::UIStateProxy::Invalidatable,
                     style::DOMParticipant
{
    HeaderPanel(SCXTEditor *ed);
    ~HeaderPanel();
    void paint(juce::Graphics &g) override;

    void resized() override;

    std::array<std::unique_ptr<widgets::OutlinedTextButton>, N_SAMPLER_PARTS> partsButtons;

    std::unique_ptr<widgets::CompactVUMeter> vuMeter0;
    std::unique_ptr<widgets::PolyphonyDisplay> polyDisplay;

    std::unique_ptr<widgets::OutlinedTextButton> zonesButton, partButton, fxButton, menuButton,
        aboutButton;

    void onProxyUpdate() override;

    SCXTEditor *editor{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderPanel);
};
} // namespace components
} // namespace scxt
#endif // SHORTCIRCUIT_HEADERPANEL_H
