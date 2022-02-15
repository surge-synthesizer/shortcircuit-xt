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

struct VelocitySplit : public ContentBase
{
    VelocitySplit(const scxt::pages::PartPage &p)
        : ContentBase(p, "velocitysplit", "Velocity Split/Crossfade", juce::Colours::darkgrey)
    {
        auto &cp = parentPage.editor->currentPart;
        layers = bind<widgets::IntParamMultiSwitch>(
            widgets::IntParamMultiSwitch::Orientation::HORIZ, cp.part_vs_layers);
        gains = bind<widgets::IntParamMultiSwitch>(widgets::IntParamMultiSwitch::Orientation::VERT,
                                                   cp.part_vs_xf_equality);
        distr = bindFloatHSlider(cp.part_vs_distribution);
        xfade = bindFloatHSlider(cp.part_vs_xfade);
    }
    void resized() override
    {
        auto b = getContentsBounds();
        auto bl = b.withWidth(b.getWidth() / 2);
        auto br = b.withTrimmedLeft(b.getWidth() / 2);

        auto bt = contents::RowGenerator(bl, 2);
        layers->setBounds(bt.next());
        gains->setBounds(bt.next());

        auto bq = contents::RowGenerator(br, 2);
        distr->setBounds(bq.next().withHeight(22).reduced(1));
        xfade->setBounds(bq.next().withHeight(22).reduced(1));
    }
    std::unique_ptr<widgets::IntParamMultiSwitch> layers, gains;
    std::unique_ptr<widgets::FloatParamSlider> distr, xfade;
};

struct LayerRanges : public ContentBase
{
    LayerRanges(const scxt::pages::PartPage &p)
        : ContentBase(p, "layerranges", "Layer Ranges", juce::Colours::darkgrey)
    {
        // this is "bad"
        for (int i = 0; i < 8; ++i)
        {
            char str[2];
            str[0] = 'A' + i;
            str[1] = 0;
            auto q = std::make_unique<widgets::OutlinedTextButton>(str);
            q->setClickingTogglesState(true);
            q->setToggleState(i == 0, juce::NotificationType::dontSendNotification);
            q->setRadioGroupId(10472, juce::NotificationType::dontSendNotification);
            q->onClick = [this, i]() { this->rebindToLayer(i); };
            addAndMakeVisible(*q);
            layerSelectors[i] = std::move(q);
        }

        auto &cp = parentPage.editor->currentPart;
        for (int nc = 0; nc < 2; ++nc)
        {
            ncLow[nc] = bindIntSpinBox(cp.nc[nc].low);
            ncHigh[nc] = bindIntSpinBox(cp.nc[nc].high);
            ncSource[nc] = bindIntComboBox(cp.nc[nc].source, parentPage.editor->partNCSrc);
        }
    }
    void rebindToLayer(int i)
    {
        int off = i * 2;
        auto &cp = parentPage.editor->currentPart;
        for (int nc = 0; nc < 2; ++nc)
        {
            rebind(ncLow[nc], cp.nc[nc + off].low);
            rebind(ncHigh[nc], cp.nc[nc + off].high);
            rebind(ncSource[nc], cp.nc[nc + off].source);
        }
    }

    void resized() override
    {
        auto b = getContentsBounds();
        auto t = b.withHeight(24).reduced(1);
        auto rg = contents::RowDivider(t);
        for (int i = 0; i < 8; ++i)
            layerSelectors[i]->setBounds(rg.next(0.125).reduced(1));

        auto row = t.translated(0, 24).withHeight(20);
        for (int i = 0; i < 2; ++i)
        {
            auto ls = row.withWidth(row.getWidth() * 0.25).reduced(1, 1);
            auto rs = row.withTrimmedLeft(row.getWidth() * 0.75).reduced(1, 1);
            auto cb = row.withWidth(row.getWidth() * 0.5)
                          .translated(row.getWidth() * 0.25, 0)
                          .reduced(1, 1);
            ncLow[i]->setBounds(ls);
            ncHigh[i]->setBounds(rs);
            ncSource[i]->setBounds(cb);
            row = row.translated(0, 22);
        }
    }

    std::array<std::unique_ptr<widgets::IntParamSpinBox>, 2> ncLow;
    std::array<std::unique_ptr<widgets::IntParamSpinBox>, 2> ncHigh;
    std::array<std::unique_ptr<widgets::IntParamComboBox>, 2> ncSource;

    std::array<std::unique_ptr<widgets::OutlinedTextButton>, 8> layerSelectors;
};

struct Polymode : public ContentBase
{
    Polymode(const scxt::pages::PartPage &p)
        : ContentBase(p, "polymode", "Polymode", juce::Colours::darkgrey)
    {
        auto &cp = parentPage.editor->currentPart;
        polymode = bind<widgets::IntParamMultiSwitch>(
            widgets::IntParamMultiSwitch::Orientation::VERT, cp.polymode);
        portamento_mode = bind<widgets::IntParamMultiSwitch>(
            widgets::IntParamMultiSwitch::Orientation::HORIZ, cp.portamento_mode);
        portamento = bindFloatHSlider(cp.portamento);
    }

    void resized() override
    {
        auto b = getContentsBounds();
        auto po = b.withWidth(b.getWidth() * 0.6);
        auto pm = b.withTrimmedLeft(b.getWidth() * 0.65);
        polymode->setBounds(pm.reduced(1));
        auto r = po.withHeight(22);
        portamento_mode->setBounds(r.reduced(1));
        r = r.translated(0, 22);
        portamento->setBounds(r.reduced(1));
    }

    std::unique_ptr<widgets::IntParamMultiSwitch> polymode, portamento_mode;
    std::unique_ptr<widgets::FloatParamSlider> portamento;
};

struct Controllers : public ContentBase
{
    Controllers(const scxt::pages::PartPage &p)
        : ContentBase(p, "controllers", "Controllers", juce::Colour(0xFF335533))
    {
        activateTabs();
        auto &pd = p.editor->currentPart;
        for (int i = 0; i < 8; ++i)
        {
            names[i] = bind<widgets::SingleLineTextEditor>(pd.userparameter_name[i]);
            bipolars[i] = bind<widgets::IntParamToggleButton>(pd.userparameter_polarity[i], "+/-");
            sliders[i] = bind<widgets::FloatParamSlider>(widgets::FloatParamSlider::HSLIDER,
                                                         pd.userparameter[i]);
        }
    }

    void resized() override
    {
        ContentBase::resized();
        auto b = getContentsBounds();
        auto w2 = b.getWidth() / 2;
        auto ih = 25;
        auto im = 1;
        auto ig = 5;
        auto h = (ih + im) * 8;
        auto r = b.withWidth(w2).withHeight(ih).translated(0, im).reduced(3, 0);
        for (int i = 0; i < 8; ++i)
        {
            auto nr = r.withTrimmedRight(30);
            auto br = r.withTrimmedLeft(r.getWidth() - 30);
            auto sr = r.translated(0, ih + im);

            names[i]->setBounds(nr.reduced(1));
            bipolars[i]->setBounds(br.reduced(1));
            sliders[i]->setBounds(sr.reduced(1));

            r = r.translated(0, (ih + im) * 2 + ig);

            if (i == 3)
            {
                r = b.withWidth(w2).withHeight(ih).translated(w2, im).reduced(0, 3);
            }
        }
    }

    int getTabCount() const override { return 2; }
    std::string getTabLabel(int i) const override
    {
        if (i == 0)
            return "1-8";
        else
            return "9-16";
    }

    virtual void switchToTab(int t) override
    {
        auto &pd = parentPage.editor->currentPart;
        for (int i = 0; i < 8; ++i)
        {
            rebind(names[i], pd.userparameter_name[i + t * 8]);
            rebind(bipolars[i], pd.userparameter_polarity[i + t * 8]);
            rebind(sliders[i], pd.userparameter[i + t * 8]);
        }
    }
    std::array<std::unique_ptr<widgets::SingleLineTextEditor>, 8> names;
    std::array<std::unique_ptr<widgets::FloatParamSlider>, 8> sliders;
    std::array<std::unique_ptr<widgets::IntParamToggleButton>, 8> bipolars;
};

struct Main : public ContentBase
{
    Main(const scxt::pages::PartPage &p) : ContentBase(p, "main", "Main", juce::Colours::darkgrey)
    {
        auto &part = parentPage.editor->currentPart;

        savePart = std::make_unique<widgets::OutlinedTextButton>("save part");
        savePart->onClick = [this]() { this->savePM(false); };
        addAndMakeVisible(*savePart);
        saveMulti = std::make_unique<widgets::OutlinedTextButton>("save multi");
        saveMulti->onClick = [this]() { this->savePM(true); };
        addAndMakeVisible(*saveMulti);
        /*
         * std::unique_ptr<widgets::IntParamSpinBox> polylimit, midichannel, transpose, formant;
std::unique_ptr<juce::Label> polyL, midiL, transposeL, formantL;
std::unique_ptr<widgets::SingleLineTextEditor> name;
         */

        polylimit = bindIntSpinBox(part.polylimit);
        polyL = whiteLabel("poly limit");
        midichannel = bindIntSpinBox(part.midichannel);
        midiL = whiteLabel("midi channel");
        transpose = bindIntSpinBox(part.transpose);
        transposeL = whiteLabel("transpose");
        formant = bindIntSpinBox(part.formant);
        formantL = whiteLabel("formant");
        name = bind<widgets::SingleLineTextEditor>(part.name);
    }

    void resized() override
    {
        auto b = getContentsBounds();

        auto nr = b.withHeight(20);
        name->setBounds(nr.reduced(1));

        nr = nr.translated(0, 22).withWidth(b.getWidth() - 110);
        polylimit->setBounds(nr.withWidth(100).reduced(1));
        polyL->setBounds(nr.withTrimmedLeft(103));
        nr = nr.translated(0, 20);
        midichannel->setBounds(nr.withWidth(100).reduced(1));
        midiL->setBounds(nr.withTrimmedLeft(103));
        nr = nr.translated(0, 20);
        transpose->setBounds(nr.withWidth(100).reduced(1));
        transposeL->setBounds(nr.withTrimmedLeft(103));
        nr = nr.translated(0, 20);
        formant->setBounds(nr.withWidth(100).reduced(1));
        formantL->setBounds(nr.withTrimmedLeft(103));

        auto mp = b.withTrimmedTop(b.getHeight() - 40).withTrimmedLeft(b.getWidth() - 100);
        mp = mp.withHeight(20);
        savePart->setBounds(mp);
        mp = mp.translated(0, 20);
        saveMulti->setBounds(mp);
    }
    std::unique_ptr<juce::FileChooser> fc;
    void savePM(bool isMulti)
    {
        auto nm = std::string("Save Part");
        if (isMulti)
            nm = "Save Multi";
        fc = std::make_unique<juce::FileChooser>(nm.c_str());
        fc->launchAsync(
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                juce::FileBrowserComponent::warnAboutOverwriting,
            [this, isMulti](const juce::FileChooser &c) {
                auto result = c.getResults();
                if (result.isEmpty() || result.size() > 1)
                    return;

                auto p = fs::path{result[0].getFullPathName().toStdString()};
                if (isMulti)
                    p.replace_extension(fs::path{".sc2m"});
                else
                    p = p.replace_extension(fs::path{".sc2p"});
                this->parentPage.editor->audioProcessor.sc3->editorProxy.savepart_path.set(p);
                actiondata ad;
                ad.id = ip_none;
                ad.subid = -1;
                ad.actiontype = isMulti ? vga_save_multi : vga_save_patch;
                this->parentPage.editor->sendActionToEngine(ad);
            });
    }
    std::unique_ptr<widgets::OutlinedTextButton> savePart, saveMulti;
    std::unique_ptr<widgets::IntParamSpinBox> polylimit, midichannel, transpose, formant;
    std::unique_ptr<juce::Label> polyL, midiL, transposeL, formantL;
    std::unique_ptr<widgets::SingleLineTextEditor> name;
};

struct ModulationRouting : public ContentBase
{
    ModulationRouting(const scxt::pages::PartPage &p)
        : ContentBase(p, "modulation_routing", "Modulation Routing", juce::Colours::darkgrey)
    {
        auto &mm = p.editor->currentPart.mm;
        for (int i = 0; i < 6; ++i)
        {
            active[i] = bind<widgets::IntParamToggleButton>(mm[i].active, std::to_string(i + 1));
            source[i] = bindIntComboBox(mm[i].source, p.editor->partMMSrc);
            source2[i] = bindIntComboBox(mm[i].source2, p.editor->partMMSrc2);
            destination[i] = bindIntComboBox(mm[i].destination, p.editor->partMMDst);
            curve[i] = bindIntComboBox(mm[i].curve, p.editor->partMMCurve);
            strength[i] = bindFloatSpinBox(mm[i].strength);
        }
    }

    void resized() override
    {
        ContentBase::resized();
        auto b = getContentsBounds();
        auto rh = 22;
        auto row = b.withHeight(rh);

        auto w0 = row.getWidth();
        for (int i = 0; i < 6; ++i)
        {
            auto div = contents::RowDivider(row);
            active[i]->setBounds(div.next(0.05));
            source[i]->setBounds(div.next(0.23));
            source2[i]->setBounds(div.next(0.23));
            strength[i]->setBounds(div.next(0.16));
            destination[i]->setBounds(div.next(0.23));
            curve[i]->setBounds(div.rest());
            row = row.translated(0, rh);
        }
    }

    std::array<std::unique_ptr<widgets::IntParamComboBox>, 6> source, source2, destination, curve;
    std::array<std::unique_ptr<widgets::FloatParamSpinBox>, 6> strength;
    std::array<std::unique_ptr<widgets::IntParamToggleButton>, 6> active;
};

struct Effects : public ContentBase
{
    Effects(const scxt::pages::PartPage &p)
        : ContentBase(p, "effects", "Effects", juce::Colour(0xFF555577))
    {
        for (int i = 0; i < 2; ++i)
        {
            bind(filters[i], parentPage.editor->currentPart.filters[i],
                 parentPage.editor->partFilterTypeNames);
        }
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

struct Output : public ContentBase
{
    Output(const scxt::pages::PartPage &p)
        : ContentBase(p, "output", "Output", juce::Colour(0xFF555577))
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
            output[i]->setBounds(rg.next().reduced(0, 1));
            if (i == 0)
            {
                level[i]->setBounds(rg.next());
                balance[i]->setBounds(rg.next());
            }
            else
            {
                outmode[i]->setBounds(ab.withWidth(50).withTrimmedTop(ab.getHeight() / 3 + 2));
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
PartPage::PartPage(SCXTEditor *e, SCXTEditor::Pages p) : PageBase(e, p, "part")
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
    layerRanges->setBounds(juce::Rectangle<int>(0, y0, w1, h1 * 0.25).reduced(1, 1));
    y0 += h1 * 0.2;
    velocitySplit->setBounds(juce::Rectangle<int>(0, y0, w1, h1 * 0.25).reduced(1, 1));
    y0 += h1 * 0.3;

    output->setBounds(juce::Rectangle<int>(w1, 0, w - w1, h1 * 0.22).reduced(1, 1));
    effects->setBounds(juce::Rectangle<int>(w1, h1 * 0.22, w - w1, h1 * 0.78).reduced(1, 1));

    modulationRouting->setBounds(juce::Rectangle<int>(0, h1, w / 2, h - h1).reduced(1, 1));
    controllers->setBounds(juce::Rectangle<int>(w / 2, h1, w / 2, h - h1).reduced(1, 1));
}

} // namespace pages
} // namespace scxt