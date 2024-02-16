/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

    zoneElements = std::make_unique<ZoneOrGroupElements<ZoneTraits>>(this);
    groupElements = std::make_unique<ZoneOrGroupElements<GroupTraits>>(this);

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

    zoneElements->layoutInto(mainRect);
    groupElements->layoutInto(mainRect);
}

void MultiScreen::onVoiceInfoChanged() { sample->repaint(); }

void MultiScreen::updateSamplePlaybackPosition(int64_t samplePos)
{
    sample->updateSamplePlaybackPosition(samplePos);
}

void MultiScreen::setSelectionMode(scxt::ui::MultiScreen::SelectionMode m)
{
    if (selectionMode == m)
        return;
    selectionMode = m;

    switch (selectionMode)
    {
    case SelectionMode::NONE:
        zoneElements->setVisible(false);
        groupElements->setVisible(false);
        break;

    case SelectionMode::ZONE:
        zoneElements->setVisible(true);
        groupElements->setVisible(false);
        break;

    case SelectionMode::GROUP:
        zoneElements->setVisible(false);
        groupElements->setVisible(true);
        break;
    }

    repaint();
}

template <typename ZGTrait>
MultiScreen::ZoneOrGroupElements<ZGTrait>::ZoneOrGroupElements(MultiScreen *parent)
{
    for (int i = 0; i < scxt::processorsPerZoneAndGroup; ++i)
    {
        auto ff = std::make_unique<multi::ProcessorPane>(parent->editor, i, forZone);
        ff->hasHamburger = true;
        processors[i] = std::move(ff);
        parent->addChildComponent(*(processors[i]));
    }
    modPane =
        std::make_unique<multi::ModPane<typename ZGTrait::ModPaneTraits>>(parent->editor, forZone);
    outPane = std::make_unique<multi::OutputPane<typename ZGTrait::OutPaneTraits>>(parent->editor);
    parent->addChildComponent(*modPane);
    parent->addChildComponent(*outPane);

    for (int i = 0; i < scxt::egPerGroup; ++i)
    {
        auto egt = std::make_unique<multi::AdsrPane<typename ZGTrait::AdsrTraits>>(parent->editor,
                                                                                   i, forZone);
        eg[i] = std::move(egt);
        parent->addChildComponent(*eg[i]);
    }
    lfo = std::make_unique<multi::LfoPane>(parent->editor, forZone);
    parent->addChildComponent(*lfo);

    if (!forZone)
    {
        outPane->setCustomClass(connectors::SCXTStyleSheetCreator::GroupMultiNamedPanel);
        modPane->setCustomClass(connectors::SCXTStyleSheetCreator::GroupMultiNamedPanel);
        lfo->setCustomClass(connectors::SCXTStyleSheetCreator::GroupMultiNamedPanel);
        lfo->tabNames = {"GLFO 1", "GLFO 2", "GLFO 3"};
        lfo->resetTabState();
        int idx{1};
        for (const auto &e : eg)
        {
            e->setCustomClass(connectors::SCXTStyleSheetCreator::GroupMultiNamedPanel);
            e->setName("GRP EG" + std::to_string(idx++));
        }
        for (const auto &p : processors)
            p->setCustomClass(connectors::SCXTStyleSheetCreator::GroupMultiNamedPanel);
    }
}

template <typename ZGTrait>
MultiScreen::ZoneOrGroupElements<ZGTrait>::~ZoneOrGroupElements() = default;

template <typename ZGTrait> void MultiScreen::ZoneOrGroupElements<ZGTrait>::setVisible(bool b)
{
    outPane->setVisible(b);
    lfo->setVisible(b);
    modPane->setVisible(b);
    for (auto &e : eg)
        e->setVisible(b);
    for (auto &p : processors)
        p->setVisible(b);
}

template <typename ZGTrait>
void MultiScreen::ZoneOrGroupElements<ZGTrait>::layoutInto(const juce::Rectangle<int> &mainRect)
{
    auto wavHeight = mainRect.getHeight() - envHeight - modHeight - fxHeight;

    auto fxRect = mainRect.withTrimmedTop(wavHeight).withHeight(fxHeight);
    auto fw = fxRect.getWidth() * 0.25;
    auto tfr = fxRect.withWidth(fw);
    for (int i = 0; i < 4; ++i)
    {
        processors[i]->setBounds(tfr);
        tfr.translate(fw, 0);
    }

    auto modRect = mainRect.withTrimmedTop(wavHeight + fxHeight).withHeight(modHeight);
    auto mw = modRect.getWidth() * 0.750;
    modPane->setBounds(modRect.withWidth(mw));
    auto xw = modRect.getWidth() * 0.250;
    outPane->setBounds(modRect.withWidth(xw).translated(mw, 0));

    auto envRect = mainRect.withTrimmedTop(wavHeight + fxHeight + modHeight).withHeight(envHeight);
    auto ew = envRect.getWidth() * 0.25;
    eg[0]->setBounds(envRect.withWidth(ew));
    eg[1]->setBounds(envRect.withWidth(ew).translated(ew, 0));
    lfo->setBounds(envRect.withWidth(ew * 2).translated(ew * 2, 0));
}

template struct MultiScreen::ZoneOrGroupElements<typename MultiScreen::ZoneTraits>;
template struct MultiScreen::ZoneOrGroupElements<typename MultiScreen::GroupTraits>;
} // namespace scxt::ui
