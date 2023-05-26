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
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/components/HSliderFilled.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "messaging/client/client_serial.h"

namespace scxt::ui::multi
{
namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;

struct ModRow : juce::Component, HasEditor
{
    int index{0};
    ModPane *parent{nullptr};
    std::unique_ptr<
        connectors::BooleanPayloadDataAttachment<ModRow, modulation::VoiceModMatrix::Routing>>
        powerAttachment;
    using attachment_t =
        connectors::PayloadDataAttachment<ModRow, modulation::VoiceModMatrix::Routing>;
    std::unique_ptr<attachment_t> depthAttachment;
    std::unique_ptr<jcmp::ToggleButton> power;
    std::unique_ptr<jcmp::MenuButton> source, sourceVia, curve, target;
    std::unique_ptr<juce::Component> x1, x2, a1, a2;

    std::unique_ptr<jcmp::HSlider> depth;

    // std::unique_ptr<jcmp::HSlider> slider;
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
        power = std::make_unique<jcmp::ToggleButton>();
        power->setLabel(std::to_string(index + 1));
        power->setSource(powerAttachment.get());
        power->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationMatrixToggle);
        addAndMakeVisible(*power);

        source = std::make_unique<jcmp::MenuButton>();
        source->setLabel("Source");
        source->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showSourceMenu(false);
        });
        source->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationMatrixMenu);
        addAndMakeVisible(*source);

        sourceVia = std::make_unique<jcmp::MenuButton>();
        sourceVia->setLabel("Via");
        sourceVia->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showSourceMenu(true);
        });
        sourceVia->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationMatrixMenu);
        addAndMakeVisible(*sourceVia);

        depthAttachment = std::make_unique<attachment_t>(
            this, datamodel::pmd().asPercentBipolar().withName("Depth"),
            [w = juce::Component::SafePointer(this)](const auto &a) {
                if (w)
                {
                    w->pushRowUpdate();
                    w->updateTooltip(a);
                }
            },
            [](const auto &pl) { return pl.depth; }, row.depth);
        depth = std::make_unique<jcmp::HSliderFilled>();
        depth->setSource(depthAttachment.get());
        depth->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationEditorVSlider);
        depth->onBeginEdit = [w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            w->editor->showTooltip(*(w->depth));
            w->updateTooltip(*(w->depthAttachment));
        };
        depth->onEndEdit = [w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            w->editor->hideTooltip();
        };

        addAndMakeVisible(*depth);

        curve = std::make_unique<jcmp::MenuButton>();
        curve->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showCurveMenu();
        });
        curve->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationMatrixMenu);
        addAndMakeVisible(*curve);

        target = std::make_unique<jcmp::MenuButton>();
        target->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showTargetMenu();
        });
        target->setCustomClass(connectors::SCXTStyleSheetCreator::ModulationMatrixMenu);
        addAndMakeVisible(*target);

        refreshRow();

        auto mg = [this](auto g) {
            auto r = std::make_unique<jcmp::GlyphPainter>(g);
            r->setCustomClass(connectors::SCXTStyleSheetCreator::InformationLabel);
            addAndMakeVisible(*r);
            return r;
        };
        x1 = mg(jcmp::GlyphPainter::CROSS);
        x2 = mg(jcmp::GlyphPainter::CROSS);
        a1 = mg(jcmp::GlyphPainter::ARROW_L_TO_R);
        a2 = mg(jcmp::GlyphPainter::ARROW_L_TO_R);
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

        auto map = [&b](const auto &wd, auto sz, int redH = 0) {
            wd->setBounds(b.withWidth(sz).reduced(0, redH));
            b = b.translated(sz + 2, 0);
        };

        map(power, b.getHeight());
        map(source, 90);
        map(x1, 6);
        map(sourceVia, 90);
        map(x2, 6);
        map(depth, 120, 2);
        map(a1, 10);
        map(curve, 60);
        map(a2, 10);
        map(target, getWidth() - b.getX());
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
            {
                if (sn.empty())
                {
                    source->setIsInactiveValue(true);
                    source->setLabel("Source");
                }
                else
                {
                    source->setIsInactiveValue(false);
                    source->setLabel(sn);
                }
            }
            if (si == row.srcVia)
            {
                if (sn.empty())
                {
                    sourceVia->setIsInactiveValue(true);
                    sourceVia->setLabel("Via");
                }
                else
                {
                    sourceVia->setIsInactiveValue(false);
                    sourceVia->setLabel(sn);
                }
            }
        }

        for (const auto &[di, dn] : dsts)
        {
            if (di == row.dst)
            {
                if (dn.empty())
                {
                    target->setIsInactiveValue(true);
                    target->setLabel("Target");
                }
                else
                {
                    target->setIsInactiveValue(false);
                    target->setLabel(dn);
                }
            }
        }

        for (const auto &[ci, cn] : crvs)
        {
            if (ci == row.curve)
            {
                if (cn.empty())
                {
                    curve->setIsInactiveValue(true);
                    curve->setLabel("Curve");
                }
                else
                {
                    curve->setIsInactiveValue(false);
                    curve->setLabel(cn);
                }
            }
        }

        repaint();
    }

    void updateTooltip(const attachment_t &at)
    {
        // TODO: This should be in modulation units of the target not in percentage depth
        auto sl = source->getLabel();
        auto vl = sourceVia->getLabel();
        auto tl = target->getLabel();

        if (sl == "Source")
            sl = "Unmapped";
        if (vl == "Via")
        {
            vl = "";
        }
        else
        {
            vl = " x " + vl;
        }
        sl += vl;

        editor->setTooltipContents(sl + " " + u8"\U00002192" + " " + tl,
                                   at.description.valueToString(at.value).value_or("Error"));
    }

    void pushRowUpdate()
    {
        sendToSerialization(cmsg::IndexedRoutingRowUpdated({index, parent->routingTable[index]}));
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

            p.addItem(sn.empty() ? "Off" : sn, true, selected,
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

        p.showMenuAsync(editor->defaultPopupMenuOptions());
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
            p.addItem(sn.empty() ? "Off" : sn, true, selected,
                      [sidx = si, w = juce::Component::SafePointer(this)]() {
                          if (!w)
                              return;
                          auto &row = w->parent->routingTable[w->index];

                          row.curve = sidx;

                          w->pushRowUpdate();
                          w->refreshRow();
                      });
        }

        p.showMenuAsync(editor->defaultPopupMenuOptions());
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
            p.addItem(sn.empty() ? "None" : sn, true, selected,
                      [sidx = si, w = juce::Component::SafePointer(this)]() {
                          if (!w)
                              return;
                          auto &row = w->parent->routingTable[w->index];

                          row.dst = sidx;

                          w->pushRowUpdate();
                          w->refreshRow();
                      });
        }

        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }
};

ModPane::ModPane(SCXTEditor *e) : jcmp::NamedPanel(""), HasEditor(e)
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
            rows[i]->setBounds(r.toNearestIntEdges().withTrimmedBottom(1));
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
