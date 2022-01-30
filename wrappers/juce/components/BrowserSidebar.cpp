//
// Created by Paul Walker on 1/22/22.
//

#include "BrowserSidebar.h"
#include "SCXTEditor.h"
#include "ZoneKeyboardDisplay.h"

namespace scxt
{
namespace components
{
void rdump(const std::unique_ptr<scxt::content::ContentBrowser::Content> &r, const std::string &pfx)
{
    std::cout << pfx << r->displayPath << " " << r->type << "\n";
    for (const auto &q : r->children)
    {
        rdump(q, pfx + "  ");
    }
};

struct BrowserDragThingy : public juce::Component
{
    BrowserDragThingy(std::string fn) : name(fn)
    {
        setSize(20, 200);
        setOpaque(false);
    }
    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds();
        g.setColour(juce::Colours::darkgrey);
        g.fillRoundedRectangle(r.reduced(1, 1).toFloat(), 3.0);

        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(r.reduced(1, 1).toFloat(), 3.0, 1.0);

        g.setColour(juce::Colours::white);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
        g.drawText(name, r, juce::Justification::centred);
    }
    std::string name;
    fs::path fullPath;
};

BrowserSidebar::BrowserSidebar(SCXTEditor *ed)
    : editor(ed), root(editor->audioProcessor.sc3->browser.root)
{
    rdump(root, std::string());
}

BrowserSidebar::~BrowserSidebar() = default;

void BrowserSidebar::paintContainer(juce::Graphics &g, const content_raw_t &content, int offset,
                                    int off0)
{
    auto r = getLocalBounds().withTrimmedTop(20).withHeight(20).translated(0, offset * 20);

    switch (content->type)
    {
    case content::ContentBrowser::Content::ROOT:
        g.setColour(juce::Colour(0xFF151515));
        break;
    case content::ContentBrowser::Content::DIR:
        g.setColour(juce::Colour(0xFF444477));
        break;
    case content::ContentBrowser::Content::PATCH:
        g.setColour(juce::Colour(0xFF556655));
        break;
    case content::ContentBrowser::Content::SAMPLE:
        g.setColour(juce::Colour(0xFF665555));
        break;
    }
    g.fillRoundedRectangle(r.reduced(1, 1).toFloat(), 3.0);

    g.setColour(juce::Colours::white);
    g.drawRoundedRectangle(r.reduced(1, 1).toFloat(), 3.0, 1.0);

    g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
    g.setColour(juce::Colours::white);
    if (content->type == content::ContentBrowser::Content::ROOT)
    {
        g.drawText("Root", r, juce::Justification::centred);
    }
    else
    {
        g.drawText(content->displayPath.u8string(), r, juce::Justification::centred);
    }
    if (off0 < 0)
        clickZones.emplace_back(r, offset);
    else
        clickZones.emplace_back(r, -(offset - off0 + 1));
}
void BrowserSidebar::paint(juce::Graphics &g)
{
    // it sucks we do this here but oh well
    clickZones.clear();
    g.fillAll(juce::Colour(0xFF000020));
    auto hb = getLocalBounds().withHeight(20);
    SCXTLookAndFeel::fillWithGradientHeaderBand(g, hb, juce::Colour(0xFF774444));
    g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
    g.setColour(juce::Colours::white);
    g.drawText("Samples and Patches", hb, juce::Justification::centred);

    content_raw_t rp = root.get();
    int idx = 0;
    paintContainer(g, rp, idx++);

    for (const auto &pathI : childStack)
    {
        rp = rp->children[pathI].get();
        paintContainer(g, rp, idx++);
    }

    idx++;
    auto idx0 = idx;
    for (auto &c : rp->children)
    {
        paintContainer(g, c.get(), idx++, idx0);
    }
}

void BrowserSidebar::mouseDown(const juce::MouseEvent &e) {}
void BrowserSidebar::mouseUp(const juce::MouseEvent &e)
{
    if (dragComponent)
    {
        auto p = e.position.translated(getBounds().getX(), getBounds().getY());

        getParentComponent()->removeChildComponent(dragComponent.get());
        auto c = getParentComponent()->getComponentAt(p.toInt());
        std::cout << c << " " << typeid(*c).name() << std::endl;
        if (dynamic_cast<ZoneKeyboardDisplay *>(c))
        {
            juce::StringArray df;
            df.add(dragComponent->fullPath.u8string());
            editor->filesDropped(df, e.position.x, e.position.y);
        }
        dragComponent.reset(nullptr);
        return;
    }
    for (const auto &[r, idx] : clickZones)
    {
        if (r.contains(e.position.toInt()))
        {
            if (idx >= 0)
            {
                auto csc = childStack;
                childStack.clear();
                for (int i = 0; i < idx; ++i)
                    childStack.push_back(csc[i]);
                repaint();
            }
            else
            {
                content_raw_t rp = root.get();
                for (const auto &pathI : childStack)
                {
                    rp = rp->children[pathI].get();
                }
                auto ci = -idx - 1;
                const auto &k = rp->children[ci];

                switch (k->type)
                {
                case content::ContentBrowser::Content::DIR:
                    childStack.push_back(ci);
                    repaint();
                    break;
                default:
                    break;
                }
            }
        }
    }
}

void BrowserSidebar::mouseDrag(const juce::MouseEvent &e)
{
    if (e.getDistanceFromDragStart() > 2 && dragComponent == nullptr)
    {
        for (const auto &[r, idx] : clickZones)
        {
            if (r.contains(e.position.toInt()) && idx < 0)
            {
                content_raw_t rp = root.get();
                for (const auto &pathI : childStack)
                {
                    rp = rp->children[pathI].get();
                }
                auto ci = -idx - 1;
                const auto &k = rp->children[ci];

                if (k->type == content::ContentBrowser::Content::DIR ||
                    k->type == content::ContentBrowser::Content::ROOT)
                    return;

                dragComponent = std::make_unique<BrowserDragThingy>(k->displayPath.u8string());
                dragComponent->fullPath = k->fullPath;
                getParentComponent()->addAndMakeVisible(*dragComponent);

                auto b = r.translated(getBounds().getX(), getBounds().getY());

                dragComponent->setBounds(b);
                dragger.startDraggingComponent(dragComponent.get(), e);
            }
        }
    }
    else if (dragComponent)
    {
        dragger.dragComponent(dragComponent.get(), e, nullptr);
    }
}
} // namespace components
} // namespace scxt