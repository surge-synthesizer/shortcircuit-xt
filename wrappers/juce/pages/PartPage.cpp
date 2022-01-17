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

struct Effects : public ContentBase
{
    Effects(const scxt::pages::PartPage &p) : ContentBase(p, "Effects", juce::Colour(0xFF555577)) {}
};

struct Output : public ContentBase
{
    Output(const scxt::pages::PartPage &p) : ContentBase(p, "Output", juce::Colour(0xFF555577))
    {
        for (int i = 0; i < 3; ++i)
        {
            auto &aux = parentPage.editor->currentPart.aux[i];
            output[i] = bindIntComboBox(aux.output, parentPage.editor->partAuxOutputNames);
            level[i] = bindFloatHSlider(aux.level);
            balance[i] = bindFloatHSlider(aux.balance);
            if (i != 0)
                outmode[i] = bind<widgets::IntParamMultiSwitch>(widgets::IntParamMultiSwitch::VERT,
                                                                aux.outmode);
        }
    }
    void resized() override
    {
        auto cb = getContentsBounds();
        auto hd = contents::RowDivider(cb);
        for (int i = 0; i < 3; ++i)
        {
            auto ab = hd.next(1.0 / 3.0).reduced(2, 2);
            auto rg = contents::RowGenerator(ab, 3);
            output[i]->setBounds(rg.next());
            if (i == 0)
            {
                level[i]->setBounds(rg.next());
                balance[i]->setBounds(rg.next());
            }
            else
            {
                outmode[i]->setBounds(ab.withWidth(50).withTrimmedTop(ab.getHeight() / 3));
                level[i]->setBounds(rg.next().withTrimmedLeft(52));
                balance[i]->setBounds(rg.next().withTrimmedLeft(52));
            }
        }
    }
    void paintContentInto(juce::Graphics &g, const juce::Rectangle<int> &b) override
    {
        contents::SectionDivider::divideSectionVertically(g, b, 3, juce::Colours::black);
    }
    std::array<std::unique_ptr<widgets::IntParamComboBox>, 3> output;
    std::array<std::unique_ptr<widgets::FloatParamSlider>, 3> level, balance;
    std::array<std::unique_ptr<widgets::IntParamMultiSwitch>, 3> outmode; // 0 will be null
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

    output->setBounds(juce::Rectangle<int>(w1, 0, w - w1, h1 * 0.22).reduced(1, 1));
    effects->setBounds(juce::Rectangle<int>(w1, h1 * 0.22, w - w1, h1 * 0.78).reduced(1, 1));

    modulationRouting->setBounds(juce::Rectangle<int>(0, h1, w / 2, h - h1).reduced(1, 1));
    controllers->setBounds(juce::Rectangle<int>(w / 2, h1, w / 2, h - h1).reduced(1, 1));
}

} // namespace pages
} // namespace scxt