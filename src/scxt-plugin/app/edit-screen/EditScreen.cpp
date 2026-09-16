/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
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
#include "messaging/client/structure_messages.h"

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

    // These are all stored as 0 based tab indices, and each pane's tab list is longer than
    // the count of things it modulates - the LFO panes carry MISC and AUDIO too - so bound
    // against the pane rather than against the configuration constant
    auto gts = editor->queryTabSelection(tabKey("multi.group.lfo"));
    if (!gts.empty())
    {
        auto gt = std::atoi(gts.c_str());
        if (gt >= 0 && gt < (int)groupElements->lfo->tabNames.size())
            groupElements->lfo->selectTab(gt);
    }
    auto zts = editor->queryTabSelection(tabKey("multi.zone.lfo"));
    if (!zts.empty())
    {
        auto zt = std::atoi(zts.c_str());
        if (zt >= 0 && zt < (int)zoneElements->lfo->tabNames.size())
            zoneElements->lfo->selectTab(zt);
    }

    auto zeg = editor->queryTabSelection(tabKey("multi.zone.eg"));
    if (!zeg.empty())
    {
        auto zt = std::atoi(zeg.c_str());
        // eg[1] tabs EG2 up, so its tab 0 is the first restorable one
        if (zt >= 0 && zt < (int)zoneElements->eg[1]->tabNames.size())
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

void EditScreen::selectAllInPart(bool forZone)
{
    const auto &lead =
        forZone ? editor->currentLeadZoneSelection : editor->currentLeadGroupSelection;

    std::vector<selection::SelectionManager::SelectActionContents> actions;
    for (const auto &r : partSidebar->pgzStructure)
    {
        const auto &a = r.address;
        if (a.part != editor->selectedPart || a.group < 0 || (a.zone >= 0) != forZone)
            continue;

        auto se = selection::SelectionManager::SelectActionContents(a);
        se.selecting = true;
        se.distinct = false;
        se.selectingAsLead = lead.has_value() && *lead == a;
        se.forZone = forZone;
        actions.push_back(se);
    }
    if (actions.empty())
        return;

    if (!lead.has_value())
        actions.front().selectingAsLead = true;
    editor->doSelectionAction(actions);
}

namespace
{
using za_t = selection::SelectionManager::ZoneAddress;

// a lead can arrive holding an unset address, which says there is no lead
bool isLeadSet(const std::optional<za_t> &lead) { return lead.has_value() && lead->group >= 0; }

std::vector<za_t> selectionOrLead(const selection::SelectionManager::selectedZones_t &all,
                                  const std::optional<za_t> &lead)
{
    std::vector<za_t> res(all.begin(), all.end());
    if (res.empty() && isLeadSet(lead))
        res.push_back(*lead);
    return res;
}
} // namespace

bool EditScreen::doZoneEditCommand(KeyCommands command)
{
    namespace cmsg = scxt::messaging::client;
    const auto &lead = editor->currentLeadZoneSelection;
    auto targets = selectionOrLead(editor->allZoneSelections, lead);

    switch (command)
    {
    case SELECT_ALL:
        selectAllInPart(true);
        return true;
    case COPY:
    case CUT:
        if (targets.empty())
            return false;
        sendToSerialization(cmsg::CopyZones(targets));
        if (command == CUT)
            sendToSerialization(cmsg::DeleteAllSelectedZones(true));
        return true;
    case PASTE:
    {
        if (editor->clipboardType != engine::Clipboard::ContentType::ZONE)
            return false;
        // after the lead zone, else the end of the lead group, else wherever a new zone goes
        auto into = za_t{editor->selectedPart, -1, -1};
        const auto &leadGroup = editor->currentLeadGroupSelection;
        if (isLeadSet(lead))
            into = *lead;
        else if (isLeadSet(leadGroup))
            into = *leadGroup;
        sendToSerialization(cmsg::PasteZone(into));
        return true;
    }
    case DUPLICATE:
        if (targets.empty())
            return false;
        sendToSerialization(cmsg::DuplicateZones(targets));
        return true;
    case DELETE_SELECTED:
        if (targets.empty())
            return false;
        sendToSerialization(cmsg::DeleteAllSelectedZones(true));
        return true;
    default:
        break;
    }
    return false;
}

bool EditScreen::doGroupEditCommand(KeyCommands command)
{
    namespace cmsg = scxt::messaging::client;
    const auto &lead = editor->currentLeadGroupSelection;
    auto targets = selectionOrLead(editor->allGroupSelections, lead);

    switch (command)
    {
    case SELECT_ALL:
        selectAllInPart(false);
        return true;
    case COPY:
    case CUT:
        if (targets.empty())
            return false;
        sendToSerialization(cmsg::CopyGroups(targets));
        if (command == CUT)
            sendToSerialization(cmsg::DeleteAllSelectedGroups(true));
        return true;
    case PASTE:
        if (editor->clipboardType != engine::Clipboard::ContentType::GROUP)
            return false;
        // after the lead group, else the end of the part
        sendToSerialization(
            cmsg::PasteGroup(isLeadSet(lead) ? *lead : za_t{editor->selectedPart, -1, -1}));
        return true;
    case DUPLICATE:
        if (targets.empty())
            return false;
        sendToSerialization(cmsg::DuplicateGroups(targets));
        return true;
    case DELETE_SELECTED:
        if (targets.empty())
            return false;
        sendToSerialization(cmsg::DeleteAllSelectedGroups(true));
        return true;
    default:
        break;
    }
    return false;
}

template struct EditScreen::ZoneOrGroupElements<typename EditScreen::ZoneTraits>;
template struct EditScreen::ZoneOrGroupElements<typename EditScreen::GroupTraits>;
} // namespace scxt::ui::app::edit_screen
