//
// Created by Paul Walker on 1/7/22.
//

#include "FXPage.h"
#include "widgets/ComboBox.h"
#include "widgets/ParamEditor.h"
#include "widgets/OutlinedTextButton.h"
#include "sst/cpputils.h"

namespace scxt
{
namespace pages
{
namespace fx_contents
{
struct SingleFX : public juce::Component, public scxt::data::UIStateProxy::Invalidatable
{
    SingleFX(SCXTEditor *ed, int i) : editor(ed), idx(i)
    {
        typeSelector = std::make_unique<scxt::widgets::ComboBox>();
        typeSelector->onChange = [this]() { typeSelectorChanged(); };
        addAndMakeVisible(*typeSelector);

        outputTarget = std::make_unique<scxt::widgets::ComboBox>();
        outputTarget->onChange = [this]() {
            editor->multi.filter_output[idx].sendValue(outputTarget->getSelectedId(), editor);
        };
        addAndMakeVisible(*outputTarget);

        muteButton = std::make_unique<widgets::OutlinedTextButton>("mute");
        muteButton->onClick = [this]() {
            auto &v = editor->multi.filters[idx].bypass;
            v.sendValue(v.val != 0 ? 0 : 1, editor);
            muteButton->setToggleState(v.val, juce::dontSendNotification);
            muteButton->repaint();
        };
        addAndMakeVisible(*muteButton);

        for (auto i = 0; i < n_filter_parameters; ++i)
        {
            auto q = std::make_unique<widgets::FloatParamSlider>(
                widgets::FloatParamSlider::HSLIDER, editor->multi.filters[idx].p[i], ed);
            addChildComponent(*q);
            fParams[i] = std::move(q);
        }

        for (auto i = 0; i < n_filter_iparameters; ++i)
        {
            auto q = std::make_unique<widgets::IntParamMultiSwitch>(
                widgets::IntParamMultiSwitch::VERT, editor->multi.filters[idx].ip[i], editor);
            addChildComponent(*q);
            iParams[i] = std::move(q);
        }
    }

    ~SingleFX() = default;

    void paint(juce::Graphics &g) override
    {
        g.fillAll(findColour(SCXTColours::fxPanelBackground));

        auto r = getLocalBounds().withHeight(20);

        SCXTLookAndFeel::fillWithGradientHeaderBand(
            g, r, findColour(SCXTColours::fxPanelHeaderBackground));
        g.setColour(findColour(SCXTColours::fxPanelHeaderText));
        g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
        g.drawText("Effect " + std::to_string(idx + 1), r, juce::Justification::centred);
    }

    void resized() override
    {
        auto r = getLocalBounds().withHeight(20).translated(0, 25).reduced(2, 0);
        typeSelector->setBounds(r.withTrimmedRight(50));
        muteButton->setBounds(r.withTrimmedLeft(getWidth() - 48));

        auto q = r.translated(0, 25)
                     .withHeight(24 * (n_filter_parameters - 1) / 2)
                     .withTrimmedLeft(r.getWidth() - 48);
        r = r.translated(0, 25).withHeight(24).withTrimmedRight(50);
        for (const auto &p : fParams)
        {
            p->setBounds(r);
            r = r.translated(0, 25);
        }

        r = r.translated(0, 25).withHeight(20);
        outputTarget->setBounds(r);
        for (const auto &p : iParams)
        {
            p->setBounds(q);
            q = q.translated(0, 24 * (n_filter_parameters - 1) / 2 + 24);
        }
    }

    static constexpr int idOff = 1023;
    void onProxyUpdate() override
    {
        outputTarget->clear(juce::dontSendNotification);
        for (const auto [id, nm] : sst::cpputils::enumerate(editor->partAuxOutputNames.data))
        {
            if (!nm.empty())
                outputTarget->addItem(nm, id);
        }
        outputTarget->setSelectedId(editor->multi.filter_output[idx].val,
                                    juce::dontSendNotification);

        typeSelector->clear(juce::dontSendNotification);
        for (const auto &[fidx, t] : sst::cpputils::enumerate(editor->multiFilterTypeNames.data))
        {
            typeSelector->addItem(t, fidx + idOff);
        }
        typeSelector->setSelectedId(editor->multi.filters[idx].type.val + idOff,
                                    juce::dontSendNotification);

        muteButton->setToggleState(editor->multi.filters[idx].bypass.val,
                                   juce::dontSendNotification);
        const auto &fx = editor->multi.filters[idx];
        for (const auto &[fidx, t] : sst::cpputils::enumerate(fx.p))
        {
            fParams[fidx]->setVisible(!t.hidden);
        }
        for (const auto &[fidx, t] : sst::cpputils::enumerate(fx.ip))
        {
            auto lbs = std::vector<std::string>();
            if (!t.hidden)
            {
                auto lb = t.label;
                auto p = lb.find(";");
                while (p != std::string::npos)
                {
                    lbs.push_back(lb.substr(0, p));
                    lb = lb.substr(p + 1);
                    p = lb.find(";");
                }
                lbs.push_back(lb);
                iParams[fidx]->setLabels(lbs);
            }
            iParams[fidx]->setVisible(!t.hidden);
        }
    }

    SCXTEditor *editor{nullptr};
    int idx{-1};

    void typeSelectorChanged()
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

    std::array<std::unique_ptr<widgets::FloatParamSlider>, n_filter_parameters> fParams;
    std::array<std::unique_ptr<widgets::IntParamMultiSwitch>, n_filter_iparameters> iParams;

    std::unique_ptr<widgets::ComboBox> typeSelector;
    std::unique_ptr<widgets::OutlinedTextButton> muteButton;

    std::unique_ptr<widgets::ComboBox> outputTarget;
};
} // namespace fx_contents
FXPage::FXPage(SCXTEditor *e, SCXTEditor::Pages p) : PageBase(e, p)
{
    for (auto i = 0; i < num_fxunits; ++i)
    {
        auto q = std::make_unique<fx_contents::SingleFX>(editor, i);

        addAndMakeVisible(*q);
        fxComponents[i] = std::move(q);
    }
}
FXPage::~FXPage() = default;

void FXPage::resized()
{
    jassert(num_fxunits == 8);
    auto w = getWidth() / 4;
    auto h = getHeight() / 2;

    auto r = getLocalBounds().withWidth(w).withHeight(h);
    for (int i = 0; i < num_fxunits; ++i)
    {
        fxComponents[i]->setBounds(r.reduced(1, 1));
        r = r.translated(w, 0);
        if (i == 3)
            r = r.translated(-4 * w, h);
    }
}

void FXPage::onProxyUpdate()
{
    for (const auto &q : fxComponents)
        q->onProxyUpdate();
}

} // namespace pages
} // namespace scxt