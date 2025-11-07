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

#include "ModPane.h"
#include "app/SCXTEditor.h"
#include "connectors/PayloadDataAttachment.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/components/HSliderFilled.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/components/Viewport.h"
#include "sst/jucegui/component-adapters/ThrowRescaler.h"
#include "messaging/client/client_serial.h"
#include "app/shared/MenuValueTypein.h"

namespace scxt::ui::app::edit_screen
{
namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;
namespace jcad = sst::jucegui::component_adapters;

template <typename GZTrait> struct ModRow : juce::Component, HasEditor
{
    int index{0};
    ModPane<GZTrait> *parent{nullptr};
    std::unique_ptr<connectors::BooleanPayloadDataAttachment<typename GZTrait::routing::Routing>>
        powerAttachment;
    using attachment_t = connectors::PayloadDataAttachment<typename GZTrait::routing::Routing>;
    std::unique_ptr<attachment_t> depthAttachment;

    std::unique_ptr<jcad::CubicThrowRescaler<attachment_t>> depthRescaler;
    std::unique_ptr<jcmp::ToggleButton> power;
    std::unique_ptr<jcmp::MenuButton> source, sourceVia, curve, target;
    std::unique_ptr<jcmp::GlyphPainter> x1, x2, a1, a2;
    std::unique_ptr<jcmp::TextPushButton> consistentButton;

    std::unique_ptr<jcmp::HSliderFilled> depth;

    bool allowsMultiplicative{false};

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
        source->setLabel("SOURCE");
        source->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showSourceMenu(false);
        });
        addAndMakeVisible(*source);

        sourceVia = std::make_unique<jcmp::MenuButton>();
        sourceVia->setLabel("VIA");
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
        depthRescaler =
            std::make_unique<jcad::CubicThrowRescaler<attachment_t>>(depthAttachment.get());
        depth = std::make_unique<jcmp::HSliderFilled>();
        depth->verticalReduction = 3;

        depth->setSource(depthRescaler.get());
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
        depth->onWheelEditOccurred = [w = juce::Component::SafePointer(depth.get())]() {
            if (w)
                w->immediatelyInitiateIdleAction(1000);
        };
        depth->onPopupMenu = [w = juce::Component::SafePointer(this)](auto mods) {
            if (!w)
                return;
            w->editor->hideTooltip();
            w->showDepthPopup();
        };

        addAndMakeVisible(*depth);

        curve = std::make_unique<jcmp::MenuButton>();
        curve->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showCurveMenu();
        });
        addAndMakeVisible(*curve);
        curve->centerTextAndExcludeArrow = true;

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
        map(depth, 120, 0);
        depth->verticalReduction = 3; // retain the hit zone just shrink the paint
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

            for (const auto &[di, dn, ca, isen] : dsts)
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
        source->setLabel("SOURCE");
        sourceVia->setLabel("VIA");
        target->setLabel("TARGET");
        curve->setLabel("-");

        auto makeSourceName = [](auto &si, auto &sn) {
            auto nm = sn.second;

            if (si.gid == 'zmac' || si.gid == 'gmac')
            {
                if (nm != scxt::engine::Macro::defaultNameFor(si.index))
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

        for (const auto &[di, dn, ca, en] : dsts)
        {
            if (di == row.target)
            {
                target->setLabel(dn.first + ": " + dn.second);
                target->paintLabelNonEnabled = !en;
                allowsMultiplicative = ca;
            }
        }

        for (const auto &[ci, cn] : crvs)
        {
            if (ci == row.curve)
            {
                curve->setLabel(cn.second);
            }
        }

        if (allowsMultiplicative &&
            row.applicationMode == sst::basic_blocks::mod_matrix::ApplicationMode::MULTIPLICATIVE)
        {
            a2->glyph = sst::jucegui::components::GlyphPainter::MODULATION_MULTIPLICATIVE;
        }
        else
        {
            a2->glyph = sst::jucegui::components::GlyphPainter::MODULATION_ADDITIVE;
        }

        repaint();
    }

    void updateTooltip(const attachment_t &at)
    {
        // TODO: This should be in modulation units of the target not in percentage depth
        auto sl = source->getLabel();
        auto vl = sourceVia->getLabel();
        auto tl = target->getLabel();

        if (sl == "SOURCE")
            sl = "NO SOURCE";
        if (vl == "VIA")
        {
            vl = " ";
        }
        else
        {
            vl = " " + std::string(u8"\U000000D7") + " " + vl;
        }
        sl += vl;

        std::vector<jcmp::ToolTip::Row> rows;
        auto lineOne = sl + " " + u8"\U00002192" + " " + tl;
        rows.push_back(jcmp::ToolTip::Row(lineOne));

        auto &epo = parent->routingTable.routes[index].extraPayload;
        if (!epo.has_value())
        {
            editor->setTooltipContents(
                lineOne, {at.description.valueToString(at.value).value_or("Error"), "No Metadata"});

            return;
        }
        auto ep = *epo;
        datamodel::pmd &md = ep.targetMetadata;

        bool isSourceBipolar{false}; // fixme - we shoudl determine this one day
        auto v = md.modulationNaturalToString(ep.targetBaseValue,
                                              at.value * (md.maxVal - md.minVal), isSourceBipolar);

        auto rDepth = jcmp::ToolTip::Row();
        auto rDelta = jcmp::ToolTip::Row();
        rDelta.drawLRArrow = true;
        if (isSourceBipolar)
            rDelta.drawRLArrow = true;

        if (v.has_value())
        {
            if (isSourceBipolar)
            {
                rDelta.rowLeadingGlyph = jcmp::GlyphPainter::GlyphType::LEFT_RIGHT;
                rDelta.leftAlignText = v->valDown;
                rDelta.centerAlignText = v->baseValue;
                rDelta.rightAlignText = v->valUp;
            }
            else
            {
                rDelta.rowLeadingGlyph = jcmp::GlyphPainter::GlyphType::LEFT_RIGHT;
                rDelta.leftAlignText = v->baseValue;
                rDelta.rightAlignText = v->valUp;
            }
            rDepth.rowLeadingGlyph = jcmp::GlyphPainter::GlyphType::VOLUME;
            // rDepth.leftAlignText = fmt::format("{} ({:.2f} %)", v->value, at.value * 100);
            rDepth.leftAlignText = fmt::format("{}", v->value);
        }
        editor->setTooltipContents(lineOne, {rDepth, rDelta});
    }

    void pushRowUpdate(bool forceUpdate = false)
    {
        if constexpr (GZTrait::forZone)
        {
            sendToSerialization(cmsg::UpdateZoneRoutingRow(
                {index, parent->routingTable.routes[index], forceUpdate}));
        }
        else
        {
            sendToSerialization(cmsg::UpdateGroupRoutingRow(
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

                auto defaultLagForSource = GZTrait::defaultSmoothingTimeFor(sidx);
                if (isVia)
                {
                    row.sourceVia = sidx;
                    row.sourceViaLagExp = true;
                    row.sourceViaLagMS = defaultLagForSource;
                }
                else
                {
                    row.source = sidx;
                    row.sourceLagExp = true;
                    row.sourceLagMS = defaultLagForSource;
                }
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
        std::string sourceName{""};
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
                if (nm != scxt::engine::Macro::defaultNameFor(si.index))
                {
                    nm = scxt::engine::Macro::defaultNameFor(si.index) + " (" + nm + ")";
                }
            }
            sub.addItem(nm, true, selected, mkCallback(si));
            if (selected)
                sourceName = nm;
        }

        if (sub.getNumItems() > 0)
        {
            p.addSubMenu(lastCat, sub, true, nullptr, subTicked);
        }

        const auto &row = parent->routingTable.routes[index];

        bool smoothingAvailable = true;
        if (isVia)
            smoothingAvailable = row.sourceVia.has_value();
        else
            smoothingAvailable = row.source.has_value();

        if (smoothingAvailable)
        {
            p.addSeparator();
            std::string lab = "Smoothing ";
            auto st = row.sourceLagMS;
            auto se = row.sourceLagExp;

            if (isVia)
            {
                st = row.sourceViaLagMS;
                se = row.sourceViaLagExp;
            }

            if (st == 0)
            {
                lab += "(Off)";
            }
            else
            {
                lab += "(" + std::to_string(st) + "ms, " + (se ? "Exp" : "Lin") + ")";
            }

            auto smoothSubMenu = juce::PopupMenu();
            smoothSubMenu.addSectionHeader(sourceName + " Smoothing");
            smoothSubMenu.addSeparator();
            for (auto vals : {0, 10, 25, 50, 100, 250, 500})
            {
                smoothSubMenu.addItem(vals == 0 ? "No Smoothing" : (std::to_string(vals) + "ms"),
                                      true, vals == st,
                                      [isVia, w = juce::Component::SafePointer(this), vals]() {
                                          if (!w)
                                              return;
                                          auto &row = w->parent->routingTable.routes[w->index];
                                          if (isVia)
                                              row.sourceViaLagMS = vals;
                                          else
                                              row.sourceLagMS = vals;
                                          w->pushRowUpdate();
                                          w->refreshRow();
                                      });
            }
            smoothSubMenu.addSeparator();
            smoothSubMenu.addItem("Exponential", true, se,
                                  [isVia, w = juce::Component::SafePointer(this)]() {
                                      if (!w)
                                          return;
                                      auto &row = w->parent->routingTable.routes[w->index];
                                      if (isVia)
                                          row.sourceViaLagExp = true;
                                      else
                                          row.sourceLagExp = true;
                                      w->pushRowUpdate();
                                      w->refreshRow();
                                  });
            smoothSubMenu.addItem("Linear", true, !se,
                                  [isVia, w = juce::Component::SafePointer(this)]() {
                                      if (!w)
                                          return;
                                      auto &row = w->parent->routingTable.routes[w->index];
                                      if (isVia)
                                          row.sourceViaLagExp = false;
                                      else
                                          row.sourceLagExp = false;
                                      w->pushRowUpdate();
                                      w->refreshRow();
                                  });

            p.addSubMenu(lab, smoothSubMenu);
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
            row.applicationMode = sst::basic_blocks::mod_matrix::ApplicationMode::ADDITIVE;
            w->pushRowUpdate(true);
            w->refreshRow();
        });

        const auto &tgts = std::get<2>(parent->matrixMetadata);

        std::string lastPath{};
        bool checkPath{false};
        juce::PopupMenu subMenu;

        for (const auto &[ti, tn, canAdditive, isEnabled] : tgts)
        {
            if (tn.second.empty())
                continue;

            const auto &row = parent->routingTable.routes[index];
            auto selected = (row.target == ti);

            auto mop = [tidx = ti, ca = canAdditive, w = juce::Component::SafePointer(this)]() {
                if (!w)
                    return;
                auto &row = w->parent->routingTable.routes[w->index];

                row.target = tidx;

                w->allowsMultiplicative = ca;

                row.applicationMode =
                    ca ? sst::basic_blocks::mod_matrix::ApplicationMode::MULTIPLICATIVE
                       : sst::basic_blocks::mod_matrix::ApplicationMode::ADDITIVE;
                w->pushRowUpdate(true);
            };

            if (tn.first.empty())
            {
                p.addItem(tn.second, isEnabled, selected, mop);
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
                subMenu.addItem(tn.second, isEnabled, selected, mop);
                checkPath = checkPath || selected;
            }
        }
        if (subMenu.getNumItems())
        {
            p.addSubMenu(lastPath, subMenu, true, nullptr, checkPath);
        }

        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }

    struct ModDepthTypein : shared::MenuValueTypeinBase
    {
        ModRow *row{nullptr};
        ModDepthTypein(SCXTEditor *e, ModRow *r) : row(r), shared::MenuValueTypeinBase(e) {}

        std::string getInitialText() const override
        {
            auto dv = row->depthAttachment->getValue();
            auto ep = row->parent->routingTable.routes[row->index].extraPayload;
            if (!ep.has_value())
                return "Error";
            auto md = ep->targetMetadata;

            auto v = md.modulationNaturalToString(
                ep->targetBaseValue, row->depthAttachment->value * (md.maxVal - md.minVal), false);

            return v->value;
        }
        void setValueString(const std::string &s) override
        {
            auto dv = row->depthAttachment->getValue();
            auto ep = row->parent->routingTable.routes[row->index].extraPayload;
            if (!ep.has_value())
                return;
            auto md = ep->targetMetadata;

            std::string emsg;
            auto v = md.modulationNaturalFromString(s, ep->targetBaseValue, emsg);

            if (!v.has_value())
            {
                SCLOG(emsg);
                editor->displayError(
                    "Unable to convert modulation type-in",
                    "Modulation type in '" + s + "' did not convert to a valid moduation " +
                        "from base value " + std::to_string(ep->targetBaseValue) +
                        ". Please report " + "this to developers. Internal error is [" + emsg +
                        "] and param is [" + md.name + "]");
                v = 0;
            }

            row->depthAttachment->setValueFromGUI(*v / (md.maxVal - md.minVal));
            row->repaint();
            return;
        }
        juce::Colour getValueColour() const override
        {
            return editor->themeColor(theme::ColorMap::accent_2b);
        }
    };
    void showDepthPopup()
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader(target->getLabel());

        p.addSeparator();
        p.addCustomItem(-1, std::make_unique<ModDepthTypein>(editor, this));

        auto &route = parent->routingTable.routes[index];
        if (route.target.has_value() && allowsMultiplicative)
        {
            p.addSeparator();

            p.addItem("Additive Application", true,
                      route.applicationMode ==
                          sst::basic_blocks::mod_matrix::ApplicationMode::ADDITIVE,
                      [w = juce::Component::SafePointer(this)]() {
                          if (!w)
                              return;
                          auto &route = w->parent->routingTable.routes[w->index];
                          route.applicationMode =
                              sst::basic_blocks::mod_matrix::ApplicationMode::ADDITIVE;

                          w->pushRowUpdate(true);
                          w->refreshRow();
                      });
            p.addItem("Multiplicative Application", true,
                      route.applicationMode ==
                          sst::basic_blocks::mod_matrix::ApplicationMode::MULTIPLICATIVE,
                      [w = juce::Component::SafePointer(this)]() {
                          if (!w)
                              return;
                          auto &route = w->parent->routingTable.routes[w->index];
                          route.applicationMode =
                              sst::basic_blocks::mod_matrix::ApplicationMode::MULTIPLICATIVE;

                          w->pushRowUpdate(true);
                          w->refreshRow();
                      });
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

    hasHamburger = false;
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
} // namespace scxt::ui::app::edit_screen
