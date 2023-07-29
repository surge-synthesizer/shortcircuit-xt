/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "MultiScreen.h"
#include "browser/BrowserPane.h"
#include "multi/AdsrPane.h"
#include "multi/LFOPane.h"
#include "multi/MappingPane.h"
#include "multi/ModPane.h"
#include "multi/OutputPane.h"
#include "multi/ProcessorPane.h"
#include "multi/PartGroupSidebar.h"

#include "connectors/SCXTStyleSheetCreator.h"

namespace scxt::ui
{

static_assert(engine::processorCount == MultiScreen::numProcessorDisplays);

struct DebugRect : public sst::jucegui::components::NamedPanel
{
    struct CL : juce::Component
    {
        juce::Colour color;
        std::string label;
        void paint(juce::Graphics &g) override
        {
            g.setFont(juce::Font("Comic Sans MS", 40, juce::Font::plain));

            g.setColour(color);
            g.drawText(label, getLocalBounds(), juce::Justification::centred);
        }
    };
    std::unique_ptr<CL> cl;
    DebugRect(const juce::Colour &c, const std::string &s) : sst::jucegui::components::NamedPanel(s)
    {
        cl = std::make_unique<CL>();
        cl->color = c;
        cl->label = s;
        addAndMakeVisible(*cl);
    }
    void resized() override { cl->setBounds(getContentArea()); }
};

MultiScreen::MultiScreen(SCXTEditor *e) : HasEditor(e)
{
    parts = std::make_unique<multi::PartGroupSidebar>(editor);
    addAndMakeVisible(*parts);

    auto br = std::make_unique<browser::BrowserPane>(editor);
    browser = std::move(br);
    addAndMakeVisible(*browser);
    sample = std::make_unique<multi::MappingPane>(editor);
    addAndMakeVisible(*sample);

    zoneGroupElements[0] = std::make_unique<ZoneOrGroupElements>(ZoneGroupIndex::ZONE);
    zoneGroupElements[1] = std::make_unique<ZoneOrGroupElements>(ZoneGroupIndex::GROUP);

    for (const auto &ctr : zoneGroupElements)
    {
        for (int i = 0; i < 4; ++i)
        {
            auto ff = std::make_unique<multi::ProcessorPane>(editor, i,
                                                             ctr->index == ZoneGroupIndex::ZONE);
            ff->hasHamburger = true;
            ctr->processors[i] = std::move(ff);
            addChildComponent(*(ctr->processors[i]));
        }
        ctr->mod = std::make_unique<multi::ModPane>(editor);
        addChildComponent(*ctr->mod);
        ctr->output = std::make_unique<multi::OutputPane>(editor);
        addChildComponent(*ctr->output);

        for (int i = 0; i < 2; ++i)
        {
            ctr->eg[i] =
                std::make_unique<multi::AdsrPane>(editor, i, ctr->index == ZoneGroupIndex::ZONE);
            addChildComponent(*(ctr->eg[i]));
        }
        ctr->lfo = std::make_unique<multi::LfoPane>(editor);
        addChildComponent(*ctr->lfo);

        if (ctr->index == ZoneGroupIndex::GROUP)
        {
            ctr->output->setCustomClass(connectors::SCXTStyleSheetCreator::GroupMultiNamedPanel);
            ctr->mod->setCustomClass(connectors::SCXTStyleSheetCreator::GroupMultiNamedPanel);
            ctr->lfo->setCustomClass(connectors::SCXTStyleSheetCreator::GroupMultiNamedPanel);
            ctr->lfo->tabNames = {"GLFO 1", "GLFO 2", "GLFO 3"};
            ctr->lfo->resetTabState();
            int idx{1};
            for (const auto &e : ctr->eg)
            {
                e->setCustomClass(connectors::SCXTStyleSheetCreator::GroupMultiNamedPanel);
                e->setName("GRP EG" + std::to_string(idx++));
            }
            for (const auto &p : ctr->processors)
                p->setCustomClass(connectors::SCXTStyleSheetCreator::GroupMultiNamedPanel);
        }
    }

    setSelectionMode(SelectionMode::ZONE);
}

MultiScreen::~MultiScreen() = default;

void MultiScreen::layout()

{
    parts->setBounds(pad, pad, sideWidths, getHeight() - 3 * pad);
    browser->setBounds(getWidth() - sideWidths - pad, pad, sideWidths, getHeight() - 3 * pad);

    auto mainRect = juce::Rectangle<int>(
        sideWidths + 3 * pad, pad, getWidth() - 2 * sideWidths - 6 * pad, getHeight() - 3 * pad);

    auto wavHeight = mainRect.getHeight() - envHeight - modHeight - fxHeight;
    sample->setBounds(mainRect.withHeight(wavHeight));

    for (const auto &ctr : zoneGroupElements)
    {
        auto fxRect = mainRect.withTrimmedTop(wavHeight).withHeight(fxHeight);
        auto fw = fxRect.getWidth() * 0.25;
        auto tfr = fxRect.withWidth(fw);
        for (int i = 0; i < 4; ++i)
        {
            ctr->processors[i]->setBounds(tfr);
            tfr.translate(fw, 0);
        }

        auto modRect = mainRect.withTrimmedTop(wavHeight + fxHeight).withHeight(modHeight);
        auto mw = modRect.getWidth() * 0.750;
        ctr->mod->setBounds(modRect.withWidth(mw));
        auto xw = modRect.getWidth() * 0.250;
        ctr->output->setBounds(modRect.withWidth(xw).translated(mw, 0));

        auto envRect =
            mainRect.withTrimmedTop(wavHeight + fxHeight + modHeight).withHeight(envHeight);
        auto ew = envRect.getWidth() * 0.25;
        ctr->eg[0]->setBounds(envRect.withWidth(ew));
        ctr->eg[1]->setBounds(envRect.withWidth(ew).translated(ew, 0));
        ctr->lfo->setBounds(envRect.withWidth(ew * 2).translated(ew * 2, 0));
    }
}

void MultiScreen::onVoiceInfoChanged() { sample->repaint(); }

void MultiScreen::setSelectionMode(scxt::ui::MultiScreen::SelectionMode m)
{
    if (selectionMode == m)
        return;
    selectionMode = m;

    switch (selectionMode)
    {
    case SelectionMode::NONE:
        zoneGroupElements[0]->setVisible(false);
        zoneGroupElements[1]->setVisible(false);
        break;

    case SelectionMode::ZONE:
        zoneGroupElements[(int)ZoneGroupIndex::ZONE]->setVisible(true);
        zoneGroupElements[(int)ZoneGroupIndex::GROUP]->setVisible(false);
        break;

    case SelectionMode::GROUP:
        zoneGroupElements[(int)ZoneGroupIndex::ZONE]->setVisible(false);
        zoneGroupElements[(int)ZoneGroupIndex::GROUP]->setVisible(true);
        break;
    }

    repaint();
}

MultiScreen::ZoneOrGroupElements::ZoneOrGroupElements(scxt::ui::MultiScreen::ZoneGroupIndex z)
    : index(z)
{
}

MultiScreen::ZoneOrGroupElements::~ZoneOrGroupElements() = default;

void MultiScreen::ZoneOrGroupElements::setVisible(bool b)
{
    output->setVisible(b);
    lfo->setVisible(b);
    mod->setVisible(b);
    for (auto &e : eg)
        e->setVisible(b);
    for (auto &p : processors)
        p->setVisible(b);
}
} // namespace scxt::ui