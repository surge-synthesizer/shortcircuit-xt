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

#include "SCXTEditor.h"
#include "ZonePage.h"
#include "PageContentBase.h"
#include "components/ZoneKeyboardDisplay.h"
#include "components/WaveDisplay.h"

#include "proxies/ZoneDataProxy.h"
#include "proxies/ZoneListDataProxy.h"

#include "widgets/ParamEditor.h"
#include "widgets/ComboBox.h"
#include "widgets/OutlinedTextButton.h"

#include "sst/cpputils.h"
#include "synthesis/steplfo.h"

namespace scxt
{

namespace pages
{

namespace zone_contents
{
typedef scxt::pages::contents::PageContentBase<scxt::pages::ZonePage> ContentBase;

struct NamesAndRanges : public ContentBase
{
    NamesAndRanges(const scxt::pages::ZonePage &p)
        : ContentBase(p, "names_ranges", "Names & Ranges", juce::Colour(0xFF555555))
    {
        for (const auto &[i, l] :
             sst::cpputils::enumerate(std::array{"xf", "low", "root", "hi", "xf"}))
        {
            rowLabels[i] = whiteLabel(l, juce::Justification::centred);
        }

        auto &cz = parentPage.editor->currentZone;
        nameEd = bind<widgets::SingleLineTextEditor>(cz.name);

        kParams[0] = bindIntSpinBox(cz.key_low_fade);
        kParams[1] = bindIntSpinBox(cz.key_low);
        kParams[2] = bindIntSpinBox(cz.key_root);
        kParams[3] = bindIntSpinBox(cz.key_high);
        kParams[4] = bindIntSpinBox(cz.key_high_fade);

        kParams[1]->onSend = [this]() { parentPage.zoneKeyboardDisplay->repaint(); };
        kParams[3]->onSend = [this]() { parentPage.zoneKeyboardDisplay->repaint(); };

        vParams[0] = bindIntSpinBox(cz.velocity_low_fade);
        vParams[1] = bindIntSpinBox(cz.velocity_low);
        vParams[2] = bindIntSpinBox(cz.velocity_high);
        vParams[3] = bindIntSpinBox(cz.velocity_high_fade);

        for (const auto &[i, l] : sst::cpputils::enumerate(std::array{"low", "source", "high"}))
        {
            ncLabels[i] = whiteLabel(l);
        }
        for (int nc = 0; nc < 2; ++nc)
        {
            ncLow[nc] = bindIntSpinBox(cz.nc[nc].low);
            ncHigh[nc] = bindIntSpinBox(cz.nc[nc].high);
            ncSource[nc] = bindIntComboBox(cz.nc[nc].source, parentPage.editor->zoneNCSrc);
        }
    }

    void resized() override
    {
        auto b = getContentsBounds().reduced(2, 2);
        auto h = b.getHeight();
        auto rh = h / 7;
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
        for (int i = 0; i < 3; ++i)
        {
            auto ls = row.withWidth(row.getWidth() * 0.25).reduced(1, 1);
            auto rs = row.withTrimmedLeft(row.getWidth() * 0.75).reduced(1, 1);
            auto cb = row.withWidth(row.getWidth() * 0.5)
                          .translated(row.getWidth() * 0.25, 0)
                          .reduced(1, 1);
            if (i == 2)
            {
                ncLabels[0]->setBounds(ls);
                ncLabels[2]->setBounds(rs);
                ncLabels[1]->setBounds(cb);
            }
            else
            {
                ncLow[i]->setBounds(ls);
                ncHigh[i]->setBounds(rs);
                ncSource[i]->setBounds(cb);
            }
            row = row.translated(0, rh);
        }
    }

    std::unique_ptr<widgets::SingleLineTextEditor> nameEd;

    std::array<std::unique_ptr<juce::Label>, 5> rowLabels;
    std::array<std::unique_ptr<widgets::IntParamSpinBox>, 5> kParams;
    std::array<std::unique_ptr<widgets::IntParamSpinBox>, 4> vParams;

    std::array<std::unique_ptr<widgets::IntParamSpinBox>, 2> ncLow;
    std::array<std::unique_ptr<widgets::IntParamSpinBox>, 2> ncHigh;
    std::array<std::unique_ptr<widgets::IntParamComboBox>, 2> ncSource;

    std::array<std::unique_ptr<juce::Label>, 2> kLabels;
    std::array<std::unique_ptr<juce::Label>, 2> vLabels;
    std::array<std::unique_ptr<juce::Label>, 3> ncLabels;
};

struct Envelope : public ContentBase
{
    const int whichEnv{-1};
    Envelope(const scxt::pages::ZonePage &p, const scxt::style::Selector &sel,
             const std::string &label, int we)
        : ContentBase(p, sel, label, juce::Colour(0xFF447744)), whichEnv(we)
    {
        auto &env = parentPage.editor->currentZone.env[we];
        shapes[0] = bindFloatSpinBox(env.s0);
        shapes[1] = bindFloatSpinBox(env.s1);
        shapes[2] = bindFloatSpinBox(env.s2);

        ahdsr[0] = bindFloatVSlider(env.a, "A");
        ahdsr[1] = bindFloatVSlider(env.h, "H");
        ahdsr[2] = bindFloatVSlider(env.d, "D");
        ahdsr[3] = bindFloatVSlider(env.s, "S");
        ahdsr[4] = bindFloatVSlider(env.r, "R");
    }

    void resized() override
    {
        auto b = getContentsBounds().reduced(2, 2);

        auto top = b.withHeight(24);
        auto bot = b.withTrimmedTop(24);

        auto wb3 = top.getWidth() / 3.0;
        auto tb = top.withWidth(wb3);
        for (int i = 0; i < 3; ++i)
        {
            shapes[i]->setBounds(tb.reduced(1, 1));
            tb = tb.translated(wb3, 0);
        }

        auto wb5 = bot.getWidth() / 5.0;
        auto wb = bot.withWidth(wb5);
        for (int i = 0; i < 5; ++i)
        {
            ahdsr[i]->setBounds(wb.reduced(1, 1));
            wb = wb.translated(wb5, 0);
        }
    }

    std::array<std::unique_ptr<widgets::FloatParamSlider>, 5> ahdsr;
    std::array<std::unique_ptr<widgets::FloatParamSpinBox>, 3> shapes;
};

struct Routing : public ContentBase
{
    Routing(const scxt::pages::ZonePage &p)
        : ContentBase(p, "modulation_routing", "Modulation Routing", juce::Colour(0xFF555555))
    {
        auto &mm = p.editor->currentZone.mm;
        for (int i = 0; i < 6; ++i)
        {
            active[i] = bind<widgets::IntParamToggleButton>(mm[i].active, std::to_string(i + 1));
            source[i] = bindIntComboBox(mm[i].source, p.editor->zoneMMSrc);
            source2[i] = bindIntComboBox(mm[i].source2, p.editor->zoneMMSrc2);
            destination[i] = bindIntComboBox(mm[i].destination, p.editor->zoneMMDst);
            curve[i] = bindIntComboBox(mm[i].curve, p.editor->zoneMMCurve);
            strength[i] = bindFloatSpinBox(mm[i].strength);
        }

        activateTabs();
    }

    int getTabCount() const override { return 2; }
    std::string getTabLabel(int i) const override
    {
        if (i == 0)
            return "1-6";
        return "7-12";
    }
    void switchToTab(int idx) override
    {
        auto &mm = parentPage.editor->currentZone.mm;
        for (int i = 0; i < 6; ++i)
        {
            rebind(active[i], mm[i + idx * 6].active);
            active[i]->setButtonText(std::to_string(i + 1 + idx * 6));
            rebind(source[i], mm[i + idx * 6].source);
            rebind(source2[i], mm[i + idx * 6].source2);
            rebind(destination[i], mm[i + idx * 6].destination);
            rebind(curve[i], mm[i + idx * 6].curve);
            rebind(strength[i], mm[i + idx * 6].strength);
        }
    }
    void resized() override
    {
        ContentBase::resized();
        auto b = getContentsBounds();
        auto rh = b.getHeight() / 6;
        auto row = b.withHeight(rh);

        auto w0 = row.getWidth();
        for (int i = 0; i < 6; ++i)
        {
            auto div = contents::RowDivider(row);
            active[i]->setBounds(div.next(0.05));
            source[i]->setBounds(div.next(0.23));
            source2[i]->setBounds(div.next(0.23));
            strength[i]->setBounds(div.next(0.10));
            destination[i]->setBounds(div.next(0.23));
            curve[i]->setBounds(div.rest());
            row = row.translated(0, rh);
        }
    }

    std::array<std::unique_ptr<widgets::IntParamComboBox>, 6> source, source2, destination, curve;
    std::array<std::unique_ptr<widgets::FloatParamSpinBox>, 6> strength;
    std::array<std::unique_ptr<widgets::IntParamToggleButton>, 6> active;
};

struct Filters : public ContentBase
{
    Filters(const ZonePage &p) : ContentBase(p, "filters", "Filters", juce::Colour(0xFF444477))
    {
        for (int i = 0; i < 2; ++i)
        {
            auto &ft = parentPage.editor->currentZone.filter[i];
            bind(filters[i], ft, parentPage.editor->zoneFilterType);
        }
    }
    void paintContentInto(juce::Graphics &g, const juce::Rectangle<int> &bounds) override
    {
        contents::SectionDivider::divideSectionVertically(g, bounds, 2, juce::Colours::black);
    }

    void resized() override
    {
        auto cb = getContentsBounds();
        auto hw = cb.getWidth() / 2;
        auto b = cb.withWidth(hw);
        auto sideCol = 60;
        for (int i = 0; i < 2; ++i)
        {
            auto &fr = filters[i];
            auto fSide = b.reduced(3, 1).withTrimmedRight(sideCol);
            auto iSide = b.translated(b.getWidth() - sideCol, 0).withWidth(sideCol).reduced(1, 1);
            if (i == 1)
            {
                fSide = fSide.translated(sideCol, 0);
                iSide = iSide.translated(sideCol - b.getWidth(), 0);
            }
            auto rg = contents::RowGenerator(fSide, 1 + n_filter_parameters);
            fr.type->setBounds(rg.next());

            for (auto q = 0; q < n_filter_parameters; ++q)
                fr.fp[q]->setBounds(rg.next());

            auto srg = contents::RowGenerator(iSide, 10);
            fr.bypass->setBounds(srg.next());
            fr.mix->setBounds(srg.next());
            for (int q = 0; q < n_filter_iparameters; ++q)
                fr.ip[q]->setBounds(srg.next(4));

            b = b.translated(hw, 0);
        }
    }

    std::array<ContentBase::FilterRegion, 2> filters;
};

struct Outputs : public ContentBase
{
    Outputs(const ZonePage &p) : ContentBase(p, "outputs", "Outputs", juce::Colour(0xFF444477))
    {
        for (int i = 0; i < 3; ++i)
        {
            auto &aux = parentPage.editor->currentZone.aux[i];
            output[i] = bindIntComboBox(aux.output, parentPage.editor->zoneAuxOutput);
            level[i] = bindFloatHSlider(aux.level);
            balance[i] = bindFloatHSlider(aux.balance);
            if (i != 0)
                outmode[i] = bind<widgets::IntParamMultiSwitch>(widgets::IntParamMultiSwitch::HORIZ,
                                                                aux.outmode);
        }
        for (const auto &[i, l] : sst::cpputils::enumerate(std::array{"main", "aux1", "aux2"}))
        {
            outLabel[i] = whiteLabel(l, juce::Justification::right);
        }

        auto &cz = parentPage.editor->currentZone;
        mute = bind<widgets::IntParamToggleButton>(cz.mute, "mute");
        pfg = bindFloatHSlider(cz.pre_filter_gain);
    }
    void paintContentInto(juce::Graphics &g, const juce::Rectangle<int> &bounds) override
    {
        contents::SectionDivider::divideSectionHorizontally(g, bounds, 4, juce::Colours::black);
    }

    void resized() override
    {
        auto cb = getContentsBounds();
        auto rg = contents::RowGenerator(cb, 4);
        for (int i = 0; i < 3; ++i)
        {
            auto outB = rg.next().reduced(2, 2);
            auto rrg = contents::RowGenerator(outB, 5);
            output[i]->setBounds(rrg.next());
            if (i != 0)
                outmode[i]->setBounds(rrg.next().reduced(1, 1));
            level[i]->setBounds(rrg.next());
            balance[i]->setBounds(rrg.next());
            outLabel[i]->setBounds(rrg.next());
        }

        auto mutePFG = rg.next().reduced(2, 2);

        auto mbox = mutePFG.withHeight(20).translated(0, 20);
        pfg->setBounds(mbox.reduced(1));
        mbox = mbox.translated(0, 20);
        mute->setBounds(mbox.reduced(1));
    }

    std::array<std::unique_ptr<juce::Label>, 3> outLabel;
    std::array<std::unique_ptr<widgets::IntParamComboBox>, 3> output;
    std::array<std::unique_ptr<widgets::FloatParamSlider>, 3> level, balance;
    std::array<std::unique_ptr<widgets::IntParamMultiSwitch>, 3> outmode; // 0 will be null
    std::unique_ptr<widgets::IntParamToggleButton> mute;
    std::unique_ptr<widgets::FloatParamSlider> pfg;
};

struct Pitch : ContentBase
{
    Pitch(const ZonePage &p) : ContentBase(p, "pitch", "Pitch & etc", juce::Colour(0xFF555555))
    {
        for (const auto &[i, l] :
             sst::cpputils::enumerate(std::array{"PB Range", "Coarse", "Mute Group"}))
            leftLabel[i] = whiteLabel(l, juce::Justification::right);
        for (const auto &[i, l] :
             sst::cpputils::enumerate(std::array{"Keytrack", "Fine", "Vel Sense"}))
            rightLabel[i] = whiteLabel(l);
        auto &cz = parentPage.editor->currentZone;

        leftColumn[0] = bindIntSpinBox(cz.pitch_bend_depth);
        leftColumn[1] = bindIntSpinBox(cz.transpose);
        leftColumn[2] = bindIntSpinBox(cz.mute_group);
        rightColumn[0] = bindFloatSpinBox(cz.keytrack);
        rightColumn[1] = bindFloatSpinBox(cz.finetune);
        rightColumn[2] = bindFloatSpinBox(cz.velsense);

        ignorePM = bind<widgets::IntParamToggleButton>(cz.ignore_part_polymode, "ignore playmode");
        lags[0] = bindFloatSpinBox(cz.lag_generator[0]);
        lags[1] = bindFloatSpinBox(cz.lag_generator[1]);
        lagLabel = whiteLabel("Lag Gen 1/2");
    }

    void resized() override
    {
        auto cb = getContentsBounds();
        auto rg = contents::RowGenerator(cb, 4);
        for (int i = 0; i < 3; ++i)
        {
            auto row = rg.next();
            auto rd = contents::RowDivider(row);
            leftLabel[i]->setBounds(rd.next(0.3));
            leftColumn[i]->setBounds(rd.next(0.2));
            rightColumn[i]->setBounds(rd.next(0.25));
            rightLabel[i]->setBounds(rd.rest());
        }
        auto row = rg.next();
        auto rd = contents::RowDivider(row);
        ignorePM->setBounds(rd.next(0.4));
        lags[0]->setBounds(rd.next(0.15));
        lags[1]->setBounds(rd.next(0.15));
        lagLabel->setBounds(rd.rest());
    }

    std::array<std::unique_ptr<juce::Label>, 3> leftLabel, rightLabel;
    std::array<std::unique_ptr<widgets::IntParamSpinBox>, 3> leftColumn;
    std::array<std::unique_ptr<widgets::FloatParamSpinBox>, 3> rightColumn;
    std::unique_ptr<widgets::IntParamToggleButton> ignorePM;
    std::array<std::unique_ptr<widgets::FloatParamSpinBox>, 2> lags;
    std::unique_ptr<juce::Label> lagLabel;
};

struct Sample : ContentBase
{
    Sample(const ZonePage &p) : ContentBase(p, "sample", "Sample", juce::Colour(0xFF444477))
    {
        auto &cz = parentPage.editor->currentZone;
        playmode = bind<widgets::IntParamComboBox>(cz.playmode, parentPage.editor->zonePlaymode);
        reverse = bind<widgets::IntParamToggleButton>(cz.reverse, "reverse");

        transposeL = whiteLabel("transpose");
        transpose = bindIntSpinBox(cz.transpose);
    }

    void resized() override
    {
        auto b = getContentsBounds();
        auto r = b.withHeight(20);
        playmode->setBounds(r.reduced(1));

        auto q = b.withTrimmedTop(50).withWidth(150);
        reverse->setBounds(q.reduced(1));

        r = r.translated(0, 22).withTrimmedLeft(100);
        transposeL->setBounds(r.withWidth(r.getWidth() / 2));
        transpose->setBounds(r.withTrimmedLeft(r.getWidth() / 2));
    }

    std::unique_ptr<widgets::IntParamComboBox> playmode;
    std::unique_ptr<widgets::IntParamToggleButton> reverse;
    std::unique_ptr<widgets::IntParamSpinBox> transpose;
    std::unique_ptr<juce::Label> transposeL;
};

struct LFO : ContentBase
{
    struct LFOStepsComponent : public juce::Component, style::DOMParticipant
    {
        LFO &content;
        std::reference_wrapper<data::LFOData> lf;
        LFOStepsComponent(LFO &c, data::LFOData &lf)
            : content(c), lf(lf), style::DOMParticipant("lfo_display")
        {
            setIsDOMContainer(false);
        }

        void paint(juce::Graphics &g) override
        {
            g.fillAll(juce::Colours::black);
            auto bg = juce::Colour(0xAAFFFF00);
            auto bar = juce::Colours::blue;
            auto lfop = juce::Path();
            bool startedPath{false};
            int n_steps = lf.get().repeat.val;
            auto rb =
                getLocalBounds().toFloat().withWidth(getWidth() / std::max(1, lf.get().repeat.val));
            for (const auto &[i, p] : sst::cpputils::enumerate(lf.get().data))
            {
                if (i > n_steps)
                    break;
                auto qb = rb.reduced(0.5, 0.5);
                g.setColour(bg);
                g.fillRect(qb);
                auto v = std::clamp(p.val, -1.f, 1.f);
                if (v > 0)
                {
                    auto bb = qb.withHeight(qb.getHeight() / 2.0)
                                  .withTrimmedTop((1 - v) * qb.getHeight() / 2);
                    g.setColour(bar);
                    g.fillRect(bb);
                }
                else
                {
                    auto bb = qb.withTrimmedTop(qb.getHeight() / 2.0)
                                  .withTrimmedBottom((1 + v) * qb.getHeight() / 2);
                    g.setColour(bar);
                    g.fillRect(bb);
                }
                if (n_steps > 0)
                {
                    float h[4];
                    h[0] = lf.get().data[(i + 1) % n_steps].val;
                    h[1] = lf.get().data[(i + 0) % n_steps].val;
                    h[2] = lf.get().data[(n_steps + i - 1) % n_steps].val;
                    h[3] = lf.get().data[(n_steps + i - 2) % n_steps].val;
                    float m = 1.f / ((float)(rb.getWidth() + 1.f));

                    for (int x = 0; x < (int)(rb.getWidth() + 1); x++)
                    {
                        float phase = ((float)x) * m;
                        float f = lfo_ipol(h, phase, lf.get().smooth.val, i & 1);
                        int xp = x + rb.getX();
                        float yp = (1 - f) * rb.getHeight() * 0.5;
                        if (!startedPath)
                        {
                            lfop.startNewSubPath(xp, yp);
                            startedPath = true;
                        }
                        else
                        {
                            lfop.lineTo(xp, yp);
                        }
                    }
                }
                rb = rb.translated(rb.getWidth(), 0);
            }
            g.setColour(juce::Colours::white);
            g.strokePath(lfop, juce::PathStrokeType(1.0));
        }

        void setLFOFromMouseEvent(const juce::MouseEvent &e)
        {
            int step = std::clamp(
                (int)std::floor(e.position.x * std::max(1, lf.get().repeat.val) / getWidth()), 0,
                31);
            float yPos =
                std::clamp(1.f * (getHeight() - e.position.y) / getHeight(), 0.f, 1.f) * 2.f - 1.f;

            auto &datum = lf.get().data[step];
            actiondata ad;
            ad.id = datum.id;
            ad.subid = datum.subid;
            ad.actiontype = vga_steplfo_data_single;
            ad.data.i[0] = step;
            ad.data.f[1] = yPos;

            datum.val = yPos;
            content.parentPage.editor->sendActionToEngine(ad);
            repaint();
        }
        void mouseUp(const juce::MouseEvent &e) override
        {
            if (getLocalBounds().contains(e.position.toInt()))
                setLFOFromMouseEvent(e);
        }
        void mouseDrag(const juce::MouseEvent &e) override { setLFOFromMouseEvent(e); }
    };

    struct ForceRepaint : data::ParameterProxy<int>::Listener
    {
        LFO &lf;
        ForceRepaint(LFO &lf) : lf(lf) {}
        void onChanged() override { lf.repaint(); }
    };
    std::unique_ptr<ForceRepaint> repaintL;

    LFO(const ZonePage &p) : ContentBase(p, "lfo", "LFO", juce::Colour(0xFF447744))
    {
        repaintL = std::make_unique<ForceRepaint>(*this);
        activateTabs();

        currentLFO = 0;
        auto &lf = parentPage.editor->currentZone.lfo[currentLFO];
        lfoRegion = std::make_unique<LFOStepsComponent>(*this, lf);
        addAndMakeVisible(*lfoRegion);

        repeat = bindIntSpinBox(lf.repeat);
        rate = bindFloatHSlider(lf.rate);
        smooth = bindFloatSpinBox(lf.smooth);
        shuffle = bindFloatSpinBox(lf.shuffle);
        temposync = bind<widgets::IntParamToggleButton>(lf.temposync, "temposync");
        triggermode =
            bind<widgets::IntParamMultiSwitch>(widgets::IntParamMultiSwitch::HORIZ, lf.triggermode);
        cyclemode =
            bind<widgets::IntParamMultiSwitch>(widgets::IntParamMultiSwitch::HORIZ, lf.cyclemode);
        onlyonce = bind<widgets::IntParamToggleButton>(lf.onlyonce, "once");
        preset = bindIntComboBox(lf.presetLoad, parentPage.editor->zoneLFOPresets);
        lf.presetLoad.addListener(repaintL.get());
        shapeLab = whiteLabel("shape");
        stepLab = whiteLabel("steps");
    }

    ~LFO()
    {
        parentPage.editor->currentZone.lfo[currentLFO].presetLoad.removeListener(repaintL.get());
    }

    void resized() override
    {
        ContentBase::resized();

        auto b = getContentsBounds();
        auto ls = b.withWidth(120).reduced(1, 1);
        auto rs = b.withTrimmedLeft(120).reduced(1, 1);

        auto lrg = contents::RowGenerator(ls, 5);
        rate->setBounds(lrg.next().reduced(1, 1));
        lrg.next();
        temposync->setBounds(lrg.next().reduced(1, 1));
        cyclemode->setBounds(lrg.next().reduced(1, 1));
        triggermode->setBounds(lrg.next().reduced(1, 1));

        auto rrg = contents::RowGenerator(rs, 5);
        lfoRegion->setBounds(rrg.next(4));
        auto rd = contents::RowDivider(rrg.next());
        shapeLab->setBounds(rd.next(.125));
        smooth->setBounds(rd.next(.175));
        stepLab->setBounds(rd.next(.125));
        repeat->setBounds(rd.next(.175));
        onlyonce->setBounds(rd.next(.15));
        preset->setBounds(rd.next(.25));
    }

    int getTabCount() const override { return 3; }
    std::string getTabLabel(int i) const override { return std::to_string(i + 1); }

    virtual void switchToTab(int i) override
    {
        parentPage.editor->currentZone.lfo[currentLFO].presetLoad.removeListener(repaintL.get());
        currentLFO = i;
        auto &lf = parentPage.editor->currentZone.lfo[i];
        lfoRegion->lf = lf;
        rebind(repeat, lf.repeat);
        rebind(rate, lf.rate);
        rebind(smooth, lf.smooth);
        rebind(shuffle, lf.shuffle);
        rebind(temposync, lf.temposync);
        rebind(triggermode, lf.triggermode);
        rebind(cyclemode, lf.cyclemode);
        rebind(onlyonce, lf.onlyonce);
        rebind(preset, lf.presetLoad);
        lf.presetLoad.addListener(repaintL.get());
        repaint();
    }

    std::unique_ptr<LFOStepsComponent> lfoRegion;

    std::unique_ptr<widgets::IntParamSpinBox> repeat;
    std::unique_ptr<widgets::FloatParamSlider> rate;
    std::unique_ptr<widgets::FloatParamSpinBox> smooth, shuffle;
    std::unique_ptr<widgets::IntParamToggleButton> temposync;
    std::unique_ptr<widgets::IntParamMultiSwitch> triggermode;
    std::unique_ptr<widgets::IntParamMultiSwitch> cyclemode;
    std::unique_ptr<widgets::IntParamToggleButton> onlyonce;
    std::unique_ptr<widgets::IntParamComboBox> preset;
    std::unique_ptr<juce::Label> shapeLab, stepLab;

    int currentLFO{0};
};

} // namespace zone_contents

ZonePage::ZonePage(SCXTEditor *ed, SCXTEditor::Pages p) : PageBase(ed, p, "zone")
{
    zoneKeyboardDisplay = std::make_unique<components::ZoneKeyboardDisplay>(editor, editor);
    addAndMakeVisible(*zoneKeyboardDisplay);

    waveDisplay = std::make_unique<components::WaveDisplay>(editor, editor);
    addAndMakeVisible(*waveDisplay);

    for (int i = 0; i < num_layers; ++i)
    {
        std::string ln = "A";
        ln[0] += i;
        layerButtons[i] = std::make_unique<scxt::widgets::OutlinedTextButton>(ln);
        layerButtons[i]->setClickingTogglesState(true);
        layerButtons[i]->setToggleState(i == 0, juce::dontSendNotification);
        layerButtons[i]->setRadioGroupId(172, juce::NotificationType::dontSendNotification);
        layerButtons[i]->onClick = [this, i]() {
            editor->selectLayer(i);
            editor->repaint();
        };
        addAndMakeVisible(*layerButtons[i]);
    }

    namespace zc = zone_contents;
    sample = makeContent<zc::Sample>(*this);
    namesAndRanges = makeContent<zc::NamesAndRanges>(*this);
    pitch = makeContent<zone_contents::Pitch>(*this);
    envelopes[0] =
        makeContent<zone_contents::Envelope>(*this, scxt::style::Selector{"aeg"}, "AEG", 0);
    envelopes[1] =
        makeContent<zone_contents::Envelope>(*this, scxt::style::Selector{"eg2"}, "EG2", 1);
    filters = makeContent<zone_contents::Filters>(*this);
    routing = makeContent<zone_contents::Routing>(*this);
    lfo = makeContent<zone_contents::LFO>(*this);
    outputs = makeContent<zone_contents::Outputs>(*this);
}
ZonePage::~ZonePage() = default;

void ZonePage::resized()
{
    auto r = getLocalBounds();

    zoneKeyboardDisplay->setBounds(r.withHeight(80).withTrimmedLeft(20));
    waveDisplay->setBounds(r.withHeight(100).translated(0, 80).withTrimmedLeft(20));

    auto layerR = r.withWidth(20).withHeight(180);
    auto rg = contents::RowGenerator(layerR, num_layers);
    for (const auto &lb : layerButtons)
        lb->setBounds(rg.next());

    auto lower = r.withTrimmedTop(180);

    auto wa = getWidth() * 0.3;
    auto wb = getWidth() * 0.5;
    auto wc = getWidth() - wa - wb;

    outputs->setBounds(lower.withTrimmedLeft(getWidth() - wc).reduced(1, 1));

    auto s1 = lower.withWidth(wa);
    auto s2 = lower.withWidth(wb).translated(wa, 0);
    auto ha = lower.getHeight() * 0.7;
    auto hb = lower.getHeight() - ha;

    sample->setBounds(s1.withHeight(ha * 0.3).reduced(1, 1));
    namesAndRanges->setBounds(s1.withHeight(ha * 0.4).translated(0, ha * 0.3).reduced(1, 1));
    pitch->setBounds(s1.withHeight(ha * 0.3).translated(0, ha * 0.7).reduced(1, 1));

    filters->setBounds(s2.withHeight(ha * 0.6).reduced(1, 1));
    routing->setBounds(s2.withHeight(ha * 0.4).translated(0, ha * 0.6).reduced(1, 1));

    auto eBox = s1.withHeight(hb).translated(0, ha);
    envelopes[0]->setBounds(eBox.withWidth(eBox.getWidth() * 0.5).reduced(1, 1));
    envelopes[1]->setBounds(
        eBox.withWidth(eBox.getWidth() * 0.5).translated(eBox.getWidth() * 0.5, 0).reduced(1, 1));
    lfo->setBounds(s2.withHeight(hb).translated(0, ha).reduced(1, 1));
}

void ZonePage::connectProxies()
{
    // Probably change this
    editor->zoneListProxy->clients.insert(zoneKeyboardDisplay.get());
    editor->uiStateProxies.insert(waveDisplay.get());
}

void ZonePage::onProxyUpdate()
{
    PageBase::onProxyUpdate();
    if (editor->selectedLayer >= 0 && editor->selectedLayer < num_layers)
        layerButtons[editor->selectedLayer]->setToggleState(true, juce::dontSendNotification);
    repaint();
}

} // namespace pages
} // namespace scxt
