//
// Created by Paul Walker on 1/7/22.
//

#include "SingleFX.h"
#include "SCXTEditor.h"
#include "SCXTLookAndFeel.h"
#include "widgets/ComboBox.h"
#include "widgets/ParamEditor.h"

#include "sst/cpputils.h"

namespace scxt
{
namespace components
{
SingleFX::SingleFX(SCXTEditor *ed, int i) : editor(ed), idx(i)
{
    typeSelector = std::make_unique<scxt::widgets::ComboBox>();
    typeSelector->onChange = [this]() { typeSelectorChanged(); };
    addAndMakeVisible(*typeSelector);

    for (auto i = 0; i < n_filter_parameters; ++i)
    {
        auto q = std::make_unique<widgets::FloatParamEditor>(widgets::FloatParamEditor::HSLIDER,
                                                             editor->multi.filters[idx].p[i], ed);
        addChildComponent(*q);
        fParams[i] = std::move(q);
    }

    for (auto i = 0; i < n_filter_iparameters; ++i)
    {
        auto q = std::make_unique<juce::Label>("IP " + std::to_string(i));
        q->setText("IP " + std::to_string(i), juce::NotificationType::dontSendNotification);
        addChildComponent(*q);
        iParams[i] = std::move(q);
    }
}
SingleFX::~SingleFX() = default;

void SingleFX::paint(juce::Graphics &g)
{
    g.fillAll(findColour(SCXTColours::fxPanelBackground));

    auto r = getLocalBounds().withHeight(20);

    SCXTLookAndFeel::fillWithGradientHeaderBand(g, r,
                                                findColour(SCXTColours::fxPanelHeaderBackground));
    g.setColour(findColour(SCXTColours::fxPanelHeaderText));
    g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
    g.drawText("Effect " + std::to_string(idx + 1), r, juce::Justification::centred);
}

void SingleFX::resized()
{
    auto r = getLocalBounds().withHeight(20).translated(0, 25).reduced(2, 0);
    typeSelector->setBounds(r);

    r = r.translated(0, 25).withHeight(24);
    for (const auto &p : fParams)
    {
        p->setBounds(r);
        r = r.translated(0, 25);
    }
    for (const auto &p : iParams)
    {
        p->setBounds(r);
        r = r.translated(0, 25);
    }
}

static constexpr int idOff = 1023;
void SingleFX::onProxyUpdate()
{
    typeSelector->clear(juce::dontSendNotification);
    for (const auto &[fidx, t] : sst::cpputils::enumerate(editor->filterTypeNames))
    {
        typeSelector->addItem(t, fidx + idOff);
    }
    typeSelector->setSelectedId(editor->multi.filters[idx].type.val + idOff,
                                juce::dontSendNotification);

    const auto &fx = editor->multi.filters[idx];
    for (const auto &[fidx, t] : sst::cpputils::enumerate(fx.p))
    {
        fParams[fidx]->setVisible(!t.hidden);
    }
    for (const auto &[fidx, t] : sst::cpputils::enumerate(fx.ip))
    {
        iParams[fidx]->setVisible(!t.hidden);
        iParams[fidx]->setText(t.label, juce::dontSendNotification);
    }
}

void SingleFX::typeSelectorChanged()
{
    auto cidx = typeSelector->getSelectedId();
    auto ftype = cidx - idOff;

    actiondata ad;
    ad.id = ip_multi_filter_type;
    ad.actiontype = vga_intval;
    ad.subid = idx;
    ad.data.i[0] = ftype;
    editor->sendActionToEngine(ad);
}
} // namespace components
} // namespace scxt