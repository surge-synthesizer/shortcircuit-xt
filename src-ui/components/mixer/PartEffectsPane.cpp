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
#include "components/SCXTEditor.h"
#include "components/MixerScreen.h"

#include "connectors/JSONLayoutConsumer.h"

#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/RuledLabel.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/JogUpDownButton.h"
#include "sst/jucegui/layouts/ExplicitLayout.h"

namespace scxt::ui::mixer
{
namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;
void PartEffectsPane::showHamburger() { mixer->showFXSelectionMenu(busAddress, fxSlot); }

PartEffectsPane::~PartEffectsPane()
{
    removeAllChildren();
    // order matters here
    components.clear();
    floatAttachments.clear();
}

void PartEffectsPane::paintMetadata(juce::Graphics &g, const juce::Rectangle<int> &into)
{
    auto &b = mixer->busEffectsData[busAddress][fxSlot];

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

void PartEffectsPane::rebuild()
{
    auto &b = mixer->busEffectsData[busAddress][fxSlot];
    auto &params = b.first;
    auto &data = b.second;
    auto t = data.type;
    name = mixer->effectDisplayName(t, false);

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

    switch (t)
    {
        CS(reverb1);
        CS(reverb2)
        CS(flanger);
        CS(delay);
        CS(nimbus);
        CS(phaser);
        CS(treemonster);
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

        auto &data = mixer->busEffectsData[busAddress][fxSlot];
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

        auto at = std::make_unique<boolAttachment_t>(
            "Active", onGuiChange, mixer->busEffectsData[busAddress][fxSlot].second.isTemposync);
        res->setSource(at.get());
        addAndMakeVisible(*res);

        boolAttachments.insert(std::move(at));
        addAdditionalHamburgerComponent(std::move(res));
    }

    repaint();
}

template <typename T> T *PartEffectsPane::attachWidgetToFloat(int pidx)
{
    auto &data = mixer->busEffectsData[busAddress][fxSlot];
    auto &pmd = data.first[pidx];
    auto onGuiChange = [w = juce::Component::SafePointer(this), pidx](auto &a) {
        if (w)
            w->busEffectStorageChangedFromGUI(a, pidx);
    };

    auto at = std::make_unique<attachment_t>(
        pmd, onGuiChange, mixer->busEffectsData[busAddress][fxSlot].second.params[pidx]);

    if (pmd.canTemposync)
    {
        at->setTemposyncFunction([w = juce::Component::SafePointer(this)](const auto &a) {
            if (w)
                return w->mixer->busEffectsData[w->busAddress][w->fxSlot].second.isTemposync;
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

    setupWidgetForValueTooltip(w, at);
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

template <typename T> juce::Component *PartEffectsPane::attachWidgetToInt(int pidx)
{
    return nullptr;
}

juce::Component *PartEffectsPane::attachMenuButtonToInt(int index)
{
    // TODO - this really needs some contemplation. I bet it wil lbe a pretty
    // common pattern. Where to put it?
    auto &b = mixer->busEffectsData[busAddress][fxSlot];
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
        auto &b = w->mixer->busEffectsData[w->busAddress][w->fxSlot];
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
                        auto &bes = w->mixer->busEffectsData[w->busAddress][w->fxSlot].second;
                        bes.params[index] = i;
                        if (r)
                        {
                            r->setLabel(*sv);
                            r->repaint();
                        }

                        w->sendToSerialization(
                            cmsg::SetBusEffectStorage({w->busAddress, w->fxSlot, bes}));
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

juce::Component *PartEffectsPane::attachToggleToDeactivated(int index)
{
    auto res = std::make_unique<jcmp::ToggleButton>();
    res->drawMode = sst::jucegui::components::ToggleButton::DrawMode::GLYPH;
    res->setGlyph(jcmp::GlyphPainter::GlyphType::SMALL_POWER_LIGHT);

    auto &data = mixer->busEffectsData[busAddress][fxSlot];
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

    auto at = std::make_unique<boolAttachment_t>(
        "Active", onGuiChange, mixer->busEffectsData[busAddress][fxSlot].second.deact[index]);
    at->isInverted = true; // deact is no power light mkay
    res->setSource(at.get());
    addAndMakeVisible(*res);

    auto retVal = res.get();
    components.insert(std::move(res));
    boolAttachments.insert(std::move(at));

    return retVal;
}

template <typename T> juce::Component *PartEffectsPane::addTypedLabel(const std::string &txt)
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

void PartEffectsPane::rebuildFromJSONLibrary(const std::string &path)
{
    bool parseWorked{false};
    auto dlyjs = connectors::JSONLayoutLibrary::jsonForComponent(path);
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
    namespace jlay = sst::jucegui::layout;
    using np = jlay::ExplicitLayout::NamedPosition;

    const auto &metadata = mixer->busEffectsData[busAddress][fxSlot].first;

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

void PartEffectsPane::rebuildDefaultLayout()
{
    namespace jcmp = sst::jucegui::components;
    // Just a stack of modulation style hsliders with labels
    int labht{15};
    static_assert(engine::BusEffectStorage::maxBusEffectParams <= 12,
                  "If this is false, this 3x4 grid will be bad");
    auto bx = std::min(getHeight() / 4.f - labht, getWidth() / 3.f) * 0.9f;
    auto &b = mixer->busEffectsData[busAddress][fxSlot];

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

template <typename Att> void PartEffectsPane::busEffectStorageChangedFromGUI(const Att &at, int idx)
{
    updateValueTooltip(at);
    auto &data = mixer->busEffectsData[busAddress][fxSlot];

    sendToSerialization(cmsg::SetBusEffectStorage({busAddress, fxSlot, data.second}));
}

} // namespace scxt::ui::mixer