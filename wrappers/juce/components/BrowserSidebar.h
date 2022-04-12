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

#ifndef SHORTCIRCUIT_SAMPLESIDEBAR_H
#define SHORTCIRCUIT_SAMPLESIDEBAR_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "SCXTLookAndFeel.h"
#include "sampler.h"

class SCXTEditor;

namespace scxt
{
namespace components
{

struct BrowserDragThingy;
struct BrowserSidebar : public juce::Component
{
    BrowserSidebar(SCXTEditor *ed);
    ~BrowserSidebar();
    SCXTEditor *editor{nullptr};

    typedef std::unique_ptr<scxt::content::ContentBrowser::Content> content_t;
    typedef scxt::content::ContentBrowser::Content *content_raw_t;
    void paint(juce::Graphics &g) override;
    void paintContainer(juce::Graphics &g, const content_raw_t &content, int offset, int off0 = -1);

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;

    const content_t &root;
    std::vector<int> childStack;

    // This is a gross solution
    std::vector<std::pair<juce::Rectangle<int>, int>> clickZones;
    std::unique_ptr<BrowserDragThingy> dragComponent;
    juce::ComponentDragger dragger;

    bool samplePlaying{false};
};
} // namespace components
} // namespace scxt

#endif // SHORTCIRCUIT_SAMPLESIDEBAR_H
