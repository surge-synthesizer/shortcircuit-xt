//
// Created by Paul Walker on 1/6/22.
//

#include "SCXTEditor.h"
#include "ZonePage.h"
#include "components/ZoneKeyboardDisplay.h"
#include "components/WaveDisplay.h"

#include "proxies/ZoneDataProxy.h"
#include "proxies/ZoneListDataProxy.h"

#include "widgets/ParamEditor.h"
#include "widgets/ComboBox.h"

#include "sst/cpputils.h"

namespace scxt
{
namespace zone_contents
{
struct StandIn : public juce::Component
{
    StandIn(const std::string &lab) : label(lab) {}
    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::yellow);
        g.setColour(juce::Colours::black);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(9));
        g.drawText(label, getLocalBounds(), juce::Justification::centred);
    }
    std::string label;
};

struct PaintBase : public juce::Component, UIStateProxy::Invalidatable
{
    PaintBase(const scxt::pages::ZonePage &p, const std::string &header, const juce::Colour &col)
        : zonePage(p), header(header), col(col)
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
    juce::Rectangle<int> getContentsBounds() { return getLocalBounds().withTrimmedTop(16); }
    virtual void paintContentInto(juce::Graphics &g, const juce::Rectangle<int> &bounds) {}
    void onProxyUpdate() override {}
    std::string header;
    juce::Colour col;

    const scxt::pages::ZonePage &zonePage;
};

#define STUB(x, c)                                                                                 \
    struct x : public PaintBase                                                                    \
    {                                                                                              \
        x(const scxt::pages::ZonePage &p) : PaintBase(p, #x, c) {}                                 \
    };

STUB(Sample, juce::Colour(0xFF444477));

struct NamesAndRanges : public PaintBase
{
    NamesAndRanges(const scxt::pages::ZonePage &p)
        : PaintBase(p, "Names & Ranges", juce::Colour(0xFF555555))
    {
        nameEd = std::make_unique<StandIn>("name");
        addAndMakeVisible(*nameEd);

        for (const auto &[i, l] :
             sst::cpputils::enumerate(std::array{"xf", "low", "root", "hi", "xf"}))
        {
            auto lb = std::make_unique<juce::Label>(l);
            lb->setText(l, juce::dontSendNotification);
            lb->setFont(SCXTLookAndFeel::getMonoFontAt(9));
            lb->setColour(juce::Label::textColourId, juce::Colours::white);
            lb->setJustificationType(juce::Justification::centred);
            addAndMakeVisible(*lb);
            rowLabels[i] = std::move(lb);
        }

        auto &cz = zonePage.editor->currentZone;
        kParams[0] = std::make_unique<widgets::IntParamSpinBox>(cz.key_low_fade, zonePage.editor);
        kParams[1] = std::make_unique<widgets::IntParamSpinBox>(cz.key_low, zonePage.editor);
        kParams[2] = std::make_unique<widgets::IntParamSpinBox>(cz.key_root, zonePage.editor);
        kParams[3] = std::make_unique<widgets::IntParamSpinBox>(cz.key_high, zonePage.editor);
        kParams[4] = std::make_unique<widgets::IntParamSpinBox>(cz.key_high_fade, zonePage.editor);

        kParams[1]->onSend = [this]() { zonePage.zoneKeyboardDisplay->repaint(); };
        kParams[3]->onSend = [this]() { zonePage.zoneKeyboardDisplay->repaint(); };

        for (const auto &k : kParams)
            addAndMakeVisible(*k);

        vParams[0] =
            std::make_unique<widgets::IntParamSpinBox>(cz.velocity_low_fade, zonePage.editor);
        vParams[1] = std::make_unique<widgets::IntParamSpinBox>(cz.velocity_low, zonePage.editor);
        vParams[2] = std::make_unique<widgets::IntParamSpinBox>(cz.velocity_high, zonePage.editor);
        vParams[3] =
            std::make_unique<widgets::IntParamSpinBox>(cz.velocity_high_fade, zonePage.editor);

        for (const auto &k : vParams)
            addAndMakeVisible(*k);
    }

    void resized() override
    {
        auto b = getContentsBounds().reduced(2, 2);
        auto h = b.getHeight();
        auto rh = h / 8;
        auto row = b.withHeight(rh);
        nameEd->setBounds(row.reduced(1, 1));
        row = row.translated(0, rh);

        auto xi = b.getWidth() / 5;
        auto labb = row.withWidth(xi);
        for (int i = 0; i < 5; ++i)
        {
            rowLabels[i]->setBounds(labb);
            labb = labb.translated(xi, 0);
        }

        row = row.translated(0, rh);
        labb = row.withWidth(xi);
        for (int i = 0; i < 5; ++i)
        {
            kParams[i]->setBounds(labb.reduced(1, 1));
            labb = labb.translated(xi, 0);
        }

        row = row.translated(0, rh);
        labb = row.withWidth(xi);
        for (int i = 0; i < 4; ++i)
        {
            vParams[i]->setBounds(labb.reduced(1, 1));
            labb = labb.translated(xi, 0);
            if (i == 1)
                labb = labb.translated(xi, 0);
        }

        row = row.translated(0, rh * 1.5);
    }

    std::unique_ptr<StandIn> nameEd;

    std::array<std::unique_ptr<juce::Label>, 5> rowLabels;
    std::array<std::unique_ptr<widgets::IntParamSpinBox>, 5> kParams;
    std::array<std::unique_ptr<widgets::IntParamSpinBox>, 4> vParams;

    std::array<std::unique_ptr<widgets::IntParamSpinBox>, 4> ncLLHH;
    std::array<std::unique_ptr<widgets::ComboBox>, 2> ncSS;

    std::array<std::unique_ptr<juce::Label>, 2> kLabels;
    std::array<std::unique_ptr<juce::Label>, 2> vLabels;
    std::array<std::unique_ptr<juce::Label>, 3> ncLabels;
};

STUB(Pitch, juce::Colour(0XFF555555));
STUB(Routing, juce::Colour(0XFF555555));
STUB(Envelopes, juce::Colour(0xFF447744));
STUB(LFO, juce::Colour(0xFF447744));
STUB(Filters, juce::Colour(0xFF444477));
STUB(Outputs, juce::Colour(0xFF444477));

} // namespace zone_contents
namespace pages
{
ZonePage::ZonePage(SCXTEditor *ed, SCXTEditor::Pages p) : PageBase(ed, p)
{
    zoneKeyboardDisplay = std::make_unique<components::ZoneKeyboardDisplay>(editor, editor);
    addAndMakeVisible(*zoneKeyboardDisplay);

    waveDisplay = std::make_unique<components::WaveDisplay>(editor, editor);
    addAndMakeVisible(*waveDisplay);

    sample = std::make_unique<zone_contents::Sample>(*this);
    addAndMakeVisible(*sample);
    namesAndRanges = std::make_unique<zone_contents::NamesAndRanges>(*this);
    addAndMakeVisible(*namesAndRanges);
    pitch = std::make_unique<zone_contents::Pitch>(*this);
    addAndMakeVisible(*pitch);
    envelopes = std::make_unique<zone_contents::Envelopes>(*this);
    addAndMakeVisible(*envelopes);
    filters = std::make_unique<zone_contents::Filters>(*this);
    addAndMakeVisible(*filters);
    routing = std::make_unique<zone_contents::Routing>(*this);
    addAndMakeVisible(*routing);
    lfo = std::make_unique<zone_contents::LFO>(*this);
    addAndMakeVisible(*lfo);
    outputs = std::make_unique<zone_contents::Outputs>(*this);
    addAndMakeVisible(*outputs);
}
ZonePage::~ZonePage() = default;

void ZonePage::resized()
{
    auto r = getLocalBounds();

    zoneKeyboardDisplay->setBounds(r.withHeight(80));
    waveDisplay->setBounds(r.withHeight(100).translated(0, 80));

    auto lower = r.withTrimmedTop(180);

    auto wa = getWidth() * 0.3;
    auto wb = getWidth() * 0.5;
    auto wc = getWidth() - wa - wb;

    outputs->setBounds(lower.withTrimmedLeft(getWidth() - wc).reduced(1, 1));

    auto s1 = lower.withWidth(wa);
    auto s2 = lower.withWidth(wb).translated(wa, 0);
    auto ha = lower.getHeight() * 0.75;
    auto hb = lower.getHeight() - ha;

    sample->setBounds(s1.withHeight(ha * 0.3).reduced(1, 1));
    namesAndRanges->setBounds(s1.withHeight(ha * 0.4).translated(0, ha * 0.3).reduced(1, 1));
    pitch->setBounds(s1.withHeight(ha * 0.3).translated(0, ha * 0.7).reduced(1, 1));

    filters->setBounds(s2.withHeight(ha * 0.6).reduced(1, 1));
    routing->setBounds(s2.withHeight(ha * 0.4).translated(0, ha * 0.6).reduced(1, 1));

    envelopes->setBounds(s1.withHeight(hb).translated(0, ha).reduced(1, 1));
    lfo->setBounds(s2.withHeight(hb).translated(0, ha).reduced(1, 1));
}

void ZonePage::connectProxies()
{
    editor->zoneListProxy->clients.insert(zoneKeyboardDisplay.get());
    // Probably change this
    editor->uiStateProxies.insert(waveDisplay.get());
}

} // namespace pages
} // namespace scxt
