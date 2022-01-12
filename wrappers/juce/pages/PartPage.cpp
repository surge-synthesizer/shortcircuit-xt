//
// Created by Paul Walker on 1/11/22.
//

#include "PartPage.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "SCXTLookAndFeel.h"

namespace scxt
{
namespace pages
{
namespace part_contents
{
struct PaintBase : public juce::Component, UIStateProxy::Invalidatable
{
    PaintBase(const scxt::pages::PartPage &p, const std::string &header, const juce::Colour &col)
        : partPage(p), header(header), col(col)
    {
    }
    void paint(juce::Graphics &g) override
    {
        g.fillAll(col);
        auto h = getLocalBounds().withHeight(16);
        SCXTLookAndFeel::fillWithGradientHeaderBand(g, h, col);
        g.setColour(juce::Colours::white);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
        g.drawText(header, h, juce::Justification::centred);

        auto b = getLocalBounds().withTrimmedBottom(16);
        auto gs = juce::Graphics::ScopedSaveState(g);
        g.addTransform(juce::AffineTransform().translated(0, 16));
        paintContentInto(g, b);
    }
    virtual void paintContentInto(juce::Graphics &g, const juce::Rectangle<int> &bounds) {}
    void onProxyUpdate() override {}
    std::string header;
    juce::Colour col;

    const scxt::pages::PartPage &partPage;
};

#define STUB(x, c)                                                                                 \
    struct x : public PaintBase                                                                    \
    {                                                                                              \
        x(const scxt::pages::PartPage &p) : PaintBase(p, #x, c) {}                                 \
    };

STUB(Main, juce::Colours::darkgrey);
STUB(Polymode, juce::Colours::darkgrey);
STUB(LayerRanges, juce::Colours::darkgrey);
STUB(VelocitySplit, juce::Colours::darkgrey);
STUB(ModulationRouting, juce::Colours::darkgrey);
STUB(Controllers, juce::Colour(0xFF335533));
STUB(Effects, juce::Colour(0xFF555577));

struct Output : public PaintBase
{
    Output(const scxt::pages::PartPage &p) : PaintBase(p, "Output", juce::Colour(0xFF555577)) {}
    void paintContentInto(juce::Graphics &g, const juce::Rectangle<int> &b) override
    {
        // dividers
        auto q = b.reduced(0, 2).withWidth(1).translated(getWidth() / 3 - 0.5, 0);
        g.setColour(juce::Colours::black);
        g.fillRect(q);
        q = q.translated(getWidth() / 3 - 0.5, 0);
        g.fillRect(q);

        auto &aux = partPage.editor->parts[partPage.editor->selectedPart].aux;
        g.setColour(juce::Colours::white);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
        for (int a = 0; a < num_aux_busses; ++a)
        {
            auto xp = a * b.getWidth() / 3;
            auto yp = 0;

            auto h = b.getHeight() / 4;
            auto tr = juce::Rectangle<int>(xp, yp, b.getWidth() / 3, h);
            auto txt =
                std::string("output ") + partPage.editor->partAuxOutputNames[aux[a].output.val];
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
    // todo
    main = std::make_unique<part_contents::Main>(*this);
    polyMode = std::make_unique<part_contents::Polymode>(*this);
    layerRanges = std::make_unique<part_contents::LayerRanges>(*this);
    velocitySplit = std::make_unique<part_contents::VelocitySplit>(*this);
    modulationRouting = std::make_unique<part_contents::ModulationRouting>(*this);
    controllers = std::make_unique<part_contents::Controllers>(*this);
    output = std::make_unique<part_contents::Output>(*this);
    effects = std::make_unique<part_contents::Effects>(*this);

    addAndMakeVisible(*main);
    addAndMakeVisible(*polyMode);
    addAndMakeVisible(*layerRanges);
    addAndMakeVisible(*velocitySplit);
    addAndMakeVisible(*modulationRouting);
    addAndMakeVisible(*controllers);
    addAndMakeVisible(*output);
    addAndMakeVisible(*effects);
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

void PartPage::onProxyUpdate() {}
} // namespace pages
} // namespace scxt