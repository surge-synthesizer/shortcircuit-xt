//
// Created by Paul Walker on 1/11/22.
//

#include "PartPage.h"
#include "PageContentBase.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "SCXTLookAndFeel.h"

namespace scxt
{
namespace pages
{
namespace part_contents
{

typedef scxt::pages::contents::PageContentBase<scxt::pages::PartPage> ContentBase;

#define STUB(x, c)                                                                                 \
    struct x : public ContentBase                                                                  \
    {                                                                                              \
        x(const scxt::pages::PartPage &p) : ContentBase(p, #x, c) {}                               \
    };

STUB(Main, juce::Colours::darkgrey);
STUB(Polymode, juce::Colours::darkgrey);
STUB(LayerRanges, juce::Colours::darkgrey);
STUB(VelocitySplit, juce::Colours::darkgrey);
STUB(ModulationRouting, juce::Colours::darkgrey);
STUB(Controllers, juce::Colour(0xFF335533));
STUB(Effects, juce::Colour(0xFF555577));

struct Output : public ContentBase
{
    Output(const scxt::pages::PartPage &p) : ContentBase(p, "Output", juce::Colour(0xFF555577)) {}
    void paintContentInto(juce::Graphics &g, const juce::Rectangle<int> &b) override
    {
        // dividers
        auto q = b.reduced(0, 2).withWidth(1).translated(getWidth() / 3 - 0.5, 0);
        g.setColour(juce::Colours::black);
        g.fillRect(q);
        q = q.translated(getWidth() / 3 - 0.5, 0);
        g.fillRect(q);

        auto &aux = parentPage.editor->parts[parentPage.editor->selectedPart].aux;
        g.setColour(juce::Colours::white);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
        for (int a = 0; a < num_aux_busses; ++a)
        {
            auto xp = a * b.getWidth() / 3;
            auto yp = 0;

            auto h = b.getHeight() / 4;
            auto tr = juce::Rectangle<int>(xp, yp, b.getWidth() / 3, h);
            auto txt = std::string("output ") +
                       parentPage.editor->partAuxOutputNames.data[aux[a].output.val];
            g.drawText(txt, tr.reduced(2, 0), juce::Justification::left);
            tr = tr.translated(0, h);

            txt = std::string("outmode ") + std::to_string(aux[a].outmode.val);
            g.drawText(txt, tr.reduced(2, 0), juce::Justification::left);
            tr = tr.translated(0, h);

            txt = std::string("level ") + std::to_string(aux[a].level.val);
            g.drawText(txt, tr.reduced(2, 0), juce::Justification::left);
            tr = tr.translated(0, h);

            txt = std::string("balance ") + std::to_string(aux[a].balance.val);
            g.drawText(txt, tr.reduced(2, 0), juce::Justification::left);
        }
    }
};

} // namespace part_contents
PartPage::PartPage(SCXTEditor *e, SCXTEditor::Pages p) : PageBase(e, p)
{
    main = makeContent<part_contents::Main>(*this);
    polyMode = makeContent<part_contents::Polymode>(*this);
    layerRanges = makeContent<part_contents::LayerRanges>(*this);
    velocitySplit = makeContent<part_contents::VelocitySplit>(*this);
    modulationRouting = makeContent<part_contents::ModulationRouting>(*this);
    controllers = makeContent<part_contents::Controllers>(*this);
    output = makeContent<part_contents::Output>(*this);
    effects = makeContent<part_contents::Effects>(*this);
}

PartPage::~PartPage() = default;

void PartPage::resized()
{
    auto h = getHeight();
    auto w = getWidth();

    auto h1 = h * 0.6;
    auto w1 = w * 0.4;

    int y0 = 0;
    main->setBounds(juce::Rectangle<int>(0, y0, w1, h1 * 0.3).reduced(1, 1));
    y0 += h1 * 0.3;
    polyMode->setBounds(juce::Rectangle<int>(0, y0, w1, h1 * 0.2).reduced(1, 1));
    y0 += h1 * 0.2;
    layerRanges->setBounds(juce::Rectangle<int>(0, y0, w1, h1 * 0.2).reduced(1, 1));
    y0 += h1 * 0.2;
    velocitySplit->setBounds(juce::Rectangle<int>(0, y0, w1, h1 * 0.3).reduced(1, 1));
    y0 += h1 * 0.3;

    output->setBounds(juce::Rectangle<int>(w1, 0, w - w1, h1 * 0.3).reduced(1, 1));
    effects->setBounds(juce::Rectangle<int>(w1, h1 * 0.3, w - w1, h1 * 0.7).reduced(1, 1));

    modulationRouting->setBounds(juce::Rectangle<int>(0, h1, w / 2, h - h1).reduced(1, 1));
    controllers->setBounds(juce::Rectangle<int>(w / 2, h1, w / 2, h - h1).reduced(1, 1));
}

} // namespace pages
} // namespace scxt