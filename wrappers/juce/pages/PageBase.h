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

#ifndef SHORTCIRCUIT_PAGEBASE_H
#define SHORTCIRCUIT_PAGEBASE_H

#include "SCXTEditor.h"
#include "SCXTLookAndFeel.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <vector>
#include "style/StyleSheet.h"

namespace scxt
{
namespace pages
{
struct PageBase : public juce::Component,
                  public scxt::data::UIStateProxy::Invalidatable,
                  public scxt::style::DOMParticipant
{
    PageBase(SCXTEditor *ed, const SCXTEditor::Pages &p, const scxt::style::Selector &s)
        : editor(ed), page(p), scxt::style::DOMParticipant(s)
    {
        setupJuceAccessibility();
    }
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
