/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#include "EditScreen.h"
#include "app/browser-ui/BrowserPane.h"
#include "app/edit-screen/components/AdsrPane.h"
#include "app/edit-screen/components/LFOPane.h"
#include "app/edit-screen/components/MacroMappingVariantPane.h"
#include "app/edit-screen/components/ModPane.h"
#include "app/edit-screen/components/RoutingPane.h"
#include "app/edit-screen/components/ProcessorPane.h"
#include "app/edit-screen/components/PartGroupSidebar.h"
#include "app/edit-screen/components/PartEditScreen.h"

#include "app/SCXTEditor.h"

namespace scxt::ui::app::edit_screen
{

static_assert(engine::processorCount == EditScreen::numProcessorDisplays);

EditScreen::EditScreen(SCXTEditor *e) : HasEditor(e)
{
    partSidebar = std::make_unique<edit_screen::PartGroupSidebar>(editor);
    addAndMakeVisible(*partSidebar);

    auto br = std::make_unique<browser_ui::BrowserPane>(editor);
    browser = std::move(br);
    addAndMakeVisible(*browser);
    mappingPane = std::make_unique<edit_screen::MacroMappingVariantPane>(editor);
    addAndMakeVisible(*mappingPane);

    zoneElements = std::make_unique<ZoneOrGroupElements<ZoneTraits>>(this);
    groupElements = std::make_unique<ZoneOrGroupElements<GroupTraits>>(this);

    partEditScreen = std::make_unique<PartEditScreen>(this);
    addAndMakeVisible(*partEditScreen);

    setSelectionMode(SelectionMode::ZONE);
}

EditScreen::~EditScreen() = default;

void EditScreen::layout()

{
    partSidebar->setBounds(pad, pad, sideWidths, getHeight() - 3 * pad);
    browser->setBounds(getWidth() - sideWidths - pad, pad, sideWidths, getHeight() - 3 * pad);

    auto mainRect = juce::Rectangle<int>(
        sideWidths + 3 * pad, pad, getWidth() - 2 * sideWidths - 6 * pad, getHeight() - 3 * pad);

    partEditScreen->setBounds(mainRect);

    auto wavHeight = mainRect.getHeight() - envHeight - modHeight - fxHeight;
    mappingPane->setBounds(mainRect.withHeight(wavHeight));

    zoneElements->layoutInto(mainRect);
    groupElements->layoutInto(mainRect);
}

void EditScreen::onVoiceInfoChanged()
{
    voiceCountByZoneAddress.clear();
    for (const auto &v : editor->sharedUiMemoryState.voiceDisplayItems)
    {
        if (v.active)
        {
            auto sa = selection::SelectionManager::ZoneAddress(v.part, v.group, v.zone);
            if (voiceCountByZoneAddress.find(sa) == voiceCountByZoneAddress.end())
                voiceCountByZoneAddress[sa] = 0;
            voiceCountByZoneAddress[sa]++;
        }
    }

    mappingPane->repaint();
    partSidebar->repaint();
}

void EditScreen::addSamplePlaybackPosition(size_t sampleIndex, int64_t samplePos)
{
    mappingPane->addSamplePlaybackPosition(sampleIndex, samplePos);
}

void EditScreen::clearSamplePlaybackPositions() { mappingPane->clearSamplePlaybackPositions(); }

void EditScreen::setSelectionMode(EditScreen::SelectionMode m)
{
    if (selectionMode == m)
        return;
    selectionMode = m;

    switch (selectionMode)
    {
    case SelectionMode::NONE:
        zoneElements->setVisible(false);
        groupElements->setVisible(false);
        mappingPane->setVisible(true);
        partEditScreen->setVisible(false);
        break;

    case SelectionMode::ZONE:
        zoneElements->setVisible(true);
        groupElements->setVisible(false);
        mappingPane->setVisible(true);
        partEditScreen->setVisible(false);
        break;

    case SelectionMode::GROUP:
        zoneElements->setVisible(false);
        groupElements->setVisible(true);
        mappingPane->setVisible(true);
        partEditScreen->setVisible(false);
        break;

    case SelectionMode::PART:
        zoneElements->setVisible(false);
        groupElements->setVisible(false);
        mappingPane->setVisible(false);
        partEditScreen->setVisible(true);
        break;
    }

    repaint();
}

template <typename ZGTrait>
EditScreen::ZoneOrGroupElements<ZGTrait>::ZoneOrGroupElements(EditScreen *parent)
{
    for (int i = 0; i < scxt::processorsPerZoneAndGroup; ++i)
    {
        auto ff = std::make_unique<edit_screen::ProcessorPane>(parent->editor, i, forZone);
        ff->hasHamburger = true;
        processors[i] = std::move(ff);
        parent->addChildComponent(*(processors[i]));
    }
    modPane = std::make_unique<edit_screen::ModPane<typename ZGTrait::ModPaneTraits>>(
        parent->editor, forZone);
    routingPane = std::make_unique<edit_screen::RoutingPane<typename ZGTrait::RoutingPaneTraits>>(
        parent->editor);
    for (int i = 0; i < scxt::processorsPerZoneAndGroup; ++i)
    {
        routingPane->addWeakProcessorPaneReference(
            i, juce::Component::SafePointer(processors[i].get()));
    }
    routingPane->updateFromProcessorPanes();
    parent->addChildComponent(*modPane);
    parent->addChildComponent(*routingPane);

    for (int i = 0; i < scxt::egsPerGroup; ++i)
    {
        auto egt = std::make_unique<edit_screen::AdsrPane>(parent->editor, i, forZone);
        eg[i] = std::move(egt);
        parent->addChildComponent(*eg[i]);
    }
    lfo = std::make_unique<edit_screen::LfoPane>(parent->editor, forZone);
    parent->addChildComponent(*lfo);

    auto &theme = parent->editor->themeApplier;
    if (forZone)
    {
        theme.applyZoneMultiScreenTheme(routingPane.get());
        for (const auto &p : processors)
            theme.applyZoneMultiScreenTheme(p.get());
        theme.applyZoneMultiScreenModulationTheme(modPane.get());
        theme.applyZoneMultiScreenModulationTheme(eg[0].get());
        theme.applyZoneMultiScreenModulationTheme(eg[1].get());
        theme.applyZoneMultiScreenModulationTheme(lfo.get());
    }
    else
    {
        lfo->setTabsForGLFO();
        eg[0]->setName("GRP EG1");
        eg[1]->setName("GRP EG2");

        theme.applyGroupMultiScreenTheme(routingPane.get());
        for (const auto &p : processors)
            theme.applyGroupMultiScreenTheme(p.get());
        theme.applyGroupMultiScreenModulationTheme(modPane.get());
        theme.applyGroupMultiScreenModulationTheme(eg[0].get());
        theme.applyGroupMultiScreenModulationTheme(eg[1].get());
        theme.applyGroupMultiScreenModulationTheme(lfo.get());
    }
}

template <typename ZGTrait>
EditScreen::ZoneOrGroupElements<ZGTrait>::~ZoneOrGroupElements() = default;

template <typename ZGTrait> void EditScreen::ZoneOrGroupElements<ZGTrait>::setVisible(bool b)
{
    routingPane->setVisible(b);
    lfo->setVisible(b);
    modPane->setVisible(b);
    for (auto &e : eg)
        e->setVisible(b);
    for (auto &p : processors)
        p->setVisible(b);
}

template <typename ZGTrait>
void EditScreen::ZoneOrGroupElements<ZGTrait>::layoutInto(const juce::Rectangle<int> &mainRect)
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
    auto mw = modRect.getWidth() * 0.625;
    modPane->setBounds(modRect.withWidth(mw));
    auto xw = modRect.getWidth() * 0.375;
    routingPane->setBounds(modRect.withWidth(xw).translated(mw, 0));

    auto envRect = mainRect.withTrimmedTop(wavHeight + fxHeight + modHeight).withHeight(envHeight);
    auto ew = envRect.getWidth() * 0.25;
    eg[0]->setBounds(envRect.withWidth(ew));
    eg[1]->setBounds(envRect.withWidth(ew).translated(ew, 0));
    lfo->setBounds(envRect.withWidth(ew * 2).translated(ew * 2, 0));
}

void EditScreen::onOtherTabSelection()
{
    auto pgz = editor->queryTabSelection(tabKey("multi.pgz"));
    if (pgz.empty())
    {
    }
    else if (pgz == "part")
        partSidebar->setSelectedTab(0);
    else if (pgz == "group")
        partSidebar->setSelectedTab(1);
    else if (pgz == "zone")
        partSidebar->setSelectedTab(2);
    else
        partSidebar->setSelectedTab(2); // default to zone on unknown

    auto gts = editor->queryTabSelection(tabKey("multi.group.lfo"));
    if (!gts.empty())
    {
        auto gt = std::atoi(gts.c_str());
        if (gt >= 0 && gt < lfosPerGroup + 1) // for misc
            groupElements->lfo->selectTab(gt);
    }
    auto zts = editor->queryTabSelection(tabKey("multi.zone.lfo"));
    if (!zts.empty())
    {
        auto zt = std::atoi(zts.c_str());
        if (zt >= 0 && zt < lfosPerZone + 1) // for misc
            zoneElements->lfo->selectTab(zt);
    }

    auto zeg = editor->queryTabSelection(tabKey("multi.zone.eg"));
    if (!zeg.empty())
    {
        auto zt = std::atoi(zeg.c_str());
        if (zt >= 1 && zt < egsPerZone + 1) // for misc
            zoneElements->eg[1]->selectTab(zt);
    }

    auto mts = editor->queryTabSelection(tabKey("multi.mapping"));
    if (!mts.empty())
    {
        auto mt = std::atoi(mts.c_str());
        if (mt >= 0 && mt < 3)
            mappingPane->setSelectedTab(mt);
    }

    auto pt = editor->queryTabSelection(tabKey("multi.part.top"));
    if (!pt.empty())
    {
        auto mt = std::atoi(pt.c_str());
        if (mt >= 0 && mt < 2)
            partEditScreen->topPanel->selectTab(mt);
    }
}

void EditScreen::selectedPartChanged()
{
    if (partSidebar)
        partSidebar->selectedPartChanged();
    if (mappingPane)
        mappingPane->selectedPartChanged();
    if (partEditScreen)
        partEditScreen->selectedPartChanged();
}

void EditScreen::macroDataChanged(int part, int index)
{
    mappingPane->macroDataChanged(part, index);
    partEditScreen->macroDataChanged(part, index);
}

template struct EditScreen::ZoneOrGroupElements<typename EditScreen::ZoneTraits>;
template struct EditScreen::ZoneOrGroupElements<typename EditScreen::GroupTraits>;
} // namespace scxt::ui::app::edit_screen
