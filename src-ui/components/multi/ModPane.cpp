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

#include "ModPane.h"
#include "components/SCXTEditor.h"
#include "connectors/SCXTStyleSheetCreator.h"
#include "connectors/PayloadDataAttachment.h"
#include "sst/jucegui/components/HSlider.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "messaging/client/client_serial.h"

namespace scxt::ui::multi
{
namespace cmsg = scxt::messaging::client;

struct ModRow : juce::Component, HasEditor
{
    int index{0};
    ModPane *parent{nullptr};
    std::unique_ptr<
        connectors::BooleanPayloadDataAttachment<ModRow, modulation::VoiceModMatrix::Routing>>
        powerAttachment;
    std::unique_ptr<connectors::PayloadDataAttachment<ModRow, modulation::VoiceModMatrix::Routing>>
        depthAttachment;
    std::unique_ptr<sst::jucegui::components::ToggleButton> power;
    std::unique_ptr<juce::TextButton> source, sourceVia, curve, target;
    std::unique_ptr<sst::jucegui::components::HSlider> depth;
    // std::unique_ptr<sst::jucegui::components::HSlider> slider;
    ModRow(SCXTEditor *e, int i, ModPane *p) : HasEditor(e), index(i), parent(p)
    {
        auto &row = parent->routingTable[i];

        powerAttachment = std::make_unique<
            connectors::BooleanPayloadDataAttachment<ModRow, modulation::VoiceModMatrix::Routing>>(
            this, "Power",
            [w = juce::Component::SafePointer(this)](const auto &a) {
                if (w)
                    w->pushRowUpdate();
            },
            [](const auto &pl) { return pl.active; }, row.active);
        power = std::make_unique<sst::jucegui::components::ToggleButton>();
        power->setLabel(std::to_string(index + 1));
        power->setSource(powerAttachment.get());
        addAndMakeVisible(*power);

        source = std::make_unique<juce::TextButton>("Source", "Source");
        source->onClick = [w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showSourceMenu(false);
        };
        addAndMakeVisible(*source);

        sourceVia = std::make_unique<juce::TextButton>("Via", "Via");
        sourceVia->onClick = [w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showSourceMenu(true);
        };
        addAndMakeVisible(*sourceVia);

        depthAttachment = std::make_unique<
            connectors::PayloadDataAttachment<ModRow, modulation::VoiceModMatrix::Routing>>(
            this, datamodel::pmd().asPercentBipolar().withName("Depth"),
            [w = juce::Component::SafePointer(this)](const auto &a) {
                if (w)
                    w->pushRowUpdate();
            },
            [](const auto &pl) { return pl.depth; }, row.depth);
        depth = std::make_unique<sst::jucegui::components::HSlider>();
        depth->setSource(depthAttachment.get());
        addAndMakeVisible(*depth);

        curve = std::make_unique<juce::TextButton>("Crv", "Crv");
        curve->onClick = [w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showCurveMenu();
        };
        addAndMakeVisible(*curve);

        target = std::make_unique<juce::TextButton>("Target", "Target");
        target->onClick = [w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showTargetMenu();
        };
        addAndMakeVisible(*target);

        refreshRow();
    }

    ~ModRow()
    {
        // remove these explicitly before their attachments
        power.reset(nullptr);
        depth.reset(nullptr);
    }

    void resized()
    {
        auto b = getLocalBounds().reduced(1, 1);

        power->setBounds(b.withWidth(b.getHeight()));
        b = b.translated(b.getHeight() + 2, 0);
        source->setBounds(b.withWidth(90));
        b = b.translated(92, 0);
        sourceVia->setBounds(b.withWidth(90));
        b = b.translated(92, 0);
        depth->setBounds(b.withWidth(170));
        b = b.translated(172, 0);
        curve->setBounds(b.withWidth(60));
        b = b.translated(62, 0);
        b = b.withWidth(getWidth() - b.getX());
        target->setBounds(b);
    }
    void refreshRow()
    {
        const auto &row = parent->routingTable[index];
        const auto &srcs = std::get<1>(parent->matrixMetadata);
        const auto &dsts = std::get<2>(parent->matrixMetadata);
        const auto &crvs = std::get<3>(parent->matrixMetadata);

        // this linear search kinda sucks bot
        for (const auto &[si, sn] : srcs)
        {
            if (si == row.src)
                source->setButtonText(sn);
            if (si == row.srcVia)
                sourceVia->setButtonText(sn);
        }

        for (const auto &[di, dn] : dsts)
        {
            if (di == row.dst)
                target->setButtonText(dn);
        }

        for (const auto &[ci, cn] : crvs)
        {
            if (ci == row.curve)
                curve->setButtonText(cn);
        }

        repaint();
    }

    void pushRowUpdate()
    {
        cmsg::clientSendToSerialization(
            cmsg::IndexedRoutingRowUpdated({index, parent->routingTable[index]}), editor->msgCont);
    }

    void showSourceMenu(bool isVia)
    {
        auto p = juce::PopupMenu();
        if (isVia)
            p.addSectionHeader("Source (Via)");
        else
            p.addSectionHeader("Source");
        p.addSeparator();
        const auto &srcs = std::get<1>(parent->matrixMetadata);

        // this linear search kinda sucks bot
        for (const auto &[si, sn] : srcs)
        {
            const auto &row = parent->routingTable[index];
            auto selected = isVia ? (row.srcVia == si) : (row.src == si);
            p.addItem(sn, true, selected,
                      [sidx = si, isVia, w = juce::Component::SafePointer(this)]() {
                          if (!w)
                              return;
                          auto &row = w->parent->routingTable[w->index];
                          if (isVia)
                              row.srcVia = sidx;
                          else
                              row.src = sidx;
                          w->pushRowUpdate();
                          w->refreshRow();
                      });
        }

        p.showMenuAsync({});
    }

    void showCurveMenu()
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Curve");
        p.addSeparator();
        const auto &srcs = std::get<3>(parent->matrixMetadata);

        // this linear search kinda sucks bot
        for (const auto &[si, sn] : srcs)
        {
            const auto &row = parent->routingTable[index];
            auto selected = row.curve == si;
            p.addItem(sn, true, selected, [sidx = si, w = juce::Component::SafePointer(this)]() {
                if (!w)
                    return;
                auto &row = w->parent->routingTable[w->index];

                row.curve = sidx;

                w->pushRowUpdate();
                w->refreshRow();
            });
        }

        p.showMenuAsync({});
    }

    void showTargetMenu()
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Target");
        p.addSeparator();
        const auto &srcs = std::get<2>(parent->matrixMetadata);

        // this linear search kinda sucks bot
        for (const auto &[si, sn] : srcs)
        {
            const auto &row = parent->routingTable[index];
            auto selected = row.dst == si;
            p.addItem(sn, true, selected, [sidx = si, w = juce::Component::SafePointer(this)]() {
                if (!w)
                    return;
                auto &row = w->parent->routingTable[w->index];

                row.dst = sidx;

                w->pushRowUpdate();
                w->refreshRow();
            });
        }

        p.showMenuAsync({});
    }
};

ModPane::ModPane(SCXTEditor *e) : sst::jucegui::components::NamedPanel(""), HasEditor(e)
{
    setCustomClass(connectors::SCXTStyleSheetCreator::ModulationTabs);

    hasHamburger = true;
    isTabbed = true;
    tabNames = {"MOD 1-6", "MOD 7-12"};

    resetTabState();

    onTabSelected = [wt = juce::Component::SafePointer(this)](int i) {
        if (!wt)
            return;

        wt->tabRange = i;
        wt->rebuildMatrix();
    };

    setActive(false);
}

ModPane::~ModPane() = default;

void ModPane::resized()
{
    auto en = isEnabled();
    if (!en)
        return;
    auto r = getContentArea().toFloat();
    auto rh = r.getHeight() * 1.f / numRowsOnScreen;
    r = r.withHeight(rh);
    for (int i = 0; i < numRowsOnScreen; ++i)
    {
        if (rows[i])
            rows[i]->setBounds(r.toNearestIntEdges());
        r = r.translated(0, rh);
    }
}

void ModPane::setActive(bool b)
{
    setEnabled(b);
    rebuildMatrix();
}

void ModPane::rebuildMatrix()
{
    auto en = isEnabled();
    removeAllChildren();
    for (auto &a : rows)
        a.reset(nullptr);
    if (!en)
        return;
    for (int i = 0; i < numRowsOnScreen; ++i)
    {
        rows[i] = std::make_unique<ModRow>(editor, i + tabRange * numRowsOnScreen, this);
        addAndMakeVisible(*(rows[i]));
    }
    resized();
}

void ModPane::refreshMatrix()
{
    assert(isEnabled());
    for (auto &r : rows)
    {
        if (r)
            r->refreshRow();
    }
}
} // namespace scxt::ui::multi
