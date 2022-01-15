//
// Created by Paul Walker on 1/6/22.
//

#ifndef SHORTCIRCUIT_PAGEBASE_H
#define SHORTCIRCUIT_PAGEBASE_H

#include "SCXTEditor.h"
#include "SCXTLookAndFeel.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <vector>

namespace scxt
{
namespace pages
{
struct PageBase : public juce::Component, public scxt::data::UIStateProxy::Invalidatable
{
    PageBase(SCXTEditor *ed, const SCXTEditor::Pages &p) : editor(ed), page(p) {}
    virtual ~PageBase() = default;

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::darkred);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(40));
        g.setColour(juce::Colours::white);
        g.drawText(SCXTEditor::pageName(page), getLocalBounds(), juce::Justification::centred);
    }

    virtual void connectProxies() {}

    SCXTEditor *editor{nullptr};
    SCXTEditor::Pages page{SCXTEditor::ZONE};

    std::vector<scxt::data::UIStateProxy::Invalidatable *> contentWeakPointers;
    template <typename T, class... Args> auto makeContent(Args &&...args)
    {
        auto q = std::make_unique<T>(std::forward<Args>(args)...);
        addAndMakeVisible(*q);
        contentWeakPointers.push_back(q.get());
        return q;
    }

    void onProxyUpdate() override
    {
        // by default just invalidate my content
        for (auto c : contentWeakPointers)
            c->onProxyUpdate();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageBase);
};
} // namespace pages
} // namespace scxt
#endif // SHORTCIRCUIT_PAGEBASE_H
