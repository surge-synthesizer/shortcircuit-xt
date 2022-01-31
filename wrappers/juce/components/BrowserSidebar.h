//
// Created by Paul Walker on 1/22/22.
//

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
};
} // namespace components
} // namespace scxt

#endif // SHORTCIRCUIT_SAMPLESIDEBAR_H
