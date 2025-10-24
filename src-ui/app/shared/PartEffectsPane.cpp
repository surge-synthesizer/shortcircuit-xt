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

#include "PartEffectsPane.h"
#include "app/SCXTEditor.h"
#include "app/mixer-screen/MixerScreen.h"
#include "app/edit-screen/components/PartEditScreen.h"

#include "connectors/JSONAssetSupport.h"
#include "connectors/JsonLayoutEngineSupport.h"

#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/RuledLabel.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/JogUpDownButton.h"
#include "sst/jucegui/layouts/ExplicitLayout.h"

namespace scxt::ui::app::shared
{
namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;

template <bool forBus>
void PartEffectsPane<forBus>::showFXSelectionMenu(parent_t *parent, int ba, int sl)
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Effects");
    p.addSeparator();
    auto go = [w = juce::Component::SafePointer(parent), ba, sl](auto q) {
        return [w, q, ba, sl]() {
            if (w)
            {
                w->setFXSlotToType(ba, sl, q);
            }
        };
    };
    auto add = [&go, &p](auto q) {
        p.addItem(shared::PartEffectsPane<true>::effectDisplayName(q, true), go(q));
    };
    add(engine::AvailableBusEffects::none);
    add(engine::AvailableBusEffects::reverb1);
    add(engine::AvailableBusEffects::reverb2);
    add(engine::AvailableBusEffects::delay);
    add(engine::AvailableBusEffects::floatydelay);
    add(engine::AvailableBusEffects::flanger);
    add(engine::AvailableBusEffects::phaser);
    add(engine::AvailableBusEffects::treemonster);
    add(engine::AvailableBusEffects::nimbus);
    add(engine::AvailableBusEffects::bonsai);
    p.showMenuAsync(parent->editor->defaultPopupMenuOptions());
}

template <bool forBus> PartEffectsPane<forBus>::~PartEffectsPane()
{
    removeAllChildren();
    // order matters here
    components.clear();
    floatAttachments.clear();
}

template <bool forBus>
void PartEffectsPane<forBus>::paintMetadata(juce::Graphics &g, const juce::Rectangle<int> &into)
{
    auto &b = getPartFXStorage();

    auto r = into.withHeight(20);
    int idx{0};
    for (const auto &p : b.first)
    {
        if (p.type != sst::basic_blocks::params::ParamMetaData::NONE)
        {
            g.setColour(juce::Colours::white);
            g.drawText(p.name, r, juce::Justification::left);

            auto s = p.valueToString(b.second.params[idx]);
            if (s.has_value())
                g.drawText(*s, r, juce::Justification::right);
        }
        r = r.translated(0, 20);
        idx++;
    }
}

template <bool forBus>
typename PartEffectsPane<forBus>::partFXStorage_t &PartEffectsPane<forBus>::getPartFXStorage()
{
    if (busAddressOrPart < 0 || fxSlot < 0)
    {
        // once you find this fix those maxes below
        SCLOG_ONCE("Investigate: PartEffectsPane<"
                   << forBus << "> busAddress or fxSlot misconfigured " << busAddressOrPart << " "
                   << fxSlot << " - will use zero");
    }
    if constexpr (forBus)
    {
        return parent->busEffectsData[std::max(busAddressOrPart, 0)][std::max(fxSlot, 0)];
    }
    else
    {
        return parent->partsEffectsData[std::max(busAddressOrPart, 0)][std::max(fxSlot, 0)];
    }
}

template <bool forBus> void PartEffectsPane<forBus>::rebuild()
{
    auto &b = getPartFXStorage();
    auto &params = b.first;
    auto &data = b.second;
    auto t = data.type;
    name = effectDisplayName(t, false);

    removeAllChildren();
    components.clear();
    // intAttachments.clear();
    // floatAttachments.clear();

    if (getHeight() < 10)
        return;

#define CS(x)                                                                                      \
    case engine::AvailableBusEffects::x:                                                           \
        rebuildFromJSONLibrary("bus-effects/" #x ".json");                                         \
        break;

#define NS(x)                                                                                      \
    case engine::AvailableBusEffects::x:                                                           \
        rebuildWithJsonLayoutEngine("partfx-layouts/" #x ".json");                                 \
        break;

    switch (t)
    {
        CS(reverb1);
        CS(reverb2)
        NS(flanger);
        CS(delay);
        CS(floatydelay);
        CS(nimbus);
        NS(phaser);
        CS(treemonster);
        CS(rotaryspeaker);
        CS(bonsai);

    case engine::AvailableBusEffects::none:
        // Explicitly do nothing
        break;
    }

#undef CS

    clearAdditionalHamburgerComponents();
    bool hasTemposync{false};
    for (auto &p : params)
        hasTemposync = hasTemposync || p.canTemposync;

    if (hasTemposync)
    {
        auto res = std::make_unique<jcmp::ToggleButton>();
        res->drawMode = sst::jucegui::components::ToggleButton::DrawMode::GLYPH;
        res->setGlyph(jcmp::GlyphPainter::GlyphType::METRONOME);

        auto &data = getPartFXStorage();
        auto onGuiChange = [w = juce::Component::SafePointer(this)](auto &a) {
            if (w)
            {
                // Fix this 0
                w->busEffectStorageChangedFromGUI(a, 0);
                // Need to finish this gui action before rebuilding, which could destroy myself
                juce::Timer::callAfterDelay(0, [w]() {
                    if (w)
                        w->rebuild();
                });
                // w->rebuild();
            }
        };

        auto at = std::make_unique<boolAttachment_t>("Active", onGuiChange,
                                                     getPartFXStorage().second.isTemposync);
        res->setSource(at.get());
        addAndMakeVisible(*res);

        boolAttachments.insert(std::move(at));
        addAdditionalHamburgerComponent(std::move(res));
    }

    repaint();
}

template <bool forBus>
template <typename T>
T *PartEffectsPane<forBus>::attachWidgetToFloat(int pidx)
{
    auto &data = getPartFXStorage();
    auto &pmd = data.first[pidx];
    auto onGuiChange = [w = juce::Component::SafePointer(this), pidx](auto &a) {
        if (w)
            w->busEffectStorageChangedFromGUI(a, pidx);
    };

    auto at =
        std::make_unique<attachment_t>(pmd, onGuiChange, getPartFXStorage().second.params[pidx]);

    if (pmd.canTemposync)
    {
        at->setTemposyncFunction([w = juce::Component::SafePointer(this)](const auto &a) {
            if (w)
                return w->getPartFXStorage().second.isTemposync;
            return false;
        });
    }
    auto w = std::make_unique<T>();
    w->setSource(at.get());

    if (pmd.canDeactivate && data.second.isDeactivated(pidx))
    {
        w->setEnabled(false);
    }
    else
    {
        w->setEnabled(true);
    }

    setupWidgetForValueTooltip(w.get(), at);
    addAndMakeVisible(*w);

    if constexpr (std::is_same_v<T, jcmp::Knob>)
    {
        w->setDrawLabel(false);
    }

    auto retVal = w.get();
    components.insert(std::move(w));
    floatAttachments.insert(std::move(at));

    return retVal;
}

template <bool forBus>
template <typename T>
juce::Component *PartEffectsPane<forBus>::attachWidgetToInt(int pidx)
{
    return nullptr;
}

template <bool forBus> juce::Component *PartEffectsPane<forBus>::attachMenuButtonToInt(int index)
{
    // TODO - this really needs some contemplation. I bet it wil lbe a pretty
    // common pattern. Where to put it?
    auto &b = getPartFXStorage();
    auto &pmd = b.first;
    auto &bes = b.second;

    auto r = std::make_unique<jcmp::MenuButton>();
    auto sv = pmd[index].valueToString(bes.params[index]);
    if (sv.has_value())
        r->setLabel(*sv);
    r->setOnCallback([r = juce::Component::SafePointer(r.get()),
                      w = juce::Component::SafePointer(this), index]() {
        if (!w)
            return;
        auto &b = w->getPartFXStorage();
        auto &pmd = b.first[index];
        auto &bes = b.second;

        auto p = juce::PopupMenu();
        p.addSectionHeader(pmd.name);
        p.addSeparator();
        for (int i = pmd.minVal; i <= pmd.maxVal; ++i)
        {
            auto sv = pmd.valueToString(i);
            if (sv.has_value())
                p.addItem(*sv, [w, r, sv, i, index]() {
                    if (w)
                    {
                        auto &bes = w->getPartFXStorage().second;
                        bes.params[index] = i;
                        if (r)
                        {
                            r->setLabel(*sv);
                            r->repaint();
                        }

                        if (forBus)
                        {
                            w->sendToSerialization(cmsg::SetBusEffectStorage(
                                {w->busAddressOrPart, -1, w->fxSlot, bes}));
                        }
                        else
                        {
                            w->sendToSerialization(cmsg::SetBusEffectStorage(
                                {-1, w->busAddressOrPart, w->fxSlot, bes}));
                        }
                    }
                });
        }
        p.showMenuAsync(w->editor->defaultPopupMenuOptions());
    });
    addAndMakeVisible(*r);
    auto retVal = r.get();
    components.insert(std::move(r));
    return retVal;
}

template <bool forBus>
juce::Component *PartEffectsPane<forBus>::attachToggleToDeactivated(int index)
{
    auto res = std::make_unique<jcmp::ToggleButton>();
    res->drawMode = sst::jucegui::components::ToggleButton::DrawMode::GLYPH;
    res->setGlyph(jcmp::GlyphPainter::GlyphType::SMALL_POWER_LIGHT);

    auto &data = getPartFXStorage();
    auto &pmd = data.first[index];
    auto onGuiChange = [w = juce::Component::SafePointer(this), index](auto &a) {
        if (w)
        {
            w->busEffectStorageChangedFromGUI(a, index);
            // Need to finish this gui action before rebuilding, which could destroy myself
            juce::Timer::callAfterDelay(0, [w]() {
                if (w)
                    w->rebuild();
            });
            // w->rebuild();
        }
    };

    auto at = std::make_unique<boolAttachment_t>("Active", onGuiChange,
                                                 getPartFXStorage().second.deact[index]);
    at->isInverted = true; // deact is no power light mkay
    res->setSource(at.get());
    addAndMakeVisible(*res);

    auto retVal = res.get();
    components.insert(std::move(res));
    boolAttachments.insert(std::move(at));

    return retVal;
}

template <bool forBus>
template <typename T>
juce::Component *PartEffectsPane<forBus>::addTypedLabel(const std::string &txt)
{
    auto l = std::make_unique<T>();
    l->setText(txt);
    if constexpr (std::is_same_v<T, jcmp::Label>)
        l->setJustification(juce::Justification::centred);
    addAndMakeVisible(*l);
    auto res = l.get();
    components.insert(std::move(l));
    return res;
}

template <bool forBus> void PartEffectsPane<forBus>::rebuildFromJSONLibrary(const std::string &path)
{
    bool parseWorked{false};
    auto dlyjs = connectors::JSONAssetLibrary::jsonForAsset(path);
    connectors::JSONLayoutConsumer con;
    try
    {
        tao::json::events::from_string(con, dlyjs);
        parseWorked = !con.result.empty();
    }
    catch (const std::exception &e)
    {
        SCLOG("JSON Parsing failed on '" << path << "' : " << e.what());
    }

    if (!parseWorked)
    {
        SCLOG("Parsing and loading components for json '"
              << path << "' didn't yield layout. Using default.");
        rebuildDefaultLayout();
        return;
    }

    SCLOG("JSON Layout for bus effect '" << path << "' yielded " << con.result.size()
                                         << " components");

    namespace jcmp = sst::jucegui::components;
    namespace jlay = sst::jucegui::layouts;
    using np = jlay::ExplicitLayout::NamedPosition;

    const auto &metadata = getPartFXStorage().first;

    auto elo = jlay::ExplicitLayout();
    const auto &cr = getContentArea();

    int idx{0};
    for (const auto &currentComponent : con.result)
    {
        auto ctag = [&currentComponent](auto key, auto val) -> std::string {
            auto itv = currentComponent.map.find(key);
            if (itv == currentComponent.map.end())
                return val;
            return itv->second;
        };

        auto cm = ctag("coordinate-system", "relative");
        auto nm = ctag("name", std::string("anon_") + std::to_string(idx));

        if (cm != "relative")
            throw std::runtime_error("Coming soon");
        auto &co = currentComponent.coords;
        auto el = np(nm).scaled(cr, co[0], co[1], co[2], co[3]);
        auto lab = ctag("label", "");

        if (currentComponent.index >= 0)
        {
            auto comp = ctag("component", "knob");
            if (!lab.empty())
            {
                elo.addNamedPositionAndLabel(el);
            }
            else
            {
                elo.addNamedPosition(el);
            }

            if (comp == "knob")
            {
                if (!lab.empty())
                {
                    layoutWidgetToFloat<jcmp::Knob>(elo, currentComponent.index, nm, lab);
                }
                else
                {
                    layoutWidgetToFloat<jcmp::Knob>(elo, currentComponent.index, nm);
                }
            }
            else if (comp == "menubutton")
            {
                attachMenuButtonToInt(currentComponent.index)->setBounds(elo.positionFor(nm));
            }
            else if (comp == "jogupdown")
            {
                // attachWidgetToInt<jcmp::JogUpDownButton>(currentComponent.index)
                //     ->setBounds(elo.positionFor(nm));
            }
            else
            {
                SCLOG("Unknown component : " << comp);
            }

            if (metadata[currentComponent.index].canDeactivate)
            {
                elo.addPowerButtonPositionTo(nm, 8);
                attachToggleToDeactivated(currentComponent.index)
                    ->setBounds(elo.powerButtonPositionFor(nm));
            }
        }
        else
        {
            auto comp = ctag("component", "");
            if (comp.empty())
                throw std::runtime_error("No Component");
            if (comp == "subheader")
            {
                elo.addNamedPosition(el);
                addTypedLabel<jcmp::RuledLabel>(lab)->setBounds(elo.positionFor(nm));
            }
            else if (comp == "label")
            {
                elo.addNamedPosition(el);
                addLabel(lab)->setBounds(elo.positionFor(nm));
            }
            else
            {
                SCLOG("Unknown component : " << comp);
            }
        }

        idx++;
    }
}

template <bool forBus> void PartEffectsPane<forBus>::rebuildDefaultLayout()
{
    namespace jcmp = sst::jucegui::components;
    // Just a stack of modulation style hsliders with labels
    int labht{15};
    static_assert(engine::BusEffectStorage::maxBusEffectParams <= 12,
                  "If this is false, this 3x4 grid will be bad");
    auto bx = std::min(getHeight() / 4.f - labht, getWidth() / 3.f) * 0.9f;
    auto &b = getPartFXStorage();

    auto into = getContentArea();
    auto r =
        into.withHeight(bx + labht).withWidth(bx).translated((into.getWidth() - bx * 3) * 0.5, 0);
    int idx{0};
    bool hasTemposync{false};
    for (const auto &p : b.first)
    {
        if (p.canTemposync)
            hasTemposync = true;
        if (p.type != sst::basic_blocks::params::ParamMetaData::NONE)
        {
            auto l = std::make_unique<jcmp::Label>();
            l->setText(p.name);
            l->setBounds(r);
            l->setJustification(juce::Justification::centredBottom);
            addAndMakeVisible(*l);
            components.insert(std::move(l));

            if (p.type == sst::basic_blocks::params::ParamMetaData::FLOAT)
            {
                // make slidera
                auto w = attachWidgetToFloat<jcmp::Knob>(idx);
                w->setBounds(r.withHeight(bx).reduced(2));
            }
            else if (p.type == sst::basic_blocks::params::ParamMetaData::INT)
            {
                // make menu
                auto w = attachMenuButtonToInt(idx);
                w->setBounds(r.withHeight(bx).withSizeKeepingCentre(r.getWidth(), 24));
            }
            else
            {
                SCLOG("No widget for type " << p.type << " / " << p.name);
            }

            if (p.canDeactivate)
            {
                auto pbsz{12};
                auto da = attachToggleToDeactivated(idx);
                auto q =
                    r.withTrimmedLeft(r.getWidth() - pbsz).withTrimmedBottom(r.getHeight() - pbsz);
                da->setBounds(q);
            }

            r = r.translated(bx, 0);
            if (!into.contains(r))
            {
                r = r.withX(into.getX()).translated((into.getWidth() - bx * 3) * 0.5, bx + labht);
            }
        }
        idx++;
    }
}

template <bool forBus>
template <typename Att>
void PartEffectsPane<forBus>::busEffectStorageChangedFromGUI(const Att &at, int idx)
{
    updateValueTooltip(at);
    auto &data = getPartFXStorage();

    if (forBus)
        sendToSerialization(cmsg::SetBusEffectStorage({busAddressOrPart, -1, fxSlot, data.second}));
    else
        sendToSerialization(cmsg::SetBusEffectStorage({-1, busAddressOrPart, fxSlot, data.second}));
}

template <bool forBus>
std::string PartEffectsPane<forBus>::effectDisplayName(engine::AvailableBusEffects t, bool forMenu)
{
    switch (t)
    {
    case engine::none:
        return forMenu ? "None" : "FX";
    case engine::flanger:
        return forMenu ? "Flanger" : "FLANGER";
    case engine::phaser:
        return forMenu ? "Phaser" : "PHASER";
    case engine::delay:
        return forMenu ? "Delay" : "DELAY";
    case engine::floatydelay:
        return forMenu ? "Floaty Delay" : "FLOATY DELAY";
    case engine::reverb1:
        return forMenu ? "Reverb 1" : "REVERB 1";
    case engine::reverb2:
        return forMenu ? "Reverb 2" : "REVERB 2";
    case engine::nimbus:
        return forMenu ? "Nimbus" : "NIMBUS";
    case engine::treemonster:
        return forMenu ? "TreeMonster" : "TREEMONSTER";
    case engine::bonsai:
        return forMenu ? "Bonsai" : "BONSAI";
    case engine::rotaryspeaker:
        return forMenu ? "Rotary Speaker" : "ROTARY";
    }

    return "GCC gives strictly correct, but not useful in this case, warnings";
}

template <bool forBus> void PartEffectsPane<forBus>::mouseDrag(const juce::MouseEvent &event)
{
    if (event.getDistanceFromDragStart() > 2)
    {
        if (!isDragging)
        {
            if (auto *container = juce::DragAndDropContainer::findParentDragContainerFor(this))
            {
                container->startDragging("BusEffect", this);
                isDragging = true;
                swapFX = !event.mods.isShiftDown();
            }
        }
    }
}

template <bool forBus>
void PartEffectsPane<forBus>::itemDropped(const SourceDetails &dragSourceDetails)
{
    setIsAccented(false);
    repaint();
    if (dragSourceDetails.sourceComponent.get() != this)
    {
        auto fxs = dragSourceDetails.sourceComponent.get();
        auto fc = dynamic_cast<FXSlotBearing *>(fxs);
        if (fc && fc != this)
        {
            parent->swapEffects(fc->busAddressOrPart, fc->fxSlot, busAddressOrPart, fxSlot);
        }
    }
}

template <bool forBus>
void PartEffectsPane<forBus>::rebuildWithJsonLayoutEngine(const std::string &path)
{
    // order matters here
    components.clear();
    floatAttachments.clear();
    std::fill(intProxyAttachments.begin(), intProxyAttachments.end(), nullptr);

    auto eng = sst::jucegui::layouts::JsonLayoutEngine(*this);

    auto res = eng.processJsonPath(path);
    if (!res)
        editor->displayError("JSON Parsing Error", *res.error);
}

template <bool forBus>
std::string PartEffectsPane<forBus>::resolveJsonPath(const std::string &path) const
{
    auto res = scxt::ui::connectors::jsonlayout::resolveJsonFile(path);
    return res;
}

template <bool forBus>
void PartEffectsPane<forBus>::createBindAndPosition(
    const sst::jucegui::layouts::json_document::Control &ctrl,
    const sst::jucegui::layouts::json_document::Class &cls)
{
    auto onError = [this, ctrl](const auto &s) {
        editor->displayError("JSON Error : " + ctrl.name, s);
    };
    auto intAttI = [](int i) { return 0; };

    auto zeroPoint = getContentArea().getTopLeft();

    if (ctrl.binding.has_value() && ctrl.binding->type == "float")
    {
        auto pidx = ctrl.binding->index;
        auto &data = getPartFXStorage();
        auto &pmd = data.first[pidx];
        auto onGuiChange = [w = juce::Component::SafePointer(this), pidx](auto &a) {
            if (w)
                w->busEffectStorageChangedFromGUI(a, pidx);
        };

        auto at = std::make_unique<attachment_t>(pmd, onGuiChange,
                                                 getPartFXStorage().second.params[pidx]);

        // auto viz = scxt::ui::connectors::jsonlayout::isVisible(ctrl, intAttI, onError);
        // if (!viz)
        //            return;

        auto ed = connectors::jsonlayout::createContinuousWidget(ctrl, cls);
        if (!ed)
        {
            onError("Could not create widget for " + ctrl.name + " with unknown control type " +
                    cls.controlType);
            return;
        }

        connectors::jsonlayout::attachAndPosition(this, ed, at, ctrl, zeroPoint);
        addAndMakeVisible(*ed);

        const auto &metadata = getPartFXStorage().first;

        if (metadata[pidx].canDeactivate)
        {
            auto power = attachToggleToDeactivated(pidx);
            power->setBounds(zeroPoint.x + ctrl.position.x + ctrl.position.w - 4,
                             zeroPoint.y + ctrl.position.y, 10, 10);
            auto en = !getPartFXStorage().second.deact[pidx];
            ed->setEnabled(en);
        }

        if (auto lab = connectors::jsonlayout::createControlLabel(ctrl, cls, *this, zeroPoint))
        {
            addAndMakeVisible(*lab);
            jsonLabels.push_back(std::move(lab));
        }

        components.insert(std::move(ed));
        floatAttachments.insert(std::move(at));
    }
    else if (ctrl.binding.has_value() && ctrl.binding->type == "int")
    {
        auto pidx = ctrl.binding->index;
        if (pidx < 0 || pidx >= scxt::maxBusEffectParams)
        {
            onError("Index " + std::to_string(pidx) + " is out of range on " + ctrl.name);
            return;
        }
        auto &data = getPartFXStorage();
        auto &pmd = data.first[pidx];
        auto onGuiChange = [w = juce::Component::SafePointer(this), pidx](auto &a) {
            if (w)
                w->busEffectStorageChangedFromGUI(a, pidx);
        };

        auto at = std::make_unique<attachment_t>(pmd, onGuiChange,
                                                 getPartFXStorage().second.params[pidx]);
        auto ipt = std::make_unique<intProxyAttachment_t>(*at);

        auto ed = connectors::jsonlayout::createDiscreteWidget(ctrl, cls, onError);
        if (!ed)
        {
            onError("Unable to create editor");
            return;
        }

        addAndMakeVisible(*ed);
        connectors::jsonlayout::attachAndPosition(this, ed, ipt, ctrl, zeroPoint);

        floatAttachments.insert(std::move(at));
        intProxyAttachments[pidx] = std::move(ipt);
        components.insert(std::move(ed));
    }
    else if (cls.controlType == "label")
    {
        if (!ctrl.label.has_value())
        {
            onError("Label has no 'label' member " + ctrl.name);
            return;
        }
        auto lab = std::make_unique<jcmp::Label>();
        lab->setText(*ctrl.label);
        lab->setJustification(juce::Justification::centred);
        lab->setBounds(ctrl.position.x + zeroPoint.x, ctrl.position.y + zeroPoint.y,
                       ctrl.position.w, ctrl.position.h);
        addAndMakeVisible(*lab);
        jsonLabels.push_back(std::move(lab));
    }
    else if (cls.controlType == "ruled-label")
    {
        if (!ctrl.label.has_value())
        {
            onError("Label has no 'label' member " + ctrl.name);
            return;
        }
        auto lab = std::make_unique<jcmp::RuledLabel>();
        lab->setText(*ctrl.label);
        lab->setBounds(ctrl.position.x + zeroPoint.x, ctrl.position.y + zeroPoint.y,
                       ctrl.position.w, ctrl.position.h);
        addAndMakeVisible(*lab);
        jsonLabels.push_back(std::move(lab));
    }
}

// Explicit Instantiate
template struct PartEffectsPane<true>;
template struct PartEffectsPane<false>;

} // namespace scxt::ui::app::shared
