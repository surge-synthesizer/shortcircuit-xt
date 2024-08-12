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

#include "ModPane.h"
#include "components/SCXTEditor.h"
#include "connectors/PayloadDataAttachment.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/components/HSliderFilled.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/components/Viewport.h"
#include "messaging/client/client_serial.h"

namespace scxt::ui::multi
{
namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;

template <typename GZTrait> struct ModRow : juce::Component, HasEditor
{
    int index{0};
    ModPane<GZTrait> *parent{nullptr};
    std::unique_ptr<connectors::BooleanPayloadDataAttachment<typename GZTrait::routing::Routing>>
        powerAttachment;
    using attachment_t = connectors::PayloadDataAttachment<typename GZTrait::routing::Routing>;
    std::unique_ptr<attachment_t> depthAttachment;
    std::unique_ptr<jcmp::ToggleButton> power;
    std::unique_ptr<jcmp::MenuButton> source, sourceVia, curve, target;
    std::unique_ptr<jcmp::GlyphPainter> x1, x2, a1, a2;
    std::unique_ptr<jcmp::TextPushButton> consistentButton;

    std::unique_ptr<jcmp::HSlider> depth;

    // std::unique_ptr<jcmp::HSlider> slider;
    ModRow(SCXTEditor *e, int i, ModPane<GZTrait> *p) : HasEditor(e), index(i), parent(p)
    {
        auto &row = parent->routingTable.routes[i];

        consistentButton = std::make_unique<jcmp::TextPushButton>();
        consistentButton->setLabel("Make Consistent");
        consistentButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
            {
                w->pushRowUpdate(true);
            }
        });
        addChildComponent(*consistentButton);

        powerAttachment = std::make_unique<
            connectors::BooleanPayloadDataAttachment<typename GZTrait::routing::Routing>>(
            "Power",
            [w = juce::Component::SafePointer(this)](const auto &a) {
                if (w)
                {
                    w->pushRowUpdate();
                }
            },
            row.active);
        power = std::make_unique<jcmp::ToggleButton>();
        power->setLabel(std::to_string(index + 1));
        power->setSource(powerAttachment.get());
        addAndMakeVisible(*power);

        source = std::make_unique<jcmp::MenuButton>();
        source->setLabel("Source");
        source->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showSourceMenu(false);
        });
        addAndMakeVisible(*source);

        sourceVia = std::make_unique<jcmp::MenuButton>();
        sourceVia->setLabel("Via");
        sourceVia->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showSourceMenu(true);
        });
        addAndMakeVisible(*sourceVia);

        // The generic value tooltip doesn't work for this and only this control
        depthAttachment = std::make_unique<attachment_t>(
            datamodel::pmd().asPercentBipolar().withName("Depth"),
            [w = juce::Component::SafePointer(this)](const auto &a) {
                if (w)
                {
                    w->pushRowUpdate();
                    w->updateTooltip(a);
                }
            },
            row.depth);
        depth = std::make_unique<jcmp::HSliderFilled>();
        depth->setSource(depthAttachment.get());
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
        depth->onIdleHover = depth->onBeginEdit;
        depth->onIdleHoverEnd = depth->onEndEdit;

        addAndMakeVisible(*depth);

        curve = std::make_unique<jcmp::MenuButton>();
        curve->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showCurveMenu();
        });
        addAndMakeVisible(*curve);

        target = std::make_unique<jcmp::MenuButton>();
        target->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showTargetMenu();
        });
        addAndMakeVisible(*target);

        auto mg = [this](auto g) {
            auto r = std::make_unique<jcmp::GlyphPainter>(g);
            addAndMakeVisible(*r);
            return r;
        };
        x1 = mg(jcmp::GlyphPainter::CLOSE);
        x2 = mg(jcmp::GlyphPainter::CLOSE);
        a1 = mg(jcmp::GlyphPainter::ARROW_L_TO_R);
        a2 = mg(jcmp::GlyphPainter::MODULATION_ADDITIVE);

        refreshRow();
    }

    ~ModRow() {}

    void resized()
    {
        auto b = getLocalBounds().reduced(1, 1);

        consistentButton->setBounds(b);

        auto map = [&b](const auto &wd, auto sz, int redH = 0) {
            wd->setBounds(b.withWidth(sz).reduced(0, redH));
            b = b.translated(sz + 2, 0);
        };

        auto sqr = [&b](const auto &wd, auto sz) {
            auto bx = b.withWidth(sz);
            if (bx.getHeight() > sz)
                bx = bx.reduced(0, (b.getHeight() - sz) * 0.5f);
            wd->setBounds(bx);
            b = b.translated(sz + 2, 0);
        };

        map(power, b.getHeight());
        map(source, 90);
        sqr(x1, 12);
        map(sourceVia, 90);
        sqr(x2, 12);
        map(depth, 120, 2);
        sqr(a1, 12);
        map(curve, 60);
        sqr(a2, 12);
        map(target, getWidth() - b.getX());
    }
    void refreshRow()
    {
        const auto &row = parent->routingTable.routes[index];
        const auto &srcs = std::get<1>(parent->matrixMetadata);
        const auto &dsts = std::get<2>(parent->matrixMetadata);
        const auto &crvs = std::get<3>(parent->matrixMetadata);

        auto sc = !row.extraPayload.has_value() || row.extraPayload->selConsistent;

        power->setVisible(sc);
        source->setVisible(sc);
        sourceVia->setVisible(sc);
        depth->setVisible(sc);
        curve->setVisible(sc);
        target->setVisible(sc);
        x1->setVisible(sc);
        x2->setVisible(sc);
        a1->setVisible(sc);
        a2->setVisible(sc);
        consistentButton->setVisible(!sc);
        if (!sc)
        {
            std::string s, sv, c, d;
            for (const auto &[si, sn] : srcs)
            {
                if (si == row.source)
                    s = sn.second;
                if (si == row.sourceVia)
                    sv = sn.second;
            }
            for (const auto &[ci, cn] : crvs)
            {
                if (ci == row.curve)
                    c = cn.second;
            }

            for (const auto &[di, dn] : dsts)
            {
                if (di == row.target)
                    d = dn.second;
            }

            auto lbl = s.empty() ? "off" : s;
            if (!sv.empty())
                lbl += " x " + sv;
            if (!c.empty())
                lbl += " through " + c;
            lbl += " to " + (d.empty() ? "off" : d);
            if (!row.active)
                lbl += " (inactive)";

            consistentButton->setLabel("Set selected zones to : " + lbl);
            return;
        }

        // this linear search kinda sucks bot
        source->setLabel("Source");
        sourceVia->setLabel("Via");
        target->setLabel("Target");
        curve->setLabel("-");

        auto makeSourceName = [](auto &si, auto &sn) {
            // This is the second location where we are assuming default macro name
            // as mentioned in part.h
            auto nm = sn.second;

            if (si.gid == 'zmac' || si.gid == 'gmac')
            {
                auto defname = "Macro " + std::to_string(si.index + 1);
                if (nm != defname)
                {
                    nm = "M" + std::to_string(si.index + 1) + ": " + nm;
                }
            }

            return nm;
        };

        for (const auto &[si, sn] : srcs)
        {
            if (si == row.source)
            {
                source->setLabel(makeSourceName(si, sn));
            }
            if (si == row.sourceVia)
            {
                sourceVia->setLabel(makeSourceName(si, sn));
            }
        }

        for (const auto &[di, dn] : dsts)
        {
            if (di == row.target)
            {
                target->setLabel(dn.first + " - " + dn.second);
            }
        }

        for (const auto &[ci, cn] : crvs)
        {
            if (ci == row.curve)
            {
                curve->setLabel(cn.second);
            }
        }

        if (row.target.has_value())
        {
            if (GZTrait::isMultiplicative(*(row.target)))
            {
                a2->glyph = sst::jucegui::components::GlyphPainter::MODULATION_MULTIPLICATIVE;
            }
            else
            {
                a2->glyph = sst::jucegui::components::GlyphPainter::MODULATION_ADDITIVE;
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

        auto lineOne = sl + " " + u8"\U00002192" + " " + tl;

        auto &epo = parent->routingTable.routes[index].extraPayload;
        if (!epo.has_value())
        {
            editor->setTooltipContents(
                lineOne, {at.description.valueToString(at.value).value_or("Error"), "No Metadata"});

            return;
        }
        auto ep = *epo;
        datamodel::pmd &md = ep.targetMetadata;

        auto v = md.modulationNaturalToString(ep.targetBaseValue,
                                              at.value * (md.maxVal - md.minVal), false);

        std::string modLineOne{}, modLineTwo{};

        if (v.has_value())
        {
            modLineOne = v->singleLineModulationSummary;
            modLineTwo =
                fmt::format("depth={:.2f}%, {}={}", at.value * 100, u8"\U00000394", v->changeUp);
        }
        editor->setTooltipContents(lineOne, {modLineOne, modLineTwo});
    }

    void pushRowUpdate(bool forceUpdate = false)
    {
        if constexpr (GZTrait::forZone)
        {
            sendToSerialization(cmsg::IndexedZoneRoutingRowUpdated(
                {index, parent->routingTable.routes[index], forceUpdate}));
        }
        else
        {
            sendToSerialization(cmsg::IndexedGroupRoutingRowUpdated(
                {index, parent->routingTable.routes[index], forceUpdate}));
        }
    }

    void showSourceMenu(bool isVia)
    {
        auto p = juce::PopupMenu();
        if (isVia)
            p.addSectionHeader("Source (Via)");
        else
            p.addSectionHeader("Source");
        p.addSeparator();

        p.addItem("Off", true, false, [isVia, w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            auto &row = w->parent->routingTable.routes[w->index];

            if (isVia)
                row.sourceVia = std::nullopt;
            else
                row.source = std::nullopt;

            w->pushRowUpdate();
            w->refreshRow();
        });

        const auto &srcs = std::get<1>(parent->matrixMetadata);

        // First add all the sources without a name
        auto mkCallback = [isVia, w = juce::Component::SafePointer(this)](auto sidx) {
            return [isVia, w, sidx]() {
                if (!w)
                    return;
                auto &row = w->parent->routingTable.routes[w->index];
                if (isVia)
                    row.sourceVia = sidx;
                else
                    row.source = sidx;
                w->pushRowUpdate();
                w->refreshRow();
            };
        };
        bool hasCat{false};
        for (const auto &[si, sn] : srcs)
        {
            if (!sn.first.empty())
            {
                hasCat = true;
                continue;
            }

            const auto &row = parent->routingTable.routes[index];
            auto selected = isVia ? (row.sourceVia == si) : (row.source == si);

            auto nm = sn.second;
            p.addItem(nm, true, selected, mkCallback(si));
        }

        if (hasCat)
            p.addSeparator();

        auto sub = juce::PopupMenu();
        auto subTicked{false};
        std::string lastCat{};
        for (const auto &[si, sn] : srcs)
        {
            if (sn.first.empty())
                continue;

            if (lastCat != sn.first)
            {
                if (sub.getNumItems() > 0)
                {
                    p.addSubMenu(lastCat, sub, true, nullptr, subTicked);
                }
                sub = juce::PopupMenu();
                lastCat = sn.first;
                subTicked = false;
            }

            const auto &row = parent->routingTable.routes[index];
            auto selected = isVia ? (row.sourceVia == si) : (row.source == si);
            if (selected)
                subTicked = true;

            auto nm = sn.second;
            if (si.gid == 'gmac' || si.gid == 'zmac')
            {
                // This is where we are assuming default macro name
                // from part.h
                auto defName = "Macro " + std::to_string(si.index + 1);
                if (nm != defName)
                {
                    nm = defName + " (" + nm + ")";
                }
            }
            sub.addItem(nm, true, selected, mkCallback(si));
        }

        if (sub.getNumItems() > 0)
        {
            p.addSubMenu(lastCat, sub, true, nullptr, subTicked);
        }

        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }

    void showCurveMenu()
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Curve");
        p.addSeparator();

        const auto &row = parent->routingTable.routes[index];

        p.addItem("-", true, !row.curve.has_value(), [w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            auto &row = w->parent->routingTable.routes[w->index];

            row.curve = std::nullopt;

            w->pushRowUpdate();
            w->refreshRow();
        });

        const auto &srcs = std::get<3>(parent->matrixMetadata);

        // First add all the sources without a name
        auto mkCallback = [w = juce::Component::SafePointer(this)](auto sidx) {
            return [w, sidx]() {
                if (!w)
                    return;
                auto &row = w->parent->routingTable.routes[w->index];

                row.curve = sidx;
                w->pushRowUpdate();
                w->refreshRow();
            };
        };
        bool hasCat{false};
        for (const auto &[si, sn] : srcs)
        {
            if (!sn.first.empty())
            {
                hasCat = true;
                continue;
            }

            const auto &row = parent->routingTable.routes[index];
            auto selected = row.curve == si;

            auto nm = sn.second;
            p.addItem(nm, true, selected, mkCallback(si));
        }

        if (hasCat)
            p.addSeparator();

        auto sub = juce::PopupMenu();
        auto subTicked{false};
        std::string lastCat{};
        for (const auto &[si, sn] : srcs)
        {
            if (sn.first.empty())
                continue;

            if (lastCat != sn.first)
            {
                if (sub.getNumItems() > 0)
                {
                    p.addSubMenu(lastCat, sub, true, nullptr, subTicked);
                }
                sub = juce::PopupMenu();
                lastCat = sn.first;
                subTicked = false;
            }

            const auto &row = parent->routingTable.routes[index];
            auto selected = row.curve == si;
            if (selected)
                subTicked = true;

            auto nm = sn.second;
            sub.addItem(nm, true, selected, mkCallback(si));
        }

        if (sub.getNumItems() > 0)
        {
            p.addSubMenu(lastCat, sub, true, nullptr, subTicked);
        }

        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }

    void showTargetMenu()
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Target");
        p.addSeparator();
        p.addItem("Off", true, false, [w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            auto &row = w->parent->routingTable.routes[w->index];

            row.target = std::nullopt;
            w->pushRowUpdate(true);
            w->refreshRow();
        });

        const auto &tgts = std::get<2>(parent->matrixMetadata);

        std::string lastPath{};
        bool checkPath{false};
        juce::PopupMenu subMenu;

        for (const auto &[ti, tn] : tgts)
        {
            if (tn.second.empty())
                continue;

            const auto &row = parent->routingTable.routes[index];
            auto selected = (row.target == ti);

            auto mop = [tidx = ti, w = juce::Component::SafePointer(this)]() {
                if (!w)
                    return;
                auto &row = w->parent->routingTable.routes[w->index];

                row.target = tidx;
                w->pushRowUpdate(true);
                w->refreshRow();
            };

            if (tn.first.empty())
            {
                p.addItem(tn.first + " - " + tn.second, true, selected, mop);
            }
            else
            {
                if (tn.first != lastPath)
                {
                    if (subMenu.getNumItems())
                    {
                        p.addSubMenu(lastPath, subMenu, true, nullptr, checkPath);
                    }
                    lastPath = tn.first;
                    checkPath = false;
                    subMenu = juce::PopupMenu();
                    subMenu.addSectionHeader(tn.first);
                    subMenu.addSeparator();
                }
                subMenu.addItem(tn.first + " - " + tn.second, true, selected, mop);
                checkPath = checkPath || selected;
            }
        }
        if (subMenu.getNumItems())
        {
            p.addSubMenu(lastPath, subMenu, true, nullptr, checkPath);
        }

        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }
};

template <typename GZTrait>
ModPane<GZTrait>::ModPane(SCXTEditor *e, bool fz) : jcmp::NamedPanel(""), HasEditor(e), forZone(fz)
{
    viewPort = std::make_unique<sst::jucegui::components::Viewport>();
    viewPortComponents = std::make_unique<juce::Component>();
    viewPort->setViewedComponent(viewPortComponents.get(), false);

    // getContentAreaComponent()->addAndMakeVisible(*viewPort);
    addAndMakeVisible(*viewPort);

    hasHamburger = true;
    if (GZTrait::forZone)
    {
        setName("MOD MATRIX");
    }
    else
    {
        setName("GROUP MOD MATRIX");
    }

    setActive(false);
}

template <typename GZTrait> ModPane<GZTrait>::~ModPane() = default;

template <typename GZTrait> void ModPane<GZTrait>::resized()
{
    auto en = isEnabled();
    if (!en)
        return;
    auto vp = viewPort->getVerticalScrollBar().getCurrentRangeStart();

    auto r = getContentArea();

    auto rowHeight = r.getHeight() / 6;
    auto newBounds = juce::Rectangle<int>(
        0, 0, r.getWidth() - viewPort->getScrollBarThickness() - 2, rowHeight * GZTrait::rowCount);
    if (viewPortComponents->getBounds() != newBounds)
        viewPortComponents->setBounds(newBounds);
    if (viewPort->getBounds() != r)
        viewPort->setBounds(r);

    auto r0 = juce::Rectangle<int>(0, 0, viewPortComponents->getWidth(), rowHeight - 1);
    for (auto &row : rows)
    {
        if (row && row->getBounds() != r0)
        {
            row->setBounds(r0);
        }
        r0 = r0.translated(0, rowHeight);
    }

    viewPort->getVerticalScrollBar().setCurrentRangeStart(vp);
}

template <typename GZTrait> void ModPane<GZTrait>::setActive(bool b)
{
    auto enChange = b != isEnabled();
    setEnabled(b);
    rebuildMatrix(enChange);
    if (enChange)
        resized();
}

template <typename GZTrait> void ModPane<GZTrait>::rebuildMatrix(bool enableChanged)
{
    auto en = isEnabled();

    int idx{0};
    bool needsResize{false};
    for (auto &a : rows)
    {
        auto ioc = !a ? -1 : viewPortComponents->getIndexOfChildComponent(a.get());
        if (enableChanged && ioc >= 0)
        {
            viewPortComponents->removeChildComponent(ioc);
        }
        if (en)
        {
            if (!a || enableChanged)
            {
                needsResize = true;
                a = std::make_unique<ModRow<GZTrait>>(editor, idx, this);
                viewPortComponents->addAndMakeVisible(*a);
            }
            else
            {
                a->refreshRow();
            }
        }
        idx++;
    }
    if (enableChanged || needsResize)
        resized();

    if (GZTrait::forZone)
    {
        editor->themeApplier.applyZoneMultiScreenModulationTheme(this);
    }
    else
    {
        editor->themeApplier.applyGroupMultiScreenModulationTheme(this);
    }
}

template <typename GZTrait> void ModPane<GZTrait>::refreshMatrix()
{
    assert(isEnabled());
    for (auto &r : rows)
    {
        if (r)
            r->refreshRow();
    }
}

template struct ModPane<ModPaneZoneTraits>;
template struct ModPane<ModPaneGroupTraits>;
} // namespace scxt::ui::multi
