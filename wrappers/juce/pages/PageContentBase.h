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

#ifndef SHORTCIRCUIT_PAGECOMPONENTBASE_H
#define SHORTCIRCUIT_PAGECOMPONENTBASE_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "widgets/ParamEditor.h"
#include "data/SCXTData.h"
#include "data/SSTJuceGuiDataAdapters.h"
#include "SCXTLookAndFeel.h"
#include "SCXTLayoutValues.h"
#include "widgets/OutlinedTextButton.h"
#include "style/StyleSheet.h"

#include <sst/jucegui/components/HSlider.h>
#include <sst/jucegui/components/MultiSwitch.h>
#include <sst/jucegui/components/ToggleButton.h>

namespace scxt
{
namespace pages
{
namespace contents
{

struct RowDivider
{
    juce::Rectangle<int> row;
    float howFar{0.f};
    int rx, ry;
    RowDivider(const juce::Rectangle<int> &r, int rx = 1, int ry = 1) : row(r), rx(rx), ry(ry) {}
    juce::Rectangle<int> next(float q)
    {
        auto res = row.withWidth(q * row.getWidth())
                       .translated(howFar * row.getWidth(), 0)
                       .reduced(rx, ry);
        howFar += q;
        return res;
    }

    juce::Rectangle<int> rest() { return next(1.0 - howFar); }
};

struct RowGenerator
{
    juce::Rectangle<int> parent;
    juce::Rectangle<int> row;
    float dHeight{1.f};
    int nSteps{0};
    RowGenerator(const juce::Rectangle<int> &r, int nSteps) : parent(r), nSteps(nSteps)
    {
        dHeight = r.getHeight() * 1.f / nSteps;
        row = r.withHeight(dHeight);
    }
    juce::Rectangle<int> next(int span = 1)
    {
        auto res = row.withHeight(dHeight * span);
        row = row.translated(0, dHeight * span);
        return res;
    }
};

struct ItemHeightRowGenerator
{
    juce::Rectangle<int> parent;
    juce::Rectangle<int> row;
    ItemHeightRowGenerator(const juce::Rectangle<int> &r) : parent(r)
    {
        row = r.withHeight(scxt::layout::itemRowHeight);
    }
    juce::Rectangle<int> next(int span = 1)
    {
        auto res = row.withHeight(scxt::layout::itemRowHeight * span);
        row = row.translated(0, scxt::layout::itemRowHeight * span);
        return res;
    }
    juce::Rectangle<int> nextGap()
    {
        auto res = row.withHeight(scxt::layout::itemRowHeightGap);
        row = row.translated(0, scxt::layout::itemRowHeightGap);
        return res;
    }
};

struct SectionDivider
{
    static void divideSectionVertically(juce::Graphics &g, const juce::Rectangle<int> &bounds,
                                        int nRegions, juce::Colour bar)
    {
        auto q = bounds.withWidth(1).reduced(0, 4);
        auto dw = bounds.getWidth() * 1.f / nRegions;
        for (int i = 0; i < nRegions - 1; ++i)
        {
            q = q.translated(dw, 0);
            g.setColour(bar);
            g.fillRect(q);
        }
    }
    static void divideSectionHorizontally(juce::Graphics &g, const juce::Rectangle<int> &bounds,
                                          int nRegions, juce::Colour bar)
    {
        auto q = bounds.withHeight(1).reduced(4, 0);
        auto dh = bounds.getHeight() * 1.f / nRegions;
        for (int i = 0; i < nRegions - 1; ++i)
        {
            q = q.translated(0, dh);
            g.setColour(bar);
            g.fillRect(q);
        }
    }
};

template <typename T, typename C = juce::Component>
struct PageContentBase : public C,
                         scxt::data::UIStateProxy::Invalidatable,
                         scxt::style::DOMParticipant
{
    PageContentBase(const T &p, const scxt::style::Selector &sel, const std::string &header,
                    const juce::Colour &col)
        : C(header), parentPage(p), scxt::style::DOMParticipant(sel), header(header), col(col)
    {
        setupJuceAccessibility();
    }
    static constexpr int headerSize = 16;

    void paint(juce::Graphics &g) override
    {
        // LEGACY
        if constexpr (std::is_same<C, juce::Component>::value)
        {
            g.fillAll(col);
            auto h = this->getLocalBounds().withHeight(headerSize);
            SCXTLookAndFeel::fillWithGradientHeaderBand(g, h, col);
            g.setColour(parentPage.editor->sheet->resolveColour(*this, style::headerTextColor));
            g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
            g.drawText(header, h, juce::Justification::centred);

            auto b = this->getLocalBounds().withTrimmedBottom(headerSize);
            auto gs = juce::Graphics::ScopedSaveState(g);
            g.addTransform(juce::AffineTransform().translated(0, headerSize));
            paintContentInto(g, b);
        }
        else
        {
            C::paint(g);
        }
    }

    juce::Rectangle<int> getContentsBounds()
    {
        if constexpr (std::is_same<C, juce::Component>::value)
        {
            return this->getLocalBounds().withTrimmedTop(headerSize);
        }
        else
        {
            return this->getContentArea();
        }
    }
    virtual void paintContentInto(juce::Graphics &g, const juce::Rectangle<int> &bounds) {}
    void onProxyUpdate() override
    {
        for (auto q : this->getChildren())
            if (auto iv = dynamic_cast<scxt::data::UIStateProxy::Invalidatable *>(q))
                iv->onProxyUpdate();
    }
    std::string header;
    juce::Colour col;

    std::vector<std::unique_ptr<widgets::OutlinedTextButton>> tabButtons;
    void activateTabs()
    {
        if (getTabCount() <= 1)
            return;
        for (int i = 0; i < getTabCount(); ++i)
        {
            auto q = std::make_unique<widgets::OutlinedTextButton>(getTabLabel(i));
            q->onClick = [this, i]() {
                if (tabButtons[i]->getToggleState())
                    switchToTab(i);
            };
            q->setRadioGroupId(842, juce::NotificationType::dontSendNotification);
            q->setClickingTogglesState(true);
            q->setToggleState(i == 0, juce::NotificationType::dontSendNotification);
            this->addAndMakeVisible(*q);
            tabButtons.push_back(std::move(q));
        }
    }
    void resized() override
    {
        if (getTabCount() <= 1)
            return;

        auto bw = 40;
        auto hb = this->getLocalBounds().withHeight(headerSize).withWidth(bw);
        hb = hb.translated(this->getWidth() - (getTabCount()) * bw - 5, 0);
        for (const auto &t : tabButtons)
        {
            t->setBounds(hb.reduced(1, 1));
            hb = hb.translated(bw, 0);
        }
    }
    virtual int getTabCount() const { return 1; }
    virtual std::string getTabLabel(int i) const { return ""; }
    virtual void switchToTab(int i) {}

    template <typename W, typename... Args> auto bindJuceGUI(Args &&...args)
    {
        auto q = std::make_unique<W>(std::forward<Args>(args)...);
        this->addAndMakeVisible(*q);
        juce::Component *psc = q.get();
        // This is pretty inefficient
        sst::jucegui::style::StyleConsumer *ultimateStyleParent{nullptr};
        while (psc)
        {
            if (auto sc = dynamic_cast<sst::jucegui::style::StyleConsumer *>(psc))
            {
                ultimateStyleParent = sc;
            }
            psc = psc->getParentComponent();
        }

        q->setStyle(ultimateStyleParent->style());
        return q;
    }

    template <typename W, typename... Args> auto bind(Args &&...args)
    {
        auto q = std::make_unique<W>(std::forward<Args>(args)..., parentPage.editor);
        this->addAndMakeVisible(*q);
        return q;
    }

    template <typename P> auto bindFloatSpinBox(P &param)
    {
        return bind<widgets::FloatParamSpinBox>(param);
    }
    template <typename P> auto bindFloatSlider(P &param, widgets::FloatParamSlider::Style o)
    {
        return bind<widgets::FloatParamSlider>(o, param);
    }
    template <typename P> auto bindFloatVSlider(P &param, const std::string &fallbackLabel)
    {
        auto res = bind<widgets::FloatParamSlider>(widgets::FloatParamSlider::VSLIDER, param);
        res->fallbackLabel = fallbackLabel;
        return res;
    }
    template <typename P> auto bindFloatHSlider(P &param)
    {
        return bind<widgets::FloatParamSlider>(widgets::FloatParamSlider::HSLIDER, param);
    }
    template <typename P> auto bindIntSpinBox(P &param)
    {
        return bind<widgets::IntParamSpinBox>(param);
    }
    template <typename P> auto bindIntComboBox(P &param, const scxt::data::NameList &lab)
    {
        return bind<widgets::IntParamComboBox>(param, lab);
    }

    template <typename W, typename Q> auto rebind(const W &wid, Q &par)
    {
        wid->param = par;
        wid->onRebind();
        wid->repaint();
    }

    struct DOMLabel : public juce::Label, style::DOMParticipant
    {
        DOMLabel(const juce::String &s) : juce::Label(s), style::DOMParticipant("label")
        {
            setIsDOMContainer(false);
        }
    };
    auto whiteLabel(const std::string &txt, juce::Justification j = juce::Justification::left)
    {
        auto q = std::make_unique<DOMLabel>(txt);
        q->setText(txt, juce::dontSendNotification);
        q->setColour(juce::Label::textColourId, juce::Colours::white);
        q->setJustificationType(juce::Justification::centred);
        q->setFont(SCXTLookAndFeel::getMonoFontAt(9));
        q->setJustificationType(j);
        this->addAndMakeVisible(*q);
        return q;
    }

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

    auto standIn(const std::string &txt)
    {
        auto q = std::make_unique<StandIn>(txt);
        addAndMakeVisible(*q);
        return q;
    }

    struct FilterRegion
    {
        ~FilterRegion()
        {
            for (const auto &f : fp)
                f->setSource(nullptr);
            for (const auto &i : ip)
                i->setSource(nullptr);
            bypass->setSource(nullptr);
        }
        std::unique_ptr<widgets::IntParamComboBox> type;

        std::unique_ptr<data::juceguiadapters::BinaryDiscrete> bypassSource;
        std::unique_ptr<sst::jucegui::components::ToggleButton> bypass;

        std::unique_ptr<widgets::FloatParamSpinBox> mix;

        std::array<std::unique_ptr<sst::jucegui::components::HSlider>, n_filter_parameters> fp;
        std::array<std::unique_ptr<data::juceguiadapters::ContinuousFloat>, n_filter_parameters>
            fpSource;

        std::array<std::unique_ptr<sst::jucegui::components::MultiSwitch>, n_filter_iparameters> ip;
        std::array<std::unique_ptr<data::juceguiadapters::Discrete>, n_filter_iparameters> ipSource;
    };

    void bind(FilterRegion &fr, data::FilterData &ft, scxt::data::NameList &choices,
              bool bindMix = true)
    {
        fr.type = bindIntComboBox(ft.type, choices);
        for (int q = 0; q < n_filter_parameters; ++q)
        {
            fr.fpSource[q] = std::make_unique<data::juceguiadapters::ContinuousFloat>(
                ft.p[q], parentPage.editor);
            ft.p[q].addListener(fr.fpSource[q].get());

            fr.fp[q] = bindJuceGUI<sst::jucegui::components::HSlider>();
            fr.fp[q]->setSource(fr.fpSource[q].get());
        }
        fr.bypassSource =
            std::make_unique<data::juceguiadapters::BinaryDiscrete>(ft.bypass, parentPage.editor);
        fr.bypass = bindJuceGUI<sst::jucegui::components::ToggleButton>();
        fr.bypass->setSource(fr.bypassSource.get());
        fr.bypass->setLabel("mute");

        if (bindMix)
            fr.mix = bindFloatSpinBox(ft.mix);

        for (int q = 0; q < n_filter_iparameters; ++q)
        {
            fr.ipSource[q] =
                std::make_unique<data::juceguiadapters::Discrete>(ft.ip[q], parentPage.editor);
            ft.ip[q].addListener(fr.ipSource[q].get());

            fr.ip[q] = bindJuceGUI<sst::jucegui::components::MultiSwitch>(
                sst::jucegui::components::MultiSwitch::VERTICAL);
            fr.ip[q]->setElementSize(scxt::layout::multiswitchItemHeight);

            fr.ip[q]->setSource(fr.ipSource[q].get());
        }
    }

    const T &parentPage;
};
} // namespace contents
} // namespace pages
} // namespace scxt

#endif // SHORTCIRCUIT_PAGECOMPONENTBASE_H
